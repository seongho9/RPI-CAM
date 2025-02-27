# RPI-CAM
라즈베리파이5 기반 IP 카메라

## 역할
- 프로젝트 설계
- 다양한 카메라 디바이스를 카메라로 지원하여 변경 할 수 있도록 구현(v4l2 이용)
  - USE 웹캠, 라즈베리파이의 CSI 인터페이스를 통한 카메라 연결
- 이벤트 프로그램 동적 로드/삭제 기능 구현
  - `.so` 파일을 프로그램 런타임에 동적으로 로드 및 실행
- 영상 프레임 분배 큐 구현(Thread-Safe)
  - 카메라 디바이스에서 받아온 영상 프레임을 각 이벤트에 분배
  - 영상 프레임을 분배 할 때, 복사하여 스레드 동기화 문제 해결
 
### 최적화

#### 기존

- RTSP 스트리밍에서의 문제

  1개의 RTSP 세션에 1개의 미디어 스트림 파이프라인을 생성

  이 때, `VideoQueue` 객체에 이벤트를 등록하고, 이를 통해 영상 프레임을 받아오게 되는데, 이 부분에서 영상 프레임의 복제가 이루어짐

  만약 초당 30프레임으로 영상을 스트리밍하고 있다면, 세션 1개당 초당 30장의 용량만큼 메모리를 사용

- MP4파일의 생성

  MP4파일의 생성이 RTSP 세션 미디어 스트림 파이프라인에 의존

  즉 세션이 많아 질수록 MP4파일의 생성도 증가

- Gstreamer의 `appsrc` 비동기 문제

  `appsrc`는 필요시마다 비동기적으로 호출되는 방식으로 진행

  기존 코드는 `need-data` 시그널 발생 시 스레드를 생성하여 별도로 `appsrc`에 버퍼를 push하도록 동작

  > <strong> 별도의 스레드를 생성한 이유 </strong>
  >
  > 위의 세션당 미디어 파이프라인을 생성하였기 때문에, 관련 frame count, 세션 식별 등 각 세션마다 고유하게 사용해야 하는 데이터가 존재하였음
  >
  > 비동기 처리 이후 호출되는 콜백에서 이를 식별하는 방법은 복잡하고, 추가 개발소요가 많았기 때문에 스레드를 생성하고 스레드 내부 stack 공간의 변수를 활용하여 세션을 식별하는 방식으로 개발

  이 방식은 아래와 같은 문제점을 동반하고 있음

  - `VideoQueue`는 이벤트를 추가/삭제 하는 방식으로 영상 프레임의 복사 개수를 동적으로 조절함

    이 때, 미디어 스트림 파이프라인 종료 이후, 영상 프레임 복사 개수를 줄여야 한다. 하지만 Gstreamer의 RTSP 서버를 사용하였기 때문에 세션을 세밀하게 조절하여 영상 프레임의 복제 개수를 설정 할 수 없었으며, 이로 인해세션을 사라졌지만, 지속적으로 영상 프레임이 복제되고 있는 상황이 발생 할 수 있음

#### 개선

- RTSP 스트리밍 문제

  프로그램 초기 수행시 RTP 미디어 전송 파이프라인을 생성 및 라우터의 IGMP 대역 IP로 전송하도록 변경

  이는 클라이언트가 없을 때 idle로 동작한다는 단점이 존재하지만, 이는 여러 클라이언트가 공유하여 자원의 추가적인 사용을 필요로 하지 않음

  즉, 추가적으로 탑제되는 이벤트 검출 프로그램이 사용하는 자원이 동적으로 변화하는 것이 아닌 고정적으로 확보됨

- MP4파일의 생성

  이 또한 RTSP 스트리밍에서 분기하여 별도의 스레드로 동작하게 하여 고정적으로 MP4파일을 생성하도록 하였음

- Gstreamer의 `appsrc` 비동기 문제

  RTSP 세션이 공유하는 미디어 스트림 파이프라인이 1개이므로 별도의 세션 id와 같은 고유한 자원이 아닌 고정적인 자원을 사용 할 수 있음

  이로 인해 별도의 스레드를 생성하지 않아도 되며, 파이프라인 스레드와 영상 프레임 복제 스레드간 동기화를 고려하지 않아도 됨

#### 검증

해당 테스트는 RTSP 세션 3개를 가지고 비교하였음

- 개선 이전

  ![prev](C:\Users\SeonghoJang\Desktop\RPI-CAM 개선\개선 이전.PNG)

  CPU는 4개의 코어에서 약 40%의 사용량

  메모리의 경우 236MB 사용

  스레드의 경우 총 21개가 생성된 것을 볼 수 있음

- 개선 이후

  ![post](C:\Users\SeonghoJang\Desktop\RPI-CAM 개선\개선 자원.PNG)

  CPU는 4개의 코어에서 약 6-7%의 사용량

  메모리의 경우 사진에는 130MB로 표기되지만, 영상 프레임의 처리로 인해 130-132MB의 사용량을 보여주고 있음

  스레드의 경우 총 7개가 생성된 것을 볼 수 있음

CPU 사용량의 측면에서는 17.5%(7% / 40%), 33%(40%-7%) 의 성능향상을 

메모리 사용량 측면에서는 100MB의 사용량을 확보하였으며, 메모리 사용률을 5% 에서 3.2%로 감소 시켰음

## 도식도

[클래스도](https://drive.google.com/file/d/1VQL-pgxViqYGfa1V3pcnVCQo6hMAFKvi/view?usp=sharing)

[설명](https://github.com/VEDA-Snackticon/RPI-CAM/blob/dev/readme/ko/README.md)

- 전반적인 프로세스 흐름
  
![image](https://github.com/user-attachments/assets/49b00dd6-fe4b-4fbd-8882-361d3de21e41)
- 프레임 분배 큐 흐름
  
![image](https://github.com/user-attachments/assets/ffdabf39-d2b2-4205-b7d5-87db3b148d48)
