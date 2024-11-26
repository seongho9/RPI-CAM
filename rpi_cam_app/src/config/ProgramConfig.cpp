#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

using namespace config;

ProgramConfig::ProgramConfig()
{
    spdlog::info("Program Config");
}

const EventConfig* ProgramConfig::event_config()
{
    if(_event_config == nullptr) {
        _event_config = new EventConfig();

        if(read_config(static_cast<Config*>(_event_config))) {
            return nullptr;
        }
    }

    return _event_config;
}

const HttpConfig* ProgramConfig::http_config()
{
    if(_http_config == nullptr) {
        _http_config = new HttpConfig();

        if(read_config(static_cast<Config*>(_http_config))) {
            return nullptr;
        }
    }

    return _http_config;
}

const VideoConfig* ProgramConfig::video_config()
{
    if(_video_config == nullptr) {
        _video_config = new VideoConfig();

        if(read_config(static_cast<Config*>(_video_config))) {
            return nullptr;
        }
    }

    return _video_config;
}

const CameraConfig* ProgramConfig::camera_config()
{
    if(_camera_config == nullptr) {
        _camera_config = new CameraConfig();

        if(read_config(static_cast<Config*>(_camera_config))) {
            return nullptr;
        }
    }

    return _camera_config;
}
int ProgramConfig::read_config(Config* config)
{
    return config->set_file("config.json");
}