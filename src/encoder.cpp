#include "../include/encoder.hpp"
#include <stdexcept>
#include <iostream>

/*
The CRF (Constant Rate Factor) in x264 can range from 0 (lossless, very large files) up to 51 (lowest quality, smallest files). In practice you’ll typically pick something between about 18 (visually lossless) and 28 (high compression) depending on your needs.

The built-in x264 presets trade off encoding speed vs. compression efficiency. From fastest (least compression) to slowest (best compression), the standard preset names are:

- ultrafast

- superfast

- veryfast

- faster

- fast

- medium (the default)

- slow

- slower

- veryslow

- placebo (only marginal gains over veryslow, but much slower)

So, for example, if you want quickest encoding with larger files you’d use -preset ultrafast, whereas for best file‐size reduction (at the cost of CPU time) you’d choose -preset veryslow (or even placebo). A common balance is -preset veryfast or -preset fast.
*/

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
