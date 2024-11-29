#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

int main(int argc, char const *argv[])
{
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    const config::HttpConfig* http = config->http_config();

    const config::VideoConfig* vid = config->video_config();

    const config::CameraConfig* cam = config->camera_config();

    return 0;
}
