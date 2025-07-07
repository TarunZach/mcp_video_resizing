// src/main.cpp
#include "video_reader.hpp"
#include "opencl_driver.hpp"
#include "encoder.hpp"
#include "bounded_queue.hpp"

#include <thread>
#include <iostream>
#include <filesystem>
#include <chrono>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0]
                  << " <input.mp4> <output.mp4>\n";
        return -1;
    }

    const std::string inPath = argv[1];
    const std::string outPath = argv[2];

    // Init components
    VideoReader reader(inPath);
    OpenCLDriver processor;
    int inW = reader.getWidth();
    int inH = reader.getHeight();
    double fps = reader.getFPS();
    int outW = (inW / 2) & ~1;
    int outH = (inH / 2) & ~1;
    Encoder encoder(outPath, outW, outH, fps);

    // Queues for each stage
    const size_t QUEUE_CAPACITY = 4;
    BoundedQueue<cv::Mat> frameQueue(QUEUE_CAPACITY);
    BoundedQueue<std::vector<uint8_t>> yuvQueue(QUEUE_CAPACITY);

    // Metrics
    size_t framesProcessed = 0;
    double totalProcSec = 0.0;
    double totalEncSec = 0.0;
    auto tStart = std::chrono::high_resolution_clock::now();

    // Reader thread
    std::thread readerThread([&]
                             {
        cv::Mat frame;
        while (reader.getNextFrame(frame)) {
            if (!frameQueue.push(frame)) break;
        }
        frameQueue.close(); });

    // Processor thread
    std::thread procThread([&]
                           {
        cv::Mat frame;
        std::vector<uint8_t> yuv;
        while (frameQueue.pop(frame)) {
            auto t0 = std::chrono::high_resolution_clock::now();
            processor.processFrame(frame, yuv, outW, outH);
            auto t1 = std::chrono::high_resolution_clock::now();
            totalProcSec += std::chrono::duration<double>(t1 - t0).count();

            if (!yuvQueue.push(yuv)) break;
        }
        yuvQueue.close(); });

    // Encoder thread
    std::thread encoderThread([&]
                              {
        std::vector<uint8_t> yuv;
        while (yuvQueue.pop(yuv)) {
            auto t2 = std::chrono::high_resolution_clock::now();
            encoder.encodeFrame(yuv);
            auto t3 = std::chrono::high_resolution_clock::now();
            totalEncSec += std::chrono::duration<double>(t3 - t2).count();
            ++framesProcessed;
        }
        encoder.finish(); });

    // Wait for completion
    readerThread.join();
    procThread.join();
    encoderThread.join();

    auto tEnd = std::chrono::high_resolution_clock::now();
    double totalSec = std::chrono::duration<double>(tEnd - tStart).count();

    // File‐size metrics
    uintmax_t inBytes = std::filesystem::file_size(inPath);
    uintmax_t outBytes = std::filesystem::file_size(outPath);
    double compressionRatio = static_cast<double>(outBytes) / inBytes;

    // Print summary
    std::cout << "\n=== Summary ===\n";
    std::cout << "Frames processed      : " << framesProcessed << "\n";
    std::cout << "Total runtime (sec)   : " << totalSec << "\n";
    std::cout << "Overall FPS           : " << (framesProcessed / totalSec) << "\n\n";
    std::cout << "--- Stage timings ---\n";
    std::cout << " Preprocessing (GPU)   : " << totalProcSec
              << " sec (avg " << (totalProcSec / framesProcessed)
              << " sec/frame)\n";
    std::cout << " Encoding (CPU)        : " << totalEncSec
              << " sec (avg " << (totalEncSec / framesProcessed)
              << " sec/frame)\n\n";
    std::cout << "--- Compression ---\n";
    std::cout << " Input size  : " << inBytes << " bytes\n";
    std::cout << " Output size : " << outBytes << " bytes\n";
    std::cout << " Ratio (out/in): " << compressionRatio << "\n";

    return 0;
}
