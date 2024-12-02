#include "http/HttpInitializer.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace http;

HttpInitializer::HttpInitializer()
{
    _config = config::ProgramConfig::get_instance()->http_config();
    _daemon = nullptr;
}

int HttpInitializer::init()
{
    if(_config->tls_enable()) {
        _daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY,
            _config->port(), NULL, NULL,
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

    return 0;
}

int HttpInitializer::start()
{
    if(_daemon == nullptr){
        spdlog::error("HTTP server is not running");

        return 1;
    }

    std::filesystem::path dir("system");
    if(!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir)) {
        std::filesystem::create_directory("system");
    }

    int cnt = 0;
    std::string uuid;

    for(const auto& entry : std::filesystem::directory_iterator(dir)) {
        if(std::filesystem::is_regular_file(entry.status())){

            if(entry.path().filename().string() == "uuid"){
                cnt++;
                break;
            }
        }
    }

    // 초기 부팅
    if(cnt==0){
        boost::uuids::random_generator generator;
        boost::uuids::uuid uids = generator();
        uuid.assign(boost::uuids::to_string(uids));

        std::ofstream file("system/uuid");
        if(file.is_open()){
            file << uuid;
            file.close();
        }
    }
    //  UUID가 생성되었음
    else if (cnt == 1){
        std::ifstream file("system/uuid");
        std::getline(file, uuid);

        file.close();
    }
    //  UUID 에러
    else{
        spdlog::error("UUID ERROR");
    }

    CURL* curl;
    int ret = EventHandlerHTTP::get_instance()->camerainfo_send(curl, uuid);

    if(ret){
        spdlog::error("UUID send Error");
        return 1;
    }
    else{
        spdlog::info("======================================");
        spdlog::info("HTTP Server is running on PORT {}", _config->port());
        spdlog::info("======================================");
        return 0;
    }
    
}
int HttpInitializer::stop()
{
    MHD_stop_daemon(_daemon);

    return 0;
}