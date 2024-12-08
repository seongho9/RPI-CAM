set(CV_LIBS 
    "-lopencv_gapi -lopencv_stitching -lopencv_ts -lopencv_alphamat -lopencv_aruco\
    -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_fuzzy -lopencv_hfs\
    -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_quality\
    -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_sfm -lopencv_stereo\
    -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow\
    -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_plot\
    -lopencv_videostab -lopencv_videoio -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc\
    -lopencv_video -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_flann -lopencv_xphoto\
    -lopencv_photo -lopencv_imgproc -lopencv_core")

# add source code

add_executable(
    test_camera_init
    camera_device/CameraInitializerTest.cpp
    ../src/camera_device/CameraInitializer.cpp ../src/camera_device/VideoQueue.cpp
    ../src/config/CameraConfig.cpp ../src/config/HttpConfig.cpp ../src/config/ProgramConfig.cpp ../src/config/VideoConfig.cpp    
)
target_include_directories(
    test_camera_init
    PRIVATE
    "${DEFAULT_LIB_INC}/opencv4"
    ${DEFAULT_LIB_INC}
    ${GSTREAMER_INC}
    "${CMAKE_SOURCE_DIR}/include"

)
target_link_libraries(
    test_camera_init
    gstreamer-1.0 gobject-2.0 glib-2.0
    ssl crypto microhttpd
    boost_json boost_unit_test_framework 
    ${CV_LIBS}
    gstreamer-1.0 gobject-2.0 glib-2.0 gstrtspserver-1.0
    ssl crypto microhttpd
    boost_json
)

# target_include_directories(
#     test
#     PRIVATE
#     "${DEFAULT_LIB_INC}/glib-2.0"
#     "${DEFAULT_LIB_INC}/glib-2.0/glib"
#     "${CMAKE_SOURCE_DIR}/include"
#     ${DEFAULT_LIB_INC}
#     ${GSTREAMER_INC}
#     "${CMAKE_SOURCE_DIR}/include"

#     ${GLIB_INC_1}
#     ${GLIB_INC_2}
# )


# add_executable(
#     test_video_queue
#     camera_device/VideoQueueTest.cpp
#     ../src/camera_device/CameraInitializer.cpp ../src/camera_device/VideoQueue.cpp
#     ../src/config/CameraConfig.cpp ../src/config/HttpConfig.cpp ../src/config/ProgramConfig.cpp ../src/config/VideoConfig.cpp
# )
# # add link libraries
# target_link_libraries(
#     test_video_queue
#     gstreamer-1.0 gobject-2.0 glib-2.0
#     ssl crypto microhttpd
#     boost_json boost_unit_test_framework
#     ${CV_LIBS}
# )


# # add source code
# add_executable(
#     test_stream
#     test_stream.cpp 
#     ../src/config/HttpConfig.cpp ../src/config/VideoConfig.cpp ../src/config/ProgramConfig.cpp ../src/config/CameraConfig.cpp
#     ../src/video/VideoInitializer.cpp ../src/video/VideoHandler.cpp ../src/video/VideoStreamerGST.cpp

# )

# target_include_directories(
#     test_stream
#     PRIVATE
#     "${DEFAULT_LIB_INC}/glib-2.0"
#     "${DEFAULT_LIB_INC}/glib-2.0/glib"
#     "${CMAKE_SOURCE_DIR}/include"
#     ${DEFAULT_LIB_INC}
#     ${GSTREAMER_INC}
#     ${GLIB_INC_1}
#     ${GLIB_INC_2}
# )

# # add link libraries
# target_link_libraries(
#     test_stream
#     gstreamer-1.0 gobject-2.0 glib-2.0 gstrtspserver-1.0
#     # ssl crypto microhttpd
#     boost_json
# )