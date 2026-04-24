#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>


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

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} pixel;

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
    MPI_File fh;
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
        MPI_File_read_at(fh, offset, pixelBuffer, 3, MPI_CHAR, &status);
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
    
    pixel *prev_array_upper = malloc(sizeof(pixel) * pixelsPerProc);
    pixel *next_array_lower = malloc(sizeof(pixel) * pixelsPerProc);
    if(rank == 0)
    {
        for(int i = 0; i < pixelsPerProc; ++i)
        {
            //set it to black for now
            prev_array_upper->red = 0;
            prev_array_upper->green = 0;
            prev_array_upper->blue = 0;
        }
        MPI_Irecv(next_array_lower, pixelsPerProc, MPI_INT, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
    }
    else if(rank == nprocs - 1)
    {
        for(int i = 0; i < pixelsPerProc; ++i)
        {
            //set it to black for now
            next_array_lower->red = 0;
            next_array_lower->green = 0;
            next_array_lower->blue = 0;
        }
        MPI_Irecv(prev_array_upper, pixelsPerProc, MPI_INT, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
    }
    else {
        MPI_Irecv(prev_array_upper, pixelsPerProc, MPI_INT, rank - 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Irecv(next_array_lower, pixelsPerProc, MPI_INT, rank + 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank + 1, 200, MPI_COMM_WORLD, &requests[request_count++]);
        MPI_Isend(myPixels, pixelsPerProc, MPI_INT, rank - 1, 100, MPI_COMM_WORLD, &requests[request_count++]);
    }

    MPI_Waitall(request_count, requests, MPI_STATUSES_IGNORE);

    //CUDA CALL HERE

    MPI_Barrier(MPI_COMM_WORLD);

    free(myPixels);
    free(prev_array_upper);
    free(next_array_lower);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;

}
