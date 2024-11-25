#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <string>

namespace config
{
    /// @brief Config File을 읽어오기 위한 동작을 추상화한 순수가상클래스
    class Config
    {
    private:
        virtual int read_config()=0;
    public:
        Config() = default;
        virtual int set_file(const std::string& json_file)=0;
    };
};

#endif