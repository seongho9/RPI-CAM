#define BOOST_TEST_MODULE VideoQueueTest

#include <boost/test/included/unit_test.hpp>
#include <cstdlib>

#include "camera_device/VideoQueue.hpp"
#include "config/ProgramConfig.hpp"

#include "spdlog/spdlog.h"


// 이벤트 정상 삽입 및 제거
BOOST_AUTO_TEST_CASE(video_queue_insert)
{
    spdlog::set_level(spdlog::level::info);

    spdlog::info("--video_queue_insert--");
    camera_device::VideoQueue* vid_q = camera_device::VideoQueue::get_instance();

    camera_device::ThreadInfo* insert_event = new camera_device::ThreadInfo();
    
    insert_event->_fps = 10;
    insert_event->_is_run = true;

    int ret = vid_q->insert_event("test", insert_event);
    
    BOOST_ASSERT(ret==0);
    BOOST_ASSERT(insert_event->_fps != 10);
    BOOST_ASSERT(insert_event->_fps == 6);
}

// // 이벤트 중복삽입
BOOST_AUTO_TEST_CASE(video_queue_insert_duplicate)
{
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("--video_queue_insert_duplicate--");
    camera_device::VideoQueue* vid_q = camera_device::VideoQueue::get_instance();

    camera_device::ThreadInfo* insert_event_1 = new camera_device::ThreadInfo();

    insert_event_1->_fps = 5;
    insert_event_1->_is_run = true;

    camera_device::ThreadInfo* insert_event_2 = new camera_device::ThreadInfo();
    insert_event_2->_fps = 10;
    insert_event_2->_is_run = true;

    int ret = vid_q->insert_event("test1", insert_event_1);
    BOOST_ASSERT(ret==0);

    ret = vid_q->insert_event("test1", insert_event_2);
    BOOST_ASSERT(ret!=0);

    vid_q->remove_event("test1");

    usleep(500);

}
// // 없는 이벤트 제거
BOOST_AUTO_TEST_CASE(video_queue_test_remove_non)
{
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("--video_queue_test_remove_non--");
    camera_device::VideoQueue* vid_q = camera_device::VideoQueue::get_instance();

    int ret = vid_q->remove_event("non_exist_event");

    BOOST_ASSERT(ret != 0);
    
    usleep(500);
}

// 버퍼 push(스레드 동기화 없는 버전)
BOOST_AUTO_TEST_CASE(video_queue_test_push_only_data)
{
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("--video_queue_test_push_only_data--");
    camera_device::VideoQueue* vid_q = camera_device::VideoQueue::get_instance();

    struct camera_device::VideoBuffer* buff = new camera_device::VideoBuffer();
    buff->buffer = (uint8_t*)malloc(sizeof(uint8_t));
    *(buff->buffer) = (uint8_t)(4);
    int ret = vid_q->push(buff);

    BOOST_ASSERT(ret == 0);
    
    usleep(500);
}

// video q에 넣었을 때, push pop 동작 여부 확인
BOOST_AUTO_TEST_CASE(video_queue_push_test)
{
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("--video_queue_push_test--");
    camera_device::VideoQueue* vid_q = camera_device::VideoQueue::get_instance();


    std::thread thread([=](){
        camera_device::ThreadInfo* thread_info = new camera_device::ThreadInfo();
        thread_info->_fps = 5;
        thread_info->_is_run = true;
        std::string name("async1");
        vid_q->insert_event(name, thread_info);

        while(thread_info->_is_run){
            spdlog::debug("test -- event name {} _fps {}", name, thread_info->_fps);
            camera_device::VideoBuffer* buff = vid_q->pop(name);

            if(buff == nullptr){
                spdlog::debug("buff is null");
                break;
            }
            else{
                spdlog::debug("{} ASSERTION expected : {} actual : {}", name, (uint8_t)1, *buff->buffer);
                BOOST_ASSERT((*(buff->buffer)) == (uint8_t)1);

                delete buff;
            }
        }
        delete thread_info;
        spdlog::info("Event {} end...", name);

        return;
    });
    thread.detach();
    sleep(1);

    camera_device::VideoBuffer* buff = new camera_device::VideoBuffer();
    buff->buffer=new uint8_t;
    buff->size=sizeof(uint8_t);
    BOOST_ASSERT(vid_q->push(buff)==0);

    usleep(500);
    spdlog::debug("remove phase");

    int ret = vid_q->remove_event("test");
    //BOOST_ASSERT(ret==0);

    usleep(500);
}

// video distributer thread free test
BOOST_AUTO_TEST_CASE(video_queue_test_thread_free)
{
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("--video_queue_test_thread_async--");
    camera_device::VideoQueue* vid_q = camera_device::VideoQueue::get_instance();


    std::thread thread([=](){
        camera_device::ThreadInfo* thread_info = new camera_device::ThreadInfo();
        thread_info->_fps = 5;
        thread_info->_is_run = true;
        std::string name("async");
        vid_q->insert_event(name, thread_info);

        while(thread_info->_is_run){
            spdlog::debug("test -- event name {} _fps {}", name, thread_info->_fps);
            camera_device::VideoBuffer* buff = vid_q->pop(name);

            if(buff == nullptr){
                spdlog::debug("buff is null");
                break;
            }
            else{
                spdlog::debug("{} ASSERTION expected : {} actual : {}", name, (uint8_t)1, *buff->buffer);
                //BOOST_ASSERT((*(buff->buffer)) == (uint8_t)1);

                delete buff;
            }
        }
        delete thread_info;
        spdlog::info("Event {} end...", name);

        return;
    });
    thread.detach();
    sleep(1);

    spdlog::debug("remove phase");

    int ret = vid_q->remove_event("test");
    //BOOST_ASSERT(ret==0);

    sleep(2);
    delete vid_q;
    
    usleep(500);
}