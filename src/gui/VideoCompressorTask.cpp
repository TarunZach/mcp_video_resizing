#include "VideoCompressorTask.hpp"
#include "video_reader.hpp"
#include "opencl_driver.hpp"
#include "encoder.hpp"
#include "bounded_queue.hpp"

#include <opencv2/opencv.hpp>
#include <thread>
#include <chrono>
#include <vector>
#include <stdexcept>

VideoCompressorTask::VideoCompressorTask(QObject *parent)
    : QObject(parent)
{
}
VideoCompressorTask::~VideoCompressorTask() {}

void VideoCompressorTask::compress(const QString &inPathQs,
                                   const QString &outPathQs,
                                   int crf,
                                   int preset,
                                   QString *errorMsg)
{
  // Convert Qt strings to std::string
  std::string inPath = inPathQs.toStdString();
  std::string outPath = outPathQs.toStdString();

  try
  {
    VideoReader reader(inPath);
    OpenCLDriver processor;
    int inW = reader.getWidth();
    int inH = reader.getHeight();
    double fps = reader.getFPS();
    int outW = (inW / 2) & ~1, outH = (inH / 2) & ~1;
    Encoder encoder(outPath, outW, outH, fps);

    const size_t QUEUE_CAP = 4;
    BoundedQueue<cv::Mat> frameQ(QUEUE_CAP);
    BoundedQueue<std::vector<uint8_t>> yuvQ(QUEUE_CAP);

    size_t processed = 0;
    auto t0 = std::chrono::high_resolution_clock::now();

    // Reader
    std::thread r([&]()
                  {
            cv::Mat f;
            while(reader.getNextFrame(f)) {
                if (!frameQ.push(f)) break;
            }
            frameQ.close(); });

    // Processor
    std::thread p([&]()
                  {
            cv::Mat f; std::vector<uint8_t> yuv;
            while(frameQ.pop(f)) {
                processor.processFrame(f, yuv, outW, outH);
                if (!yuvQ.push(yuv)) break;
            }
            yuvQ.close(); });

    // Encoder
    std::thread e([&]()
                  {
            std::vector<uint8_t> yuv;
            while(yuvQ.pop(yuv)) {
                encoder.encodeFrame(yuv);
                ++processed;
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - t0).count();
                double percent = (100.0 * processed) / reader.getFPS() /*approx frames*/;
                double eta     = elapsed * (1.0/percent*100 - 1.0);
                emit progress(percent, elapsed, eta);
                // emit progressStatsUpdateRequested();
            }
            encoder.finish(); });

    r.join();
    p.join();
    e.join();
    emit finished(true, QString());
  }
  catch (const std::exception &ex)
  {
    if (errorMsg)
      *errorMsg = QString::fromUtf8(ex.what());
    emit finished(false, *errorMsg);
  }
}
