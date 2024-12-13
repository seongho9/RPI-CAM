#ifndef _EVENT_INIT_H
#define _EVENT_INIT_H

#include "utils/Initializer.hpp"
#include "event/EventHandler.hpp"
#include <vector>

namespace event
{
    class EventInitializer: public utils::Initialzier
    {
    private:
        EventHandler* _event_handler;
        std::vector<std::string> _event_names;

    public:
        EventInitializer();

        /// @brief 파일에 있는 이벤트들 읽어옴
        /// @return 0 성공
        int init() override;
        /// @brief 시작 로그 남김
        /// @return 0 성공
        int start() override;

        /// @brief 이벤트 스레드 종료
        /// @return 0 성공
        int stop() override;
    };
};

#endif