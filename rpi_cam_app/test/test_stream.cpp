#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"
#include "video/VideoInitializer.hpp"
#include <atomic>
#include <unistd.h>


int main(int argc, char const *argv[])
{   
    spdlog::set_level(spdlog::level::level_enum::debug);
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    //spdlog::set_level(spdlog::level::level_enum::debug);

    const config::VideoConfig* vid = config->video_config();
    spdlog::info("video");
    spdlog::info( "{} * {}", vid->width(), vid->height());
    spdlog::info("format {}", vid->format());
    spdlog::info("{} {}", vid->split_time(), vid->duration());


    spdlog::info("init");


    //--------------------video-----------------------

    video::VideoInitializer* video_initializer = video::VideoInitializer::get_instance();
    video::VideoHandler* video_handler = video::VideoHandler::get_instance(); 

    video_initializer->init();

    std::thread streaming_thread([&](){
        if (video_initializer->start() != 0) {
            spdlog::error("Failed to start video streaming server.");
            return -1;
        }
    });
    while(1);
    return 0;
}
