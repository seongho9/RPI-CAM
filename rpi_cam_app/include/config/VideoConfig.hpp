#ifndef _VIDEO_CONFIG
#define _VIDEO_CONFIG

#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "config/Config.hpp"

namespace config
{
    class VideoConfig: public Config
    {
    private:
        
        boost::property_tree::ptree _props;
        /// @brief 영상의 너비
        int _width;
        /// @brief 영상의 높이
        int _height;
        /// @brief frame rate로 "frame/second" 형식으로 기술
        std::string _frame_rate;
        /// @brief 영상 포맷
        std::string _format;
        /// @brief 영상을 저장하는 단위 시간(초)
        int _split_time;
        /// @brief 이벤트 발생시 보내는 영상의 길이(초)
        int _duration;


        int read_config() override;
    public:
        VideoConfig() = default;

        int set_file(const std::string& json_file) override;

        const int& width() const;
        const int& height() const;
        const std::string& frame_rate() const;
        const std::string& foramt() const;
        const int split_time() const;
        const int duration() const;
    };
};
#endif