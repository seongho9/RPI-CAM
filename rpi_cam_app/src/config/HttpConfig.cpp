#include "config/HttpConfig.hpp"
#include "spdlog/spdlog.h"

using namespace config;

int HttpConfig::set_file(const std::string& json_file)
{
    try {
        boost::property_tree::read_json(json_file, _props);
        _props = _props.get_child("http");
    } 
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }

    int ret = read_config();

    return ret;
}

int HttpConfig::read_config()
{
    spdlog::info("Http Config");
    try {
        _crt_file = _props.get<std::string>("cert_file");
        _private_file = _props.get<std::string>("private_file");

        _thread_pool = _props.get<int>("thread_pool_size");

        _tls_enabled = _props.get<int>("tls");

        _port = _props.get<int>("port");

        _remote_server = _props.get<std::string>("remote_server");

        _upload_user = _props.get<int>("upload_clients");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }
    spdlog::info("===Http Server Information===");
    spdlog::info("thread pool size : {}", _thread_pool);
    spdlog::info("tls {}", _tls_enabled == 0 ? "disabled" : "enabled");
    spdlog::info("port : {}", _port);
    spdlog::info("Program Concurrent upload User : {}", _upload_user);
    spdlog::info("Remote Server Address : {}", _remote_server);
    if(_tls_enabled != 0){
        spdlog::info("cert file : {}", _crt_file);
        spdlog::info("key file : {}", _private_file);
    }

    return 0;
}
 
const std::string& HttpConfig::crt_file() const
{
    return _crt_file;
}

const std::string& HttpConfig::private_file() const
{
    return _private_file;
}

const int& HttpConfig::thread_pool() const
{
    return _thread_pool;
}

const int& HttpConfig::tls_enable() const
{
    return _tls_enabled;
}

const int& HttpConfig::upload_user() const
{
    return _upload_user;
}

const int& HttpConfig::port() const
{
    return _port;
}

const std::string& HttpConfig::remote_server() const
{
    return _remote_server;
}