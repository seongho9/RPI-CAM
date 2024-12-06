#include "video/VideoHandler.hpp"
#include "spdlog/spdlog.h"
#include <filesystem>
#include <algorithm>
using namespace video;

VideoHandler::VideoHandler(): _video_config(){

    _video_config = config::ProgramConfig::get_instance()->video_config();
    std::stringstream directory_path_stream;


    //  비디오 데이터 저장 디렉터리 생성
    directory_path_stream << _video_config->save_path();
    std::filesystem::create_directory(directory_path_stream.str());
    //  이벤트 저장 디렉터리 생성
    directory_path_stream << "/event";
    std::filesystem::create_directory(directory_path_stream.str());
    
}

// 서버에 보낼 비디오가 가져올 path에 있는지 확인
int VideoHandler::get_video(std::string eventId, time_t timestamp){

    std::string filename = eventId+".mp4";
    std::filesystem::path file_path(filename);

    if (!file_path.has_filename()){
        spdlog::warn("{} file doesn't exist..");
        return -1;
    }
    
    return 0;

}

// 저장되는 경로에 있는 파일 가져오는 함수
void VideoHandler::set_filename(std::vector<std::string>& files, const std::string& path)
{

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

int VideoHandler::process_video(time_t timestamp, std::string eventId)
{    

    //  gstreamer와는 달리 초단위로 조정
    int split_time = static_cast<int>(_video_config->split_time()) / 1000000000;
    const int duration = _video_config->duration();

    //  현 디렉터리에 존재하는 파일명
    std::vector<std::string> path_files;
    path_files.clear();

    //  concat_file 이동
    std::vector<std::string> concat_file;
    concat_file.clear();

    _file_lock.lock();

    // 타임스탬프 범위
    int start_time = timestamp - duration;
    int end_time = timestamp + duration;

    //  set_filename을 호출하여 관련된 파일 list를 가져옴
    set_filename(path_files, _video_config->save_path());

    // files에서 파일 이름 가져와서 저장
    for (const auto& file : path_files) {
        size_t pos = file.find('.');
        int file_timestamp = std::stoi(file.substr(0, pos));

        int end_file = file_timestamp + split_time-1;

        if (file_timestamp <= end_time && end_file >=start_time) {
            concat_file.push_back(file);
        }
    }

    if (concat_file.empty()) {
        spdlog::warn("No video files to process for event {}", eventId);
        _file_lock.unlock();
        return -1;
    }

    // 파이프라인 구성
    std::stringstream pipeline;
    pipeline << "concat name=c ! queue ! m.video_0 qtmux name=m ! filesink location=" 
             << _video_config->save_path() << "/event/" << eventId << ".mp4";

    for (size_t i = 0; i < concat_file.size(); ++i){
        pipeline<< " filesrc location=" << concat_file[i] << " ! qtdemux ! h264parse ! c.";
    }

    // 파이프라인 생성
    GstElement *pipeline_concat = gst_parse_launch(pipeline.str().c_str(), NULL);
    if (!pipeline_concat) {
        spdlog::error("Failed to launch pipeline_concat");
        _file_lock.unlock();
        return -1;
    }

    // 파이프라인 실행
    GstStateChangeReturn ret = gst_element_set_state(pipeline_concat, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        spdlog::error( "Failed to set pipeline to concat video");
        gst_element_set_state(pipeline_concat, GST_STATE_NULL);
        gst_object_unref(pipeline_concat);
        _file_lock.unlock();
        return -1;
    }

    // 파이프라인 실행(버스 이벤트 처리)
    // 버스가 없으면 파이프라인이 유지되지 않음
    GstBus* bus = gst_element_get_bus(pipeline_concat);
    GstMessage* msg = nullptr;
    gboolean terminate = FALSE;

    while (!terminate) {
        msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR);
        if (msg != NULL) {
            GError* err;
            gchar* debug_info;
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error(msg, &err, &debug_info);
                    spdlog::error("Error: {}",err->message);
                    g_clear_error(&err);
                    g_free(debug_info);
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_EOS:
                    spdlog::info("End of stream reached");
                    terminate = TRUE;
                    break;
                default:
                    break;
                }
            gst_message_unref(msg);
            }
    }

    gst_object_unref(bus);

    // 파이프라인 종료 후 리소스 정리
    gst_element_set_state(pipeline_concat, GST_STATE_NULL);
    gst_object_unref(pipeline_concat);

    _file_lock.unlock();
    return 0;

}
int VideoHandler::remove_video(int maintain_time, std::string path)
{
    
    _file_lock.lock();

    int now = time(0);


    //  현 디렉터리에 존재하는 파일명
    std::vector<std::string> path_files;
    path_files.clear();
    set_filename(path_files, _video_config->save_path());

    for (auto it = path_files.begin(); it!= path_files.end();) {

        size_t pos = it->find('.');
        
        time_t file_timestamp = static_cast<time_t>(std::stoi(it->substr(0, pos)));
      
    
        if (file_timestamp < (now-(maintain_time*60))) {  
            std::filesystem::remove(path+"/"+*it);
            spdlog::info("remove {} file successfully", *it);
            it = path_files.erase(it);
        }
        else{
            ++it;
        }
    }

    _file_lock.unlock();

    return 0;
}
