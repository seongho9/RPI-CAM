#include "video/videoHandler.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <algorithm>

video::VideoHandler::VideoHandler(): _video_config(){
    files.clear();
    concat_file.clear();
}

int video::VideoHandler::get_video(std::string eventId, time_t timestamp){

    std::string filename = eventId+".mp4";
    std::filesystem::path file_path(filename);

    if (!file_path.has_filename()){
        spdlog::warn("{} file doesn't exist..");
        return -1;
    }
    
    return 0;

}
void video::VideoHandler::set_filename(std::string path){

    for(const auto& file: std::filesystem::directory_iterator(path)){
        if (file.is_regular_file() && file.path().extension() == ".mp4") {
            std::string filename = file.path().filename().string();

            if(std::find(files.begin(), files.end(),filename)== files.end())
                files.push_back(filename);
        }
    }

    //timestamp 시간 순으로 정렬
    sort(files.begin(), files.end(), [](const std::string& a, const std::string& b) {
        return stoi(a) < stoi(b);  
    });

    
}

int video::VideoHandler::process_video(time_t timestamp, std::string eventId)
{
    //duration을 초단위로 해야..?
    int split_time = _video_config.split_time();

    int duration = _video_config.duration();

    // 타임스탬프 범위
    time_t start_time = timestamp - duration;
    time_t end_time = timestamp + duration;

    // files에서 파일 이름 가져와서 저장
    for (const auto& file : files) {
        size_t pos = file.find('.');
        time_t file_timestamp = std::stoi(file.substr(0, pos));

        if (!(file_timestamp+(split_time-1)) < start_time || file_timestamp > end_time) {
            concat_file.push_back(file);
        }
    }

    if (concat_file.empty()) {
        spdlog::warn("No video files to process for event {}", eventId);
        return -2;
    }

    // 파이프라인 구성
    std::stringstream pipeline;
    pipeline << "concat name=c ! queue ! m.video_0 qtmux name=m ! filesink location=/event/" << eventId << ".mp4";

    for (size_t i = 0; i < files.size(); ++i){
        pipeline<< " filesrc location=" << concat_file[i] << " ! qtdemux ! h264parse ! c.";
    }

    // 파이프라인 생성
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
int video::VideoHandler::remove_video(int maintain_time, std::string path){
    
    int now = time(nullptr);
    // 삭제하는 기준 정확하게 정해야
    // duration을 받지 않고 유지 시간을 config 파일에서 수정 할 수 있게 해야
    // 합쳐진 영상을 서버로 보내면 /event/done으로 옮겨서 해당 파일에 있는 영상 삭제

    for (auto it = files.begin(); it!= files.end();) {
        size_t pos = it->find('.');
        time_t file_timestamp = std::stoi(it->substr(0, pos));
        
        // maintain_time을 분 단위라 가정 
        if (file_timestamp < (now-(maintain_time*60))) {  
            std::filesystem::remove(path+"/"+*it);
            spdlog::info("remove {} file successfully", *it);
            it = files.erase(it);
        }
        else{
            ++it;
        }
    }
}
