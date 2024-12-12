# 이벤트 기반 감시 시스템

이벤트를 감지하여 감지된 이벤트를 원격 서버로 전송하는 플랫폼으로, 무인 매장 도난 감지 시스템에 집중하여 구현하였습니다.

## 하드웨어 사양

Raspberry pi 5

USB Web Cam

## 소프트웨어

- libmicrohttpd
- Gstreamer
- libcamera (v4l2 compatible layer)
- cURL
- boost 

## 빌드

`yocto/conf` 내에 설정파일을 이용하여, 이미지 빌드 및 SDK를 구성한다

```bash
bitbake -c populate_sdk rpi-test-image
```

```bash
bitbake rpi-test-image
```
이 때, 내부에 포함된 이벤트 발생 알고리즘을 이용한다면, `meta-rpicam` 레포지토리에서의 레이어를 추가하여 구성한다 (현 conf 파일에는 `meta-rpicam` 및 `ncnn` 프레임워크 포함).

해당 레포지토리를 clone하여 cmake의 SDK path를 설정해주고, 아래와 같이 빌드
```bash
cd RPI-CAM/rpi_cam_app
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 사용법

- `config.json`

  ```json
  {
      // http 관련 설정
      "http" :
      {
          //	tls 활성화 여부
          "tls":"0",
          //	인증키
          "cert_file":"./cert.pem",
          "private_file":"./private.pem",
          //	웹서버 포트
          "port":"8000",
          //	파일을 업로드 할 수 있는 클라이언트 수
          "upload_clients":"2",
  		//	웹서버의 스레드 풀 개수
          "thread_pool_size":"5",
          //	원격 서버 주소
          "remote_server":"http://192.168.0.8"
      },
      //	비디오 스트리밍 설졍
      "video":
      {
          //	비디오 너비
          "width":"640",
          //	비디오 높이
          "height":"480",
          //	비디오 framerate
          "framerate":"30/1",
          //	비디오 색상 값
          "format":"YUY2",
          //	비디오 파일을 저장하는 단위(초)
          "split-time":"40",
          //	이벤트 발생시 전송할 시간으로 앞 뒤로 설정(120->240초)(초)
          "duration":"120",
          //	비디오 저장 경로
          "save_path":"video",
          //	비디오 파일을 유지하는 시간(초)
          "maintain":"120"
      },
      //	카메라 디바이스 설정, 비디오 스트리밍 설정과 같게 유지해야하는 설정 값 존재
      "camera":
      {	
          //	카메라 디바이스 파일
          "device_path":"/dev/video0",
          //	너비
          "width":"640",
          //	높이
          "height":"480",
          //	fps
          "fps":"60",
          //	색상 포맷
          "format":"V4L2_PIX_FMT_YUYV"
      }
  }
  ```

  `/home/root`에 `config.json`, 소스코드를 빌드한 결과불을 위치 시킨 후 수행

## 구조

![RPI-CAM consistent](https://github.com/VEDA-Snackticon/RPI-CAM/blob/dev/readme/ko/RPI-CAM%20consist.PNG?raw=true)

