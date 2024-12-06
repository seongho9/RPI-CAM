#include "video/VideoInitializer.hpp"
#include "spdlog/spdlog.h"
#include <sstream>
using namespace video;

VideoInitializer::VideoInitializer(){
    _handler= VideoHandler::get_instance();
    _streamer = new VideoStreamerGST();

}
void VideoInitializer::init(){
    int argc = 0;
    char **argv = nullptr;

    // GStreamer 초기화
    gst_init(&argc, &argv); 
}                    
void VideoInitializer::event(){
    std::lock_guard<std::mutex> lock(event_mutex);
    //임의 timestamp값, 임의 eventId값, 임의 path값
    if(video_event_triggered){
        // 서버로부터 timestamp와 eventId 받아와
        _handler->set_filename(save_path);
        _handler->process_video(event_timestamp, event_Id);
        video_event_triggered = false;
    }
}
int VideoInitializer::start(){
    
    spdlog::info("Starting video streaming...");
    

    if(_streamer->start_server()!= 0){
        spdlog::error("Failed to start GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming started successfully");
    return 0;

}
int VideoInitializer::stop(){
    
    spdlog::info("Stop video streaming...");

    if(_streamer->stop_server()!= 0){
        spdlog::error("Failed to stop GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming stopped successfully");
    return 0;
}

//임시 이벤트 trigger
void VideoInitializer::set_event(std::string path, std::string eventId, int timestamp){
    std::lock_guard<std::mutex> lock(event_mutex);
    event_timestamp = timestamp;
    event_Id = eventId;
    save_path = path;
    video_event_triggered = true;
    spdlog::info("Event triggered......!");
}