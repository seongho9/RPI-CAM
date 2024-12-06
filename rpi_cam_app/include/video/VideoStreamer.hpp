#ifndef VIDEO_STREAMER_HPP
#define VIDEO_STREAMER_HPP

namespace video{
    class VideoStreamer{
        
    public:
        //VideoStreamer()=delete;
        virtual int start_server()=0;
        virtual int stop_server()=0;
    };
};

#endif 