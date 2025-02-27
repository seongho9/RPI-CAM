#include "camera_device/VideoQueue.hpp"
#include "spdlog/spdlog.h"
#include <cstring>
#include <pthread.h>
#include <ctime>

using namespace camera_device;

VideoBuffer::~VideoBuffer()
{
    if(buffer != nullptr){
        delete[] buffer;
    }
}

VideoQueue::VideoQueue()
{
    // V4L2의 framerate를 가져옴
    _device_fps = config::ProgramConfig::get_instance()->camera_config()->fps();
    _q_enable = true;

    while(!_input.empty()){
        _input.pop();
    }
    _current_frame_no = 0;

    _output.clear();
    _thread_info.clear();
    
    //  들어온 이미지를 각 이벤트에 분배하기 위해 사용하는 스레드 생성
    for(int i=0; i<6; i++){
        std::thread thread([&](){
            int ret = this->distribute_buffer();
        });
        thread.detach();
    }

    spdlog::info("============================");
    spdlog::info("===VideoQueue Initialized===");
    spdlog::info("============================");
}

VideoQueue::~VideoQueue()
{
    //  분배 스레드 제거
    std::unique_lock<std::mutex> lock(_input_mutex);
    _q_enable = false;
    _input_cond.notify_all();      
}

int VideoQueue::insert_event(std::string name, ThreadInfo* thread_info)
{
    _thread_info_mutex.lock();

    auto item = _thread_info.find(name);

    //  이미 이벤트가 존재하는 경우
    if(item != _thread_info.end()) {
        spdlog::error("Event {} has already been created", name);
        
        _thread_info_mutex.unlock();
        return 1;
    }
    else {
        spdlog::debug("device fps {}, thread fps {}", _device_fps, thread_info->_fps);

        // push pop 연산시 연산강도 경감을 위함
        //  ex. 60 / 7 = 8 -> 현 프레임에서 8의 배수 부분만 처리(8,16,24,32,40,48,56)
        thread_info->_fps = (_device_fps / thread_info->_fps);

        _thread_info.emplace(name, thread_info);
        _output.emplace(name, new std::queue<VideoBuffer*>);

        spdlog::info("Event {} has been inserted into the VideoQueue", name);

        _thread_info_mutex.unlock();

        return 0;
    }
}

int VideoQueue::remove_event(std::string name)
{
    // 반환 값 조절을 위한 플래그
    bool thread_info_flag = false;
    bool q_flag = false;

    _thread_info_mutex.lock();


    auto queue_info = _output.find(name);
    auto thread_info = _thread_info.find(name);

    if(thread_info != _thread_info.end()){
        std::unique_lock(thread_info->second->_mutex);
    }
    spdlog::debug("Event size : {} {}", _thread_info.size(), _output.size());

    //  output queue 처리
    if(queue_info == _output.end()) {
        spdlog::error("Output Queue of {} is not exist", name);
        q_flag = true;
    }
    else {
        //  기 존재하는 output queue의 내용을 제거
        while(!queue_info->second->empty()){
            spdlog::debug("name {}", queue_info->first);
            spdlog::debug("buffer addr {}", (void*)queue_info->second->front()->buffer);

            delete queue_info->second->front();
            
            queue_info->second->pop();
        }

        delete queue_info->second;
        _output.erase(queue_info->first);

        spdlog::info("Video Queue of {} has been removed", name);
    }
    
    //  thread info 처리
    if(thread_info == _thread_info.end()) {
        spdlog::error("Event {} is not exist in VideoQeue", name);
        thread_info_flag = true;
    }
    else {
        ThreadInfo* info = thread_info->second;

        info->_is_run = false;
        _thread_info.erase(name);
    }
    _thread_info_mutex.unlock();

    if(thread_info_flag){
        return 1;
    }
    if(q_flag){
        return 2;
    }

    return 0;
}

int VideoQueue::push(VideoBuffer* buffer)
{
    spdlog::trace("push {}", (void*)buffer->buffer);
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
        [&](){  
            return (!_thread_info.find(name)->second->_is_run || !_output.find(name)->second->empty());  
        });

    // event의 스레드 정보 삭제 및 nullptr를 반환하여 event 스레드의 종료를 유도
    if(!_thread_info.find(name)->second->_is_run) {
        _thread_info.erase(name);
        spdlog::info("Thread Information of {} has been removed", name);
        return nullptr;
    }
    
    VideoBuffer* ret = _output.find(name)->second->front();
    _output.find(name)->second->pop();


    return ret;
}

int VideoQueue::distribute_buffer()
{
    while(_q_enable) {
        std::unique_lock<std::mutex> lock(_input_mutex);
        // _input에 VideoBuffer가 들어올 때까지 대기
        // _input에 데이터가 있다면 굳이 대기할 필요 없음
        _input_cond.wait(lock,[&](){ 
            spdlog::trace("check wake up {}", (int)_q_enable);
            return !_q_enable || !_input.empty();});

        if(!_q_enable){
            spdlog::info("Queue distributer End...");
            continue;
        }
        VideoBuffer* input = _input.front();
        _input.pop();

        VideoBuffer* copy_to = nullptr;

        // 초당 프레임 리셋
        if((++_current_frame_no) > _device_fps){
            _current_frame_no = 1;
        }
        time_t t = time(NULL);
        
        _thread_info_mutex.lock();
        // 각 thread의 정보를 통해 복사
        for(auto& item : _thread_info){
            spdlog::debug("Queue info : info-{} output-{}", _thread_info.size(), _output.size());
            if( (_current_frame_no % item.second->_fps) == 0 ){
                //  lock 획득
                std::unique_lock<std::mutex> q_lock(item.second->_mutex);
                spdlog::debug("event name {}", item.first);

                // VideoBuffer 만큼 할당 후 복사
                copy_to = new VideoBuffer();
                memcpy(copy_to, input, sizeof(VideoBuffer));
                copy_to->buffer = new uint8_t[input->size];
                memcpy(copy_to->buffer, input->buffer, input->size);

                copy_to->timestamp = t;

                // output buffer에 reference push
                if(_output.find(item.first) != _output.end()) {
                    _output.find(item.first)->second->push(copy_to);
                    copy_to=nullptr;    
                }
                // notify thread
                item.second->_cond_t.notify_all();
            }
        }
        _thread_info_mutex.unlock();
        delete input;
    }
    spdlog::info("Video Queue End...");
    return 0;
}