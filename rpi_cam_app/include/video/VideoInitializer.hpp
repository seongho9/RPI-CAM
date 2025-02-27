#ifndef VIDEO_INITIALIZER_HPP
#define VIDEO_INITIALIZER_HPP

#include <gst/gst.h>
#include <string>
#include "video/VideoStreamer.hpp"
#include "video/VideoHandler.hpp"
#include "video/VideoStreamerGST.hpp"
#include "utils/Singleton.hpp"
#include "config/ProgramConfig.hpp"

#include "rtsp/server/RTSPListener.hpp"
#include "rtsp/handler/RTSPSessionHandlerLive.hpp"

#include <thread>
#include <mutex>
#include <thread>

namespace video{
    class VideoInitializer
    {
    private:
        const config::VideoConfig* _vid_config;
        
        VideoHandler* _handler;

        std::mutex event_mutex;
        bool video_event_triggered = false;
        int event_timestamp;
        std::string event_Id;

        std::thread* _remove_thread;
        bool _remove_enable;
         
    public:
        VideoInitializer();
        
        void init();
        int start();
        int stop();
    };
};
 
#endif 