#ifndef _PROGRAM_CONFIG_H
#define _PROGRAM_CONFIG_H

#include "config/HttpConfig.hpp"
#include "config/EventConfig.hpp"
#include "config/VideoConfig.hpp"

#include "utils/Singleton.hpp"

namespace config
{
    class ProgramConfig : public utils::Singleton<ProgramConfig>
    {
    private:
        EventConfig* _event_config = nullptr;
        HttpConfig* _http_config = nullptr;
        VideoConfig* _video_config = nullptr;

        int read_config(Config* config);

        friend class utils::Singleton<ProgramConfig>;
    protected:
        ProgramConfig();
    public:
        const EventConfig& event_config();
        const HttpConfig& http_config();
        const VideoConfig& video_config();
    };
};
#endif