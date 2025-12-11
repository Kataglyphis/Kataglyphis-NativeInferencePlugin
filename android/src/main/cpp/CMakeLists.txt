cmake_minimum_required(VERSION 3.22)
project(kataglyphis_native_inference LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{GSTREAMER_ROOT_ANDROID})
    message(FATAL_ERROR "GSTREAMER_ROOT_ANDROID environment variable must point to the GStreamer Android SDK root")
endif()

set(GSTREAMER_ROOT_ANDROID $ENV{GSTREAMER_ROOT_ANDROID})
list(PREPEND CMAKE_PREFIX_PATH ${GSTREAMER_ROOT_ANDROID})
list(PREPEND CMAKE_FIND_ROOT_PATH ${GSTREAMER_ROOT_ANDROID})

find_package(GStreamer REQUIRED COMPONENTS gstreamer-1.0 gstreamer-video-1.0)

add_library(kataglyphis_native_inference SHARED gstreamer_native.cpp)

target_link_libraries(kataglyphis_native_inference
    PRIVATE
        GStreamer::gstreamer-1.0
        GStreamer::gstreamer-video-1.0
        android
        log)

target_compile_definitions(kataglyphis_native_inference PRIVATE GST_DISABLE_DEPRECATED)
