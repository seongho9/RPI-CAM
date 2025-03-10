#include "spdlog/spdlog.h"

#include "rtsp/handler/RTSPSessionHandler.hpp"

int RTSPSessionHandlerGstFile::teardown_request()
{
    GstStateChangeReturn ret;

    ret = gst_element_set_state(_rtp_info->pipeline, GST_STATE_NULL);
    if(ret == GST_STATE_CHANGE_FAILURE) {
        spdlog::error("[RTSPFileSessionGst:teardown_request] failed to close Gstreamer Pipeline");

        return 1;
    }
    _rtp_info->session_info->rtp_socket->close();
    _rtp_info->session_info->rtcp_socket->close();

    gst_object_unref(_rtp_info->pipeline);

    delete _rtp_info;
    _rtp_info = nullptr;
    
    return 0;
}