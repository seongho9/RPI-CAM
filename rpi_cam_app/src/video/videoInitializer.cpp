#include "video/videoInitializer.hpp"
#include "spdlog/spdlog.h"
#include <sstream>

video::VideoInitializer::VideoInitializer() : streamer(nullptr), videoHandler(nullptr){
    int argc = 0;
    char **argv = nullptr;

    // GStreamer 초기화
    gst_init(&argc, &argv); 
}
void video::VideoInitializer::init(){

    streamer = new VideoStreamer();
    videoHandler = new VideoHandler();
}                    
void video::VideoInitializer::event(){

}
int video::VideoInitializer::start(){
    
    spdlog::info("Starting video streaming...");

    if(streamer->start()!= 0){
        spdlog::error("Failed to start GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming started successfully");
    return 0;

}
int video::VideoInitializer::stop(){
    
    spdlog::info("Stop video streaming...");

    if(streamer->stop()!= 0){
        spdlog::error("Failed to stop GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming stopped successfully");
    return 0;
}