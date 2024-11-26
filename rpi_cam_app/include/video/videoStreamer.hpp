#ifndef VIDEOSTREAMER_HPP
#define VIDEOSTREAMER_HPP

namespace video{
    class VideoStreamer{
    public:
        virtual int start();
        virtual int stop();
    }
}

#endif VIDEOSTREAMER_HPP;