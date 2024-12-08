
# add source code
add_executable(
    main
    main.cpp
    config/HttpConfig.cpp config/VideoConfig.cpp config/ProgramConfig.cpp config/CameraConfig.cpp
    http/EventHandlerHTTP.cpp http/HttpInitializer.cpp http/libmicrohttpd_handler.cpp http/post_request_complete.cpp
    video/VideoInitializer.cpp video/VideoHandler.cpp video/VideoStreamerGST.cpp
    camera_device/VideoQueue.cpp camera_device/CameraInitializer.cpp
    event/EventHandler.cpp event/EventInitializer.cpp event/EventProcessor.cpp
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
    gstreamer-1.0 gobject-2.0 glib-2.0 gstrtspserver-1.0
    ssl crypto microhttpd curl
    boost_json
)
