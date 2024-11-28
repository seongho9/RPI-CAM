#ifndef VIDEOSTREAMER_HPP
#define VIDEOSTREAMER_HPP

namespace video{
    class VideoStreamer{
        
    public:
        VideoStreamer();
        virtual int start_server()=0;
        virtual int stop_server()=0;
    };
}

#endif VIDEOSTREAMER_HPP