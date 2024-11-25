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
    try {
        _crt_file = _props.get<std::string>("cert_file");
        _private_file = _props.get<std::string>("private_file");

        _thread_pool = _props.get<int>("thread_pool_size");

        _tls_enabled = _props.get<int>("tls");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
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