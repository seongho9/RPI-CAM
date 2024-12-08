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

EventHandlerHTTP::EventHandlerHTTP()
{
    config::ProgramConfig* whole_config = config::ProgramConfig::get_instance();
    _server_address = whole_config->http_config()->remote_server();
    _upload_client = whole_config->http_config()->upload_user();

    
        std::ifstream file("system/uuid");
        std::getline(file, _uuid);

        file.close();

}

/// @brief "event_id를 가져오기 위한 함수"
/// @param cls event_id를 넘겨주기 위한 변수(char*로 casting하여 사용)
/// @param kid 순회하는 종류로 여기에서는 MHD_ValueKind::MHD_HEADER_KIND
/// @param key key 값
/// @param value value 값
/// @return 만약 iteratoring 중이라면 MHD_NO를 return
static MHD_Result event_header_iter(void* cls, enum MHD_ValueKind kid, const char* key, const char* value)
{
    struct event_info* info = static_cast<struct event_info*>(cls);
    struct tm tm = {0};
    if(!strcmp(key, "transactionId")){
        memcpy(info->id, value, strlen(value)+1);
    }
    else if(!strcmp(key, "event_time")){
        info->time_stamp = static_cast<time_t>(std::strtoll(value, nullptr, 10));
    }

    if(strlen(info->id)!=0 && info->time_stamp != 0){
        return MHD_Result::MHD_NO;
    }

    //  순회 지속
     return MHD_Result::MHD_YES;
}

int EventHandlerHTTP::event_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{
    struct MHD_Response* response = nullptr;
    std::string response_buffer;
    MHD_Result ret = MHD_NO;

    struct event_info event;
    event.time_stamp = 0;
    event.id = new char[POSTBUFFSIZE];
    memset(event.id, '\0', POSTBUFFSIZE);

    // 반환 값 순회한 header 값의 개수
    MHD_get_connection_values(conn, MHD_ValueKind::MHD_HEADER_KIND, event_header_iter, static_cast<void*>(&event));

    // HTTP 헤더에 event_id가 존재하지 않음
    if(strlen(event.id)==0){
        response_buffer.assign("id is empty");
        ret = send_response(conn, response_buffer, MHD_HTTP_BAD_REQUEST);
        con_cls = nullptr;

        delete event.id;

        return ret;
    }
    else if(event.time_stamp == 0){
        response_buffer.assign("time is empty");
        ret = send_response(conn, response_buffer, MHD_HTTP_BAD_REQUEST);
        con_cls = nullptr;

        delete event.id;

        return ret;
    }
    // HTTP 헤더에 event_id가 존재
    else {
        spdlog::info("event {} accept", std::string(event.id)); 

        std::string event_str;
        event_str.assign(event.id);

        int vid_ret = _video_handler->process_video(event.time_stamp, event.id);

        if(vid_ret != 0) {
            response_buffer.assign("Internal Server Error");
            ret = send_response(conn, response_buffer, MHD_HTTP_INTERNAL_SERVER_ERROR);

            delete event.id;

            return ret;
        }
        std::string vid_path;
        // 이벤트 영상 파일 있음
        if(_video_handler->get_video(vid_path, event.id, event.time_stamp) == 0){
            spdlog::debug("vid_path {}", vid_path);
            std::string event_id;
            event_id.assign(event.id);
            std::thread curl_thread = std::thread([=](){
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                spdlog::debug("in curl thread");
                CURL* curl;
                http::EventHandlerHTTP::get_instance()->video_send(curl, vid_path.c_str(), event_id.c_str());
            });
            curl_thread.detach();
            //  response로 있음을 알리고
            response_buffer.assign("success");
            ret = send_response(conn, response_buffer, MHD_HTTP_OK);
        }
        // 이벤트 영상 파일 없음
        else{
            spdlog::error("event file {} is not exist", event.id);

            response_buffer.assign("event video is not exist");
            ret = send_response(conn, response_buffer, MHD_HTTP_NOT_FOUND);
        }
    }
    con_cls = nullptr;

    delete event.id;

    return ret;
}

int EventHandlerHTTP::video_accpet(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{
    return 0;
}

/// @brief 프로그램의 내용을 받아오기 위함, key-value 형태로 사용됨
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

    //  파일명
    if(!strcmp(key, "name")) {
        con_info->file_name.assign(data, size);
    }
    //  초당 프레임
    else if(!strcmp(key, "fps")) {
        con_info->fps.assign(data, size);
    }
    //  파일 내용
    else if(!strcmp(key, "file")) {
        if(off==0){
            con_info->file_content.clear();
        }
        con_info->file_content.insert(con_info->file_content.end(), data, data+size);
    }

    //  multipart 파일 순회 완료
    if(size==0){
        spdlog::info("program size : {}", con_info->file_content.size());
        spdlog::info("multipart iteratino done");
        return MHD_NO;
    }
    //  프로그램 파일 업로드 진행 중
    else{
        return MHD_YES;
    }
}

