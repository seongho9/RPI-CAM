#ifndef _EVENT_HANDLER_HTTP_H
#define _EVENT_HANDLER_HTTP_H

#include <microhttpd.h>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <mutex>

#include "utils/Singleton.hpp"
#include "config/ProgramConfig.hpp"
#include "video/VideoHandler.hpp"
#include "event/EventHandler.hpp"

namespace http
{
    struct connection_info
    {
        int connection_type;
        struct MHD_PostProcessor* postprocessor;
    
        std::string file_name;
        std::string fps;
        std::vector<char> file_content;
        bool upload_done;
    };

    struct event_info
    {
        time_t time_stamp;
        char* id;
    };

    enum HTTP_METHOD
    {
        GET = 0,
        POST = 1
    };
    class EventHandlerHTTP : public utils::Singleton<EventHandlerHTTP>
    {
    private:
        /// @brief 이벤트를 전송 할 server_address
        std::string _server_address;

        /// @brief 카메라를 식별하는 UUID
        std::string _uuid;

        int _upload_client;
        std::mutex _upload_client_mutex;

        video::VideoHandler* _video_handler = video::VideoHandler::get_instance();

        event::EventHandler* _event_handler = event::EventHandler::get_instance();

        friend class utils::Singleton<EventHandlerHTTP>;

        EventHandlerHTTP();
        
        MHD_Result send_response(MHD_Connection* conn, const std::string& buffer, int status);
        int send_request(CURL* curl, const std::string& url, const std::string& payload);

    public:
        std::mutex& upload_client_mutex();
        
        int& upload_client();
        
        /// @brief master 카메라에서 이벤트가 발생 될 때, 이벤트가 들어오는 부분
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int event_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls);

        /// @brief WebSocket으로 실시간 영상 스트리밍
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int video_accpet(MHD_Connection* conn, const char* data, size_t* size, void** con_cls);

        /// @brief 라이브러리 프로그램 .so 파일을 추가
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int program_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls);

        /// @brief 이벤트 프로그램 실행
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int program_start_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls);
        
        /// @brief 이벤트 프로그램 중지
        /// @param conn MHD_Connection
        /// @param data http 데이터
        /// @param size http 데이터 크기
        /// @return 0:성공, others:실패
        int program_stop_accept(MHD_Connection* conn, const char* data, size_t* size, void** con_cls);

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
        int camerainfo_send(CURL* curl, const std::string& uuid);
    };
}
#endif