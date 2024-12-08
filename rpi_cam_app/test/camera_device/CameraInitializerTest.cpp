#include <cstdlib>
#include <thread>
#include <vector>
#include <string>

#include "camera_device/VideoQueue.hpp"
#include "camera_device/CameraInitializer.hpp"

#include "config/ProgramConfig.hpp"
#include "spdlog/spdlog.h"

#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>

int main()
{
    spdlog::set_level(spdlog::level::level_enum::debug);
    camera_device::CameraInitializer* init = new camera_device::CameraInitializer();
    camera_device::VideoQueue* q = camera_device::VideoQueue::get_instance();

    init->init();

    std::thread thread([&](){
        init->start();
    });

    std::thread event([&](){
        camera_device::ThreadInfo* info = new camera_device::ThreadInfo();
        info->_fps = 5;
        info->_is_run = true;
        
        q->insert_event("test", info);
        
        while(info->_is_run){
            camera_device::VideoBuffer* arr_buffer = q->pop("test");

            spdlog::debug("event called");

            std::vector<uchar> buffer(arr_buffer->buffer, arr_buffer->buffer + arr_buffer->size);
            
            cv::Mat frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
            if(frame.empty()) {
                spdlog::error("error : MJPEG 프레임 디코딩 실패");
                continue;
            }
            std::string filename(std::to_string(arr_buffer->timestamp));
            filename.append(".jpg");
            if(!cv::imwrite(filename, frame)){
                spdlog::error("error: JPEG 저장 실패");
                continue;
            }

            delete arr_buffer;
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(30));

    event.join();

}