int EventHandlerHTTP::program_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{

    struct connection_info* con_info;

    //  첫 요청인 경우
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

        //  multipart/form-data 파일 순회를 위한 processor function을 통해 받아올 객체 생성
        con_info = new connection_info();
        if(con_info == nullptr) {
            _upload_client_mutex.lock();
            _upload_client++;
            _upload_client_mutex.unlock();
            spdlog::error("failed to create post infomation");
            return send_response(conn, "Internanl Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        else {
            con_info->postprocessor=nullptr;
            con_info->file_content.clear();
            con_info->file_name.assign("");
            con_info->fps.assign("");
            con_info->upload_done = false;
        }
        //  multipart/form-data 순회를 위한 processor function 생성
        con_info->postprocessor = MHD_create_post_processor(conn, POSTBUFFSIZE, iterate_program_post, (void *)con_info);
        //  생성 실패
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
    //  multipart/form-data 요청을 받는 중인 경우
    else {
        con_info = static_cast<struct connection_info*>(*con_cls);
        //  data를 가져오는 processor function 수행
        if(*size != 0) {
            MHD_post_process(con_info->postprocessor, data, *size);
            *size = 0;
            return MHD_Result::MHD_YES;
        }
        // multipart/form-data 요청이 끝난 경우
        else {
            con_info->upload_done = true;

            if(con_info->file_name.find("/") != con_info->file_name.npos){
                spdlog::error("Invalid file name");

                send_response(conn, "Invalid file name", MHD_HTTP_BAD_REQUEST);
                return MHD_Result::MHD_YES;
            }

            try {
                int fps = std::stoi(con_info->fps);
                //  만약 시스템 설정 fps값을 넘어서면 error 호출을 추가 할거면 추가할 것

            }
            //  fps 값이 올바르지 않음
            catch(std::invalid_argument& ex){
                spdlog::error("Invalid fps value");

                send_response(conn, "Invalid fps value", MHD_HTTP_BAD_REQUEST);
                return MHD_Result::MHD_YES;
            }

            //  모든 값이 존재하지 않음
            if(con_info->file_name == "" || con_info->file_content.empty() || con_info->fps == ""){
                spdlog::info("Some element is empty");

                send_response(conn, "bad request", MHD_HTTP_BAD_REQUEST);
                return MHD_Result::MHD_YES;
            }
            
            //  저장 성공
            if(con_info->upload_done) {
                send_response(conn, "program file saved", MHD_HTTP_OK);
            }

            return MHD_Result::MHD_YES;
        }
    }
}

/// @brief 시작할 프로그램의 이름을 받기위함
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
static MHD_Result iterate_get_filename(void* coninfo_cls, 
    enum MHD_ValueKind kind, const char* key, const char* filename, const char* content_type, const char* transfer_encoding, 
    const char* data, uint64_t off, size_t size)
{
    struct connection_info* con_info = static_cast<struct connection_info*>(coninfo_cls);
    //  파일명
    if(!strcmp(key, "name")) {
        con_info->file_name.assign(data);
    }
    //  순회 완료
    if(size==0){
        spdlog::info("post iteration done");
        return MHD_NO;
    }
    //  찾는 중
    else{
        return MHD_YES;
    }
}

int EventHandlerHTTP::program_start_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{
    struct connection_info* con_info;
    // POST 요청 첫 시작
    if(*con_cls == nullptr){
        spdlog::info("/program/start accpet");
        con_info = nullptr;
        con_info = new connection_info();

        if(con_info == nullptr) {
            spdlog::error("failed to create post infomation");
            return send_response(conn, "Internal Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        else {
            con_info->postprocessor = nullptr;
            con_info->file_name.clear();
        }
        con_info->postprocessor = MHD_create_post_processor(conn, POSTBUFFSIZE, iterate_get_filename, (void*)con_info);

        if(con_info->postprocessor == 0x0) {
            delete con_info;
            spdlog::error("failed to create post processor");

            return send_response(conn, "Internal Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        *con_cls = con_info;

        return MHD_Result::MHD_YES;
    }
    else{
        con_info = static_cast<struct connection_info*>(*con_cls);
        // 요청 진행 중
        if(*size != 0) {
            MHD_post_process(con_info->postprocessor, data, *size);
            *size = 0;
            return MHD_Result::MHD_YES;
        }
        // 요청 끝
        else {
            _event_handler->start_event(con_info->file_name);

            return MHD_Result::MHD_YES;
        }
    }
}

int EventHandlerHTTP::program_stop_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls)
{
    struct connection_info* con_info;
    // POST 요청 첫 시작
    if(*con_cls == nullptr){
        spdlog::info("/program/stop accpet");
        con_info = nullptr;
        con_info = new connection_info();

        if(con_info == nullptr) {
            spdlog::error("failed to create post infomation");
            return send_response(conn, "Internal Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }
        else {
            con_info->postprocessor = nullptr;
            con_info->file_name.clear();
        }
        con_info->postprocessor = MHD_create_post_processor(conn, POSTBUFFSIZE, iterate_get_filename, (void*)con_info);

        if(con_info->postprocessor == 0x0) {
            delete con_info;
            spdlog::error("failed to create post processor");

            return send_response(conn, "Internal Server Error", MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        *con_cls = con_info;

        return MHD_Result::MHD_YES;
    }
    else{
        con_info = static_cast<struct connection_info*>(*con_cls);
        // 요청 진행 중
        if(*size != 0) {
            MHD_post_process(con_info->postprocessor, data, *size);
            *size = 0;
            return MHD_Result::MHD_YES;
        }
        // 요청 끝
        else {
            _event_handler->stop_event(con_info->file_name);

            return MHD_Result::MHD_YES;
        }
    }
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


int EventHandlerHTTP::event_send(CURL* curl, const char* event_name, time_t time)
{
    CURLcode res;


    std::stringstream payload_stream;
    std::stringstream time_stream;

    boost::property_tree::ptree payload_tree;

    std::string request_url = _server_address + "/event";
    std::string request;

    //  unix 시간을 현지 시각으로 변경
    std::tm* tm;
    tm = std::localtime(&time);
    time_stream << std::put_time(tm, "%Y-%m-%d %H:%M:%S");

    // json 객체 생성
    payload_tree.put("description", _uuid);
    payload_tree.put("unixtime", time);
    payload_tree.put("localtime", time_stream.str());

    // json 직렬화
    boost::property_tree::write_json(payload_stream, payload_tree);
    request = payload_stream.str();

    spdlog::debug("request url : {}", request_url);
    spdlog::debug("request payload : {}", request);

    curl = curl_easy_init();
    if(curl) {
        // POST 설정
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // Content-type 및 https 설정
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        // 헤더 설정
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        return send_request(curl, request_url, request);
    }
    else {
        spdlog::error("Error : Failed to Create CURL Object");

        return 1;
    }
}

int EventHandlerHTTP::video_send(CURL* curl, const char* path, const char* event_id)
{
    CURLcode res;

    std::string request_url = _server_address + "/file";

    curl_mime* form = nullptr;
    curl_mimepart* field = nullptr;

    struct curl_slist* header_list = nullptr;
    
    curl = curl_easy_init();
    if(curl) {
        // POST 설정
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // Content-type 및 https 설정
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        // 헤더 설정
        struct curl_slist *headers = NULL;
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // body 생성

        //  create form
        form = curl_mime_init(curl);

        // transactionId part
        field = curl_mime_addpart(form);
        curl_mime_name(field, "transactionId");
        curl_mime_data(field, event_id, CURL_ZERO_TERMINATED);
        
        // description part
        field = curl_mime_addpart(form);
        curl_mime_name(field, "description");
        curl_mime_data(field, _uuid.c_str(), CURL_ZERO_TERMINATED);
        spdlog::debug("uuid {}", _uuid);

        // video data part
        field = curl_mime_addpart(form);
        curl_mime_name(field, "testfile");
        curl_mime_filedata(field, path);
        //curl_mime_type(field, "video/mp4");  


        //  mutlipart/form-data를 request body에 직렬화
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        curl_easy_setopt(curl, CURLOPT_URL, request_url.c_str());
        spdlog::debug("curl send");
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            spdlog::error("Curl code {}", (int)res);
            spdlog::error("event {} video send failed", event_id);
            
            curl_easy_cleanup(curl);
            curl_mime_free(form);
            return 1;
        }
        else{
            spdlog::info("event {} succcess", event_id);
            curl_easy_cleanup(curl);
            curl_mime_free(form);

            return 0;            
        }
        spdlog::debug("curl {}", (int) res);
    }
    return 0;
}

int EventHandlerHTTP::camerainfo_send(CURL* curl, const std::string& uuid)
{    
    CURLcode res;

    std::stringstream payload_stream;

    boost::property_tree::ptree payload_tree;
    boost::property_tree::ptree event_group_tree;

    std::string request_url = _server_address + "/setup";
    std::string request;
    char* response;

    // json 객체 생성
    payload_tree.put("description", uuid);
    // json 직렬화
    boost::property_tree::write_json(payload_stream, payload_tree);
    request = payload_stream.str();
    
    spdlog::debug("request url {}", request_url);
    spdlog::debug("request payload: {} ", request);
    
    curl = curl_easy_init();
    if(curl) {
        // POST로 설정
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

        // Content-type 및 https 설정
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        return send_request(curl, request_url, request);
    }
    else{
        spdlog::error("Error : Faild to Create CURL Object");

        return 1;
    }
    
}

int EventHandlerHTTP::send_request(CURL* curl, const std::string& url, const std::string& payload)
{
    CURLcode res;
    char response[1024];
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
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
