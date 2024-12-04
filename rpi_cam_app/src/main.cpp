#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"
#include "video/VideoInitializer.hpp"
#include <atomic>
#include <unistd.h>

std::atomic<bool> event_video(false); 


void listen_for_video_events() {
    while (!event_video) {
        
        spdlog::info("이벤트가 발생했습니다");
        event_video = true; 
    }
}

int main(int argc, char const *argv[])
{
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    // config::EventConfig ev = config->event_config();
    // spdlog::info("event names ");
    // for (auto item : ev.event_name()){
    //     spdlog::info("{} {} fps", item, ev.event_fps(item));
    // }


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

    //video 부분

    video::VideoInitializer* video_initializer = video::VideoInitializer::get_instance();

    video_initializer->init();
    if (video_initializer->start() != 0) {
        spdlog::error("Failed to start video streaming server.");
        return -1;
    }
    std::thread event_listener_thread(listen_for_video_events);
    
    video::VideoHandler* video_handler = video::VideoHandler::get_instance();
    
    while (!event_video) {
        sleep(10);
    }

    std::string event_id = "event123"; // 임시 event_id 
    time_t timestamp = time(0);        // 임시 timestamp

    if (video_handler->process_video(timestamp, event_id) != 0) {
        spdlog::error("비디오 처리 실패.");
        return -1;
    }

    sleep(20);

    // 비디오 스트리밍 서버 중지
    if (video_initializer->stop() != 0) {
        spdlog::error("비디오 스트리밍 서버 중지 실패.");
        return -1;
    }

    event_listener_thread.join();

    return 0;
}
