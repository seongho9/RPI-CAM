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
    public:
        VideoHandler();
        void get_video(std::string path, time_t timestamp);
        int process_video(time_t timestamp, uint64_t duration); 
        void remove_video(uint64_t duration, std::string path); //timestamp를 넘겨받아야 할거 같은데..
    };
}
#endif VIDEO_HANDLER_HPP