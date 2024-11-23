
# add source code
add_executable(
    main
    main.cpp
)

target_include_directories(
    main
    PRIVATE
    ${DEFAULT_LIB_INC}
    ${GSTREAMER_INC}
)

# add link libraries
target_link_libraries(
    main
    gstreamer-1.0 gobject-2.0 glib-2.0
    ssl crypto microhttpd
)