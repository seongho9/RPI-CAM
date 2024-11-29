#include "config/ProgramConfig.hpp"
#include "http/HttpInitializer.hpp"

int main(int argc, char const *argv[])
{
    config::ProgramConfig* config = config::ProgramConfig::get_instance();

    const config::HttpConfig* http = config->http_config();

    const config::VideoConfig* vid = config->video_config();

    const config::CameraConfig* cam = config->camera_config();

    http::HttpInitializer* init = http::HttpInitializer::get_instance();

    init->init();
    init->start();
    

    return 0;
}
