#include <cstddef>
#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include "pixel.h"

extern "C"
{
    pixel* runSobelOnPixels(const size_t pixelsToCalcPerThread, pixel *myPixels, pixel *leftRow, pixel *rightRow, const size_t LENGTH, const int rank);
    void freeOutArray(pixel *pointer)
    {
        cudaFree(pointer);
    }
}

// CUDA kernal to perform Sobel calulation on each pixel
__global__
void sobelKernal(const size_t pixelsToCalcPerThread, pixel *currRow, pixel *leftRow, pixel *rightRow, pixel *out)
{
    int index = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    for(int i = index; i < pixelsToCalcPerThread; i += stride)
    {
        //Put Sobel Logic Here (Each pixel needs 3 Sobel calculations because there is RGB)
    }
}


pixel* runSobelOnPixels(const size_t pixelsToCalcPerThread, pixel *myPixels, pixel *leftRow, pixel *rightRow, const size_t LENGTH, const int rank)
{
    int NumDevices;
    cudaGetDeviceCount(&NumDevices);
    int Device = rank % NumDevices;
    cudaSetDevice(Device);

    pixel *leftRowCuda;
    pixel *rightRowCuda;
    pixel *currRowCuda;
    pixel *out;

    cudaMallocManaged(&leftRowCuda, sizeof(pixel) * LENGTH);
    cudaMallocManaged(&rightRowCuda, sizeof(pixel) * LENGTH);
    cudaMallocManaged(&currRowCuda, sizeof(pixel) * LENGTH);
    cudaMallocManaged(&out, sizeof(pixel) * LENGTH);

    //Copy arrays into Cuda memory (unsure if this is wise)
    for(int i = 0; i < LENGTH; ++i)
    {
        leftRowCuda[i] = leftRow[i];
        rightRowCuda[i] = rightRow[i];
        currRowCuda[i] = myPixels[i];
    }

    cudaMemPrefetchAsync(leftRowCuda, sizeof(pixel) * LENGTH, Device, 0);
    cudaMemPrefetchAsync(rightRowCuda, sizeof(pixel) * LENGTH, Device, 0);
    cudaMemPrefetchAsync(currRowCuda, sizeof(pixel) * LENGTH, Device, 0);
    cudaMemPrefetchAsync(out, sizeof(pixel) * LENGTH, Device, 0);

    const dim3 gridDim = 16384; //These numbers are a bit arbitrary
    const dim3 blockDim = 1024;
    sobelKernal<<<gridDim,blockDim>>>(pixelsToCalcPerThread, currRowCuda, leftRowCuda, rightRowCuda, out);

    cudaDeviceSynchronize();

    cudaFree(leftRowCuda);
    cudaFree(rightRowCuda);
    cudaFree(currRowCuda);

    return out;
}
