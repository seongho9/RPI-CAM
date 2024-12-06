#ifndef VIDEO_HANDLER_HPP
#define VIDEO_HANDLER_HPP

#include <gst/gst.h>
#include <string>
#include <cstdint>
#include <config/ProgramConfig.hpp>
#include <vector>
#include <mutex>
#include "config/VideoConfig.hpp"
#include "utils/Singleton.hpp"

namespace video{
    class VideoHandler : public utils::Singleton<VideoHandler>{
        
        //  각 요청마다 file 이름을 vector로 저장하는 필드
        //  요청마다 별도로 처리되어야 하므로 set_filename에 참조자로 넘겨주는 것으로 변겅
        // std::vector<std::string> files;
        
        //  process_video에서만 사용하는 vector
        //  각 요청에 의존적이므로 process_video 메소드의 지역변수로 이동
        //std::vector<std::string> concat_file;

        friend utils::Singleton<VideoHandler>; //Singleton이 이 클래스의 private 생성자에 접근 가능
        const config::VideoConfig* _video_config;

        std::mutex _file_lock;

        VideoHandler();

        /// @brief 현재 비디오 디렉터리에 저장된 파일들을 vector형태로 반환
        /// @param files 반환 받을 vector
        /// @param path 가져올 비디오 디렉터리
        void set_filename(std::vector<std::string>& files, const std::string& path);
    public:

        /// @brief 원하는 비디오가 있는지 학인
        /// @param eventId 이벤트 id
        /// @param timestamp 이벤트 발생 시각
        /// @return 0: 성공, others: 실패
        int get_video(std::string eventId, time_t timestamp);
        /// @brief 비디오 합치는 파이프라인
        /// @param timestamp 이벤트 발생 시각
        /// @param eventId 이벤트 id
        /// @return 0: 성공, others: 실패
        int process_video(time_t timestamp, std::string eventId);

        /// @brief 기간이 지난 비디오를 삭제하는 루틴
        /// @param maintain_time 유지기간(초단위)
        /// @param path 파일 저장된 경로
        /// @return 0: 성공, others: 실패
        int remove_video(int maintain_time, std::string path);
    };
};
#endif