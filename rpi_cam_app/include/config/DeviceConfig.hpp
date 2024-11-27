#ifndef _REMOTE_SERVER_CONFIG_H
#define _REMOTE_SERVER_CONFIG_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>

#include "config/Config.hpp"

namespace config
{
    enum DEVICE_MODE
    {
        NONE = 0,   // 0
        MASTER,     // 1
        SLAVE,      // 2
    };

    std::string convert_device_mode_str(DEVICE_MODE mode);
    class DeviceConfig : public Config
    {
    private:
        boost::property_tree::ptree _props;
        
        std::string _server_address;
        std::vector<std::string> _event_group;
        DEVICE_MODE _mode;

        int read_config() override;
    
    public:
        DeviceConfig() = default;

        int set_file(const std::string& json_file) override;

        /// @brief 리모트 서버의 주소
        /// @return 리모트 서버의 주소
        const std::string& server_address() const;
        /// @brief 해당 디바이스가 속한 이벤트 그룹을 가져옴
        /// @return 이벤트 그룹명 
        const std::vector<std::string>& event_group() const;
        /// @brief 해당 디바이스의 모드
        /// @return DEVICE_MODE에 있는 해당 모드
        const DEVICE_MODE& mode() const;
    };
};
#endif