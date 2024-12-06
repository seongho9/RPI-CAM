#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"
#include "video/VideoInitializer.hpp"
#include <atomic>
#include <unistd.h>


int main(int argc, char const *argv[])
{
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

    std::atomic<bool> running(true);
    std::string path1 = "";  // videoStreamerGST 파이프라인으로 영상 분할 저장된 위치 -> 현재는 파일 실행 위치로 되어 있음

    std::thread video_event_thread([&](){
        // if 서버가 이벤트를 보낸다면

        // 서버로부터 넘겨받은 timestamp, eventId값;
        int timestamp;
        std::string eventId;

        video_initializer->set_event(path1, eventId, timestamp);
    });

    //일정 시간 지나면 video 삭제 thread
    /*
    int maintain_time = 10; //임시 설정. 10분
    std::string path2 = "/home/pi/send_server"; //임시 설정. 완료된 영상을 서버에게 잘 보냈을 경우 파일이 있는 위치

    std::thread remove_video_thread([&](){
        video_handler->remove_video(maintain_time, path1);
        video_handler->remove_video(maintain_time, path2);
    });*/

    // 이벤트 처리
    while(running){
        video_initializer->event();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    streaming_thread.join();
    video_event_thread.join();


    // Stop video streaming and saving
    if (video_initializer->stop() != 0) {
        spdlog::error("Failed to stop video streaming server.");
        return -1;
    }

    return 0;
}




