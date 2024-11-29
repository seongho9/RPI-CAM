#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>  // std::put_time
#include <ctime>
#include <sys/types.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#define POSTBUFFSIZE 1073741824 // 1GB
#include "http/EventHandlerHTTP.hpp"
#include <iostream>
#include "spdlog/spdlog.h"
#include "config/ProgramConfig.hpp"


using namespace http;

/// @brief "event_id를 가져오기 위한 함수"
/// @param cls event_id를 넘겨주기 위한 변수(char*로 casting하여 사용)
/// @param kid 순회하는 종류로 여기에서는 MHD_ValueKind::MHD_HEADER_KIND
/// @param key key 값
/// @param value value 값
/// @return 만약 iteratoring 중이라면 MHD_NO를 return
static MHD_Result event_header_iter(void* cls, enum MHD_ValueKind kid, const char* key, const char* value)
{
    if(!strcmp(key, "event_id")){
        char* event_id = (char*)cls;

        memcpy(event_id, value, strlen(value)+1);
        spdlog::debug("{}", event_id);

        //  순회 중단
        return MHD_Result::MHD_NO;
    }

    //  순회 지속
     return MHD_Result::MHD_YES;
}

/// @brief event 전송 후 응답을 받기 위한 callback
/// @param contents response 데이터
/// @param size 크기
/// @param nmemb block 개수
/// @param userp 넘겨 받은 변수
/// @return userp byte 크기
static size_t event_send_callback(void* contents, size_t size, size_t nmemb, void *userp)
{
    size_t real_size = size * nmemb;

    char* data = static_cast<char*>(userp);

    memcpy(data, contents, 1024);

    return real_size;
}

/// @brief POST data를 처리하기 위해 사용, key-value 형태로 사용됨
/// @param coninfo_cls 커스텀 값으로, iterate 함수 내에서 사용
/// @param  MHD_ValueKind 값은 종류
/// @param key \0으로 끝나는 key 값
/// @param filename 업로드 된 파일명, 모르면 NULL
/// @param content_type mime-type
/// @param transfer_encoding 데이터 인코딩으로, 모르면 NULL
/// @param data 데이터
/// @param off 데이터 offset
/// @param size 데이터 크기
/// @return MHD_Result::YES : iterating을 지속, MHD_Result::NO : iterationg을 끝냄
static MHD_Result iterate_program_post(void* coninfo_cls, 
    enum MHD_ValueKind kind, const char* key, const char* filename, const char* content_type, const char* transfer_encoding, 
    const char* data, uint64_t off, size_t size)
{
    struct connection_info* con_info = static_cast<struct connection_info*>(coninfo_cls);

    if(!strcmp(key, "name")) {
        con_info->file_name.assign(data, size);
    }
    else if(!strcmp(key, "fps")) {
        con_info->fps.assign(data, size);
    }
    else if(!strcmp(key, "file")) {
        if(off==0){
            con_info->file_content.clear();
        }
        con_info->file_content.insert(con_info->file_content.end(), data, data+size);
    }

    //  프로그램 파일 업로드 완료
    if(size==0){
        spdlog::info("program size : {}", con_info->file_content.size());
        spdlog::info("File upload done");
        return MHD_NO;
    }
    //  프로그램 파일 업로드 진행 중
    else{
        return MHD_YES;
    }

}
EventHandlerHTTP::EventHandlerHTTP()
{
    config::ProgramConfig* whole_config = config::ProgramConfig::get_instance();
    _server_address = whole_config->device_config()->server_address();
    _upload_client = 2;
}

