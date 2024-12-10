#include "video/VideoStreamerGST.hpp"
#include "spdlog/spdlog.h"

#include <gst/rtsp-server/rtsp-media.h>
#include <gst/rtsp-server/rtsp-client.h>
#include <gst/rtsp-server/rtsp-context.h>
#include <gst/rtsp-server/rtsp-session.h>

#include <memory>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace video;


// format-location-full 콜백 함수
// 새로운 파일이 생성될 때 호출
// 파일 이름 timestamp로 
gchar* VideoStreamerGST::format_location_callback(GstElement* splitmux, guint fragment_id, gpointer user_data) {
    
    time_t t = time(NULL);
    std::string timestamp = std::to_string(t);
    //std::string filename = "video_" + timestamp + ".mp4";
    std::string filename =  "video/" + timestamp+".mp4";
    spdlog::info("Saving new file: {}", filename);

    // 파일 이름을 동적으로 반환
    return g_strdup(filename.c_str());
}


void VideoStreamerGST::on_media_configure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data)
{
    spdlog::info("media-configure");
    //  rtsp 파이프라인
    GstElement* element = gst_rtsp_media_get_element(media);
    //  appsrc 요소
    GstElement* appsrc = gst_bin_get_by_name_recurse_up(GST_BIN(element), "stream_src");
    //  muxsink 요소 찾아
    GstElement* muxsink = gst_bin_get_by_name_recurse_up(GST_BIN(element), "muxsink");
    
    std::string* uuid = new std::string();

    boost::uuids::random_generator generator;
    boost::uuids::uuid uids = generator();
    uuid->assign(boost::uuids::to_string(uids));
    
    //  VideoQueue 객체
    camera_device::VideoQueue* video_q = camera_device::VideoQueue::get_instance();

    camera_device::ThreadInfo* t_info = new camera_device::ThreadInfo();
    t_info->_fps = 60;
    t_info->_is_run = true;

    struct sess_info* session = new sess_info();
    session->id = uuid;
    session->info = t_info; 
    video_q->insert_event(*uuid, t_info); 
    if(appsrc) {
        // push-data
        g_signal_connect(appsrc, "need-data", G_CALLBACK(VideoStreamerGST::need_data), session);
        g_signal_connect(appsrc, "enough-data", G_CALLBACK(VideoStreamerGST::enough_data), session);

        gst_object_unref(appsrc);
    }
    else{
        
        spdlog::error("Appsrc Not configured");
    }
    if(media) {
        
        g_signal_connect(media, "unprepared", G_CALLBACK(VideoStreamerGST::media_unprepared), session);

    }
    else {
        spdlog::error("RTSPMeida not Configured");
    }

    if(muxsink) {
        // splitmuxsink의 format-location-full 시그널에 콜백 연결
        g_signal_connect(muxsink, "format-location-full", G_CALLBACK(format_location_callback), NULL);

        g_object_unref(muxsink);
    }

    gst_object_unref(element);

}
void VideoStreamerGST::need_data(GstElement* appsrc, guint size, gpointer user_data)
{
    std::thread gst_thread([&](){
        //  VideoQueue 객체
        camera_device::VideoQueue* video_q = camera_device::VideoQueue::get_instance();
        
        if(user_data == nullptr) {
            spdlog::warn("Session Info is nullptr");
            return;
        }
        std::shared_ptr<struct sess_info> info(static_cast<struct sess_info*>(user_data));
        std::string id = *info->id;

        gint64 frame_duration = gst_util_uint64_scale(1, GST_SECOND, 30);
        while(1) {

            if(!GST_IS_APP_SRC(appsrc)) {
                spdlog::error("Invalid appsrc element");
                break;
            }
            //  VideoQueue에서 가져오는 데이터
            camera_device::VideoBuffer* mem_buffer = video_q->pop(id);
            if(mem_buffer == nullptr){
                spdlog::warn("No Buffer at Queue");
                break;
            }

            GstBuffer* buffer;
            GstMapInfo map;
            guint buffer_size = mem_buffer->size;

            buffer = gst_buffer_new_allocate(NULL, mem_buffer->size, NULL);
            if (!buffer) {
                spdlog::error("Failed to allocate GstBuffer");
                break;
            }
            if(!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
                spdlog::error("Failed to map buffer");
                return;
            }
            if(mem_buffer->buffer == nullptr){
                spdlog::error("Buffer is null");
                break;
            }
            memcpy(map.data, mem_buffer->buffer, buffer_size);
            gst_buffer_unmap(buffer, &map);

        
            // timestamp
            gint64 timestamp = frame_duration * (_cnt);
            GST_BUFFER_PTS(buffer) = timestamp;
            GST_BUFFER_DTS(buffer) = timestamp;
            GST_BUFFER_DURATION(buffer) = frame_duration;


            // push data
            if(gst_element_set_state(appsrc, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE){
                spdlog::error("Appsrc Error");
            }
            else{
                GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
                if(ret != GST_FLOW_OK) {
                    spdlog::error("Failed to push buffer");
                } else{
                    _cnt++;
                }
            }
            delete mem_buffer;
        }
        spdlog::info("video streaming thread end");
    });
    gst_thread.detach();

}

void VideoStreamerGST::enough_data(GstElement* appsrc, gpointer user_data)
{

    spdlog::warn("Enough Data to pipeline ");

}

void VideoStreamerGST::client_connected(GstRTSPServer* server, GstRTSPClient* client, gpointer user_data) 
{
    spdlog::info("connected");
}

void VideoStreamerGST::media_unprepared(GstRTSPMedia* media, gpointer user_data)
{
    if(user_data == nullptr){
        return;
    }
    //  VideoQueue 객체
    camera_device::VideoQueue* video_q = camera_device::VideoQueue::get_instance();
    struct sess_info* info = static_cast<struct sess_info*>(user_data);
    std::string sess_id;
    sess_id.assign(info->id->c_str());
    if(info==nullptr){
        spdlog::warn("Session Info is nullptr");
    }

    //  파이프라인 중단
    //  내부 파이프라인 가져오기
    GstElement *pipeline = gst_rtsp_media_get_element(media);
    if (pipeline) {
        // 파이프라인을 NULL 상태로 변경하여 정지
        gst_element_set_state(pipeline, GST_STATE_NULL);
        spdlog::info("Pipeline stopped");

        // 파이프라인 객체의 참조 해제
        gst_object_unref(pipeline);
    }

    video_q->remove_event(*info->id);

    delete info->id;
    delete info->info;

    delete info;
}