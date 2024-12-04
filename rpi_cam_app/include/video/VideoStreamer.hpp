#ifndef VIDEO_STREAMER_HPP
#define VIDEO_STREAMER_HPP

namespace video{
    class VideoStreamer{
        
    public:
        VideoStreamer();
        virtual int start_server();
        virtual int stop_server();
    };
}

#endif VIDEO_STREAMER_HPP