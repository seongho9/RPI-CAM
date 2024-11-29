#include "http/HttpInitializer.hpp"
#include "spdlog/spdlog.h"

using namespace http;

static void post_request_complete(void *cls, struct MHD_Connection* conn, void** con_cls, enum MHD_RequestTerminationCode toe)
{
    struct connection_info* con_info = static_cast<struct connection_info*>(*con_cls);
    EventHandlerHTTP* _handler = EventHandlerHTTP::get_instance();

    struct MHD_Response* response = nullptr;
    std::string response_buffer;

    if(con_info == nullptr) {
        return;
    }

    if(con_info->connection_type == HTTP_METHOD::POST) {
        
        // post processor 삭제
        if(con_info->postprocessor != nullptr) {
            MHD_destroy_post_processor(con_info->postprocessor);
            _handler->upload_client_mutex().lock();
            _handler->upload_client()++;
            _handler->upload_client_mutex().unlock();
        }
        if(con_info->file_name == "") {

            response_buffer.assign("filename is empty");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            spdlog::error("filename empty");
            return;
        }
        if(con_info->file_content.empty()) {

            response_buffer.assign("file is empty");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);

            spdlog::error("file empty");
            return;
        }
        if(con_info->fps == ""){

            response_buffer.assign("fps is empty");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);
            MHD_destroy_response(response);
            
            spdlog::error("fps is empty");
            return;
        }
        // 파일 저장
        std::ofstream file(con_info->file_name, std::ios::binary);

        if(file.is_open()) {

            file.write(con_info->file_content.data(), con_info->file_content.size());
            file.close();

            spdlog::info("file {} is saved {} bytes", con_info->file_name, con_info->file_content.size());
            spdlog::info("{} fps {}", con_info->file_name, con_info->fps);

            response_buffer.assign("program file saved");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_OK, response);
            MHD_destroy_response(response);

        }
        else {
            response_buffer.assign("server could not save file");
            response = MHD_create_response_from_buffer(
                response_buffer.size(), (void*)response_buffer.c_str(), MHD_ResponseMemoryMode::MHD_RESPMEM_MUST_COPY);

            MHD_queue_response(conn, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
            MHD_destroy_response(response);
            spdlog::error("File Could not open and save {}", con_info->file_name);
        }
    }
    delete con_info;

    con_cls = nullptr;
}

HttpInitializer::HttpInitializer()
{
    _config = config::ProgramConfig::get_instance()->http_config();
    _daemon = nullptr;
}

static MHD_Result libmicrohttpd_handler(
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
    else {
        spdlog::warn("invalid request {} {}", method, url);
        
        response_buffer.assign("Invalid Request");
        response = MHD_create_response_from_buffer(response_buffer.size(), (void*)(response_buffer.c_str()), MHD_RESPMEM_MUST_COPY);
        
        ret = MHD_queue_response(conn, MHD_HTTP_NOT_FOUND, response);

        MHD_destroy_response(response);
    }
    
    return ret;
}

int HttpInitializer::init()
{
    if(_config->tls_enable()) {
        _daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY,
            8000, NULL, NULL,
            &libmicrohttpd_handler, NULL,
            MHD_OPTION_THREAD_POOL_SIZE, _config->thread_pool(),
            MHD_OPTION_HTTPS_MEM_KEY, _config->private_file(),
            MHD_OPTION_HTTPS_MEM_CERT, _config->crt_file(), 
            MHD_OPTION_NOTIFY_COMPLETED, post_request_complete, NULL,
            MHD_OPTION_END);
    }
    else {
        _daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY,
            8000, NULL, NULL,
            &libmicrohttpd_handler, NULL,
            MHD_OPTION_THREAD_POOL_SIZE, _config->thread_pool(),
            MHD_OPTION_NOTIFY_COMPLETED, post_request_complete, NULL,
            MHD_OPTION_END);
    }

    if(_daemon == nullptr) {
        spdlog::error("Failed to start HTTP server");
        return 1;
    }
    spdlog::info("======================================");
    spdlog::info("HTTP Server is running on PORT {}", 8000);
    spdlog::info("======================================");

    return 0;
}

int HttpInitializer::start()
{
    if(_daemon == nullptr){
        spdlog::error("HTTP server is not running");

        return 1;
    }
    return 0;
}
int HttpInitializer::stop()
{
    MHD_stop_daemon(_daemon);

    return 0;
}