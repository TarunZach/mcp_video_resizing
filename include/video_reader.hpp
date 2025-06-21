#ifndef VIDEO_READER_HPP
#define VIDEO_READER_HPP

#include <string>
#include <opencv2/opencv.hpp>

class VideoReader {
public:
    VideoReader(const std::string &path);
    bool getNextFrame(cv::Mat &frame);

private:
    cv::VideoCapture cap_;
};

#endif