#ifndef _HTTP_INIT_H
#define _HTTP_INIT_H

#include <unordered_map>
#include <microhttpd.h>

#include "utils/Initializer.hpp"
#include "utils/Singleton.hpp"

#include "http/EventHandlerHTTP.hpp"

#include "config/HttpConfig.hpp"

namespace http
{

    /// @brief microhttpd 데몬 생성시 들어가는 핸들러 함수
    MHD_Result libmicrohttpd_handler(
            void* cls, struct MHD_Connection*conn, const char* url, const char* method,
            const char* version, const char* upload_data, size_t* data_size, void** con_cls);

    void post_request_complete(
        void *cls, struct MHD_Connection* conn, void** con_cls, enum MHD_RequestTerminationCode toe);

    class HttpInitializer : public utils::Initialzier
    {
    private:
        std::string _uuid;
        const config::HttpConfig* _config;
        struct MHD_Daemon* _daemon;


    public:
        HttpInitializer();
        
        int init() override;
        int start() override;
        int stop() override;
    };
};
#endif