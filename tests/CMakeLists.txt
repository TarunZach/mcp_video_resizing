add_executable(tests tests.cpp)

target_include_directories(tests
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(tests
  PRIVATE
    ResizerLib
    VideoReaderLib
    EncoderLib
    ${OpenCV_LIBS}
    ${OpenCL_LIBRARY}
    GTest::GTest
    GTest::Main
)

add_test(
  NAME ResizeTests
  COMMAND tests
)
