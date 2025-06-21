#ifndef OPENCL_DRIVER_HPP
#define OPENCL_DRIVER_HPP

#include <string>
#include <vector>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <opencv2/core.hpp>

class OpenCLDriver {
public:
    OpenCLDriver();
    ~OpenCLDriver();

    void processFrame(const cv::Mat &input, std::vector<uint8_t> &outputYUV);

private:
    cl_context       context_;
    cl_command_queue queue_;
    cl_program       program_;
    cl_kernel        resizeKernel_;
    cl_kernel        convertKernel_;
    cl_device_id     device_;

    void initOpenCL();
    void loadKernel(const std::string &filePath);
};

#endif // OPENCL_DRIVER_HPP