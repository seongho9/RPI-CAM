#include "video/videoHandler.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <algorithm>

video::VideoHandler::VideoHandler(){
    files.clear();
    concat_file.clear();
}
void video::VideoHandler::get_video(std::string path, time_t timestamp){

    for(const auto& file: std::filesystem::directory_iterator(path)){
        if (file.is_regular_file() && file.path().extension() == ".mp4") {
            files.push_back(file.path().filename().string());
        }
    }
    sort(files.begin(), files.end(), [](const string& a, const string& b) {
        return stoi(a) < stoi(b);  
    });
    
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
    //파이프라인 요소 구성
    /*concat요소 : 여러 비디오 스트림을 하나로 결합하는 역할
      qtmux: mp4 컨테이너를 생성하는 요소/ 비디오 스트림을 mp4파일로 패킹하는 역할
      filesrc: location에 해당하는 파일을 읽어드리는 요소
      qtdemux: mp4파일 포맷을 분해하는 요소 / mp4파일을 분석하여 포함되어 있는 비디오와 오디오 스트림 분리
      h264parse: 비디오 스트림을 H.264 형식으로 파싱하는 요소
      c. : concat의 첫 번째 입력포드에 데이터 전달*/
    std::stringstream pipeline;
    pipeline << "concat name=c ! queue ! m.video_0 qtmux name=m ! filesink location=output_video.mp4";

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
void video::VideoHandler::remove_video(int maintain_time, std::string path){
    
    int now = std::to_integer(time(nullptr));
    // 삭제하는 기준 정확하게 정해야
    // duration을 받지 않고 유지 시간을 config 파일에서 수정 할 수 있게 해야
    // 합쳐진 영상을 보냈다면, 서버가 합쳐진 영상을 잘 보냈다면
    if(){
        std::filesystem::remove(path+"/output_video.mp4");
    } 

    for (const auto& file : files) {
        size_t pos = file.find('.');
        time_t file_timestamp = std::stoi(file.substr(0, pos));
        
        // maintain_time을 분 단위라 가정 
        if (file_timestamp < (now-(maintain_time*60))) {   
            std::filesystem::remove(path+"/"+files[i]);
        }
    }
}
