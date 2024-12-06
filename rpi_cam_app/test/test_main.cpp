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

    std::atomic<bool> running(true);

    std::thread video_event_thread([&](){
        // 스트리밍 30초 후 1번 이벤트가 발생한다고 가정
        std::this_thread::sleep_for(std::chrono::seconds(30));
        int timestamp = 1738129293;  // 해당 timestamp 사용하려면 영상 파일 2개 필요(ex. 173812929.mp4, 173812930.mp4)
        std::string eventId = "event1";
        std::string path = "/home/pi/final5";
        video_initializer->set_event(path, eventId, timestamp);
    });

    //일정 시간 지나면 video 삭제 thread
    /*
    int maintain_time = 10; //임시 설정. 10분
    std::string path = "/home/pi/final5"; //임시 설정. 완료된 영상을 서버에게 잘 보냈을 경우 파일이 있는 위치

    std::thread remove_video_thread([&](){
         video_handler->remove_video(maintain_time, path);
    });*/

    // 이벤트 처리
    while(running){
        //임시 값
        video_initializer->event();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    streaming_thread.join();
    video_event_thread.join();
    //remove_video_thread.join();

    // Stop video streaming and saving
    if (video_initializer->stop() != 0) {
        spdlog::error("Failed to stop video streaming server.");
        return -1;
    }

    return 0;
}
