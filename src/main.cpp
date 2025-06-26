#include "../include/video_reader.hpp"
#include "../include/opencl_driver.hpp"
#include "../include/encoder.hpp"
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

    int width = reader.getWidth();
    int height = reader.getHeight();
    double fps = reader.getFPS();

    Encoder encoder(outPath, width, height, fps);

    cv::Mat frame;
    std::vector<uint8_t> yuv;
    while (reader.getNextFrame(frame))
    {
        processor.processFrame(frame, yuv);
        encoder.encodeFrame(yuv);
    }

    encoder.finish();
    return 0;
}
