#ifndef _CAMERA_INITIALIZER_H
#define _CAMERA_INITIALIZER_H

#include <cstdint>
#include "utils/Initializer.hpp"
#include "config/CameraConfig.hpp"
#include "camera_device/VideoQueue.hpp"

#define N_CAM_BUF 8

namespace camera_device
{
    class CameraInitializer : public utils::Initialzier
    {
    private:
        const config::CameraConfig* _cam_config;
        bool _is_run;
        uint8_t* _camera_buffers[N_CAM_BUF];
        VideoQueue* _queue;
        int _fd;

        int set_v4l2_format();
        int set_v4l2_buffer();

        int queue_v4l2_buffer(int index);
        struct VideoBuffer* deque_v4l2_buffer(int* index);

        int loop();
    public:
        CameraInitializer();
        /// @brief 해당 설정 값이 연결된 카메라 디바이스에서 지원하는지 확인
        /// @return 0: 성공, others: 실패
        int init() override;
        /// @brief 이벤트 처리를 위한 카메라 영상 촬영 시작
        /// @return 0: 성공, others: 실패
        int start() override;
        /// @brief 카메라 영상촬영 중지
        /// @return 0: 성공, others: 실패
        int stop() override;
    };
};
#endif