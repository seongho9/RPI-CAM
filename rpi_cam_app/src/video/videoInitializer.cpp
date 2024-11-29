#include "video/videoInitializer.hpp"
#include "spdlog/spdlog.h"
#include <sstream>

video::VideoInitializer::VideoInitializer(): _streamer(), _handler(*utils::Singleton<VideoHandler>::get_instance()){

}
void video::VideoInitializer::init(){
    int argc = 0;
    char **argv = nullptr;

    // GStreamer 초기화
    gst_init(&argc, &argv); 
}                    
void video::VideoInitializer::event(){
    //이 함수에 무슨 내용을 

}
int video::VideoInitializer::start(){
    
    spdlog::info("Starting video streaming...");

    if(_streamer.start_server()!= 0){
        spdlog::error("Failed to start GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming started successfully");
    return 0;

}
int video::VideoInitializer::stop(){
    
    spdlog::info("Stop video streaming...");

    if(_streamer.stop_server()!= 0){
        spdlog::error("Failed to stop GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming stopped successfully");
    return 0;
}