#include "event/EventProcessor.hpp"
#include "event/EventHandler.hpp"
#include "event/EventInitializer.hpp"
#include "spdlog/spdlog.h"

#include <filesystem>

using namespace event;

EventInitializer::EventInitializer()
{
    _event_handler = EventHandler::get_instance();
}

int EventInitializer::init()
{
    for(const auto& entry: std::filesystem::directory_iterator("event")){
        std::string name = entry.path().filename().string();

        int ret = _event_handler->start_event(name);
        _event_names.push_back(name);

        spdlog::info("Event \"{}\" started", name);
    }

    return 0 ;
}

int EventInitializer::start()
{
    spdlog::info("all event done!");

    return 0;
}

int EventInitializer::stop()
{
    for(std::string& name : _event_names){
        _event_handler->stop_event(name);
    }

    return 0;
}