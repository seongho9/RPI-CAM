#ifndef _PROGRAM_CONFIG_H
#define _PROGRAM_CONFIG_H

#include "config/HttpConfig.hpp"
#include "config/VideoConfig.hpp"
#include "config/CameraConfig.hpp"

#include "utils/Singleton.hpp"

namespace config
{
    class ProgramConfig : public utils::Singleton<ProgramConfig>
    {
    private:
        HttpConfig* _http_config = nullptr;
        VideoConfig* _video_config = nullptr;
        CameraConfig* _camera_config = nullptr;

        int read_config(Config* config);

        friend class utils::Singleton<ProgramConfig>;
    protected:
        ProgramConfig();
    public:
        /// @brief HttpConfig 객체를 가져옴(HTTP 관련 설정 값)
        /// @return 실패시 nullptr, 성공시 해당하는 객체의 주소
        const HttpConfig* http_config();
        /// @brief VideoConfig 객체를 가져옴(Video Streaming 관련 설정 값)
        /// @return 실패시 nullptr, 성공시 해당하는 객체의 주소
        const VideoConfig* video_config();
        /// @brief CameraConfig 객체를 가져옴(Camera device 관련 설정 값)
        /// @return 실패시 nullptr, 성공시 해당하는 객체의 주소
        const CameraConfig* camera_config();
    };
};
#endif