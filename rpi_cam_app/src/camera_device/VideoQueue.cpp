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
    _thread_info_mutex.lock();

    auto item = _thread_info.find(name);

    if(item != _thread_info.end()) {
        spdlog::error("Event {} was created", name);
        
        _thread_info_mutex.unlock();
        return 1;
    }
    else {
        // push pop 연산시 연산강도 경감을 위함
        //  ex. 60 / 7 = 8 -> 현 프레임에서 8의 배수 부분만 처리(8,16,24,32,40,48,56)
        thread_info->_fps = (_device_fps / thread_info->_fps);

        _thread_info.emplace(name, thread_info);
        spdlog::info("Event {} has been inserted into the VideoQueue", name);

        _thread_info_mutex.unlock();
        return 0;
    }
}

int VideoQueue::remove_event(std::string name)
{
    _thread_info_mutex.lock();

    auto item = _thread_info.find(name);
    if(item == _thread_info.end()) {
        spdlog::error("Event {} is not exist in VideoQeue", name);

        _thread_info_mutex.unlock();
        return 1;
    }
    else {
        item->second->_is_run = false;
        item->second->_cond_t.notify_all();

        _thread_info.erase(name);

        spdlog::info("Event {} Thread has been removed from VideoQeue", name);

        _thread_info_mutex.unlock();
        return 0;
    }
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