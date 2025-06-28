#include "opencl_driver.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

OpenCLDriver::OpenCLDriver()
{
    initOpenCL();
    loadKernel("../kernels/opencl_preprocess.cl");
    std::cout << "OpenCL driver loaded." << std::endl;
    resizeKernel_ = clCreateKernel(program_, "resize_bilinear", nullptr);
    convertKernel_ = clCreateKernel(program_, "bgr_to_yuv420", nullptr);
}

OpenCLDriver::~OpenCLDriver()
{
    clReleaseKernel(resizeKernel_);
    clReleaseKernel(convertKernel_);
    clReleaseProgram(program_);
    clReleaseCommandQueue(queue_);
    clReleaseContext(context_);
}

void OpenCLDriver::initOpenCL()
{
    cl_int err;
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clGetPlatformIDs failed";
        exit(1);
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clGetDeviceIDs failed";
        exit(1);
    }

    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clCreateContext failed";
        exit(1);
    }

    queue_ = clCreateCommandQueue(context_, device_, 0, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clCreateCommandQueue failed";
        exit(1);
    }
}

void OpenCLDriver::loadKernel(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Cannot open " << filePath;
        exit(1);
    }
    std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const char *source = src.c_str();
    cl_int err;
    program_ = clCreateProgramWithSource(context_, 1, &source, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clCreateProgramWithSource failed";
        exit(1);
    }
    err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        size_t logSize;
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> log(logSize);
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        std::cerr << log.data();
        exit(1);
    }
}

void OpenCLDriver::processFrame(const cv::Mat &input, std::vector<uint8_t> &outputYUV, int targetWidth, int targetHeight)
{
    cl_int err;
    // TODO: Define target dimensions for resizing
    // int targetWidth = targ;
    // int targetHeight = 480;

    // Step 1: Create buffers
    size_t inputSize = input.total() * input.elemSize();
    size_t resizedSize = targetWidth * targetHeight * 3; // still BGR
    size_t yuvSize = targetWidth * targetHeight * 3 / 2; // YUV420

    cl_mem inputBuffer = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        inputSize, (void *)input.data, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create inputBuffer\n";
        exit(1);
    }

    cl_mem resizedBuffer = clCreateBuffer(context_, CL_MEM_READ_WRITE,
                                          resizedSize, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create resizedBuffer\n";
        exit(1);
    }

    cl_mem yuvBuffer = clCreateBuffer(context_, CL_MEM_WRITE_ONLY,
                                      yuvSize, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create yuvBuffer\n";
        exit(1);
    }

    // Step 2: Set arguments for resize_bilinear
    err = clSetKernelArg(resizeKernel_, 0, sizeof(cl_mem), &inputBuffer);
    err |= clSetKernelArg(resizeKernel_, 1, sizeof(int), &input.cols);
    err |= clSetKernelArg(resizeKernel_, 2, sizeof(int), &input.rows);
    err |= clSetKernelArg(resizeKernel_, 3, sizeof(cl_mem), &resizedBuffer);
    err |= clSetKernelArg(resizeKernel_, 4, sizeof(int), &targetWidth);
    err |= clSetKernelArg(resizeKernel_, 5, sizeof(int), &targetHeight);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to set resize kernel args\n";
        exit(1);
    }

    size_t globalSizeResize[2] = {(size_t)targetWidth, (size_t)targetHeight};
    err = clEnqueueNDRangeKernel(queue_, resizeKernel_, 2, nullptr, globalSizeResize, nullptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Resize kernel launch failed\n";
        exit(1);
    }

    // Step 3: Set arguments for bgr_to_yuv420
    err = clSetKernelArg(convertKernel_, 0, sizeof(cl_mem), &resizedBuffer);
    err |= clSetKernelArg(convertKernel_, 1, sizeof(cl_mem), &yuvBuffer);
    err |= clSetKernelArg(convertKernel_, 2, sizeof(int), &targetWidth);
    err |= clSetKernelArg(convertKernel_, 3, sizeof(int), &targetHeight);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to set convert kernel args\n";
        exit(1);
    }

    size_t globalSizeConvert[2] = {(size_t)targetWidth, (size_t)targetHeight};
    err = clEnqueueNDRangeKernel(queue_, convertKernel_, 2, nullptr, globalSizeConvert, nullptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Convert kernel launch failed\n";
        exit(1);
    }

    // Step 4: Read back YUV420 output
    outputYUV.resize(yuvSize);
    err = clEnqueueReadBuffer(queue_, yuvBuffer, CL_TRUE, 0, yuvSize, outputYUV.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to read YUV buffer\n";
        exit(1);
    }

    // Step 5: Cleanup
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(resizedBuffer);
    clReleaseMemObject(yuvBuffer);
}