int EventHandlerHTTP::event_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{
    struct MHD_Response* response = nullptr;
    std::string response_buffer;
    MHD_Result ret = MHD_NO;

    char* event_id = new char[POSTBUFFSIZE];

    // 반환 값 순회한 header 값의 개수
    MHD_get_connection_values(conn, MHD_ValueKind::MHD_HEADER_KIND, event_header_iter, event_id);

    // HTTP 헤더에 event_id가 존재하지 않음
    if(event_id == nullptr){
        ret = send_response(conn, response_buffer, MHD_HTTP_BAD_REQUEST);
        con_cls = nullptr;

        return ret;
    }
    // HTTP 헤더에 event_id가 존재
    else {
        spdlog::info("event {} accept", event_id); 

        std::string event;
        event.assign(event_id);

        event += ".mp4";

        // video processing 호출 /////////  
        //                             //
        /////////////////////////////////

        // 이벤트 파일 존재
        if(access(event.c_str(), F_OK) != -1){
            
            response_buffer.assign("success");
            ret = send_response(conn, response_buffer, MHD_HTTP_OK);

            CURL* curl;
            video_send(curl, event.c_str(), event_id);
        }
        // 이벤트 파일 없음
        else{
            spdlog::error("event file {} is not exist", event);

            response_buffer.assign("event video is not exist");
            ret = send_response(conn, response_buffer, MHD_HTTP_NOT_FOUND);
        }
    }
    con_cls = nullptr;
    return ret;
}

