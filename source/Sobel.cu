#include <cmath>
#include <cstddef>
#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "pixel.h"

extern "C"
{
    float* runSobelOnPixels(const size_t totalPixels, surroundingPixels *myPixels, const size_t WIDTH, const size_t HEIGHT, const int rank);
    void freeOutArray(float *pointer)
    {
        cudaFree(pointer);
    }
}

// CUDA kernal to perform Sobel calulation on each pixel
__global__
void sobelKernal(const size_t totalPixels, surroundingPixels *in, float* out)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    for(int i = index; i < totalPixels; i += stride)
    {
        //Sobel Logic Here
        float Gx = (-1 * in[i].topLeft) + (1 * in[i].topRight) + (-2 * in[i].left) +
                        (2 * in[i].right) + (-1 * in[i].bottomLeft) + (1 * in[i].bottomRight);
        float Gy = (-1 * in[i].topLeft) + (-2 * in[i].topMiddle) + (-1 * in[i].topRight) +
                        (1 * in[i].bottomLeft) + (2 * in[i].bottomMiddle) + (1 * in[i].bottomRight);
        float GxPow = std::pow(Gx, 2);
        float GyPow = std::pow(Gy, 2);
        float magnitude = std::sqrt(GxPow + GyPow);
        out[i] = magnitude;
    }
}


float* runSobelOnPixels(const size_t totalPixels, surroundingPixels *myPixels, const size_t WIDTH, const size_t HEIGHT, const int rank)
{
    int NumDevices;
    cudaGetDeviceCount(&NumDevices);
    int Device = rank % NumDevices;
    cudaSetDevice(Device);

    surroundingPixels *in;
    float *out;

    cudaMallocManaged(&in,  sizeof(surroundingPixels) * totalPixels);
    cudaMallocManaged(&out, sizeof(float)             * totalPixels);

    //Copy in arraya into Cuda memory
    for(size_t i = 0; i < totalPixels; ++i)
    {
        in[i] = myPixels[i];
    }

    //cudaMemPrefetchAsync(in, sizeof(surroundingPixels) * LENGTH, Device, 0);
    //cudaMemPrefetchAsync(out, sizeof(float) * totalPixels, Device, 0);

    const dim3 gridDim = 32;
    const dim3 blockDim = 128;
    sobelKernal<<<gridDim,blockDim>>>(totalPixels, in, out);

    cudaDeviceSynchronize();

    cudaFree(in);
    return out;
}
