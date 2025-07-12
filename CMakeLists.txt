cmake_minimum_required(VERSION 3.10)
project(GPUVideoCompressor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Where our headers live
set(INC_DIR ${CMAKE_SOURCE_DIR}/include)
file(GLOB PROJECT_HEADERS "${INC_DIR}/*.hpp")

# Find dependencies
find_package(OpenCV   REQUIRED)
find_package(OpenCL   REQUIRED)
find_package(Qt6      REQUIRED COMPONENTS Core Widgets Concurrent Charts)
find_package(Threads  REQUIRED)
find_package(GTest    QUIET)

#
# Backend libraries
#

# ResizerLib: OpenCL + OpenCV
add_library(ResizerLib
    src/opencl_driver.cpp
)
target_include_directories(ResizerLib
    PUBLIC
      ${INC_DIR}
      ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(ResizerLib
    PUBLIC
      OpenCL::OpenCL
      ${OpenCV_LIBS}
)

# VideoReaderLib: OpenCV
add_library(VideoReaderLib
    src/video_reader.cpp
)
target_include_directories(VideoReaderLib
    PUBLIC
      ${INC_DIR}
      ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(VideoReaderLib
    PUBLIC
      ${OpenCV_LIBS}
)

# EncoderLib: your own headers (add FFmpeg later if needed)
add_library(EncoderLib
    src/encoder.cpp
)
target_include_directories(EncoderLib
    PUBLIC
      ${INC_DIR}
)

#
# CLI executable
#
add_executable(video_compressor
    src/main.cpp
)
target_include_directories(video_compressor
    PRIVATE
      ${INC_DIR}
)
target_link_libraries(video_compressor
    PRIVATE
      ResizerLib
      VideoReaderLib
      EncoderLib
      Threads::Threads
)

#
# GUI executable
#
add_executable(video_gui
    src/gui/main.cpp
    src/gui/MainWindow.cpp
    src/gui/ProcessMonitor.cpp
    src/gui/TimerEventFilter.cpp
    src/gui/VideoCompressorTask.cpp
    ${PROJECT_HEADERS} 
)
target_include_directories(video_gui
    PRIVATE
      ${INC_DIR}
)
target_link_libraries(video_gui
    PRIVATE
      Qt6::Core
      Qt6::Widgets
      Qt6::Concurrent
      Qt6::Charts
      ResizerLib
      VideoReaderLib
      EncoderLib
      Threads::Threads
)

#
# Optional tests
#
if(GTest_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()
