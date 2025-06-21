add_executable(tests test.cpp)

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

enable_testing()
add_test(
  NAME ResizeTests
  COMMAND tests
)
