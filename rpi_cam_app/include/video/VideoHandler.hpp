#ifndef VIDEO_HANDLER_HPP
#define VIDEO_HANDLER_HPP

#include <gst/gst.h>
#include <string>
#include <cstdint>
#include <vector>
#include "config/VideoConfig.hpp"
#include "utils/Singleton.hpp"

namespace video{
    class VideoHandler : public utils::Singleton<VideoHandler>{
        std::vector<std::string> files;
        std::vector<std::string> concat_file;
        friend utils::Singleton<VideoHandler>; //Singleton이 이 클래스의 private 생성자에 접근 가능
        config::VideoConfig* _video_config;
        VideoHandler();
    public:
        int get_video(std::string eventId, time_t timestamp);
        int process_video(time_t timestamp, std::string eventId); 
        void set_filename(std::string path);
        int remove_video(int maintain_time, std::string path); //maintain_time를 넘겨받아야 
    };
};
#endif