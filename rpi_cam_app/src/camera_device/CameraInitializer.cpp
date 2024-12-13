extern "C"
{
    #include<fcntl.h>
    #include<unistd.h>
    #include<errno.h>
    #include<sys/mman.h>
    #include<sys/ioctl.h>
    #include<linux/videodev2.h>
    #include<memory.h>
    #include<sys/select.h>
}
#include <ctime>

#include "spdlog/spdlog.h"
#include "camera_device/CameraInitializer.hpp"

using namespace camera_device;

CameraInitializer::CameraInitializer()
{
    _cam_config = config::ProgramConfig::get_instance()->camera_config();
    _is_run = false;
    _queue = VideoQueue::get_instance();
    _fd = -1;
}

int CameraInitializer::init()
{
    int ret = set_v4l2_format();
    if(ret != 0){
        spdlog::error("Error : set_v4l2_format()");
        return ret;
    }

    ret = set_v4l2_buffer();
    if(ret != 0){
        spdlog::error("Error : set_v4l2_buffer()");
        return ret;   
    }
    for(int i=0; i<N_CAM_BUF; i++) {
        queue_v4l2_buffer(i);
    }

    return 0;
}

int CameraInitializer::set_v4l2_format()
{
    struct v4l2_capability cap;

    _fd = open(_cam_config->device_path().c_str(), O_RDWR);
    if(_fd == -1){
        spdlog::error("Device \"{}\" could not open", _cam_config->device_path());

        return 1;
    }

    // 지원여부 확인
    if(ioctl(_fd, VIDIOC_QUERYCAP, &cap) == -1){
        spdlog::error("V4L2 capability query failed");
        return 2;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        spdlog::error("Device {} does not support capture", _cam_config->device_path());
    }
    if(!(cap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        spdlog::warn("Device {} does not support video output", _cam_config->device_path());
    }
    if(!(cap.capabilities & V4L2_CAP_READWRITE)) {
        spdlog::warn("Device {} does not support read write", _cam_config->device_path());
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
    spdlog::debug("width {} height {}", _cam_config->metadata().get_width(), _cam_config->metadata().get_height());
    // color map
    format.fmt.pix.pixelformat = _cam_config->metadata().get_type();

    spdlog::info("====V4L2 set====");
    spdlog::info("size : {} * {}", format.fmt.pix.width, format.fmt.pix.height);
    spdlog::info("color map : {} {} {} {}", 
        (char)((format.fmt.pix.pixelformat)       & 0xFF),
        (char)((format.fmt.pix.pixelformat >> 8)  & 0xFF),
        (char)((format.fmt.pix.pixelformat >> 16) & 0xFF),
        (char)((format.fmt.pix.pixelformat >> 24) & 0xFF));
    
    if(ioctl(_fd, VIDIOC_S_FMT, &format) == -1) {
        spdlog::error("Failed to set video foramt");
        return 4;
    }

    // framerate 설정
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(struct v4l2_streamparm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.denominator = _cam_config->fps();
    parm.parm.capture.timeperframe.numerator = 1;
    if(ioctl(_fd, VIDIOC_S_PARM, &parm)){
        perror("Framerate setting");
        return 5;
    }
    
    return 0;
}

int CameraInitializer::set_v4l2_buffer()
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(struct v4l2_requestbuffers));

    req.count = N_CAM_BUF;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if(ioctl(_fd, VIDIOC_REQBUFS, &req) == -1){
        perror("Request Buffers");
        return 1;
    }
    if(req.count > N_CAM_BUF){
        spdlog::error("Requested Buffer is over {} : {}", N_CAM_BUF, req.count);

        return 2;
    }

    for(int i=0; i < N_CAM_BUF; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(struct v4l2_buffer));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if(ioctl(_fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("could not query buffer");
            return 3;
        }

        _camera_buffers[i] = (uint8_t*)mmap(NULL, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, _fd, buf.m.offset);
    }  
    spdlog::info("Buffer Request done");

    return 0;
}

int CameraInitializer::queue_v4l2_buffer(int index)
{
    struct v4l2_buffer qbuf;
    memset(&qbuf, 0, sizeof(struct v4l2_buffer));

    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qbuf.memory = V4L2_MEMORY_MMAP;
    qbuf.index = index;

    if(ioctl(_fd, VIDIOC_QBUF, &qbuf) == -1){
        perror("V4L2 ioctl() VIDIOC_QBUF");

        return 0;
    }

    return qbuf.bytesused;
}

struct VideoBuffer* CameraInitializer::deque_v4l2_buffer(int* index)
{
    struct v4l2_buffer dqbuf;
    memset(&dqbuf, 0, sizeof(struct v4l2_buffer));

    dqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    dqbuf.memory = V4L2_MEMORY_MMAP;
    dqbuf.index = 0;

    if(ioctl(_fd, VIDIOC_DQBUF, &dqbuf) == -1) {
        perror("V4L2 ioctl() VIDIOC_DQBUF");

        return nullptr;
    }

    struct VideoBuffer* ret_buffer = new struct VideoBuffer;

    ret_buffer->timestamp = time(NULL);
    ret_buffer->metadata = _cam_config->metadata();
    ret_buffer->size = dqbuf.bytesused;
    ret_buffer->buffer = (uint8_t*)malloc(dqbuf.bytesused);
    memcpy(ret_buffer->buffer, _camera_buffers[dqbuf.index], dqbuf.bytesused);

    *index = dqbuf.index;

    return ret_buffer;
}

int CameraInitializer::loop()
{
    while(true) {
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(_fd, &fds);

        struct timeval tv;
        memset(&tv, 0, sizeof(struct timeval)); 
        // 0.5 sec
        tv.tv_sec = 0;
        tv.tv_usec = 500000;

        int r = select(_fd+1, &fds, NULL, NULL, &tv);

        if(r == -1){
            spdlog::error("Waiting for Frame");
        }
        else if(r == 0) {
            perror("Timeout");
            spdlog::error("V4L2 Frame IO timeout");
            return 1;
        }
        int index = -1;
        VideoBuffer* vid_buf = deque_v4l2_buffer(&index);

        if(vid_buf == nullptr) {
            spdlog::error("Error : Deqeue from V4L2");
            stop();
            break;
        }
        _queue->push(vid_buf);

        queue_v4l2_buffer(index);

    }
    return 0;
}

int CameraInitializer::start()
{
    unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(_fd, VIDIOC_STREAMON, &type) == -1) {
        perror("V4L2 ioctl() VIDIOC_STREAMON");
        return 1;
    }
    return loop();
}

int CameraInitializer::stop()
{
    unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(_fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("V4L2 ioctl() VIDIOC_STREAMOFF");
        return 1;
    }
    
    return 0;
}