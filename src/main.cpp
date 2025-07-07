// src/main.cpp
#include "video_reader.hpp"
#include "opencl_driver.hpp"
#include "encoder.hpp"

#include <iostream>
#include <chrono>
#include <filesystem>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <input.mp4> <output.mp4>\n";
        return -1;
    }

    const std::string inPath = argv[1];
    const std::string outPath = argv[2];

    // Setup
    VideoReader reader(inPath);
    OpenCLDriver processor;
    Encoder encoder(outPath,
                    /*width*/ reader.getWidth() / 2 & ~1,
                    /*height*/ reader.getHeight() / 2 & ~1,
                    reader.getFPS());

    cv::Mat frame;
    std::vector<uint8_t> yuv;

    // Timing accumulators
    size_t framesProcessed = 0;
    double totalProcSec = 0.0; // GPU resize+convert time
    double totalEncSec = 0.0;  // encoding time

    auto tStart = std::chrono::high_resolution_clock::now();

    // Main loop
    while (reader.getNextFrame(frame))
    {
        // 1) Pre‐process (OpenCL)
        auto t0 = std::chrono::high_resolution_clock::now();
        processor.processFrame(frame, yuv,
                               reader.getWidth() / 2 & ~1,
                               reader.getHeight() / 2 & ~1);
        auto t1 = std::chrono::high_resolution_clock::now();
        totalProcSec += std::chrono::duration<double>(t1 - t0).count();

        // 2) Encode (FFmpeg pipe)
        auto t2 = std::chrono::high_resolution_clock::now();
        encoder.encodeFrame(yuv);
        auto t3 = std::chrono::high_resolution_clock::now();
        totalEncSec += std::chrono::duration<double>(t3 - t2).count();

        ++framesProcessed;
    }

    encoder.finish();

    auto tEnd = std::chrono::high_resolution_clock::now();
    double totalSec = std::chrono::duration<double>(tEnd - tStart).count();

    //  Filesize & Compression Ratio
    uintmax_t inBytes = std::filesystem::file_size(inPath);
    uintmax_t outBytes = std::filesystem::file_size(outPath);
    double compressionRatio = static_cast<double>(outBytes) / inBytes;

    //  Print summary
    std::cout << "\n Processing Summary \n";
    std::cout << "Frames processed      : " << framesProcessed << "\n";
    std::cout << "Total runtime (sec)   : " << totalSec << "\n";
    std::cout << "Overall FPS           : " << (framesProcessed / totalSec) << "\n\n";

    std::cout << " Stage timings \n";
    std::cout << "  Preprocessing (GPU) : " << totalProcSec
              << " sec (avg " << (totalProcSec / framesProcessed) << " sec/frame)\n";
    std::cout << "  Encoding (CPU)      : " << totalEncSec
              << " sec (avg " << (totalEncSec / framesProcessed) << " sec/frame)\n\n";

    std::cout << " Compression \n";
    std::cout << "  Input size  : " << inBytes << " bytes\n";
    std::cout << "  Output size : " << outBytes << " bytes\n";
    std::cout << "  Ratio (out/in): " << compressionRatio << "\n";

    return 0;
}
