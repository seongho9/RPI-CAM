#ifndef _EVENT_HANDLER_HTTP_H
#define _EVENT_HANDLER_HTTP_H

#include <microhttpd.h>
#include <curl/curl.h>
#include <string>
#include <vector>

#include "utils/Singleton.hpp"
#include "config/ProgramConfig.hpp"

namespace http
{
    class EventHandlerHTTP : public utils::Singleton<EventHandlerHTTP>
    {
    private:
    
        /// @brief tls 활성화 여부 0:false, 1:true
        int _tls_enabled;

        /// @brief 이벤트를 전송 할 server_address
        std::string _server_address;

        /// @brief 처리가능한 event group 목록
        std::vector<std::string> _event_group;

        /// @brief device 모드
        config::DEVICE_MODE _device_mode;

        friend class utils::Singleton<EventHandlerHTTP>;

        EventHandlerHTTP();

        int get_current_ipv4(std::string* ip_addr);
        
    public:
        /// @brief master 카메라에서 이벤트가 발생 될 때, 이벤트가 들어오는 부분
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int event_accept(MHD_Connection* conn, const char* data, size_t size);

        /// @brief WebSocket으로 실시간 영상 스트리밍
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int video_accpet(MHD_Connection* conn, const char* data, size_t size);

        /// @brief master 카메라일 경우 이벤트 발생
        /// @param group_name event group 이름
        /// @param time 이벤트 발생 시각
        /// @return 0:성공 others:실패
        int event_send(CURL* curl, const char* group_name, time_t time);

        /// @brief event가 들어왔을 때, 영상을 전송
        /// @param path video 경로
        /// @param event_id event id
        /// @return 0:성공 others:실패
        int video_send(CURL* curl, const char* path, const char* event_id);

        /// @brief 카메라 기동 시 서버로 정보 전송
        /// @param payload 보낼 데이터
        /// @return 0:성공 others:실패
        int camerainfo_send(CURL* curl, const char* payload);
    };
}
#endif