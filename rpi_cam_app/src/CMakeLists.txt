
# add source code
add_executable(
    main
    main.cpp
    config/HttpConfig.cpp config/VideoConfig.cpp config/ProgramConfig.cpp config/CameraConfig.cpp
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
    gstreamer-1.0 gobject-2.0 glib-2.0
    ssl crypto microhttpd
    boost_json
)