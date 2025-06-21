cmake_minimum_required(VERSION 3.10)
project(GPUVideoCompressor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include Headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Dependencies
find_package(OpenCV REQUIRED)
if(APPLE)
    find_library(OpenCL_LIBRARY OpenCL)
    set(OpenCL_INCLUDE_DIR /System/Library/Frameworks/OpenCL.framework/Headers)
else()
    find_package(OpenCL REQUIRED)
endif()
find_package(GTest QUIET)

# Include Directories
include_directories(${OpenCL_INCLUDE_DIR})
include_directories(${OpenCV_INCLUDE_DIRS})

# Libraries
add_library(ResizerLib
    src/opencl_driver.cpp
)
target_link_libraries(ResizerLib PUBLIC ${OpenCL_LIBRARY})

add_library(VideoReaderLib
    src/video_reader.cpp
)
target_link_libraries(VideoReaderLib PUBLIC ${OpenCV_LIBS})


add_library(EncoderLib
    src/encoder.cpp
)

# Main Executable
add_executable(main
    src/main.cpp
)
target_link_libraries(main PRIVATE
    ResizerLib
    VideoReaderLib
    EncoderLib
    ${OpenCV_LIBS}
    ${OpenCL_LIBRARY}
)

# Tests
if(GTest_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(tests)
endif()
