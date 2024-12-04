#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"
#include "video/VideoInitializer.hpp"
#include <atomic>
#include <unistd.h>


int main(int argc, char const *argv[])
{
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    config::EventConfig ev = config->event_config();
    spdlog::info("event names ");
    for (auto item : ev.event_name()){
        spdlog::info("{} {} fps", item, ev.event_fps(item));
    }


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

    //--------------------video-----------------------

    video::VideoInitializer* video_initializer = video::VideoInitializer::get_instance();
    video::VideoHandler* video_handler = video::VideoHandler::get_instance();
    video_initializer->init();
    if (video_initializer->start() != 0) {
        spdlog::error("Failed to start video streaming server.");
        return -1;
    }
    
    std::thread streaming([&](){
        spdlog::info("Start Video streaming and saving");
        video_initializer->start();
    });
    std::thread event([&]{
        spdlog::info("Listening event.....");
        video_initializer->set_event();
    });
    while(true){
        video_initializer->event();
        sleep(30);
    }
    int maintain_time = 10; //임시 설정. 10분
    std::string path = "/home/pi/send"; //임시 설정. 완료된 영상을 서버에게 잘 보냈을 경우 파일이 있는 위치
    std::thread remove_video([&]{
        video_handler->remove_video(maintain_time, path);
    });
    streaming.join();
    event.join();
    remove_video.join();

    // Stop video streaming and saving
    if (video_initializer->stop() != 0) {
        spdlog::error("Failed to stop video streaming server.");
        return -1;
    }

    return 0;
}
