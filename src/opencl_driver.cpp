#include "opencl_driver.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

OpenCLDriver::OpenCLDriver() {
    initOpenCL();
    loadKernel("../kernels/opencl_preprocess.cl");
    std::cout << "OpenCL driver loaded." << std::endl;
    resizeKernel_  = clCreateKernel(program_, "resize_bilinear", nullptr);
    convertKernel_ = clCreateKernel(program_, "bgr_to_yuv420", nullptr);
}

OpenCLDriver::~OpenCLDriver() {
    clReleaseKernel(resizeKernel_);
    clReleaseKernel(convertKernel_);
    clReleaseProgram(program_);
    clReleaseCommandQueue(queue_);
    clReleaseContext(context_);
}

void OpenCLDriver::initOpenCL() {
    cl_int err;
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, nullptr);
    if(err != CL_SUCCESS) { std::cerr << "clGetPlatformIDs failed"; exit(1); }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr);
    if(err != CL_SUCCESS) { std::cerr << "clGetDeviceIDs failed"; exit(1); }

    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
    if(err != CL_SUCCESS) { std::cerr << "clCreateContext failed"; exit(1); }

    queue_ = clCreateCommandQueue(context_, device_, 0, &err);
    if(err != CL_SUCCESS) { std::cerr << "clCreateCommandQueue failed"; exit(1); }
}

void OpenCLDriver::loadKernel(const std::string &filePath) {
    std::ifstream file(filePath);
    if(!file.is_open()) { std::cerr << "Cannot open " << filePath; exit(1); }
    std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const char *source = src.c_str();
    cl_int err;
    program_ = clCreateProgramWithSource(context_, 1, &source, nullptr, &err);
    if(err != CL_SUCCESS) { std::cerr << "clCreateProgramWithSource failed"; exit(1); }
    err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
    if(err != CL_SUCCESS) {
        size_t logSize;
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> log(logSize);
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        std::cerr << log.data(); exit(1);
    }
}

void OpenCLDriver::processFrame(const cv::Mat &input, std::vector<uint8_t> &outputYUV) {
    // TODO: create buffers, set kernel args, enqueue kernels, read back
}