int EventHandlerHTTP::video_accpet(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{
    return 0;
}

int EventHandlerHTTP::program_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{

    struct connection_info* con_info;

    if(*con_cls == nullptr) {
        spdlog::info("/program accept");
        con_info = nullptr;

        _upload_client_mutex.lock();
        if(_upload_client <= 0) {
            _upload_client_mutex.unlock();
            spdlog::info("program upload busy");

            return send_response(conn, "program upload busy, try again later", MHD_HTTP_SERVICE_UNAVAILABLE);
        }
        _upload_client--;
        _upload_client_mutex.unlock();

        con_info = new connection_info();
        if(con_info == nullptr) {
            _upload_client_mutex.lock();
            _upload_client++;
            _upload_client_mutex.unlock();
            spdlog::error("failed to create post infomation");
            return send_response(conn, "Internanl Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        con_info->postprocessor=nullptr;
        con_info->file_content.clear();
        con_info->file_name.assign("");
        con_info->fps.assign("");
        con_info->upload_done = false;


        con_info->postprocessor = MHD_create_post_processor(conn, POSTBUFFSIZE, iterate_program_post, (void *)con_info);
        
        if(con_info->postprocessor == 0x0) {
            delete con_info;
            spdlog::error("failed to create post processor");
            spdlog::debug("postprocessor addr {}", (void*)con_info->postprocessor);
            spdlog::debug("{} {}", (void*)nullptr, (void*)NULL);
            return send_response(conn, "Internanl Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }


        con_info->connection_type = HTTP_METHOD::POST;
        
        *con_cls = con_info;

        return MHD_Result::MHD_YES;
    }
    else {
        con_info = static_cast<struct connection_info*>(*con_cls);
        if(*size != 0) {
            spdlog::debug("data size {}", *size);
            MHD_post_process(con_info->postprocessor, data, *size);
            *size = 0;
            return MHD_Result::MHD_YES;
        }
        else {
            con_info->upload_done = true;
            spdlog::debug("data size 0");

            if(con_info->file_name == "" || con_info->file_content.empty() || con_info->fps == ""){
                spdlog::info("Bad Request");

                send_response(conn, "bad request", MHD_HTTP_BAD_REQUEST);

                return MHD_Result::MHD_YES;
            }
            
            if(con_info->upload_done) {
                send_response(conn, "program file saved", MHD_HTTP_OK);
            }
            return MHD_Result::MHD_YES;
        }
    }
}

int EventHandlerHTTP::event_send(CURL* curl, const char* event_name, time_t time)
{
    CURLcode res;

    std::tm* tm;

    std::stringstream payload_stream;
    std::stringstream time_stream;

    boost::property_tree::ptree payload_tree;

    std::string request_url = _server_address + "/event";
    std::string request;

    tm = std::localtime(&time);

    time_stream << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");

    payload_tree.put("description ", "hello world");
    payload_tree.put("time", time);
    payload_tree.put("localtime", time_stream.str());

    // payload 설정
    boost::property_tree::write_json(payload_stream, payload_tree);
    request = payload_stream.str();

    spdlog::debug("request url : {}", request_url);
    spdlog::debug("request payload : {}", request);

    return send_request(curl, request_url, request);
}

int EventHandlerHTTP::video_send(CURL* curl, const char* path, const char* event_id)
{
   CURLcode res;

    std::string request_url = _server_address + "/video";

    curl_mime* form = nullptr;
    curl_mimepart* field = nullptr;

    struct curl_slist* header_list = nullptr;
    
    curl = curl_easy_init();
    if(curl) {
        //  create form
        form = curl_mime_init(curl);
        
        // event_id part
        field = curl_mime_addpart(form);
        curl_mime_name(field, "event_id");
        curl_mime_data(field, event_id, CURL_ZERO_TERMINATED);

        // video data part
        field = curl_mime_addpart(form);
        curl_mime_name(field, "data");
        curl_mime_filedata(field, path);
        curl_mime_type(field, "video/mp4");

        curl_easy_setopt(curl, CURLOPT_URL, request_url);
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            spdlog::error("event {} video send failed", event_id);

            curl_easy_cleanup(curl);
            curl_mime_free(form);
            return 1;
        }
        else{
            curl_easy_cleanup(curl);
            curl_mime_free(form);

            return 0;            
        }
    }
    return 0;
}

int EventHandlerHTTP::camerainfo_send(CURL* curl, const std::string& uuid)
{    
   CURLcode res;

    std::stringstream payload_stream;

    boost::property_tree::ptree payload_tree;
    boost::property_tree::ptree event_group_tree;

    std::string request_url = _server_address + "/info";
    std::string request;
    char* response;

    payload_tree.put("cam_id", uuid);

    boost::property_tree::write_json(payload_stream, payload_tree);
    request = payload_stream.str();
    
    spdlog::debug("request url {}", request_url);
    spdlog::debug("request payload: {} ", request);
    
    return send_request(curl, request_url, request);
}

int EventHandlerHTTP::send_request(CURL* curl, const std::string& url, const std::string& payload)
{
    CURLcode res;
    char response[1024];
    curl = curl_easy_init();
    if(curl) {
        // 요청 url
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // response 처리
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, event_send_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // user-agent 설정
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());

        // 전송
        res = curl_easy_perform(curl);

        if(res != CURLE_OK){
            spdlog::error("event send faield : {}", curl_easy_strerror(res));
            spdlog::error("error message : {}", response);

            curl_easy_cleanup(curl);
            return 2;
        }
        else {
            spdlog::info("event send : {}", response);

            curl_easy_cleanup(curl);
            return 0;
        }
    }   
    else {
        spdlog::error("failed to create curl object");
        curl_easy_cleanup(curl);
        return 1;
    }
}

MHD_Result EventHandlerHTTP::send_response(MHD_Connection* conn, const std::string& payload, int status)
{
    MHD_Response* response = nullptr;
    MHD_Result ret = MHD_NO;
    response = MHD_create_response_from_buffer(payload.size(), (void*)payload.c_str(), MHD_RESPMEM_MUST_COPY);

    if(response == nullptr) {
        return ret;
    }

    ret = MHD_queue_response(conn, status, response);

    if(ret == MHD_Result::MHD_YES) {
        MHD_destroy_response(response);
        return ret;
    }
    else {
        MHD_destroy_response(response);
        spdlog::error("HTTP response send failed");
        return ret;
    }
}

std::mutex& EventHandlerHTTP::upload_client_mutex()
{
    return _upload_client_mutex;
}
int& EventHandlerHTTP::upload_client()
{
    return _upload_client;
}
