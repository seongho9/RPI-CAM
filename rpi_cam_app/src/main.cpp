#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

#include "video/VideoInitializer.hpp"
#include "http/HttpInitializer.hpp"

#include <atomic>
#include <unistd.h>


int main(int argc, char const *argv[])
{

    spdlog::set_level(spdlog::level::info);
    config::ProgramConfig* config = config::ProgramConfig::get_instance();


    //spdlog::set_level(spdlog::level::level_enum::debug);
    const config::HttpConfig* http = config->http_config();
    spdlog::info("http");
    spdlog::info("crt file path : {} ", http->crt_file());
    spdlog::info("private file path : {}", http->private_file());
    spdlog::info("thread pool : {}", http->thread_pool());
    spdlog::info("tls enable : {}", http->tls_enable());

    const config::VideoConfig* vid = config->video_config();
    spdlog::info("video");
    spdlog::info( "{} * {}", vid->width(), vid->height());
    spdlog::info("format {}", vid->format());
    spdlog::info("{} {}", vid->split_time(), vid->duration());

    const config::CameraConfig* cam = config->camera_config();
    spdlog::info("camera");
    spdlog::info("{} * {}", cam->metadata().get_width(), cam->metadata().get_height());
    spdlog::info("type {}",  cam->metadata().get_type());
    spdlog::info("device path {}", cam->device_path());


    spdlog::info("init");

    //--------------------http------------------------
    http::HttpInitializer* init = http::HttpInitializer::get_instance();

    init->init();
    init->start();
    //--------------------video-----------------------
    video::VideoInitializer* video_initializer = video::VideoInitializer::get_instance();
    video::VideoHandler* video_handler = video::VideoHandler::get_instance(); 

    video_initializer->init();
    video_initializer->start();



    return 0;
}




