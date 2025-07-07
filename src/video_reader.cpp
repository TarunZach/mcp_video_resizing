#include "video_reader.hpp"
#include <stdexcept>

VideoReader::VideoReader(const std::string &path)
    : cap_(path)
{
    if (!cap_.isOpened())
        throw std::runtime_error("Failed to open video file");
}

bool VideoReader::getNextFrame(cv::Mat &frame)
{
    return cap_.read(frame);
}
int VideoReader::getWidth() const
{
    return static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_WIDTH));
}

int VideoReader::getHeight() const
{
    return static_cast<int>(cap_.get(cv::CAP_PROP_FRAME_HEIGHT));
}

double VideoReader::getFPS() const
{
    return cap_.get(cv::CAP_PROP_FPS);
}