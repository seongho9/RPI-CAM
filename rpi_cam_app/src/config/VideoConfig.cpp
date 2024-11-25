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
    try {
        _width = _props.get<int>("width");
        _height = _props.get<int>("height");

        _frame_rate = _props.get<std::string>("framerate");
        _foramt = _props.get<std::string>("foramt");

        _split_time = _props.get<int>("split-time");
        _duration = _props.get<int>("duration");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }

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

const std::string& VideoConfig::foramt() const
{
    return _foramt;
}

const int VideoConfig::split_time() const
{
    return _split_time * static_cast<int>(pow(10, 9));
}

const int VideoConfig::duration() const
{
    return _duration * static_cast<int>(pow(10, 9));
}