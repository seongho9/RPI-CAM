#include "video/GstStreamer.hpp"
#include <iostream>
#include <sys/types.h>
#include <ctime>
#include <sstream>
#include "spdlog/spdlog.h"

// format-location-full 콜백 함수
// 새로운 파일이 생성될 때 호출
// 파일 이름 timestamp로 
gchar* format_location_callback(GstElement* splitmux, guint fragment_id, gpointer user_data) {
  
    std::string timestamp = std::to_string(time(nullptr));
    //std::string filename = "video_" + timestamp + ".mp4";
    std::string filename = timestamp+".mp4";
    spdlog::info("Saving new file: {}", filename);

    // 파일 이름을 동적으로 반환
    return g_strdup(filename.c_str());
}

video::GstStreamer::GstStreamer(){
    rtsp_server = nullptr;
    mountPoint = nullptr;
    mfactory = nullptr;
    main_loop = nullptr;
}

video::GstStreamer::~GstStreamer() {
    // 마운트 포인트 객체 참조 해제
    g_object_unref(mountPoint);
    // 미디어 팩토리 객체 참조 해제
    gst_object_unref(mfactory);
    stop();  
}

void video::GstStreamer::make_server(){
        // RTSP 서버 생성
        rtsp_server = gst_rtsp_server_new();
        gst_rtsp_server_set_service(rtsp_server, "8550");
        
        // 메인 루프 생성
        main_loop = g_main_loop_new(NULL, FALSE);

        // 마운트 포인트 생성
        mountPoint = gst_rtsp_server_get_mount_points(rtsp_server);

        // 미디어 팩토리 생성
        mfactory = gst_rtsp_media_factory_new();
        // 미디어 팩토리가 여러 클라이언트에서 공유될 수 있도록 설정
        gst_rtsp_media_factory_set_shared(mfactory, TRUE);
        // 경로를 미디어 팩토리에 추가
        gst_rtsp_mount_points_add_factory(mountPoint, "/test", mfactory);

        //파이프라인 생성
        pipeline = createPipeline(const VideoConfig& config);

}

std::string video::GstStreamer::createPipeline(const VideoConfig& config) {
    
    //pipeline 구성 
    //config 파일에서 직접 가지고 와 사용
    std::stringstream pipeline_stream;
    pipeline_stream << "( libcamerasrc ! "
                    << "video/x-raw,width=" << config.width() 
                    << ",height=" << config.height() 
                    << ",framerate=" << config.frame_rate()
                    << ",format=" << config.format() 
                    << " ! tee name=t ! "
                    << "queue ! videoconvert ! x264enc speed-preset=ultrafast tune=fastdecode ! "
                    << "video/x-h264,profile=high ! rtph264pay config-interval=1 name=pay0 pt=96 "
                    << "t. ! queue ! videoconvert ! x264enc speed-preset=ultrafast tune=fastdecode ! "
                    << "splitmuxsink name=muxsink max-size-time=20000000000 )";

    std::string pipeline_str = pipeline_stream.str();
    return pipeline_str;
}

int video::GstStreamer::start(){
    
    // 서버 활성화
    gst_rtsp_media_factory_set_launch(mfactory, pipeline.c_str());

    if (gst_rtsp_server_attach(rtsp_server, NULL) != TRUE) {
        spdlog::error("Failed to attach RTSP server");
        return -1; 
    }
    spdlog::info("RTSP server running....");

    // 미디어가 준비될 때 splitmuxsink에 콜백을 설정
    g_signal_connect(mfactory, "media-configure", G_CALLBACK(+[](GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data) {
        //media객체에서 파이프라인 가져와
        GstElement* pipeline = gst_rtsp_media_get_element(media);
        //파이프라인에서 muxsink 요소 찾아
        GstElement* muxsink = gst_bin_get_by_name_recurse_up(GST_BIN(pipeline), "muxsink");

        if (muxsink) {
            // splitmuxsink의 format-location-full 시그널에 콜백 연결
            g_signal_connect(muxsink, "format-location-full", G_CALLBACK(format_location_callback), NULL);
            g_object_unref(muxsink);
        }
    }), NULL);

    // 메인 루프 실행
    g_main_loop_run(main_loop);

    return 0;
}

int video::GstStreamer::stop(){
    // 메인 루프 종료
    if (main_loop) {
        g_main_loop_unref(main_loop);
        main_loop = NULL;
    }

    // 서버 종료
    if (rtsp_server) {
        g_object_unref(rtsp_server);
        if (rtsp_server != NULL) {  // 실패 시 확인
            spdlog::error("Failed to unref server");
            return -1;  // 서버 해제 실패 시 오류 코드 반환
        }
        rtsp_server = NULL;
    }

    return 0;
}