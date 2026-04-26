#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <stdint.h>
#include "pixel.h"

#define pixelBytes 3

extern float* runSobelOnPixels(const size_t pixelsPerThread, surroundingPixels *myPixels, const size_t WIDTH, const size_t HEIGHT, const int rank);
extern void freeOutArray(float *pointer);

#define GHOSTPIXELVALUE 0

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

float* readToGrayscale(const char* filename, size_t totalPixelCount) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
      printf("File could not be opened\n");
      exit(1);
    }
    unsigned char* raw = malloc(totalPixelCount * 3);
    float* result = malloc(totalPixelCount * sizeof(float));

    if (fread(raw, 1, totalPixelCount * 3, f) == (size_t)totalPixelCount * 3) {
        for (int i = 0; i < totalPixelCount; i++) {
            result[i] = (0.299f * raw[i*3] + 0.587f * raw[i*3+1] + 0.114f * raw[i*3+2]);
        }
    }
    free(raw);
    fclose(f);
    return result;
}

int main(int argc, char* argv[])
{
    //Validate and Define Argument Parameters
    if(argc != 5)
    {
        printf("Incorrect Arguments: WIDTH HEIGHT FILENAME OUTPUTFILENAME\n");
        exit(1);
    }

    const size_t WIDTH = atoi(argv[1]);
    const size_t HEIGHT = atoi(argv[2]);
    char *filename = argv[3];
    char *outputFilename = argv[4];
    size_t totalPixelCount = HEIGHT * WIDTH;

    if(SIZE_MAX < HEIGHT || SIZE_MAX < WIDTH)
    {
        printf("WIDTH OR HEIGHT IS TOO LARGE!\n");
        exit(1);
    }

    int rank, nprocs;
    MPI_File fh, ofh;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);


    float* allPixels;
    surroundingPixels* pixelMesh;
    if(rank == 0)
    {
        allPixels = readToGrayscale(filename, totalPixelCount);

        //Produce array of pixel meshes
        pixelMesh = malloc(sizeof(surroundingPixels) * totalPixelCount);
        for(int i = 0; i < totalPixelCount; ++i)
        {
            if(i == 0)
            { //Top left
                //printf("Top Left\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = GHOSTPIXELVALUE;
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = allPixels[i + HEIGHT];
                pixelMesh[i].bottomRight = allPixels[i + HEIGHT + 1];
            }
            else if(i == HEIGHT - 1)
            { //Top Right
                //printf("Top Right\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = GHOSTPIXELVALUE;
                pixelMesh[i].bottomLeft = allPixels[i + HEIGHT - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + HEIGHT];
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if(i == totalPixelCount - HEIGHT)
            { //Bottom Left
                //printf("Bottom Left\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = allPixels[i - HEIGHT];
                pixelMesh[i].topRight = allPixels[i - HEIGHT + 1];
                pixelMesh[i].left = GHOSTPIXELVALUE;
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if(i == totalPixelCount - 1)
            { //Bottom Right
                //printf("Bottom Right\n");
                pixelMesh[i].topLeft = allPixels[i - HEIGHT - 1];
                pixelMesh[i].topMiddle = allPixels[i - HEIGHT];
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = GHOSTPIXELVALUE;
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if(i < HEIGHT)
            { //Top Side
                //printf("Top Side\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = allPixels[i + HEIGHT - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + HEIGHT];
                pixelMesh[i].bottomRight = allPixels[i + HEIGHT + 1];
            }
            else if(i < totalPixelCount && i > totalPixelCount - HEIGHT - 1)
            { //Bottom
                //printf("Bottom\n");
                pixelMesh[i].topLeft = allPixels[i - HEIGHT - 1];
                pixelMesh[i].topMiddle = allPixels[i - HEIGHT];
                pixelMesh[i].topRight = allPixels[i - HEIGHT + 1];
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if (i % HEIGHT == 0)
            { //Left
                //printf("Left\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = allPixels[i - HEIGHT];
                pixelMesh[i].topRight = allPixels[i - HEIGHT + 1];
                pixelMesh[i].left = GHOSTPIXELVALUE;
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = allPixels[i + HEIGHT];
                pixelMesh[i].bottomRight = allPixels[i + HEIGHT + 1];
            }
            else if(i % HEIGHT == HEIGHT - 1)
            { //Right
                //printf("Right\n");
                pixelMesh[i].topLeft = allPixels[i - HEIGHT - 1];
                pixelMesh[i].topMiddle = allPixels[i - HEIGHT];
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = GHOSTPIXELVALUE;
                pixelMesh[i].bottomLeft = allPixels[i + HEIGHT - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + HEIGHT];
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else 
            {
                //printf("ELSE CASE\n");
                pixelMesh[i].topLeft = allPixels[i - HEIGHT - 1];
                pixelMesh[i].topMiddle = allPixels[i - 1];
                pixelMesh[i].topRight = allPixels[i - HEIGHT + 1];
                pixelMesh[i].left = allPixels[i - HEIGHT];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = allPixels[i + HEIGHT - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + HEIGHT];
                pixelMesh[i].bottomRight = allPixels[i + HEIGHT + 1];
            }
        }
    }
    size_t pixelsPerProc;
    size_t recvcount;    
    
    if(nprocs == 1){//case to prevent divide by 0
    pixelsPerProc = (HEIGHT * WIDTH);
    recvcount = (HEIGHT*WIDTH);
    //test prints
    printf("%ld pixelsPerProc\n",pixelsPerProc);
    printf("%ld recvcount\n",recvcount);
    }
    else{
    pixelsPerProc = (HEIGHT * WIDTH) / (nprocs-1);
    recvcount = (rank == (nprocs-1)) ? (HEIGHT * WIDTH) % (nprocs-1) : pixelsPerProc;
    
    //test prints
    printf("%ld pixelsPerProc\n",pixelsPerProc);
    printf("%ld recvcount\n",recvcount);
    }
        
    surroundingPixels * recvbuf = malloc(sizeof(surroundingPixels)*recvcount);
    int * sendcounts = malloc(sizeof(int)*nprocs);
    int * displs = malloc(sizeof(int)*nprocs);
    int sum = 0;
    for(int i = 0; i < (nprocs-1); ++i){
        sendcounts[i] = int(pixelsPerProc);
        displs[i]= sum;
        sum += sendcounts[i];
    }
    sendcounts[nprocs-1] = (HEIGHT * WIDTH) % (nprocs-1);
    
    //SCATTER ALL PIXELS BETWEEN PROCESSES
    MPI_Scatterv(pixelMesh,//check
                 sendcounts,//check
                 displs,//check
                 customType,//waiting on custom type
                 recvbuf, //check
                 recvcount,//check
                 customType,//waiting on custom type
                 0,//check
                 MPI_COMM_WORLD);//check
    //Run Cuda function (run Sobel on all pixels)
    surroundingPixels *subPixelMesh = malloc(sizeof(surroundingPixels) * 3);
    size_t pixelsPerThread = 5;

    float *out = runSobelOnPixels(pixelsPerThread, subPixelMesh, WIDTH, HEIGHT, rank);
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    //GATHER ALL PIXELS
    
    //Write all calculated data to a new binary
    MPI_Offset offset = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    
    MPI_File_open(MPI_COMM_WORLD, outputFilename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &ofh);
    
    /*
    for(int i = 0; i < pixelsPerProc; ++i)
    {
        offset = i;
        //MPI_File_write_at(ofh, offset, out, 1, MPI_FLOAT, &status);
        //printf("new: %lf", myPixels[i]);
        //printf("%d\n", i);
    }
    */
    MPI_File_close(&ofh);

    MPI_Barrier(MPI_COMM_WORLD);

    //REPORT TELEMETRY HERE
    if(rank == 0)
    {
        free(pixelMesh);
    }
    
    free(allPixels);
    freeOutArray(out);
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    MPI_Finalize();
    return 0;
    
}
