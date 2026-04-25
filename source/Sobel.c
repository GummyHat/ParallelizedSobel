#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <stdint.h>
#include "pixel.h"

#define pixelBytes 3

extern pixel* runSobelOnPixels(const size_t pixelsToCalcPerThread, pixel *myPixels, pixel *leftRow, pixel *rightRow, const size_t LENGTH, const int rank);
extern void freeOutArray(pixel *pointer);

float* readToGrayscale(const char* filename, int w, int h) {
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    int numPixels = w * h;
    unsigned char* raw = malloc(numPixels * 3);
    float* result = malloc(numPixels * sizeof(float));

    if (fread(raw, 1, numPixels * 3, f) == (size_t)pixel_count * 3) {
        for (int i = 0; i < numPixels; i++) {
            result[i] = (0.299f * raw[i*3] + 0.587f * raw[i*3+1] + 0.114f * raw[i*3+2]) / 255.0f;
        }
    }
    free(raw);
    fclose(f);
    return result;
}

/*
// IBM POWER9 System clock with 512MHZ resolution.
unsigned long long aimos_clock_read(void)
{
  unsigned int tbl, tbu0, tbu1;

  do {
    __asm__ __volatile__ ("mftbu %0" : "=r"(tbu0));
    __asm__ __volatile__ ("mftb %0" : "=r"(tbl));
    __asm__ __volatile__ ("mftbu %0" : "=r"(tbu1));
  } while (tbu0 != tbu1);

  return (((unsigned long long)tbu0) << 32) | tbl;
}
*/
int main(int argc, char* argv[])
{
    //Validate and Define Argument Parameters
    if(argc != 4)
    {
        printf("Incorrect Arguments: LENGTH WIDTH FILENAME\n");
        exit(1);
    }
    
    const size_t LENGTH = atoi(argv[1]);
    const size_t WIDTH = atoi(argv[2]);
    char *filename = argv[3];

    if(SIZE_MAX < LENGTH || SIZE_MAX < WIDTH)
    {
        printf("LENGTH OR WIDTH IS TOO LARGE!\n");
        exit(1);
    }

    int rank, nprocs;
    MPI_File fh, ofh;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);


    //This is how many pixels each process will recieve
    const size_t pixelsPerProc = (LENGTH * WIDTH) / nprocs;

    //This is remainder pixels for the last process
    //This is currently unused, but should be used! Ranks currently need to be divisible by row count.
    //If width is 100 and length is 100, there can be 100 ranks?
    const size_t remainderPixelsPerProc = (LENGTH * WIDTH) % nprocs;

    //This is an array for each process that contains all its pixels
    float* myPixels = malloc(pixelsPerProc * sizeof(float));

    unsigned char* pixelBuffer = malloc(pixelBytes);

    MPI_Barrier(MPI_COMM_WORLD);

    //READ FILE INTO RANK 0
    if (rank == 0) {
        //read in RGB file and convert to grayscale
        float * allPixels= readToGrayscale(filename,LENGTH,WIDTH);        
        //TODO:: scatter to ranks

    }
    //The logic here needs a second look
    //Current idea is that the MPI_File_read_at takes in 3 byte at a time and creates a pixel using that data
    //Then each process will have all its pixels in an array (ideally)
    for(int i = 0; i < pixelsPerProc; ++i)
    {
        offset = i * pixelBytes; 
        MPI_File_read_at(fh, offset, pixelBuffer, pixelBytes, MPI_UNSIGNED_CHAR, &status);

        //Convert to grayscale
        myPixels[i] = 0.3 * pixelBuffer[0] + 0.59 * pixelBuffer[1] + 0.11 * pixelBuffer[2];

        //For error checking
        if(WIDTH/i == 0) printf("%d ", i);
    }

    printf("\n\n");
    MPI_File_close(&fh);
    free(pixelBuffer);
    MPI_Barrier(MPI_COMM_WORLD);


    float *leftRow = malloc(pixelsPerProc * sizeof(float));
    float *rightRow = malloc(pixelsPerProc * sizeof(float));
    
    //Exchange next proc and prev procs pixel arrays
    MPI_Request requests[4];
    int request_count = 0;
    if(nprocs != 1)
    {
        if(rank == 0)
        {
            for(size_t i = 0; i < pixelsPerProc; ++i)
            {
                //set it to black for now
                leftRow[i] = 0;
            }
            MPI_Irecv(rightRow, pixelsPerProc, MPI_FLOAT, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_FLOAT, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        }
        else if(rank == nprocs - 1)
        {
            for(size_t i = 0; i < pixelsPerProc; ++i)
            {
                //set it to black for now
                rightRow[i] = 0;
            }
            MPI_Irecv(leftRow, pixelsPerProc, MPI_FLOAT, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_FLOAT, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        }
        else {
            MPI_Irecv(leftRow, pixelsPerProc, MPI_FLOAT, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Irecv(rightRow, pixelsPerProc, MPI_FLOAT, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_FLOAT, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_FLOAT, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        }
    }
    else {
        for(size_t i = 0; i < pixelsPerProc; ++i)
        {
            //set it to black for now
            leftRow[i] = 0;
        }
        for(size_t i = 0; i < pixelsPerProc; ++i)
        {
            //set it to black for now
            rightRow[i] = 0;
        }
    }

    MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE);
    
    //Run Cuda function (run Sobel on all pixels) //WIDTH is not a good value for this (will not work)
    //pixel *out = runSobelOnPixels(WIDTH, myPixels, leftRow, RightRow, WIDTH, rank);

    unsigned char* out = malloc(pixelsPerProc);
    for(int i = 0; i < pixelsPerProc; ++i)
    {
        out[i] = (unsigned char) ((int)myPixels[i]);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    //Write output file
    char* outputFilename = malloc(64);
    for(int i = 0; i < strlen(filename) - 5; ++i)
    {
        outputFilename[i] = filename[i];
    }
    outputFilename = strcat(outputFilename, "_out.data\0");

    MPI_File_open(MPI_COMM_WORLD, outputFilename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &ofh);
    for(int i = 0; i < pixelsPerProc; ++i)
    {
        offset = i;
        MPI_File_write_at(ofh, offset, out, 1, MPI_UNSIGNED_CHAR, &status);
        //printf("new: %lf", myPixels[i]);
        //printf("%d\n", i);
    }

    MPI_File_close(&ofh);

    MPI_Barrier(MPI_COMM_WORLD);

    //REPORT TELEMETRY HERE
    
    //freeOutArray(out);
    free(outputFilename);
    free(myPixels);
    free(leftRow);
    free(rightRow);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
    
}
