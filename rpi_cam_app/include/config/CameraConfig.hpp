#ifndef _CAMERA_CONFIG_H
#define _CAMERA_CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "config/Config.hpp"

namespace config
{
    class VideoMeta
    {
    private:
        uint32_t _type;
        int _width;
        int _height;
    public:
        void set_type(uint32_t type);
        void set_width(int width);
        void set_height(int height);

        const uint32_t& get_type() const;
        const int& get_width() const;
        const int& get_height() const;
    };
    class CameraConfig : public Config
    {
    private:
        boost::property_tree::ptree _props;

        /// @brief 디바이스 파일 경로
        std::string _device_path;
        /// @brief 동영상 메타데이터
        VideoMeta _metadata;

        int read_config() override;

    public:
        CameraConfig() = default;

        int set_file(const std::string& json_file) override;

        /// @brief 다비이스 파일경로 가져옴
        /// @return 디바이스 파일 경로
        const std::string& device_path() const;
        /// @brief 동영상 너비, 높이, 포맷
        /// @return VideoMeta&
        const VideoMeta& metadata() const;
    };
};

#endif