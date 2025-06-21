#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>

class Encoder
{
public:
    // Constructor: initialize with output path, frame width & height, and frames per second
    Encoder(const std::string &outputPath, int width, int height, double fps);
    ~Encoder();

    // Encode a single frame (YUV420p raw data)
    void encodeFrame(const std::vector<uint8_t> &yuvFrame);

    // Finalize encoding and close FFmpeg process
    void finish();

private:
    FILE *ffmpegPipe;
    int width;
    int height;
    double fps;
};