#ifndef _VIDEO_QUEUE_H
#define _VIDEO_QUEUE_H

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <atomic>

#include "utils/Singleton.hpp"
#include "config/ProgramConfig.hpp"

namespace camera_device
{
    struct VideoBuffer
    {
        config::VideoMeta metadata;
        time_t timestamp;
        uint8_t* buffer;
        uint64_t size;

        ~VideoBuffer();
    };

    class ThreadInfo
    {
    public:
        /// @brief 스레드를 관리하기 위한 변수
        std::atomic<bool> _is_run;
        /// @brief 조건변수의 wait()를 사용하기 위해 mutex 선언
        std::mutex _mutex;
        /// @brief _cond_t.wait(), _cond_t.notify_all()을 통해 해당 스레드를 깨움
        std::condition_variable _cond_t;
        /// @brief 초당 처리량
        int _fps;

        ThreadInfo() = default;
        // 복사 금지
        // 위 동기화 변수들의 복사를 방지
        ThreadInfo(const ThreadInfo&) = delete;
        ThreadInfo& operator=(const ThreadInfo&) = delete;
    };

    class VideoQueue : public utils::Singleton<VideoQueue>
    {
    private:
        /// @brief config에 존재하는 V4L2에 설정된 fps
        int _device_fps;
        std::queue<VideoBuffer*> _input;

        /// @brief 지금 처리 중인 fps
        int _current_frame_no;

        /// @brief _input -> _output 으로 처리시 동기화를 위해 사용
        std::mutex _input_mutex;
        std::condition_variable _input_cond;

        /// @brief VideoBuffer에 접근하기 위해 사용하는 _output으로 race condition이 발생하지 않음
        std::unordered_map<std::string, std::queue<VideoBuffer*>*> _output;
        /// @brief 각 이벤트의 thread 동기화를 위해 필요한 정보를 map으로 저장
        std::unordered_map<std::string, ThreadInfo*> _thread_info;
        /// @brief thread_info 조작시 race condition 방지
        std::mutex _thread_info_mutex;

        /// @brief distribute thread를 중지하기 위한 flag
        std::atomic<bool> _q_enable;
        
        friend utils::Singleton<VideoQueue>;    

        /// @brief distribute()를 수행하는 스레드를 8개 생성
        VideoQueue();

        /// @brief _input에 들어온 정보를 분배하며, push 정보에 맞춰서 스레드 동기화
        /// @return 0: 성공, others: 실패
        int distribute_buffer();
    public:
        /// @brief 객체가 소멸 될 때, distribute() 스레드도 동시에 종료
        ~VideoQueue();
        /// @brief event 스레드에 관한 정보를 전달
        /// @param event_name 이벤트 이름
        /// @param thread_info 동기화 관련 정보
        /// @return 0: 성공, others: 실패
        int insert_event(std::string event_name, ThreadInfo* thread_info);
        /// @brief event 스레드 제거 요청
        /// @param name 이벤트 이름
        /// @return 0: 성공, otheres: 실패
        int remove_event(std::string name);
        /// @brief CameraDevice에서 가져온 버퍼를 _input 큐에 push
        /// @param buffer 이미지 버퍼
        /// @return 0: 성공, others: 실패
        int push(VideoBuffer* buffer);
        /// @brief 각 이벤트에 맞는 버퍼를 가져옴, 내부에 비동기로 대기하는 로직 존재, 
        /// event 스레드는 해당 함수를 호출하여 대기 및 데이터 fetch 할 것
        /// @param name 이벤트 이름
        /// @return 버퍼의 주소를 반환, nullptr: 실패(event 종료)
        VideoBuffer* pop(std::string name);

    };
    
}; // namespace camera_device


#endif