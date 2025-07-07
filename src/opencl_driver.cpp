#include "opencl_driver.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstdlib>

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
        std::cerr << "clGetPlatformIDs failed\n";
        std::exit(1);
    }

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device_, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clGetDeviceIDs failed\n";
        std::exit(1);
    }

    context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clCreateContext failed\n";
        std::exit(1);
    }

    queue_ = clCreateCommandQueue(context_, device_, 0, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clCreateCommandQueue failed\n";
        std::exit(1);
    }
}

void OpenCLDriver::loadKernel(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Cannot open " << filePath << "\n";
        std::exit(1);
    }
    std::string src((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const char *source = src.c_str();
    cl_int err;
    program_ = clCreateProgramWithSource(context_, 1, &source, nullptr, &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "clCreateProgramWithSource failed\n";
        std::exit(1);
    }
    err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        size_t logSize;
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> log(logSize);
        clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        std::cerr << log.data() << "\n";
        std::exit(1);
    }
}

void OpenCLDriver::processFrame(const cv::Mat &input,
                                std::vector<uint8_t> &outputYUV,
                                int targetWidth,
                                int targetHeight)
{
    cl_int err;

    // Compute buffer sizes
    size_t inputSize = input.total() * input.elemSize();    // BGR24
    size_t resizedSize = targetWidth * targetHeight * 3;    // BGR24
    size_t ySize = targetWidth * targetHeight;              // Y plane
    size_t uvSize = (targetWidth / 2) * (targetHeight / 2); // U or V plane
    size_t yuvSize = ySize + 2 * uvSize;                    // total YUV420

    // Create OpenCL buffers
    cl_mem inputBuffer = clCreateBuffer(context_,
                                        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        inputSize,
                                        (void *)input.data,
                                        &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create inputBuffer: " << err << "\n";
        std::exit(1);
    }

    cl_mem resizedBuffer = clCreateBuffer(context_,
                                          CL_MEM_READ_WRITE,
                                          resizedSize,
                                          nullptr,
                                          &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create resizedBuffer: " << err << "\n";
        std::exit(1);
    }

    // Separate Y, U, V buffers
    cl_mem yBuffer = clCreateBuffer(context_,
                                    CL_MEM_WRITE_ONLY,
                                    ySize,
                                    nullptr,
                                    &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create yBuffer: " << err << "\n";
        std::exit(1);
    }

    cl_mem uBuffer = clCreateBuffer(context_,
                                    CL_MEM_WRITE_ONLY,
                                    uvSize,
                                    nullptr,
                                    &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create uBuffer: " << err << "\n";
        std::exit(1);
    }

    cl_mem vBuffer = clCreateBuffer(context_,
                                    CL_MEM_WRITE_ONLY,
                                    uvSize,
                                    nullptr,
                                    &err);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to create vBuffer: " << err << "\n";
        std::exit(1);
    }

    // 1) Resize kernel
    err = clSetKernelArg(resizeKernel_, 0, sizeof(cl_mem), &inputBuffer);
    err |= clSetKernelArg(resizeKernel_, 1, sizeof(int), &input.cols);
    err |= clSetKernelArg(resizeKernel_, 2, sizeof(int), &input.rows);
    err |= clSetKernelArg(resizeKernel_, 3, sizeof(cl_mem), &resizedBuffer);
    err |= clSetKernelArg(resizeKernel_, 4, sizeof(int), &targetWidth);
    err |= clSetKernelArg(resizeKernel_, 5, sizeof(int), &targetHeight);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to set resize kernel args: " << err << "\n";
        std::exit(1);
    }

    size_t globalResize[2] = {(size_t)targetWidth, (size_t)targetHeight};
    err = clEnqueueNDRangeKernel(queue_, resizeKernel_, 2, nullptr, globalResize, nullptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Resize kernel launch failed: " << err << "\n";
        std::exit(1);
    }

    // 2) Convert BGR->YUV420 kernel
    err = clSetKernelArg(convertKernel_, 0, sizeof(cl_mem), &resizedBuffer);
    err |= clSetKernelArg(convertKernel_, 1, sizeof(int), &targetWidth);
    err |= clSetKernelArg(convertKernel_, 2, sizeof(int), &targetHeight);
    err |= clSetKernelArg(convertKernel_, 3, sizeof(cl_mem), &yBuffer);
    err |= clSetKernelArg(convertKernel_, 4, sizeof(cl_mem), &uBuffer);
    err |= clSetKernelArg(convertKernel_, 5, sizeof(cl_mem), &vBuffer);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Failed to set convert kernel args: " << err << "\n";
        std::exit(1);
    }

    size_t globalConvert[2] = {(size_t)targetWidth, (size_t)targetHeight};
    err = clEnqueueNDRangeKernel(queue_, convertKernel_, 2, nullptr, globalConvert, nullptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Convert kernel launch failed: " << err << "\n";
        std::exit(1);
    }

    // 3) Read back Y, U, V planes
    std::vector<uint8_t> planeY(ySize), planeU(uvSize), planeV(uvSize);
    clEnqueueReadBuffer(queue_, yBuffer, CL_TRUE, 0, ySize, planeY.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(queue_, uBuffer, CL_TRUE, 0, uvSize, planeU.data(), 0, nullptr, nullptr);
    clEnqueueReadBuffer(queue_, vBuffer, CL_TRUE, 0, uvSize, planeV.data(), 0, nullptr, nullptr);

    // 4) Stitch into single YUV420 buffer: Y plane, then U plane, then V plane
    outputYUV.resize(yuvSize);
    size_t offset = 0;
    memcpy(outputYUV.data() + offset, planeY.data(), ySize);
    offset += ySize;
    memcpy(outputYUV.data() + offset, planeU.data(), uvSize);
    offset += uvSize;
    memcpy(outputYUV.data() + offset, planeV.data(), uvSize);

    // 5) Cleanup
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(resizedBuffer);
    clReleaseMemObject(yBuffer);
    clReleaseMemObject(uBuffer);
    clReleaseMemObject(vBuffer);
}
