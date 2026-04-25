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
    float* myPixels = malloc(pixelsPerProc);

    unsigned char* pixelBuffer = malloc(pixelBytes);

    MPI_Barrier(MPI_COMM_WORLD);

    //READ FILE INTO EACH PROCESS
    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    MPI_Offset offset = 0;

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
        printf("Gray Value:%.2f", myPixels[i]);
    }
    printf("\n\n");
    MPI_File_close(&fh);
    free(pixelBuffer);
    MPI_Barrier(MPI_COMM_WORLD);


    //Exchange next proc and prev procs pixel arrays
    MPI_Request requests[4];
    int request_count = 0;
    unsigned char *leftRow = malloc(pixelsPerProc);
    unsigned char *RightRow = malloc(pixelsPerProc);
    if(nprocs != 1)
    {
        if(rank == 0)
        {
            for(size_t i = 0; i < pixelsPerProc; ++i)
            {
                //set it to black for now
                leftRow[i] = 0;
            }
            MPI_Irecv(RightRow, pixelsPerProc, MPI_UNSIGNED_CHAR, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_UNSIGNED_CHAR, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        }
        else if(rank == nprocs - 1)
        {
            for(size_t i = 0; i < pixelsPerProc; ++i)
            {
                //set it to black for now
                RightRow[i] = 0;
            }
            MPI_Irecv(leftRow, pixelsPerProc, MPI_UNSIGNED_CHAR, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_UNSIGNED_CHAR, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        }
        else {
            MPI_Irecv(leftRow, pixelsPerProc, MPI_UNSIGNED_CHAR, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Irecv(RightRow, pixelsPerProc, MPI_UNSIGNED_CHAR, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_UNSIGNED_CHAR, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
            MPI_Isend(myPixels, pixelsPerProc, MPI_UNSIGNED_CHAR, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
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
            RightRow[i] = 0;
        }
    }

    MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE);

    //Run Cuda function (run Sobel on all pixels) //WIDTH is not a good value for this (will not work)
    //pixel *out = runSobelOnPixels(WIDTH, myPixels, leftRow, RightRow, WIDTH, rank);
    
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
        MPI_File_write_at(ofh, offset, myPixels, 1, MPI_UNSIGNED_CHAR, &status);
        printf("new: %lf", myPixels[i]);
    }
    MPI_File_close(&ofh);

    MPI_Barrier(MPI_COMM_WORLD);

    //REPORT TELEMETRY HERE

    //freeOutArray(out);
    free(outputFilename);
    free(myPixels);
    free(leftRow);
    free(RightRow);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;

}
