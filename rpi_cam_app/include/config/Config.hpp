#ifndef _CONFIG_HPP
#define _CONFIG_HPP

#include <string>

namespace config
{
    /// @brief Config File을 읽어오기 위한 동작을 추상화한 순수가상클래스
    class Config
    {
    private:

        /// @brief config.json을 파시ㅏㅇ
        /// @return 성공하면 0, 아니면 1
        virtual int read_config()=0;
    public:
        Config() = default;

        /// @brief config.json을 읽고 해당하는 object를 가져옴
        /// @param json_file json파일 경로
        /// @return 성공하면 0, 아니면 1
        virtual int set_file(const std::string& json_file)=0;
    };
};

#endif