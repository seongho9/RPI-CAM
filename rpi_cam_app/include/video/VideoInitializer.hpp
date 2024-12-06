#ifndef VIDEO_INITIALIZER_HPP
#define VIDEO_INITIALIZER_HPP

#include <gst/gst.h>
#include <string>
#include "video/VideoStreamer.hpp"
#include "video/VideoHandler.hpp"
#include "video/VideoStreamerGST.hpp"
#include "utils/Singleton.hpp"
#include <thread>
#include <mutex>

namespace video{
    class VideoInitializer: public utils::Singleton<VideoInitializer>{
    private:
        VideoStreamer* _streamer;
        VideoHandler* _handler;
        friend utils::Singleton<VideoInitializer>; //Singleton이 이 클래스의 private 생성자에 접근 가능
        VideoInitializer();
        std::mutex event_mutex;
        bool video_event_triggered = false;
        int event_timestamp;
        std::string event_Id;
        std::string save_path;
         
    public:
        void init();
        void event();
        int start();
        int stop();    
        void set_event(std::string path, std::string eventId, int timestamp);  //임시 이벤트 기다리는 함수
    };
};
 
#endif 