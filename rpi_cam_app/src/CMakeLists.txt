
# add source code
add_executable(
    main
    main.cpp
    config/HttpConfig.cpp config/VideoConfig.cpp config/ProgramConfig.cpp config/CameraConfig.cpp
    http/EventHandlerHTTP.cpp http/HttpInitializer.cpp http/libmicrohttpd_handler.cpp http/post_request_complete.cpp
    video/VideoInitializer.cpp video/VideoHandler.cpp video/GstreamerCallbacks.cpp
    camera_device/VideoQueue.cpp camera_device/CameraInitializer.cpp
    event/EventHandler.cpp event/EventInitializer.cpp event/EventProcessor.cpp

    
    rtsp/server/RTSPListener.cpp
    rtsp/session/RTSPSession.cpp rtsp/session/RTSPSessionImpl.cpp 

    rtsp/handler/gst_file/RTSPSessionHandlerGstFile.cpp 
    rtsp/handler/gst_file/RTSPSessionHandlerGstFileDescribe.cpp  rtsp/handler/gst_file/RTSPSessionHandlerGstFileOption.cpp 
    rtsp/handler/gst_file/RTSPSessionHandlerGstFilePause.cpp     rtsp/handler/gst_file/RTSPSessionHandlerGstFilePlay.cpp
    rtsp/handler/gst_file/RTSPSessionHandlerGstFileSetup.cpp     rtsp/handler/gst_file/RTSPSessionHandlerGstFileTeardown.cpp

    rtsp/handler/gst_live/MulticastStream.cpp
    rtsp/handler/gst_live/MulticastStreamV4L2.cpp               rtsp/handler/gst_live/MulticastStreamV4L2MakeSdp.cpp  
    rtsp/handler/gst_live/MulticastStreamV4L2PlayStream.cpp     rtsp/handler/gst_live/MulticastStreamV4L2SetStream.cpp
    rtsp/handler/gst_live/RTSPSessionHandlerGstLive.cpp    
    
)

target_include_directories(
    main
    PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    ${DEFAULT_LIB_INC}
    ${GSTREAMER_INC}
)

# add link libraries
target_link_libraries(
    main
    gstreamer-1.0 gobject-2.0 glib-2.0 gstrtspserver-1.0 gstrtsp-1.0 gstapp-1.0 gstsdp-1.0 gstrtp-1.0
    ssl crypto microhttpd curl
    boost_json
)
