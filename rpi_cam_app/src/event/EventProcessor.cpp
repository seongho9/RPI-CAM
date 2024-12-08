#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <exception>
#include <string>
#include <sstream>


#include "event/EventProcessor.hpp"
#include "http/EventHandlerHTTP.hpp"
#include "spdlog/spdlog.h"

using namespace event;

static bool is_positive(const std::string& str)
{
    try {
        int number = std::stoi(str);
        if(number < 0) {
            return false;
        }
        else{
            return true;
        }
    } 
    catch(const std::invalid_argument& ex) {
        return false;
    }
    catch(const std::out_of_range& ex) {
        return false;
    }
}

EventProcessor::EventProcessor(std::string event_name)
{

    _video_q = camera_device::VideoQueue::get_instance();
    _event_handler = http::EventHandlerHTTP::get_instance();
    
    _event_name.assign(event_name);

    std::string event_directory("event/");
    event_directory.append(event_name);

    try {
        for(const auto& entry : std::filesystem::directory_iterator(event_directory)){
            const std::string& filename = entry.path().filename().string();
            // enable 확인
            if(filename == "enable"){
                // 해당 파일을 읽어서 0이면 false, 아니면 true을 반환
                std::ifstream enable_file(entry.path());
                int enable;

                if(enable_file >> enable) {
                    _enable = static_cast<bool>(enable);
                }
            }
            // 파일명이 양의 정수인지 확인
            else if(is_positive(filename)){
                _fps = std::stoi(filename);
            }
        }
    }
    catch(std::filesystem::filesystem_error& ex) {
        spdlog::error("Error to Access Event Directory : {}", ex.what());
    }

    event_init();
}

int EventProcessor::event_init()
{
    std::stringstream so_stream;
    so_stream << _event_name << ".so";

    // open 시점에 로드
    _handle = dlopen(so_stream.str().c_str(), RTLD_NOW);
    if(_handle){
        spdlog::error("Error Opening Event file {}", so_stream.str());
        return 1;
    }
    // dlfcn error string 초기화
    dlerror();

    //  이벤트 프로그램의 시작함수인 event_main 함수를 찾아 링크
    _event_program = (event_function)dlsym(_handle, "event_main");
    const char* error = dlerror();
    if(error) {
        spdlog::error("Error link event program : {}", error);
        dlclose(_handle);
        return 2;
    }

    return 0;
}

int EventProcessor::event_exec(camera_device::VideoBuffer* buffer)
{
    int ret = -1;
    ret = _event_program(
            buffer->buffer,                         //  버퍼 내용
            static_cast<size_t>((buffer->size)),    //  버퍼 크기
            buffer->timestamp,                      //  해당 사진의 unix time
            buffer->metadata.get_width(),           //  버퍼 너비
            buffer->metadata.get_height()           //  버퍼 높이
        );

    if(ret<0){
        spdlog::error("Event Program Interal Error");

        return 1;
    }
    else if(ret) {
        CURL* curl;
        _event_handler->event_send(curl, "", buffer->timestamp);
    }

    return 0;
}

int EventProcessor::event_loop()
{
    int ret = 0;
    
    camera_device::ThreadInfo* info = new camera_device::ThreadInfo();
    info->_fps = _fps;
    info->_is_run = _enable;

    _video_q->insert_event(_event_name, info);
    while(_info->_is_run){

        camera_device::VideoBuffer* buffer = _video_q->pop(_event_name);

        if(buffer == nullptr) {
            spdlog::warn("Event \"{}\" get nullptr buffer", _event_name);
            break;
        }

        ret = event_exec(buffer);
        if(ret){
            //  이벤트 프로그램 내부 오류 발생
        }
    }

    return 0;
}