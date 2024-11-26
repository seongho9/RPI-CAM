#include "video/videoHandler.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>

video::VideoHandler::VideoHandler(){

}
void video::VideoHandler::get_video(std::string path, time_t timestamp){

    for(const auto& file: std::filesystem::directory_iterator(path)){
        if (file.is_regular_file() && file.path().extension() == ".mp4") {
            files.push_back(file.path().filename().string());
        }
    }
    
}
int video::VideoHandler::process_video(time_t timestamp, uint64_t duration)
{
    //duration을 초단위로 해야..?

    // 타임스탬프 범위
    time_t start_time = timestamp - duration;
    time_t end_time = timestamp + duration;

    // files에서 파일 이름 가져와서 
    for (const auto& file : files) {
        size_t pos = file.find('.');
        time_t file_timestamp = std::stoi(file.substr(0, pos));

        if (file_timestamp >= start_time && file_timestamp <= end_time) {
            concat_file.push_back(file);
        }
    }

    std::stringstream pipeline;

    for (size_t i = 0; i < concat_file.size(); ++i) {
        pipeline << "filesrc location=" << concat_file[i] << " ! decodebin name=dec" << (i+1) 
                 << " ! queue ! x264enc ! mp4mux name=mux4";
        
        if (i != concat_file.size() - 1) {
            pipeline << " ! filesink location=result.mp4 ";
        }
    }

    // GStreamer 파이프라인 실행
    GstElement *pipeline_concat = gst_parse_launch(pipeline.str().c_str(), NULL);
    
    if (!pipeline_concat) {
        spdlog::error("Failed to launch pipeline_concat");
        return -1;
    }

    // 파이프라인 실행
    gst_element_set_state(pipeline_concat, GST_STATE_PLAYING);

    // 파이프라인 종료 후 리소스 정리
    gst_element_set_state(pipeline_concat, GST_STATE_NULL);
    gst_object_unref(pipeline_concat);

    return 0;

}
void video::VideoHandler::remove_video(uint64_t durtion, std::string path){
    
    // 삭제하는 기준 정확하게 정해야
    //std::filesystem::remove(path);
}
