#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char const *argv[])
{
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    config::EventConfig ev = config->event_config();


    config::HttpConfig http = config->http_config();
    config::VideoConfig vid = config->video_config();

    spdlog::info("init");
    
    return 0;
}
