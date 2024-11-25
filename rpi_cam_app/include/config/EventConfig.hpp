#ifndef _EVENT_CONFIG
#define _EVENT_CONFIG

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <unordered_map>
#include <vector>

#include "config/Config.hpp"

namespace config
{
    class EventConfig:  public Config
    {
    private:
        boost::property_tree::ptree _props;

        /// @brief event 이름의 배열, 해당 event의 .so 파일을 읽기 위해 필요
        std::vector<std::string> _name;
        /// @brief event가 요구하는 fps, -1이면 별도 이미지를 요구하지 않음
        std::unordered_map<std::string, int> _fps;

        int read_config() override;
    public:
        EventConfig() = default;

        int set_file(const std::string& json_file) override;

        /// @brief event 이름의 배열을 받아옴
        /// @return event 이름의 vector
        const std::vector<std::string>& event_name() const;
        /// @brief event가 요구하는 fps
        /// @param name event의 명칭
        /// @return fps
        const int event_fps(const std::string& name) const;
    };
};

#endif