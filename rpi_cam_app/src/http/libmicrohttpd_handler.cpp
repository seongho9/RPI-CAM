#include "http/HttpInitializer.hpp"
#include "spdlog/spdlog.h"

MHD_Result http::libmicrohttpd_handler(
    void* cls, struct MHD_Connection*conn, const char* url, const char* method,
    const char* version, const char* upload_data, size_t* data_size, void** con_cls)
{
    
    EventHandlerHTTP* _handler = EventHandlerHTTP::get_instance();
    MHD_Response* response;
    std::string response_buffer;

    MHD_Result ret = MHD_Result::MHD_NO;
    if(!strcmp(url, "/event") && !strcmp(method, "POST")) {
        spdlog::info("/event accpet");
        ret = static_cast<MHD_Result>(_handler->event_accept(conn, upload_data, data_size, con_cls));
    }
    else if(!strcmp(url, "/video")){

        _handler->video_accpet(conn, upload_data, data_size, con_cls);

        spdlog::info("/video accept");

        response_buffer.assign("ok");

        response = MHD_create_response_from_buffer(response_buffer.size(), (void*)(response_buffer.c_str()), MHD_RESPMEM_MUST_COPY);
        ret = MHD_queue_response(conn, MHD_HTTP_OK, response);
        
        MHD_destroy_response(response);
    }
    else if(!strcmp(url, "/program") && !strcmp(method, "POST")) {
        ret =  static_cast<MHD_Result>(_handler->program_accept(conn, upload_data, data_size, con_cls));
    }
    else if(!strcmp(url, "/program/start") && !strcmp(method, "POST")){
        ret = static_cast<MHD_Result>(_handler->program_start_accept(conn, upload_data, data_size, con_cls));
    }
    else if(!strcmp(url, "/program/stop") && !strcmp(method, "POST")) {
        ret = static_cast<MHD_Result>(_handler->program_stop_accept(conn, upload_data, data_size, con_cls));
    }
    else {
        spdlog::warn("invalid request {} {}", method, url);
        
        response_buffer.assign("Invalid Request");
        response = MHD_create_response_from_buffer(response_buffer.size(), (void*)(response_buffer.c_str()), MHD_RESPMEM_MUST_COPY);
        
        ret = MHD_queue_response(conn, MHD_HTTP_NOT_FOUND, response);

        MHD_destroy_response(response);
    }
    
    return ret;
}