#ifndef VIDEOSTREAMER_HPP
#define VIDEOSTREAMER_HPP

namespace video{
    class VideoStreamer{
        
    public:
        VideoStreamer();
        virtual int start_server();
        virtual int stop_server();
    };
}

#endif VIDEOSTREAMER_HPP