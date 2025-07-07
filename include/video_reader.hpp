#ifndef VIDEO_READER_HPP
#define VIDEO_READER_HPP

#include <string>
#include <opencv2/opencv.hpp>

class VideoReader
{
public:
    VideoReader(const std::string &path);
    bool getNextFrame(cv::Mat &frame);

    int getWidth() const;
    int getHeight() const;
    double getFPS() const;

private:
    cv::VideoCapture cap_;
};

#endif
