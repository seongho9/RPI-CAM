extern "C"
{
    #include<fcntl.h>
    #include<unistd.h>
    #include<errno.h>
    #include<sys/mman.h>
    #include<sys/ioctl.h>
    #include<linux/videodev2.h>
    #include<memory.h>
}
#include <ctime>

#include "spdlog/spdlog.h"
#include "camera_device/CameraInitializer.hpp"

using namespace camera_device;

CameraInitializer::CameraInitializer()
{
    _cam_config = config::CameraConfig::get_instance();
    _is_run = false;
    _queue = VideoQueue::get_instance();
    fd = -1;
}

int CameraInitializer::init()
{
    struct v4l2_capability cap;

    fd = open(_cam_config->device_path().c_str(), O_RDWR);
    if(fd == -1){
        spdlog::error("Device {} could not open", _cam_config->device_path());

        return 1;
    }

    // 지원여부 확인
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1){
        spdlog::error("V4L2 capability query failed");

        return 2;
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        spdlog::error("Device {} does not support capture", _cam_config->device_path());
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        spdlog::error("Device {} does not support video output", _cam_config->device_path());
    }
    if(!(cap.capabilities & V4L2_CAP_READWRITE)) {
        spdlog::error("Device {} does not support read write", _cam_config->device_path());
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        spdlog::error("Device {} does not support streaming i/o", _cam_config->device_path());
    }
    
    // 비디오 포맷 설정
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    // width * height
    format.fmt.pix.width = static_cast<unsigned int>(_cam_config->metadata().get_width());
    format.fmt.pix.height = static_cast<unsigned int>(_cam_config->metadata().get_height());
    // color map
    format.fmt.pix.pixelformat = _cam_config->metadata().get_type();

    spdlog::info("====V4L2 set====");
    spdlog::info("size : {} * {}", format.fmt.pix.width, format.fmt.pix.height);
    spdlog::info("color map : {} {} {} {}", 
        (format.fmt.pix.pixelformat)       & 0xFF,
        (format.fmt.pix.pixelformat >> 8)  & 0xFF,
        (format.fmt.pix.pixelformat >> 16) & 0xFF,
        (format.fmt.pix.pixelformat >> 24) & 0xFF);
    
    if(ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        spdlog::error("Failed to set video foramt");
        return 4;
    }
}