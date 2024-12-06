#include <cmath>

#include "config/VideoConfig.hpp"
#include "spdlog/spdlog.h"

using namespace config;

int VideoConfig::set_file(const std::string& json_file)
{
    try {
        boost::property_tree::read_json(json_file, _props);

        _props = _props.get_child("video");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }

    int ret = read_config();

    return ret;
}


int VideoConfig::read_config()
{
    spdlog::info("Video Config");
    try {
        _width = _props.get<int>("width");
        _height = _props.get<int>("height");

        _frame_rate = _props.get<std::string>("framerate");
        _format = _props.get<std::string>("format");

        _split_time = _props.get<int>("split-time");
        _duration = _props.get<int>("duration");

        _vid_path = _props.get<std::string>("save_path");
        _maintain_sec = _props.get<int>("maintain");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }
    spdlog::info("===Video Streaming Information===");
    spdlog::info("{} * {}", _width, _height);
    spdlog::info("frame_rate : {}", _frame_rate);
    spdlog::info("foramt : {}", _format);

    spdlog::info("===Video Save Information===");
    spdlog::info("Saving Loop duration : {}", _split_time);
    spdlog::info("Event send length : {}", _duration);
    spdlog::info("Path : {}", _vid_path);
    spdlog::info("maitain seconds : {}", _maintain_sec);

    spdlog::info("Video Config End");
    return 0;
}

const int& VideoConfig::width() const
{
    return _width;
}

const int& VideoConfig::height() const
{
    return _height;
}

const std::string& VideoConfig::frame_rate() const
{
    return _frame_rate;
}

const std::string& VideoConfig::format() const
{
    return _format;
}

const int VideoConfig::split_time() const
{
    return _split_time * static_cast<int>(pow(10, 9));
}

const int VideoConfig::duration() const
{
    return _duration * static_cast<int>(pow(10, 9));
}

const std::string& VideoConfig::save_path() const
{
    return _vid_path;
}

const int VideoConfig::maintain() const
{
    return _maintain_sec;
}