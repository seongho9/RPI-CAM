#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

#include "video/VideoInitializer.hpp"
#include "http/HttpInitializer.hpp"
#include "camera_device/CameraInitializer.hpp"
#include "event/EventInitializer.hpp"

#include <atomic>
#include <unistd.h>


int main(int argc, char const *argv[])
{

    spdlog::set_level(spdlog::level::info);
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    const config::HttpConfig* http = config->http_config();
    const config::VideoConfig* vid = config->video_config();
    const config::CameraConfig* cam = config->camera_config();

    spdlog::info("init");

    //--------------------http------------------------
    http::HttpInitializer* init = new http::HttpInitializer();
    init->init();
    init->start();

    //-------------------camera-----------------------
    std::thread camear_thread([&](){
        camera_device::CameraInitializer* cam_init = new camera_device::CameraInitializer();
        cam_init->init();
        cam_init->start();
    });
    camear_thread.detach();
    
    //-------------------event-----------------------
    event::EventInitializer* event_init = new event::EventInitializer();
    event_init->init();
    event_init->start();

    //--------------------video-----------------------
    video::VideoInitializer* video_initializer = new video::VideoInitializer();
    video_initializer->init();
    video_initializer->start();

    return 0;
}




