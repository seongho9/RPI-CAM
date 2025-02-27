#include "rtsp/handler/RTSPSessionHandlerLive.hpp"
#include "spdlog/spdlog.h"

#include "camera_device/VideoQueue.hpp"
#include "config/ProgramConfig.hpp"

#include <gst/app/app.h>
#include <gst/sdp/gstsdpmessage.h>

extern std::string ip_addr;

static int cnt = 0;
/// @brief appsink에서 사용하는 콜백으로 sdp추출 및 rtp 송신을 담당
/// @param appsink appsink element
/// @param user_data 코덱 정보를 반환
/// @return GstFLowREturn
static GstFlowReturn multicast_sdp_new_sample_callback(GstAppSink *appsink, gpointer user_data);

static void sdp_need_data(GstElement* appsrc, guint size, gpointer user_data);

int MulticastStreamV4L2::make_sdp(std::string& sdp_str)
{
    if(_codec_info == nullptr) {
        _codec_info = new H264CodecInfo();
    }
    camera_device::VideoQueue* video_q = camera_device::VideoQueue::get_instance();

    const config::VideoConfig* vid_config = config::ProgramConfig::get_instance()->video_config();
    if(_sdp_str != "") {
        sdp_str.assign(_sdp_str);
        return 0;
    }

    GstElement* pipeline = gst_pipeline_new("streaming_pipeline");
    // src
    GstElement* app_src = gst_element_factory_make("appsrc", "source");
    g_object_set(app_src, 
        "is-live", TRUE, 
        "format", GST_FORMAT_TIME, NULL);
    camera_device::ThreadInfo* t_info = new camera_device::ThreadInfo();
    t_info->_is_run = true;
    t_info->_fps = 30;


    g_signal_connect(app_src, "need-data", G_CALLBACK(sdp_need_data), t_info);

    std::string denom, dom;
    size_t idx = vid_config->frame_rate().find("/");
    denom.assign(vid_config->frame_rate().substr(0, idx));
    idx = vid_config->frame_rate().find("/");
    dom.assign(vid_config->frame_rate().substr(idx+1, 1));
    //  capsfilter
    GstElement* caps_filter = gst_element_factory_make("capsfilter", "capsfilter");
    GstCaps* caps = gst_caps_new_simple("video/x-raw", 
        "format", G_TYPE_STRING, "YUY2",
        "width", G_TYPE_INT, 640, "height", G_TYPE_INT, 480,
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

    GstElement* sink = gst_element_factory_make("appsink", "rtp_sink");
    g_object_set(sink, "emit-signals", TRUE, NULL);
    g_signal_connect(sink, "new-sample", G_CALLBACK(multicast_sdp_new_sample_callback), _codec_info);
    // 요소들을 링크
    gst_bin_add_many(GST_BIN(pipeline), app_src, caps_filter, videoconvert, x264enc, rtph264pay, sink, NULL);
    if (!gst_element_link_many(app_src, caps_filter, videoconvert, x264enc, rtph264pay, sink, NULL)) {
        std::cerr << "요소들을 연결하는 데 실패했습니다." << std::endl;
        return -1;
    }

    
    video_q->insert_event("sdp", t_info);
    //  pipeline 실행
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    //  pipeline 실행을 잡아두기 위한 GstBus 수행
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = nullptr;
    while(1) {
        msg = gst_bus_timed_pop_filtered(bus, 500 * GST_MSECOND, 
            static_cast<GstMessageType>(GST_MESSAGE_ERROR|GST_MESSAGE_EOS));
        if(msg != nullptr) {
            if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
                spdlog::info("[RTSPFileSessionGst:describe_request] GstPipeline reach end of stream");
                gst_message_unref(msg);
    
                break;
            }
            else if(GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                GError* err = nullptr;
                gchar* debug_info = nullptr;
                gst_message_parse_error(msg, &err, &debug_info);
    
                spdlog::error("[RTSPFileSessionGst:describe_request] GstPipeline Error {} {}", err->code, err->message);
                g_free(debug_info);
                g_clear_error(&err);
                gst_message_unref(msg);
                    
                break;
            }
            gst_message_unref(msg);
        }
    }
    spdlog::debug("sdp bus end");
    gst_object_unref(bus);
    
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    video_q->remove_event("sdp");
    GstSDPMessage* sdp_msg = nullptr;
    gst_sdp_message_new(&sdp_msg);

    //  v=0
    gst_sdp_message_set_version(sdp_msg, 0);
    //  o= - <sess_id> <sess_ver> <net_type> <addr_type> <adddr>
    //  시간 기반 sess_id
    uint64_t sess_id = static_cast<uint64_t>(time(NULL));
    uint64_t sess_ver = 0;
    gst_sdp_message_set_origin(sdp_msg, 
        "-", std::to_string(sess_id).c_str(), std::to_string(sess_ver).c_str(),
        "IN", "IP4", ip_addr.c_str());
    //  s= <sess_name>
    gst_sdp_message_set_session_name(sdp_msg, "Hydra RTSP Session");
    
    //  t=
    //  생략.. t= 0 0

    //  c= <net_type> <addr_type> <addr>/<subnet_mask>
    //  multicast용 IGMP ip 주소
    gst_sdp_message_set_connection(sdp_msg, "IN", "IP4", _ip_addr.c_str(), 32, 0);
    //  m= & a= 
    GstSDPMedia* media;
    gst_sdp_media_new(&media);

    //  m= <media> <port> <proto> <format>
    gst_sdp_media_set_media(media, "video");
    gst_sdp_media_set_port_info(media, 0, 1);
    gst_sdp_media_set_proto(media, "RTP/AVP");
    //  format
    if(_codec_info->encoding_name == "H264") {
        gst_sdp_media_add_format(media,"96");

        //  a=fmtp:<format> packetization-mode=*;sprop-parameter-sets=<SPS,PPS>;profile-level-id=<profile>;
        std::string fmtp_value;
        fmtp_value.assign("96");
        fmtp_value.append(" packetization-mode=");
        fmtp_value.append(_codec_info->packetization_mode);
        fmtp_value.append(";sprop-parameter-sets=");
        fmtp_value.append(_codec_info->sprop_param);
        fmtp_value.append(";profile-level-id=");
        fmtp_value.append(_codec_info->profile_level);
        gst_sdp_media_add_attribute(media, "fmtp", fmtp_value.c_str());

        //  a=rtpmap:96 H264/90000 
        std::string rtpmap_value;
        rtpmap_value.assign("96 ");
        rtpmap_value.append(_codec_info->encoding_name);
        rtpmap_value.append("/");
        rtpmap_value.append(std::to_string(_codec_info->clock_rate));
        gst_sdp_media_add_attribute(media, "rtpmap", rtpmap_value.c_str());
    }

    //  a=framerate:<fps>
    gst_sdp_media_add_attribute(media, "framerate", _codec_info->framerate.c_str());\
    //  설정한 media 테그 정보를 sdp에 추가
    gst_sdp_message_add_media(sdp_msg, media);

    _sdp_str.assign(gst_sdp_message_as_text(sdp_msg));

    sdp_str.assign(_sdp_str);

    return 0;
}


