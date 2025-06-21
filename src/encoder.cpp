#include "encoder.hpp"
#include <iostream>
#include <cstdint>

Encoder::Encoder(const std::string &outputPath)
{
    // TODO: initialize encoder (e.g., open output file, configure codec)
    std::cout << "Encoder initialized for " << outputPath << std::endl;
}

void Encoder::encodeFrame(const std::vector<uint8_t> &yuvBuffer)
{
    // TODO: feed YUV buffer to encoder
}

Encoder::~Encoder()
{
    // TODO: cleanup encoder resources
}

/* On Windows, the standard popen() and pclose() functions are named _popen() and _pclose().

These macros ensure compatibility across platforms by redirecting to the correct names on Windows.*/
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

#include "encoder.hpp" // Header file that declares the Encoder class.
#include <stdexcept>   // Provides standard exceptions like std::runtime_error
#include <string>      // Used for C++ string operations.
#include <cstdio>      // for popen, pclose, FILE*

/*outputPath: where the MP4 video will be saved.
w, h: the width and height of each video frame.
f: the frame rate (frames per second).*/
Encoder::Encoder(const std::string &outputPath, int w, int h, double f)
    : ffmpegPipe(nullptr), width(w), height(h), fps(f)
{
    // Build FFmpeg command: read raw YUV420p frames from stdin and encode to MP4
    // If we need to use another format I need to change this code
    // Can make it user  selectable
    std::string cmd =
        "ffmpeg -y "
        "-f rawvideo -pix_fmt yuv420p "
        "-s " +
        std::to_string(width) + "x" + std::to_string(height) + " "
                                                               "-r " +
        std::to_string(fps) + " "
                              "-i - "
                              "-c:v libx264 -preset ultrafast -crf 23 " +
        outputPath;

    // Open FFmpeg process for writing
    ffmpegPipe = popen(cmd.c_str(), "w");
    if (!ffmpegPipe)
    {
        throw std::runtime_error("Failed to open FFmpeg pipe");
    }
}

Encoder::~Encoder()
{
    // Ensure the pipe is closed on destruction
    if (ffmpegPipe)
    {
        finish();
    }
}

void Encoder::encodeFrame(const std::vector<uint8_t> &yuvFrame)
{
    // Write raw frame data to FFmpeg stdin
    size_t written = fwrite(yuvFrame.data(), 1, yuvFrame.size(), ffmpegPipe);
    if (written != yuvFrame.size())
    {
        throw std::runtime_error("Failed to write frame to FFmpeg");
    }
}

void Encoder::finish()
{
    // Close the FFmpeg process
    pclose(ffmpegPipe);
    ffmpegPipe = nullptr;
}

// ────── TEST HARNESS ──────
// Define ENCODER_TEST to compile this main()
// e.g.: g++ -std=c++11 -DENCODER_TEST encoder.cpp -o encoder_test
#ifdef ENCODER_TEST

#include <vector>
#include <cstdint>
#include <iostream>

int main()
{
    // English comment: Test parameters for a gray video
    const int W = 320;         // width
    const int H = 240;         // height
    const double FPS = 30.0;   // frames per second
    const int NUM_FRAMES = 90; // total frames

    // English comment: Initialize encoder with test output file
    try
    {
        Encoder encoder("test_out.mp4", W, H, FPS);

        // English comment: Create one gray YUV420p frame
        std::vector<uint8_t> frame(W * H * 3 / 2, 128);

        // English comment: Feed frames to encoder
        for (int i = 0; i < NUM_FRAMES; ++i)
        {
            encoder.encodeFrame(frame);
        }

        // English comment: Finish encoding
        encoder.finish();
        std::cout << "Test video 'test_out.mp4' generated successfully.\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "Encoder test failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
#endif // ENCODER_TEST