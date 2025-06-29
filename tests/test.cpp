#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include "opencl_driver.hpp"
#include "video_reader.hpp"
#include "encoder.hpp"

// Test that OpenCLDriver resizes correctly
TEST(OpenCLDriverTest, ResizesFrameToHalf)
{
    OpenCLDriver driver;

    // Create a dummy 640x480 BGR image
    int inputW = 640, inputH = 480;
    cv::Mat input(inputH, inputW, CV_8UC3, cv::Scalar(100, 150, 200));

    std::vector<uint8_t> yuvOutput;
    driver.setOutputSize(inputW / 2, inputH / 2);
    driver.processFrame(input, yuvOutput);

    // YUV420p = Y (w*h) + U (w*h/4) + V (w*h/4)
    size_t expectedSize = (inputW / 2) * (inputH / 2) * 3 / 2;
    ASSERT_EQ(yuvOutput.size(), expectedSize);
}

// Test that VideoReader reads frames
TEST(VideoReaderTest, LoadsFirstFrame)
{
    std::string samplePath = "test_assets/sample.mp4";
    VideoReader reader(samplePath);

    cv::Mat frame;
    ASSERT_TRUE(reader.getNextFrame(frame));
    ASSERT_FALSE(frame.empty());
}

// Optional: minimal encoder pipeline test
TEST(EncoderTest, InitializesEncoder)
{
    std::string outputPath = "test_output.mp4";
    int w = 320, h = 240;
    double fps = 30.0;
    Encoder encoder(outputPath, w, h, fps);

    std::vector<uint8_t> dummyFrame(w * h * 3 / 2, 128); // Grey YUV frame
    encoder.encodeFrame(dummyFrame);
    encoder.finish();

    std::ifstream outFile(outputPath);
    ASSERT_TRUE(outFile.good());
}
