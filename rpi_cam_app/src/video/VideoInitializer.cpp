#include "video/VideoInitializer.hpp"
#include "spdlog/spdlog.h"
#include <sstream>
using namespace video;

VideoInitializer::VideoInitializer()
{
    _handler= VideoHandler::get_instance();
    _vid_config = config::ProgramConfig::get_instance()->video_config();
    _remove_enable = false;

}
void VideoInitializer::init(){
    // GStreamer 초기화
    gst_init(0, nullptr); 
}    

int VideoInitializer::start(){
    
    spdlog::info("Starting video streaming...");

    _remove_enable = true;

    _remove_thread = new std::thread([&](){
        int maintain_time = static_cast<int>(_vid_config->maintain());
        std::string path = static_cast<std::string>(_vid_config->save_path());

        while(_remove_enable) {
            _handler->remove_video(maintain_time, path);
            std::this_thread::sleep_for(std::chrono::seconds(maintain_time));
        }
    });
    _remove_thread->detach();
    spdlog::info("Remove Thread started successfully");

    
    const auto address = boost::asio::ip::make_address("0.0.0.0");
    int port_int = 8550;
    unsigned short port_s = static_cast<unsigned short>(port_int);

    try{
        boost::asio::io_context ioc{1};

                
        MulticastStream* handler = MulticastStream::get_instance("239.255.1.1", 5004);
        std::string sdp;
        handler->make_sdp(sdp);
        std::cout<<sdp<<std::endl;
       
        sleep(10);
        handler->set_stream();
        handler->play_stream();

        std::unordered_map<std::string, std::string> path;
        path.insert({"/live", "/dev/video"});
    
        RTSPListener* listener = 
            new RTSPListenerImpl(ioc, boost::asio::ip::tcp::endpoint(address, port_s), path);
    
        listener->do_accept();
        ioc.run();
        
    } catch(std::exception& ex) {
        spdlog::error("ERRORERROR {}", ex.what());
    }

    return 0;
}

int VideoInitializer::stop(){
    
    spdlog::info("Stop video streaming...");

    _remove_enable = false;
    _remove_thread->join();

    delete _remove_thread;

    spdlog::info("Remove Thread stopped successfully");

    return 0;
}
