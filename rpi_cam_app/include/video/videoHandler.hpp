#ifndef VIDEO_HANDLER_HPP
#define VIDEO_HANDLER_HPP

#include <gst/gst.h>
#include <string>
#include <cstdint>
#include <vector>

namespace video{
    class VideoHandler{
        std::vector<std::string> files;
        std::vector<std::string> concat_file;
        //friend Singleton<VideoHandler>; 
        VideoHandler();
    public:
        int get_video(std::string eventId, time_t timestamp);
        int process_video(time_t timestamp, uint64_t duration, std::string eventId); 
        void set_filename(std::string path);
        int remove_video(int maintain_time, std::string path); //maintain_time를 넘겨받아야..
    };
}
#endif VIDEO_HANDLER_HPP