
#include "spdlog/spdlog.h"
#include "config/EventConfig.hpp"

#include <boost/foreach.hpp>

using namespace config;

int EventConfig::set_file(const std::string& json_file)
{
    try {
        boost::property_tree::read_json(json_file, _props);
        _props = _props.get_child("events");
    } catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());
        return 1;
    }

    int ret = read_config();

    return ret;
}

int EventConfig::read_config()
{
    try {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &vt, _props)
        {
            std::string name = vt.second.get<std::string>("event_name");
            int fps = vt.second.get<int>("fps");

            _name.push_back(name);
            _fps.insert({name,fps});
        }
    } catch(boost::property_tree::ptree_bad_path& ex) {
        spdlog::error("path error : {}", ex.what());
        return 1;
    }
    return 0;
}
 
const std::vector<std::string>& EventConfig::event_name() const
{
    return _name;
}

const int EventConfig::event_fps(const std::string& name) const
{
    auto item = _fps.find(name);

    if(item == _fps.end()){
        return -1;
    }

    return item->second;
}

