#include "video/VideoInitializer.hpp"
#include "spdlog/spdlog.h"
#include <sstream>
using namespace video;

VideoInitializer::VideoInitializer()
{
    _handler= VideoHandler::get_instance();
    _streamer = new VideoStreamerGST();
    _vid_config = config::ProgramConfig::get_instance()->video_config();
    _remove_enable = false;

}
void VideoInitializer::init(){
    // GStreamer 초기화
    gst_init(0, nullptr); 
}    

int VideoInitializer::start(){
    
    spdlog::info("Starting video streaming...");
    

    if(_streamer->start_server()!= 0){
        spdlog::error("Failed to start GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming started successfully");

    _remove_enable = true;

    _remove_thread = new std::thread([&](){
        int maintain_time = static_cast<int>(_vid_config->maintain());
        std::string path = static_cast<std::string>(_vid_config->save_path());

        while(_remove_enable) {
            _handler->remove_video(maintain_time, path);
            std::this_thread::sleep_for(std::chrono::seconds(maintain_time));
        }
    });

    spdlog::info("Remove Thread started successfully");

    return 0;
}

int VideoInitializer::stop(){
    
    spdlog::info("Stop video streaming...");

    if(_streamer->stop_server()!= 0){
        spdlog::error("Failed to stop GstStreamer");
        return -1;
    }
    spdlog::info("Video streaming stopped successfully");

    _remove_enable = false;
    _remove_thread->join();

    delete _remove_thread;

    spdlog::info("Remove Thread stopped successfully");

    return 0;
}
