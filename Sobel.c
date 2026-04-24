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
    const size_t remainderPixelsPerProc = (LENGTH * WIDTH) % nprocs;

    //This is an array for each process that contains all its pixels
    pixel* pixelsInProc = malloc(sizeof(pixel) * pixelsPerProc);

    unsigned char* pixelBuffer = malloc(sizeof(unsigned char) * 3);

    MPI_Barrier(MPI_COMM_WORLD);

    //READ FILES INTO EACH PROCESS
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
        pixelsInProc->red = pixelBuffer[0];
        pixelsInProc->green = pixelBuffer[1];
        pixelsInProc->blue = pixelBuffer[2];
    }
    MPI_File_close(&fh);
    free(pixelBuffer);
    MPI_Barrier(MPI_COMM_WORLD);


    //CUDA CALLS HERE

    free(pixelsInProc);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;

}
