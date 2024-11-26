#ifndef GSTSTREAMER_HPP
#define GSTSTREAMER_HPP

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include "video/videoStreamer.hpp"
#include "config/VideoConfig.hpp"

namespace video{
    class GstStreamer : public VideoStreamer{
        GstRTSPServer *rtsp_server;
        GstRTSPMountPoints *mountPoint;  // 클라이언트가 rtsp 서버에 접속할때 요청하는 url과 실제 미디어 스트림 연결 역할
        GstRTSPMediaFactory *mfactory;   // rtsp 서버에서 스트리밍 할 미디어 파이프라인을 생성하는 객체
        GMainLoop *main_loop;
        std::string pipeline;

    public:
        GstStreamer();
        ~GstStreamer();
        std::string createPipeline(const VideoConfig& config);
        void make_server();
        int start() override;
        int stop() override;
        static gchar* format_location_callback(GstElement* splitmux, guint fragment_id, gpointer user_data);
    };  
}

#endif GSTSTREAMER_HPP