GstFlowReturn multicast_sdp_new_sample_callback(GstAppSink *appsink, gpointer user_data)
{

    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (!sample) {
        spdlog::error("[RTSPFileSessionGst:describe_request:new_sample_callback] Failed to pull sample from appsink");
        return GST_FLOW_ERROR;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    if(!caps) {
        spdlog::error("[RTSPFileSessionGst:describe_request:new_sample_callback] Failed to get caps from sample");
    }

    GstStructure* structure = gst_caps_get_structure(caps, 0);
    if(!structure) {
        spdlog::error("[RTSPFileSessionGst:describe_request:new_sample_callback] Failed to get structure from sample");
    }
    
    
    if(user_data == nullptr) {
        spdlog::error("[RTSPFileSessionGst:describe_request:new_sample_callback] user_data is null");
        return GST_FLOW_ERROR;
    }

    H264CodecInfo* codec_info = static_cast<H264CodecInfo*>(user_data);

    if (gst_structure_has_field(structure, "encoding-name")) {
        const gchar* encoding_name = gst_structure_get_string(structure, "encoding-name");
        spdlog::debug("encoding name : {}", encoding_name);
        if(encoding_name) {
            codec_info->encoding_name.assign(encoding_name);
        }
        else {
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] encoding_name is null");
        }
    }

    if (gst_structure_has_field(structure, "sprop-parameter-sets")) {
        const gchar* sprop_param = gst_structure_get_string(structure, "sprop-parameter-sets");
        spdlog::debug("sprop_param : {}", sprop_param);
        if(sprop_param) {
            codec_info->sprop_param.assign(sprop_param);
        }
        else {
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] sprop_parameter-sets is null");
        }
    }

    if (gst_structure_has_field(structure, "clock-rate")) {
        gint c_rate;
        spdlog::debug("clock rate : {}", c_rate);
        if(gst_structure_get_int(structure, "clock-rate", &c_rate)){
            codec_info->clock_rate = static_cast<uint32_t>(c_rate);
        }
        else{
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] Failed to get clock-rate");
        }

    }

    if (gst_structure_has_field(structure, "ssrc")) {
        guint ssrc;
        spdlog::debug("ssrc : {}", ssrc);
        if(gst_structure_get_uint(structure, "ssrc", &ssrc)) {
            codec_info->ssrc = static_cast<uint32_t>(ssrc);
        }
        else{
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] Failed to get ssrc");
        }
    }

    if (gst_structure_has_field(structure, "a-framerate")) {
        const gchar* framerate = nullptr;
        spdlog::debug("framerate : {}", framerate);
        framerate = gst_structure_get_string(structure, "a-framerate");
        if(framerate){
            codec_info->framerate.assign(framerate);
        }
        else{
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] a-framerate is null");
        }
    }

    if (gst_structure_has_field(structure, "profile-level-id")) {
        const gchar* profile_level = gst_structure_get_string(structure, "a-framerate");
        spdlog::debug("profile_level : {}", profile_level);
        if(profile_level){
            codec_info->profile_level.assign(profile_level);
        }
        else{
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] profile-level-id is null");
        }
    }

    if (gst_structure_has_field(structure, "packetization-mode")) {
        const gchar* packetization = gst_structure_get_string(structure, "packetization-mode");
        spdlog::debug("packetization-mode : {}", packetization);
        if(packetization){
            codec_info->packetization_mode.assign(packetization);
        }
        else{
            spdlog::warn("[RTSPFileSessionGst:describe_request:new_sample_callback] packetization-mode is null");
        }
    }

    gst_sample_unref(sample);

    bool flag = 
        codec_info->encoding_name.empty()  ||  codec_info->clock_rate == 0            ||
        codec_info->framerate.empty()      ||  codec_info->packetization_mode.empty() ||
        codec_info->profile_level.empty()  ||  codec_info->sprop_param.empty()        ||
        codec_info->ssrc == 0;
    if(flag) {
        spdlog::info("More Info need");
    }
    else{
        spdlog::info("Info done");
        return GST_FLOW_EOS;
    }

    return GST_FLOW_OK;
}

void sdp_need_data(GstElement* appsrc, guint size, gpointer user_data)
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

    camera_device::VideoBuffer* mem_buffer = video_q->pop("sdp");
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

    guint64 timestamp = frame_duration * cnt;
    GST_BUFFER_PTS(buffer) = timestamp;
    GST_BUFFER_DTS(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = frame_duration;
    // push to appsrc
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), buffer);
    if(ret != GST_FLOW_OK) {
        spdlog::error("[MulticastStream::make_sdp::need_data] Failed to push buffer");
    } else{
        cnt++;
        spdlog::debug("[MulticastStream::make_sdp::need_data] Success to push buffer {} {}", cnt, frame_duration);
    }
    delete mem_buffer;
    
}