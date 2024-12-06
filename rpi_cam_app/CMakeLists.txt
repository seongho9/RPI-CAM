# CMakeLists.txx
cmake_minimum_required(VERSION 3.12)
project(SNACK_CAM VERSION 0.1)
cmake_policy(SET CMP0079 NEW)

# set target os and architecture
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# target environment
set(CMAKE_SYSROOT "/opt/poky/4.1.23/sysroots/cortexa76-poky-linux")
set(CMAKE_FIND_ROOT_PATH "/opt/poky/4.1.23/sysroots/cortexa76-poky-linux")

# search for program in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# tool-chain
set(BUILD_PATH "/opt/poky/4.1.23/sysroots/x86_64-pokysdk-linux")
set(CMAKE_TOOLCHAIN_FILE "${BUILD_PATH}/usr/bin")
set(CMAKE_C_COMPILER "${BUILD_PATH}/usr/bin/aarch64-poky-linux/aarch64-poky-linux-gcc")
set(CMAKE_CXX_COMPILER "${BUILD_PATH}/usr/bin/aarch64-poky-linux/aarch64-poky-linux-g++")
# compiler flags
set(CMAKE_C_FLAGS 
    "-mcpu=cortex-a76 -march=armv8.2-a+crypto -mbranch-protection=standard -fstack-protector-strong -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security")
set(CMAKE_CXX_FLAGS 
    "-mcpu=cortex-a76 -march=armv8.2-a+crypto -mbranch-protection=standard -fstack-protector-strong -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security")

# CXX set
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

# debug flag
set(CMAKE_BUILD_TYPE Debug)

# libraries setting

# default library path
set(DEFAULT_LIB_INC "${CMAKE_SYSROOT}/usr/include")
set(DEFAULT_LIB_LIB "${CMAKE_SYSROOT}/usr/lib")

message(STATUS "DEFAULT_LIB_INC ${DEFAULT_LIB_INC}")
message(STATUS "DEFAULT_LIB_LIB ${DEFAULT_LIB_LIB}")

link_directories(${DEFAULT_LIB_LIB})
include_directories(${DEFAULT_LIB_INC})

# gstreamer
set(GSTREAMER_LIB "${CMAKE_SYSROOT}/usr/lib/gstreamer-1.0")
set(GSTREAMER_INC "${CMAKE_SYSROOT}/usr/include/gstreamer-1.0")

link_directories(${GSTREAMER_LIB})
include_directories(${GSTREAMER_INC})

message(STATUS "GSTREAMER_LIB: ${GSTREAMER_LIB}")
message(STATUS "GSTREAMER_INC: ${GSTREAMER_INC}")

# glib
set(GLIB_INC_1 "${CMAKE_SYSROOT}/usr/include/glib-2.0")
set(GLIB_INC_2 "${CMAKE_SYSROOT}/usr/lib/glib-2.0/include")

include_directories(${GLIB_INC_1})
include_directories(${GLIB_INC_2})

message(STATUS "GLIB_INC: ${GLIB_INC_1}")
message(STATUS "GLIB_INC: ${GLIB_INC_2}")

# project source setting

add_subdirectory(src)

add_subdirectory(test)

include_directories("${CMAKE_SOURCE_DIR}/include")

message(STATUS "${CMAKE_SOURCE_DIR}/include")