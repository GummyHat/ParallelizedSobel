#include <cmath>
#include <cstddef>
#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "pixel.h"

extern "C"
{
    float* runSobelOnPixels(const size_t pixelsPerThread, surroundingPixels *myPixels, const size_t WIDTH, const size_t HEIGHT, const int rank);
    void freeOutArray(float *pointer)
    {
        cudaFree(pointer);
    }
}

// CUDA kernal to perform Sobel calulation on each pixel
__global__
void sobelKernal(const size_t pixelsPerThread, surroundingPixels *in, float* out)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    for(int i = index; i < pixelsPerThread; i += stride)
    {
        //Sobel Logic Here
        float Gx = (-1 * in->topLeft) + (1 * in->topLeft) + (-2 * in->left) +
                        (2 * in->right) + (-1 * in->bottomLeft) + (1 * in->bottomRight);
        float Gy = (-1 * in->topLeft) + (-2 * in->topMiddle) + (-1 * in->topRight) +
                        (1 * in->bottomLeft) + (2 * in->bottomMiddle) + (1 * in->bottomRight);
        float GxPow = std::pow(Gx, 2);
        float GyPow = std::pow(Gy, 2);
        float magnitude = std::sqrt(GxPow + GyPow);
        out[i] = magnitude;
    }
}


float* runSobelOnPixels(const size_t pixelsPerThread, surroundingPixels *myPixels, const size_t WIDTH, const size_t HEIGHT, const int rank)
{
    int NumDevices;
    cudaGetDeviceCount(&NumDevices);
    int Device = rank % NumDevices;
    cudaSetDevice(Device);

    surroundingPixels *in;
    float *out;

    cudaMallocManaged(&in, sizeof(surroundingPixels) * WIDTH);
    cudaMallocManaged(&out, sizeof(surroundingPixels) * WIDTH);

    //Copy in arraya into Cuda memory
    for(int i = 0; i < WIDTH; ++i)
    {
        in[i] = myPixels[i];
    }

    //cudaMemPrefetchAsync(in, sizeof(surroundingPixels) * LENGTH, Device, 0);
    //cudaMemPrefetchAsync(out, sizeof(float) * WIDTH, Device, 0);

    const dim3 gridDim = 2; //These numbers are a bit arbitrary
    const dim3 blockDim = 2;
    sobelKernal<<<gridDim,blockDim>>>(pixelsPerThread, in, out);

    cudaDeviceSynchronize();

    cudaFree(in);
    return out;
}
