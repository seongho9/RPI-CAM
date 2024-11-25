#ifndef _HTTP_CONFIG_H
#define _HTTP_CONFIG_H

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "config/Config.hpp"

namespace config
{
    class HttpConfig: public Config
    {
    private:
        boost::property_tree::ptree _props;

        /// @brief crt 파일
        std::string _crt_file;
        /// @brief private 파일
        std::string _private_file;
        /// @brief http web server가 유지할 스레드 풀의 수
        int _thread_pool;
        /// @brief tls enabling
        int _tls_enabled;

        int read_config() override;
    public:
        HttpConfig() = default;
        
        int set_file(const std::string& json_file) override;

        /// @brief crt file의 경로를 가져옴
        /// @return 파일의 경로
        const std::string& crt_file() const;
        /// @brief private file의 경로를 가져옴
        /// @return 파일의 경로
        const std::string& private_file() const;
        /// @brief thread pool의 수
        /// @return thread pool 개수
        const int& thread_pool() const;
        /// @brief tls 활성화 여부
        /// @return 0 -> false, 1 -> true
        const int& tls_enable() const;
    };
};

#endif