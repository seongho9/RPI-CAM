#ifndef VIDEOINITIALIZER_HPP
#define VIDEOINITIALIZER_HPP

#include <gst/gst.h>
#include <string>
#include "video/videoStreamer.hpp"
#include "video/videoHandler.hpp"

namespace video{
    class VideoInitializer{
    private:
        VideoStreamer* streamer;
        VideoHandler* videoHandler;
        //friend Singleton <VideoInitializer>;
        //VideoConfig _video_config;
        VideoInitializer();
         
    public:
        void init();
        void event();
        int start();
        int stop();    
    };
}
 
#endif VIDEOINITIALIZER_HPP