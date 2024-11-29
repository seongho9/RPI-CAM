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

    class HttpInitializer : public utils::Initialzier, public utils::Singleton<HttpInitializer>
    {
    private:

        const config::HttpConfig* _config;
        struct MHD_Daemon* _daemon;

        friend utils::Singleton<HttpInitializer>;
        HttpInitializer();

    public:
        int init() override;
        int start() override;
        int stop() override;
    };
};
#endif