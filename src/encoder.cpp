#include "../include/encoder.hpp"
#include <stdexcept>
#include <iostream>

Encoder::Encoder(const std::string &outputPath, int w, int h, double f)
    : ffmpegPipe(nullptr), width(w), height(h), fps(f)
{
    std::string cmd =
        "ffmpeg -y -f rawvideo -pix_fmt yuv420p "
        "-s " +
        std::to_string(width) + "x" + std::to_string(height) +
        " -r " + std::to_string(fps) +
        " -i - -c:v libx264 -preset ultrafast -crf 23 \"" +
        outputPath + "\"";

    ffmpegPipe = popen(cmd.c_str(), "w");
    if (!ffmpegPipe)
    {
        throw std::runtime_error("Failed to open FFmpeg pipe");
    }

    std::cout << "Encoder initialized: " << outputPath << std::endl;
}

void Encoder::encodeFrame(const std::vector<uint8_t> &yuvFrame)
{
    size_t written = fwrite(yuvFrame.data(), 1, yuvFrame.size(), ffmpegPipe);
    if (written != yuvFrame.size())
    {
        throw std::runtime_error("Failed to write frame to FFmpeg");
    }
}

void Encoder::finish()
{
    if (ffmpegPipe)
    {
        pclose(ffmpegPipe);
        ffmpegPipe = nullptr;
    }
}

Encoder::~Encoder()
{
    finish();
}
