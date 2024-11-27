#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>  // std::put_time
#include <ctime>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

extern "C"
{
    #include <unistd.h>
    #include <sys/types.h>
    #include <ifaddrs.h>
    #include <netdb.h>
    #include <arpa/inet.h>

}
#include "http/EventHandlerHTTP.hpp"
#include "spdlog/spdlog.h"
#include "config/ProgramConfig.hpp"

using namespace http;

/// @brief "event_id를 가져오기 위한 함수"
/// @param cls event_id를 넘겨주기 위한 변수(char*로 casting하여 사용)
/// @param kid 순회하는 종류로 여기에서는 MHD_ValueKind::MHD_HEADER_KIND
/// @param key key 값
/// @param value value 값
/// @return 만약 iteratoring 중이라면 MHD_NO를 return
static MHD_Result event_header_iter(void *cls, enum MHD_ValueKind kid, const char* key, const char* value)
{
    if(!strcmp(key, "event_id")){
        
        char* event_id = static_cast<char*>(cls);
        event_id = new char[strlen(value)+1];

        memcpy(event_id, value, strlen(value)+1);

        //  순회 중단
        return MHD_Result::MHD_YES;
    }
    else {
        //  순회 지속
        return MHD_Result::MHD_NO;
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

    data = static_cast<char*>(malloc(real_size+1));

    memcpy(data, contents, real_size);

    return real_size;
}


int EventHandlerHTTP::event_accept(MHD_Connection* conn, const char* data, size_t size)
{
    struct MHD_Response* response = nullptr;
    MHD_Result ret = MHD_NO;

    char* event_id = nullptr;

    // 반환 값 순회한 header 값의 개수
    MHD_get_connection_values(
        conn, MHD_ValueKind::MHD_HEADER_KIND, 
        event_header_iter, event_id);
    
    // HTTP 헤더에 event_id가 존재하지 않음
    if(event_id == nullptr){
        std::string error = "event_id is not included in HTTP header";
        response = MHD_create_response_from_buffer(error.size(), (void*)error.c_str(), MHD_RESPMEM_MUST_COPY);

        spdlog::debug("event_id is not included");

        ret = MHD_queue_response(conn, MHD_HTTP_BAD_REQUEST, response);

        if(ret != MHD_Result::MHD_YES) {
            spdlog::error("HTTP response send Error");
        }
        else{
            return 1;
        }
    }
    // HTTP 헤더에 event_id가 존재
    else {
        spdlog::info("event {} accept", event_id); 

        std::string event(event_id);
        event += ".mp4";

        // video processing 호출 /////////  
        //                             //
        /////////////////////////////////

        // 이벤트 파일 존재
        if(access(event.c_str(), F_OK)){
            
            std::string ok = "success";
            response = MHD_create_response_from_buffer(ok.size(), (void*)ok.c_str(), MHD_RESPMEM_MUST_COPY);
            ret = MHD_queue_response(conn, MHD_HTTP_OK, response);

            CURL* curl;
            video_send(curl, event.c_str(), event_id);
        }
        // 이벤트 파일 없음
        else{
            spdlog::error("event file {} is not exist", event);

            std::string no_file = "event video is not exist";
            response = MHD_create_response_from_buffer(no_file.size(),(void*)no_file.c_str(), MHD_RESPMEM_MUST_COPY);

            ret = MHD_queue_response(conn, MHD_HTTP_NOT_FOUND, response);
        }
    }

    if(response == nullptr){
        spdlog::error("HTTP resonse object error");
        return MHD_Result::MHD_NO;
    }
    else{
        MHD_destroy_response(response);
        return MHD_Result::MHD_YES;
    }
}

int EventHandlerHTTP::video_accpet(MHD_Connection* conn, const char* data, size_t size)
{
    return 0;
}

int EventHandlerHTTP::event_send(CURL* curl, const char* group_name, time_t time)
{
    CURLcode res;

    std::tm* tm;

    std::stringstream payload_stream;
    std::stringstream time_stream;

    boost::property_tree::ptree payload_tree;

    std::string request_url = _server_address + "/event";
    std::string request;
    char* response;

    //  event를 전송 할 수 있는 master 디바이스 인지 확인
    if(_device_mode != config::DEVICE_MODE::MASTER) {
        spdlog::error("This device is not master device");
        return 4;
    }

    //  현 IP 카메라에 등록된 event인지 확인
    bool flag = false;
    for(auto event:_event_group) {
        if(!strcmp(event.c_str(), group_name)){
            flag = true;
            break;
        }
    }
    //  현 카메라에 등록된 event가 아니라면
    if(!flag){
        spdlog::error("No Such event group {}", group_name);
        return 3;
    }

    curl = curl_easy_init();
    if(curl) {
        tm = std::localtime(&time);

        time_stream << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");

        payload_tree.put("group", group_name);
        payload_tree.put("time", time);
        payload_tree.put("localtime", time_stream.str());
        
        // 요청 url
        curl_easy_setopt(curl, CURLOPT_URL, request_url);

        // response 처리
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, event_send_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // user-agent 설정
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // payload 설정
        boost::property_tree::write_json(payload_stream, payload_tree);
        request = payload_stream.str();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.size());

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

        return 1;
    }
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

int EventHandlerHTTP::camerainfo_send(CURL* curl, const char* payload)
{    
   CURLcode res;

    std::stringstream payload_stream;

    boost::property_tree::ptree payload_tree;
    boost::property_tree::ptree event_group_tree;

    std::string request_url = _server_address + "/info";
    std::string request;
    char* response;


    curl = curl_easy_init();
    if(curl) {

        payload_tree.put("address", get_current_dir_name());
        payload_tree.put("mode", config::convert_device_mode_str(_device_mode));
        for(auto event: _event_group){
            event_group_tree.put("", event);
        }
        payload_tree.put("event_groups", event_group_tree);
        
        // 요청 url
        curl_easy_setopt(curl, CURLOPT_URL, request_url);

        // response 처리
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, event_send_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

        // user-agent 설정
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // payload 설정
        boost::property_tree::write_json(payload_stream, payload_tree);
        request = payload_stream.str();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.size());

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

        return 1;
    }
}

int EventHandlerHTTP::get_current_ipv4(std::string* ip_addr)
{
    struct ifaddrs* ifaddr, *ifa;

    char host[NI_MAXHOST];

    if(getifaddrs(&ifaddr) == -1) {
        spdlog::error("failed to get ip address");
        return -1;
    }

    for(ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr == nullptr)
            continue;
        
        // IPv4 확인
        if(ifa->ifa_addr->sa_family == AF_INET) {
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

            inet_ntop(AF_INET, addr, host, NI_MAXHOST);
        }
    }

    ip_addr->append(host);

    return 0;
}
