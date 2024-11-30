#include "camera_device/VideoQueue.hpp"
#include "spdlog/spdlog.h"

using namespace camera_device;

VideoBuffer::~VideoBuffer()
{
    free(buffer);
}

VideoQueue::VideoQueue()
{
    _device_fps = config::CameraConfig::get_instance()->fps();
    
    while(!_input.empty()){
        _input.pop();
    }
    _current_frame_no = 0;

    _output.clear();
    _thread_info.clear();
}

int VideoQueue::insert_event(std::string name, ThreadInfo* thread_info)
{
    return 0;
}

int VideoQueue::remove_event(std::string name)
{

}


int VideoQueue::push(VideoBuffer* buffer)
{
    return 0;
}

VideoBuffer* VideoQueue::pop(std::string name)
{
    return nullptr;
}

int VideoQueue::distribute_buffer()
{
    return 0;
}