#include "video_reader.hpp"
#include "opencl_driver.hpp"
#include "encoder.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input.mp4> <output.mp4>\n";
        return -1;
    }

    std::string inPath = argv[1], outPath = argv[2];
    VideoReader reader(inPath);
    OpenCLDriver processor;

    int inputWidth = reader.getWidth();
    int inputHeight = reader.getHeight();
    double fps = reader.getFPS();

    // For now, resize to 50%
    int outputWidth = inputWidth / 2;
    int outputHeight = inputHeight / 2;

    // Make sure width and height are even (for YUV420)
    outputWidth &= ~1;
    outputHeight &= ~1;

    Encoder encoder(outPath, outputWidth, outputHeight, fps);

    cv::Mat frame;
    std::vector<uint8_t> yuv;

    while (reader.getNextFrame(frame))
    {
        processor.processFrame(frame, yuv, outputWidth, outputHeight);
        encoder.encodeFrame(yuv);
    }

    encoder.finish();
    return 0;
}
