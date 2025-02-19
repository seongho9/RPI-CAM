# RPI-CAM
라즈베리파이5 기반 IP 카메라

## 역할
- 프로젝트 설계
- 다양한 카메라 디바이스를 카메라로 지원하여 변경 할 수 있도록 구현(v4l2 이용)
  - USE 웹캠, 라즈베리파이의 CSI 인터페이스를 통한 카메라 연결
- 이벤트 프로그램 동적 로드/삭제 기능 구현
- 영상 프레임 분배 큐 구현(Thread-Safe)

## 도식도
- 전반적인 프로세스 흐름
![image](https://github.com/user-attachments/assets/49b00dd6-fe4b-4fbd-8882-361d3de21e41)
- 프레임 분배 큐 흐름
![image](https://github.com/user-attachments/assets/ffdabf39-d2b2-4205-b7d5-87db3b148d48)


[클래스도](https://drive.google.com/file/d/1Gvx9dX0S3uyN6MLYgNZ5ML7pcKwm5m4n/view?usp=sharing)

[설명](https://github.com/VEDA-Snackticon/RPI-CAM/blob/dev/readme/ko/README.md)
