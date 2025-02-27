#include "config/ProgramConfig.hpp"

#include "spdlog/spdlog.h"

#include "video/VideoInitializer.hpp"
#include "http/HttpInitializer.hpp"
#include "camera_device/CameraInitializer.hpp"
#include "event/EventInitializer.hpp"

#include "rtsp/handler/RTSPSessionHandlerLive.hpp"

#include <atomic>
#include <unistd.h>

std::string ip_addr;

int main(int argc, char *argv[])
{
    if(argc != 2) {
        spdlog::info("{}", argc);
        spdlog::error("Usage : {} <ip_address>", argv[0]);
        return 1;
    }
    ip_addr.assign(argv[1]);
    spdlog::set_level(spdlog::level::info);
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    const config::HttpConfig* http = config->http_config();
    const config::VideoConfig* vid = config->video_config();
    const config::CameraConfig* cam = config->camera_config();

    //-------------------camera-----------------------
    std::thread camear_thread([&](){
        camera_device::CameraInitializer* cam_init = new camera_device::CameraInitializer();
        cam_init->init();
        cam_init->start();
    });
    camear_thread.detach();

    // sleep(5);
    // MulticastStream* multicast = new MulticastStreamV4L2();
    // std::string sdp;
    // multicast->make_sdp(sdp);
    // std::cout << sdp << std::endl;
    //--------------------http------------------------
    http::HttpInitializer* init = new http::HttpInitializer();
    init->init();
    init->start();

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




