#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <unordered_map>
#include "event/EventProcessor.hpp"
#include "camera_device/VideoQueue.hpp"
#include "utils/Singleton.hpp"

namespace event
{
    class EventProcessor;
    
    class EventHandler :public utils::Singleton<EventHandler>
    {
    private:
        std::unordered_map< std::string, EventProcessor* > _event_thread;

        camera_device::VideoQueue* _video_q;

        friend utils::Singleton<EventHandler>;
        
        EventHandler();
        
    public:
        int start_event(std::string name);
        int stop_event(std::string name);
    };
};

#endif