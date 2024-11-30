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
    std::unique_lock<std::mutex> lock(_input_mutex);
    _input.push(buffer);
    _input_cond.notify_one();

    return 0;
}

VideoBuffer* VideoQueue::pop(std::string name)
{
    //  해당이름의 스레드 정보가 존재하지 않음
    if(_thread_info.find(name) == _thread_info.end()) {
        spdlog::error("Thread information of Event {} is not exist", name);

        return nullptr;
    }
    //  해당이름의 비디오 큐가 존재하지 않음
    if(_output.find(name) == _output.end()) {
        spdlog::error("Video Queue of Event {} is not exist", name);

        return nullptr;
    }

    std::unique_lock<std::mutex> lock(_thread_info.find(name)->second->_mutex);
    _thread_info.find(name)->second->_cond_t.wait(
        lock, 
        [&](){  return !_output.find(name)->second.empty();  });
    
    
    VideoBuffer* ret = _output.find(name)->second.front();
    _output.find(name)->second.pop();

    return ret;
}

int VideoQueue::distribute_buffer()
{
    while(true) {
        std::unique_lock<std::mutex> lock(_input_mutex);
        // _input에 VideoBuffer가 들어올 때까지 대기
        // _input에 데이터가 있다면 굳이 대기할 필요 없음
        _input_cond.wait(lock,[&](){ return !_input.empty();});

        VideoBuffer* input = _input.front();
        VideoBuffer* copy_to = nullptr;

        // 초당 프레임 리셋
        if((++_current_frame_no) > _device_fps){
            _current_frame_no = 1;
        }

        // 각 thread의 정보를 통해 복사
        for(auto& item : _thread_info){
            if( (_current_frame_no % item.second->_fps) == 0 ){
                //  lock 획득
                std::unique_lock<std::mutex> q_lock(item.second->_mutex);

                // VideoBuffer 만큼 할당 후 복사
                copy_to = new VideoBuffer();
                memcpy(copy_to, input, sizeof(*input));

                // output buffer에 reference push
                _output.find(item.first)->second.push(copy_to);
                copy_to=nullptr;

                // notify thread
                item.second->_cond_t.notify_all();
            }
        }
        delete input;
    }
}