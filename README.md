# RPI-CAM

[Github](https://github.com/seongho9/RPI-CAM)

라즈베리파이5 기반 IP 카메라

## 맡은 역할

- 프로젝트 설계
- 다양한 카메라 디바이스를 카메라로 지원하여 변경 할 수 있도록 구현(V4L2)
  - USE 웹캠, 라즈베리파이의 CSI 인터페이스를 통한 카메라 연결
- 이벤트 프로그램 동적 로드/삭제 기능 구현
  - `.so` 파일을 프로그램 런타임에 동적으로 로드 및 실행
- 영상 프레임 분배 큐 구현(Thread-Safe)
  - 카메라 디바이스에서 받아온 영상 프레임을 각 이벤트에 분배
  - 영상 프레임을 각 큐에 분배 시 복사가 발생

## 최적화

기존 완료된 프로그램의 문제를 해결하기 위해 개인적으로 진행

### 기존문제

- RTSP 스트리밍에서의 문제

  1개의 RTSP 세션에 1개의 미디어 스트림 파이프라인을 생성

  이 때, RTP 패킷 생성을 위한 미디어 파이프라인을 실행하기 위한 CPU 자원과 메모리, 복제된 영상 프레임 만큼 메모리가 세션마다 생성되어 비효율적

- MP4파일의 생성

  녹화 이후 생성되는 MP4파일이 RTP 패킷 생성 파이프라인에 의존하고 있으며, RTP 패킷 생성 파이프라인이 신규로 생성될 때마다 MP4파일을 생성하는 스레드도 같이 생성됨.

  이 때, 파일명을 UNIX 시간으로 표기하는데, 시간 싱크에 맞지 않을 뿐더러 MP4 파일 생성을 위해 자원이 중복되어 소비되고 있음

#### 개선

- RTSP 스트리밍 문제

  이를 해결하기 위해 RTSP 요청을 처리하는 RTSP 서버를 별도로 구현 [RTSP_Server](https://github.com/seongho9/rtsp_server)

  프로그램 초기 수행시 RTP 미디어 전송 파이프라인을 생성, 이후 라우터의 IGMP 대역 IP로 전송하도록 변경

  이는 클라이언트의 요청이 없을 경우에도 동작한다는 단점이 존재하지만, 이를 여러 클라이언트가 공유하여 자원의 추가적인 사용을 필요로 하지 않음

  이를 통해 고정적으로 가용가능한 자원을 확보함으로서 이벤트 프로그램이 가용가능한 자원을 예측할 수 있게 함

- MP4파일의 생성

  이 또한 RTSP 스트리밍에서 분기하여 별도의 스레드로 동작하게 하여 고정적으로 MP4파일을 생성하도록 하였음

### 검증

해당 테스트는 RTSP 세션 3개를 가지고 비교하였음

- 개선 이전
  ![prev](https://github.com/seongho9/RPI-CAM/blob/main/readme/ko/prev.PNG?raw=true)

  CPU는 4개의 코어에서 약 40%의 사용량

  메모리의 경우 236MB 사용

  스레드의 경우 총 21개가 생성된 것을 볼 수 있음

- 개선 이후
  ![post](https://github.com/seongho9/RPI-CAM/blob/main/readme/ko/post.PNG?raw=true)

  CPU는 4개의 코어에서 약 6-7%의 사용량

  메모리의 경우 사진에는 130MB로 표기되지만, 영상 프레임의 처리로 인해 130-132MB의 사용량을 보여주고 있음

  스레드의 경우 총 7개가 생성된 것을 볼 수 있음

CPU 사용량의 측면에서는 17.5%(7% / 40%) 혹은 33%(40%-7%) 의 성능향상을 
메모리 사용량 측면에서는 100MB의 사용량을 확보하였으며, 메모리 사용률을 5% 에서 3.2%로 감소 시켰음

그 밖에도 Multicast방식의 특성을 생각 했을 때, 네트워크 대역폭도 개선사항이 있을 것으로 판단됨

## 도식도

[클래스도](https://drive.google.com/file/d/1VQL-pgxViqYGfa1V3pcnVCQo6hMAFKvi/view?usp=sharing)

[설명](https://github.com/VEDA-Snackticon/RPI-CAM/blob/dev/readme/ko/README.md)

- 전반적인 프로세스 흐름

![image](https://github.com/user-attachments/assets/49b00dd6-fe4b-4fbd-8882-361d3de21e41)

- 프레임 분배 큐 흐름

![image](https://github.com/user-attachments/assets/ffdabf39-d2b2-4205-b7d5-87db3b148d48)

