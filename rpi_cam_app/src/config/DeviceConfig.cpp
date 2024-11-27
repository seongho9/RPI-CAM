#include <boost/foreach.hpp>
#include <boost/property_tree/exceptions.hpp>

#include "config/DeviceConfig.hpp"
#include "spdlog/spdlog.h"

using namespace config;


int DeviceConfig::set_file(const std::string& json_file)
{
    try {
        boost::property_tree::read_json(json_file, _props);

        _props = _props.get_child("device");
    }
    catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());

        return 1;
    }

    int ret = read_config();

    return ret;
}

int DeviceConfig::read_config()
{
    try {
        _server_address = _props.get<std::string>("remote_server");
        boost::property_tree::ptree groups = _props.get_child("event_group");

        BOOST_FOREACH(boost::property_tree::ptree::value_type& vt, groups){
            std::string g_name = vt.second.get<std::string>("");
            _event_group.push_back(g_name);
        }
        std::string mode_str = _props.get<std::string>("mode");

        if(mode_str == "master") {
            _mode = DEVICE_MODE::MASTER;
        }
        else if(mode_str == "slave") {
            _mode = DEVICE_MODE::SLAVE;
        }
        else {
            boost::property_tree::ptree_bad_data ex("No Such Mode Type",mode_str);
            throw ex;
        }
        
    }
    catch (boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());
    }
    catch (boost::property_tree::ptree_bad_data& ex) {
        spdlog::error("{}", ex.what());
    }
}

const std::string& DeviceConfig::server_address() const
{
    return _server_address;
}

const std::vector<std::string>& DeviceConfig::event_group() const
{
    return _event_group;
}

const DEVICE_MODE& DeviceConfig::mode() const
{
    return _mode;
}

std::string convert_device_mode_str(DEVICE_MODE mode)
{
    switch(mode)
    {
    case DEVICE_MODE::MASTER:
        return "master";
    case DEVICE_MODE::SLAVE:
        return "slave";
    default:
        return "none";
    }

}