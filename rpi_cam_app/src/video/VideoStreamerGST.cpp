#include "video/VideoStreamerGST.hpp"
#include "config/ProgramConfig.hpp"
#include <iostream>
#include <sys/types.h>
#include <ctime>
#include <sstream>
#include "spdlog/spdlog.h"
using namespace video;


VideoStreamerGST::VideoStreamerGST() : _video_config(){
    _rtsp_server = nullptr;
    _mountPoint = nullptr;
    _mfactory = nullptr;
    _main_loop = nullptr;
    _video_config = config::ProgramConfig::get_instance()->video_config();
}

VideoStreamerGST::~VideoStreamerGST() {
    // 마운트 포인트 객체 참조 해제
    g_object_unref(_mountPoint);
    // 미디어 팩토리 객체 참조 해제
    gst_object_unref(_mfactory);
    stop_server();  
}


std::string VideoStreamerGST::createPipeline(const config::VideoConfig* config) {

    spdlog::info("{}, {}, {}, {}, {}", config->width(), config->height(),config->frame_rate(), config->format(), config->split_time());
    
    //pipeline 구성 
    std::stringstream pipeline_stream;

    pipeline_stream << "( appsrc name=stream_src is-live=true format=time "
                    << "caps=video/x-raw,"
                    << "width=" << config->width() << ",height=" << config->height() 
                    << ",framerate=" << config->frame_rate() << ",format=" << config->format()   
                    << " ! tee name=t ! "
                    << "queue ! videoconvert ! x264enc speed-preset=ultrafast tune=fastdecode ! "
                    << "video/x-h264,profile=high ! rtph264pay config-interval=1 name=pay0 pt=96 "
                    << "t. ! queue ! videoconvert ! x264enc speed-preset=ultrafast tune=fastdecode ! "
                    << "splitmuxsink name=muxsink max-size-time="<< config->split_time()
                    << " )";
    std::string pipeline_str = pipeline_stream.str();
    spdlog::info("pipeline {}", pipeline_stream.str());
    return pipeline_str;
}

void VideoStreamerGST::make_server(){
        // RTSP 서버 생성
        _rtsp_server = gst_rtsp_server_new();
        gst_rtsp_server_set_service(_rtsp_server, "8550");
        
        // 메인 루프 생성
        _main_loop = g_main_loop_new(NULL, FALSE);

        // 마운트 포인트 생성
        _mountPoint = gst_rtsp_server_get_mount_points(_rtsp_server);

        // 미디어 팩토리 생성
        _mfactory = gst_rtsp_media_factory_new();
        // 미디어 팩토리가 여러 클라이언트에서 공유될 수 있도록 설정
        gst_rtsp_media_factory_set_shared(_mfactory, TRUE);
        // 경로를 미디어 팩토리에 추가
        gst_rtsp_mount_points_add_factory(_mountPoint, "/snackticon", _mfactory);
        //파이프라인 생성
        _pipeline = createPipeline(_video_config);

}

int VideoStreamerGST::start_server()
{   
    make_server();
    // 서버 활성화
    gst_rtsp_media_factory_set_launch(_mfactory, _pipeline.c_str());
    if (gst_rtsp_server_attach(_rtsp_server, NULL) != TRUE) {
        spdlog::error("Failed to attach RTSP server");
        return -1; 
    }
    spdlog::info("RTSP server running....");
    
    g_signal_connect(_mfactory, "media-configure", G_CALLBACK(VideoStreamerGST::on_media_configure), NULL);
    g_signal_connect(_rtsp_server, "client-connected", G_CALLBACK(VideoStreamerGST::client_connected), NULL);
    // 메인 루프 실행
    g_main_loop_run(_main_loop);

    return 0;
}

int VideoStreamerGST::stop_server(){
    // 메인 루프 종료
    if (_main_loop) {
        g_main_loop_unref(_main_loop);
        _main_loop = NULL;
    }

    // 서버 종료
    if (_rtsp_server) {
        g_object_unref(_rtsp_server);
        if (_rtsp_server != NULL) {  // 실패 시 확인
            spdlog::error("Failed to unref server");
            return -1;  // 서버 해제 실패 시 오류 코드 반환
        }
        _rtsp_server = NULL;
    }
    spdlog::info("RTSP server stop");

    return 0;
}