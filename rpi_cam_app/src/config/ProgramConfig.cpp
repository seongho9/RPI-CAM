#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

using namespace config;

ProgramConfig::ProgramConfig()
{
    spdlog::info("Program Config");
}

const EventConfig& ProgramConfig::event_config()
{
    if(_event_config == nullptr) {
        _event_config = new EventConfig();
        read_config(_event_config);
    }

    return *_event_config;
}

const HttpConfig& ProgramConfig::http_config()
{
    if(_http_config == nullptr) {
        _http_config = new HttpConfig();
        read_config(_http_config);
    }

    return *_http_config;
}

const VideoConfig& ProgramConfig::video_config()
{
    if(_video_config == nullptr) {
        _video_config = new VideoConfig();
        read_config(_video_config);
    }

    return *_video_config;
}

int ProgramConfig::read_config(Config* config)
{
    return config->set_file("config.json");
}