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
        int timestamp = 1738129293;
        std::string eventId = "event1";
        std::string path = "/home/pi/event";
        // 서버로부터 timestamp와 eventId 받아와
        _handler->set_filename(path);
        _handler->process_video(timestamp, eventId);
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
void VideoInitializer::set_event(){
    std::lock_guard<std::mutex> lock(event_mutex);
    video_event_triggered = true;
    spdlog::info("Event triggered......!");
}