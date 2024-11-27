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
    return 0;
}

int EventHandlerHTTP::video_send(CURL* curl, const char* path, const char* event_id)
{
    return 0;
}

int EventHandlerHTTP::camerainfo_send(CURL* curl, const char* payload)
{    
    return 0;
}
