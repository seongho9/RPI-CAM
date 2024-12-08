#ifndef _EVENT_PROCESS_H
#define _EVENT_PROCESS_H

#include <string>
#include <cstdint>
#include <ctime>
#include "http/EventHandlerHTTP.hpp"
#include "camera_device/VideoQueue.hpp"

// forward declaration
namespace http {
class EventHandlerHTTP;
}
namespace event
{
    typedef int (*event_function)(uint8_t* buffer, size_t size, time_t timestamp, int width, int height);
    
    class EventProcessor
    {
    private:

        std::string _event_name;
        int _fps;
        bool _enable;
        event_function _event_program = nullptr;
        camera_device::ThreadInfo* _info;
        void* _handle;

        camera_device::VideoQueue* _video_q;
        http::EventHandlerHTTP* _event_handler;

        int event_init();
        int event_exec(camera_device::VideoBuffer* buffer);

    public:
        EventProcessor() = delete;
        EventProcessor(std::string event_name);

        int event_loop();

    };
}; // namespace event

#endif