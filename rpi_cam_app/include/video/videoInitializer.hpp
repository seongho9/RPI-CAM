#ifndef VIDEOINITIALIZER_HPP
#define VIDEOINITIALIZER_HPP

#include <gst/gst.h>
#include <string>
#include "video/videoStreamer.hpp"
#include "video/videoHandler.hpp"
#include "video/videoStreamerGST.hpp"
#include "utils/Singleton.hpp"

namespace video{
    class VideoInitializer{
    private:
        VideoStreamerGST _streamer;
        VideoHandler _handler;
        friend utils::Singleton<VideoInitializer>; //Singleton이 이 클래스의 private 생성자에 접근 가능
        VideoInitializer();
         
    public:
        void init();
        void event();
        int start();
        int stop();    
    };
}
 
#endif VIDEOINITIALIZER_HPP