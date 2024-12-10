
#ifndef VIDEO_STREAMERGST_HPP
#define VIDEO_STREAMERGST_HPP

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <atomic>
#include <unordered_map>

#include "config/VideoConfig.hpp"
#include "video/VideoStreamer.hpp"
#include "camera_device/VideoQueue.hpp"

namespace video{

    struct sess_info
    {
        std::string* id;
        camera_device::ThreadInfo* info;
    };
    class VideoStreamerGST: public VideoStreamer
    {
    private: 
    
        GstRTSPServer* _rtsp_server;
        // 클라이언트가 rtsp 서버에 접속할때 요청하는 url과 실제 미디어 스트림 연결 역할
        GstRTSPMountPoints* _mountPoint;  
        // rtsp 서버에서 스트리밍 할 미디어 파이프라인을 생성하는 객체
        GstRTSPMediaFactory* _mfactory;   
        // 실행흐름을 잡는 loop
        GMainLoop* _main_loop;
        //  rtsp streamer 파이프라인
        std::string _pipeline;
        //  컨피그 파일
        const config::VideoConfig* _video_config;
    
        static void client_connected(GstRTSPServer* server, GstRTSPClient* client, gpointer user_data);

        static void need_data(GstElement* appsrc, guint size, gpointer udata);
        static void enough_data(GstElement* appsrc, gpointer user_data);
        static void media_unprepared(GstRTSPMedia* media, gpointer user_data);
        static void on_media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data);
    public:
        VideoStreamerGST();
        ~VideoStreamerGST();

        std::string createPipeline(const config::VideoConfig* config);
        void make_server();
        int start_server() override;
        int stop_server() override;
        static gchar* format_location_callback(GstElement* splitmux, guint fragment_id, gpointer user_data);
    }; 
};

#endif 