
# add source code
add_executable(
    test
    test_main.cpp 
    ../src/config/HttpConfig.cpp ../src/config/VideoConfig.cpp ../src/config/ProgramConfig.cpp ../src/config/CameraConfig.cpp
    ../src/video/VideoInitializer.cpp ../src/video/VideoHandler.cpp ../src/video/VideoStreamerGST.cpp

)

target_include_directories(
    test
    PRIVATE
    "${DEFAULT_LIB_INC}/glib-2.0"
    "${DEFAULT_LIB_INC}/glib-2.0/glib"
    "${CMAKE_SOURCE_DIR}/include"
    ${DEFAULT_LIB_INC}
    ${GSTREAMER_INC}
    ${GLIB_INC_1}
    ${GLIB_INC_2}
)

# add link libraries
target_link_libraries(
    test
    gstreamer-1.0 gobject-2.0 glib-2.0 gstrtspserver-1.0
    # ssl crypto microhttpd
    boost_json
)

# add source code
add_executable(
    test_stream
    test_stream.cpp 
    ../src/config/HttpConfig.cpp ../src/config/VideoConfig.cpp ../src/config/ProgramConfig.cpp ../src/config/CameraConfig.cpp
    ../src/video/VideoInitializer.cpp ../src/video/VideoHandler.cpp ../src/video/VideoStreamerGST.cpp

)

target_include_directories(
    test_stream
    PRIVATE
    "${DEFAULT_LIB_INC}/glib-2.0"
    "${DEFAULT_LIB_INC}/glib-2.0/glib"
    "${CMAKE_SOURCE_DIR}/include"
    ${DEFAULT_LIB_INC}
    ${GSTREAMER_INC}
    ${GLIB_INC_1}
    ${GLIB_INC_2}
)

# add link libraries
target_link_libraries(
    test_stream
    gstreamer-1.0 gobject-2.0 glib-2.0 gstrtspserver-1.0
    # ssl crypto microhttpd
    boost_json
)