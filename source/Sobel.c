#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include "pixel.h"

extern pixel* runSobelOnPixels(const size_t pixelsToCalcPerThread, pixel *myPixels, pixel *leftRow, pixel *rightRow, const size_t LENGTH, const int rank);
extern void freeOutArray(pixel *pointer);


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

int main(int argc, char* argv[])
{
    //Validate and Define Argument Parameters
    if(argc != 4)
    {
        printf("Incorrect Arguments: LENGTH WIDTH GRAYBOOL FILENAME\n");
        exit(1);
    }
    
    const size_t LENGTH = atoi(argv[1]);
    const size_t WIDTH = atoi(argv[2]);
    const int GRAYBOOL = atoi(argv[3]);

    int rank, nprocs;
    MPI_File fh, ofh;
    MPI_Status status;

    const unsigned long long FileSizeBytes = LENGTH * WIDTH * 8;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);


    //This is how many pixels each process will recieve
    const size_t pixelsPerProc = (LENGTH * WIDTH) / nprocs;

    //This is remainder pixels for the last process
    //This is currently unused, but should be used! Ranks currently need to be divisible by pixel count.
    const size_t remainderPixelsPerProc = (LENGTH * WIDTH) % nprocs;

    //This is an array for each process that contains all its pixels
    pixel* myPixels = malloc(sizeof(pixel) * pixelsPerProc);

    unsigned char* pixelBuffer = malloc(sizeof(unsigned char) * 3);

    MPI_Barrier(MPI_COMM_WORLD);

    //READ FILE INTO EACH PROCESS
    char *filename = argv[4];
    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
    MPI_Offset offset = 0;

    //The logic here is quite poor. Needs refinement.
    //Current idea is that the MPI_File_read_at takes in 3 byte at a time and creates a pixel using that data
    //Then each process will have all its pixels in an array (ideally)
    for(int i = 0; i < pixelsPerProc; ++i)
    {
        offset = (i * pixelsPerProc) * nprocs; 
        MPI_File_read_at(fh, offset, pixelBuffer, 3, MPI_UNSIGNED_CHAR, &status);
        myPixels->red = pixelBuffer[0];
        myPixels->green = pixelBuffer[1];
        myPixels->blue = pixelBuffer[2];
    }
    MPI_File_close(&fh);
    free(pixelBuffer);
    MPI_Barrier(MPI_COMM_WORLD);


    //Exchange next proc and prev procs pixel arrays
    MPI_Request requests[4];
    int request_count = 0;
    
    pixel *leftRow = malloc(sizeof(pixel) * pixelsPerProc);
    pixel *RightRow = malloc(sizeof(pixel) * pixelsPerProc);
    if(rank == 0)
    {
        for(int i = 0; i < pixelsPerProc; ++i)
        {
            //set it to black for now
            leftRow->red = 0;
            leftRow->green = 0;
            leftRow->blue = 0;
        }
        MPI_Irecv(RightRow, pixelsPerProc, MPI_INT, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
    }
    else if(rank == nprocs - 1)
    {
        for(int i = 0; i < pixelsPerProc; ++i)
        {
            //set it to black for now
            RightRow->red = 0;
            RightRow->green = 0;
            RightRow->blue = 0;
        }
        MPI_Irecv(leftRow, pixelsPerProc, MPI_INT, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
    }
    else {
        MPI_Irecv(leftRow, pixelsPerProc, MPI_INT, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Irecv(RightRow, pixelsPerProc, MPI_INT, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
    }

    MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE);

    //Run Cuda function (run Sobel on all pixels) //LENGTH is not a good value for this (will not work)
    pixel *out = runSobelOnPixels(LENGTH, myPixels, leftRow, RightRow, LENGTH, rank);

    MPI_Barrier(MPI_COMM_WORLD);

    //Write output file
    char *outputFilename = strcat(filename, "_out.data");
    MPI_File_open(MPI_COMM_WORLD, outputFilename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &ofh);
    for(int i = 0; i < pixelsPerProc; ++i) //Like above, the logic here is not so good I think. We need to fix this
    {

        offset = (i * pixelsPerProc) * nprocs; 
        MPI_File_write_at(fh, offset, out, 3, MPI_UNSIGNED_CHAR, &status);
    }
    MPI_File_close(&ofh);

    MPI_Barrier(MPI_COMM_WORLD);

    //REPORT TELEMETRY HERE

    freeOutArray(out);
    free(myPixels);
    free(leftRow);
    free(RightRow);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;

}
