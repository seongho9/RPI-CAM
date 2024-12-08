#include "event/EventProcessor.hpp"
#include "event/EventHandler.hpp"

using namespace event;

EventHandler::EventHandler()
{
    _video_q = camera_device::VideoQueue::get_instance();

    _event_thread.clear();
}

int EventHandler::start_event(std::string name)
{
    EventProcessor* event = new EventProcessor(name);
    _event_thread.emplace(name, event);
    
    std::thread event_t([=](){
        event->event_loop();
    });

    event_t.detach();

    return 0;
}

int EventHandler::stop_event(std::string name)
{
    _video_q->remove_event(name);

    EventProcessor* event = _event_thread.find(name)->second;
    delete event;

    return 0;
}