#include "rtsp/handler/RTSPSessionHandlerLive.hpp"
#include <gst/gst.h>
#include "spdlog/spdlog.h"
#include "camera_device/VideoQueue.hpp"
#include "config/ProgramConfig.hpp"

#include <gst/app/app.h>

static int frame_cnt=0;

static void play_need_data(GstElement* appsrc, guint size, gpointer user_data);

int MulticastStreamV4L2::set_stream()
{
    const config::VideoConfig* vid_config = config::ProgramConfig::get_instance()->video_config();

    GstElement* pipeline = gst_pipeline_new("streaming_pipeline");
    // src
    GstElement* app_src = gst_element_factory_make("appsrc", "source");
    g_object_set(app_src, 
        "is-live", TRUE, 
        "format", GST_FORMAT_TIME, NULL);
    
    if(_thread_info == nullptr){
        _thread_info = new camera_device::ThreadInfo();
        _thread_info->_is_run = true;
        _thread_info->_fps = 30;
    }

    g_signal_connect(app_src, "need-data", G_CALLBACK(play_need_data), _thread_info);

    //  capsfilter
    GstElement* caps_filter = gst_element_factory_make("capsfilter", "capsfilter");
    GstCaps* caps = gst_caps_new_simple("video/x-raw", 
        "format", G_TYPE_STRING, vid_config->format().c_str(),
        "width", G_TYPE_INT, vid_config->width(), "height", G_TYPE_INT, vid_config->height(),
        "framerate", GST_TYPE_FRACTION, 30, 1,
        NULL);
    g_object_set(caps_filter, "caps", caps, NULL);
    gst_caps_unref(caps);

    GstElement* videoconvert = gst_element_factory_make("videoconvert", "convert");

    GstElement* x264enc = gst_element_factory_make("x264enc", "encoder");
    g_object_set(x264enc, 
        "speed-preset", 1, //  0: none, 1: ultrafast, 2: superfast, 등
        "tune", 1, //   0: none, 1: fastdecode, 2: zerolatency, ...
        NULL);

    GstElement* rtph264pay = gst_element_factory_make("rtph264pay", "payloader");
    g_object_set(rtph264pay, "config-interval", 1, "pt", 96, NULL);

    GstElement* sink = gst_element_factory_make("udpsink", "rtp_sink");
    g_object_set(sink, "host", _ip_addr.c_str(), "port", _port, NULL);

    // 요소들을 링크
    gst_bin_add_many(GST_BIN(pipeline), app_src, caps_filter, videoconvert, x264enc, rtph264pay, sink, NULL);
    if (!gst_element_link_many(app_src, caps_filter, videoconvert, x264enc, rtph264pay, sink, NULL)) {
        spdlog::error("[MulticastStreamV4L2:set_stream] Failed to link elements");
        return -1;
    }
    
    gst_element_set_state(pipeline, GST_STATE_PAUSED);

    _rtp_info->pipeline = pipeline;

    return 0;
}

void play_need_data(GstElement* appsrc, guint size, gpointer user_data)
{

    //  VideoQueue, VideoConfig 객체 가져오기
    camera_device::VideoQueue* video_q = camera_device::VideoQueue::get_instance();
    const config::VideoConfig* video_config = config::ProgramConfig::get_instance()->video_config();
    
    std::string denom;
    size_t idx = video_config->frame_rate().find("/");
    denom.assign(video_config->frame_rate().substr(0, idx));


    guint64 frame_duration = gst_util_uint64_scale(1, GST_SECOND, 30);


    if(!GST_IS_APP_SRC(appsrc)) {
        spdlog::error("[MulticastStrem::make_sdp::need_data] Invalid appsrc element");
        return;
    }

    camera_device::VideoBuffer* mem_buffer = video_q->pop("live");
    if(mem_buffer == nullptr) {
        spdlog::warn("[MulticastStrem::make_sdp::need_data] No buffer at VideoQueue");
        delete mem_buffer;
        return;
    }

    GstBuffer* buffer;
    GstMapInfo map;
    guint buffer_size = mem_buffer->size;
            
    if(mem_buffer->buffer == nullptr){
        spdlog::error("[MulticastStream::make_sdp::need_data] VideoQueue Buffer is NULL");
        delete mem_buffer;
        return;
    }

    buffer = gst_buffer_new_allocate(NULL, buffer_size, NULL);
    if(!buffer) {
        spdlog::error("[MulticastStream::make_sdp::need_data] Failed to allocate GstBuffer");
        delete mem_buffer;
        return;
    }

    if(!gst_buffer_map(buffer, &map, GST_MAP_WRITE)){
        spdlog::error("[MulticastStream::make_sdp::need_data] Failed to map buffer");
        gst_buffer_unref(buffer);
        delete mem_buffer;
        return;
    }

    memcpy(map.data, mem_buffer->buffer, buffer_size);
    gst_buffer_unmap(buffer, &map);

    guint64 timestamp = frame_duration * frame_cnt;
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = frame_duration;
    // push to appsrc
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    if(ret != GST_FLOW_OK) {
        spdlog::error("[MulticastStream::make_sdp::need_data] Failed to push buffer");
        gst_buffer_unref(buffer);
    } else{
        frame_cnt++;
        spdlog::debug("[MulticastStream::make_sdp::need_data] Success to push buffer {} {}", frame_cnt, frame_duration);
    }
    delete mem_buffer;
    
}