
#ifndef VIDEO_STREAMERGST_HPP
#define VIDEO_STREAMERGST_HPP

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include "config/VideoConfig.hpp"
#include "video/VideoStreamer.hpp"

namespace video{
    class VideoStreamerGST: public VideoStreamer{
    private: 
        GstRTSPServer *rtsp_server;
        GstRTSPMountPoints *mountPoint;  // 클라이언트가 rtsp 서버에 접속할때 요청하는 url과 실제 미디어 스트림 연결 역할
        GstRTSPMediaFactory *mfactory;   // rtsp 서버에서 스트리밍 할 미디어 파이프라인을 생성하는 객체
        GMainLoop *main_loop;
        std::string pipeline;
        config::VideoConfig _video_config;
        
    public:
        VideoStreamerGST();
        ~VideoStreamerGST();
        std::string createPipeline(const config::VideoConfig& config);
        void make_server();
        int start_server() override;
        int stop_server() override;
        static gchar* format_location_callback(GstElement* splitmux, guint fragment_id, gpointer user_data);
    };  
}

#endif VIDEO_STREAMERGST_HPP