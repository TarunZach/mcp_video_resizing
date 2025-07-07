cmake_minimum_required(VERSION 3.10)
project(GPUVideoCompressor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Project headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Find dependencies
find_package(OpenCV REQUIRED)
find_package(OpenCL REQUIRED)
find_package(GTest QUIET)

# ResizerLib: needs OpenCL + OpenCV headers/libs
add_library(ResizerLib
    src/opencl_driver.cpp
)
target_include_directories(ResizerLib
    PUBLIC
      ${CMAKE_SOURCE_DIR}/include
      ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(ResizerLib
    PUBLIC
      OpenCL::OpenCL
      ${OpenCV_LIBS}
)

# VideoReaderLib: needs OpenCV headers/libs
add_library(VideoReaderLib
    src/video_reader.cpp
)
target_include_directories(VideoReaderLib
    PUBLIC
      ${CMAKE_SOURCE_DIR}/include
      ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(VideoReaderLib
    PUBLIC
      ${OpenCV_LIBS}
)

# EncoderLib: only your own headers (and FFmpeg if you add it later)
add_library(EncoderLib
    src/encoder.cpp
)
target_include_directories(EncoderLib
    PUBLIC
      ${CMAKE_SOURCE_DIR}/include
)

# Main executable
add_executable(video_compressor
    src/main.cpp
)
target_include_directories(video_compressor
    PRIVATE
      ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(video_compressor
    PRIVATE
      ResizerLib
      VideoReaderLib
      EncoderLib
)

# Optional tests
if(GTest_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()
