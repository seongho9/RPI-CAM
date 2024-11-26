#include <linux/videodev2.h>

#include "config/CameraConfig.hpp"
#include "spdlog/spdlog.h"

using namespace config;

void VideoMeta::set_type(uint32_t type)
{
    _type = type;
}

void VideoMeta::set_width(int width)
{
    _width = width;
}

void VideoMeta::set_height(int height)
{
    _height = height;
}

const uint32_t& VideoMeta::get_type() const
{
    return _type;
}

const int& VideoMeta::get_width() const
{
    return _width;
}

const int& VideoMeta::get_height() const
{
    return _height;
}

int CameraConfig::read_config()
{
    try {
        _device_path = _props.get<std::string>("device_path");
        int width = _props.get<int>("width");
        int height = _props.get<int>("height");
        std::string format = _props.get<std::string>("format");

        if(format == "V4L2_PIX_FMT_RGB24"){
            _metadata.set_type(V4L2_PIX_FMT_RGB24);
        }
        else if(format == "V4L2_PIX_FMT_YUYV") {
            _metadata.set_type(V4L2_PIX_FMT_YUYV);
        }
        else {
            spdlog::error("Not Supported Foramt");
            return 1;
        }
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());
    }

    return 0;
}

int CameraConfig::set_file(const std::string& json_file)
{
    try {
        boost::property_tree::read_json(json_file, _props);

        _props = _props.get_child("camera");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }

    int ret = read_config();

    return ret;
}

const std::string& CameraConfig::device_path() const
{
    return _device_path;
}

const VideoMeta& CameraConfig::metadata() const
{
    return _metadata;
}