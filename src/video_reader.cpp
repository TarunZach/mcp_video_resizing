#include "video_reader.hpp"
#include <stdexcept>

VideoReader::VideoReader(const std::string &path)
    : cap_(path)
{
    if (!cap_.isOpened())
        throw std::runtime_error("Failed to open video file");
}

bool VideoReader::getNextFrame(cv::Mat &frame) {
    return cap_.read(frame);
}
