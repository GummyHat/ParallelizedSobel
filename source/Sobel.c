#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
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

    const int WIDTH = atoi(argv[1]);
    const int HEIGHT = atoi(argv[2]);
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


    float* allPixels = NULL;
    surroundingPixels* pixelMesh = NULL;
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
                pixelMesh[i].bottomMiddle = allPixels[i + WIDTH];
                pixelMesh[i].bottomRight = allPixels[i + WIDTH + 1];
            }
            else if(i == WIDTH - 1)
            { //Top Right
                //printf("Top Right\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = GHOSTPIXELVALUE;
                pixelMesh[i].bottomLeft = allPixels[i + WIDTH - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + WIDTH];
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if(i == totalPixelCount - WIDTH)
            { //Bottom Left
                //printf("Bottom Left\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = allPixels[i - WIDTH];
                pixelMesh[i].topRight = allPixels[i - WIDTH + 1];
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
                pixelMesh[i].topLeft = allPixels[i - WIDTH - 1];
                pixelMesh[i].topMiddle = allPixels[i - WIDTH];
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = GHOSTPIXELVALUE;
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if(i < WIDTH)
            { //Top Side
                //printf("Top Side\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = allPixels[i + WIDTH - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + WIDTH];
                pixelMesh[i].bottomRight = allPixels[i + WIDTH + 1];
            }
            else if(i < totalPixelCount && i > totalPixelCount - WIDTH - 1)
            { //Bottom
                //printf("Bottom\n");
                pixelMesh[i].topLeft = allPixels[i - WIDTH - 1];
                pixelMesh[i].topMiddle = allPixels[i - WIDTH];
                pixelMesh[i].topRight = allPixels[i - WIDTH + 1];
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = GHOSTPIXELVALUE;
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else if (i % WIDTH == 0)
            { //Left
                //printf("Left\n");
                pixelMesh[i].topLeft = GHOSTPIXELVALUE;
                pixelMesh[i].topMiddle = allPixels[i - WIDTH];
                pixelMesh[i].topRight = allPixels[i - WIDTH + 1];
                pixelMesh[i].left = GHOSTPIXELVALUE;
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = GHOSTPIXELVALUE;
                pixelMesh[i].bottomMiddle = allPixels[i + WIDTH];
                pixelMesh[i].bottomRight = allPixels[i + WIDTH + 1];
            }
            else if(i % WIDTH == WIDTH - 1)
            { //Right
                //printf("Right\n");
                pixelMesh[i].topLeft = allPixels[i - WIDTH - 1];
                pixelMesh[i].topMiddle = allPixels[i - WIDTH];
                pixelMesh[i].topRight = GHOSTPIXELVALUE;
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = GHOSTPIXELVALUE;
                pixelMesh[i].bottomLeft = allPixels[i + WIDTH - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + WIDTH];
                pixelMesh[i].bottomRight = GHOSTPIXELVALUE;
            }
            else
            {
                //printf("ELSE CASE\n");
                pixelMesh[i].topLeft = allPixels[i - WIDTH - 1];
                pixelMesh[i].topMiddle = allPixels[i - WIDTH];
                pixelMesh[i].topRight = allPixels[i - WIDTH + 1];
                pixelMesh[i].left = allPixels[i - 1];
                pixelMesh[i].current = allPixels[i];
                pixelMesh[i].right = allPixels[i + 1];
                pixelMesh[i].bottomLeft = allPixels[i + WIDTH - 1];
                pixelMesh[i].bottomMiddle = allPixels[i + WIDTH];
                pixelMesh[i].bottomRight = allPixels[i + WIDTH + 1];
            }
        }
    }

    //This is how many pixels each process will recieve
    int pixelsPerProc;
    int recvcount;

    pixelsPerProc = totalPixelCount / nprocs;
    int remainder = totalPixelCount % nprocs;
    recvcount = pixelsPerProc + ((rank == nprocs - 1) ? remainder : 0);

    //test prints
    //printf("rank:%d\t %d pixelsPerProc\n", rank, pixelsPerProc);
    //printf("rank:%d\t %d recvcount\n", rank, recvcount);

    int * sendcounts = malloc(sizeof(int)*nprocs);
    int * displs = malloc(sizeof(int)*nprocs);
    int sum = 0;
    for(int i = 0; i < nprocs; ++i)
    {
        sendcounts[i] = pixelsPerProc + ((i == nprocs - 1) ? remainder : 0);
        displs[i] = sum;
        sum += sendcounts[i];
    }

    MPI_Datatype typesig[9] = {MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT};
    int blocklen[9] = {1,1,1,1,1,1,1,1,1};
    MPI_Aint displacements[9] = {
        offsetof(surroundingPixels, current),
        offsetof(surroundingPixels, topLeft),
        offsetof(surroundingPixels, topMiddle),
        offsetof(surroundingPixels, topRight),
        offsetof(surroundingPixels, left),
        offsetof(surroundingPixels, right),
        offsetof(surroundingPixels, bottomLeft),
        offsetof(surroundingPixels, bottomMiddle),
        offsetof(surroundingPixels, bottomRight)
    };
    MPI_Datatype MPI_SURROUNDING_PIXELS;
    MPI_Type_create_struct(9, blocklen, displacements, typesig, &MPI_SURROUNDING_PIXELS);
    MPI_Type_commit(&MPI_SURROUNDING_PIXELS);

    //printf("\n\nsendcounts: %d\tdispls %d\n\n", sendcounts[0], displs[0]);

    //SCATTER ALL PIXELS BETWEEN PROCESSES
    surroundingPixels *subPixelMesh = malloc(sizeof(surroundingPixels) * recvcount);
    MPI_Scatterv(pixelMesh,
                 sendcounts,
                 displs,
                 MPI_SURROUNDING_PIXELS,
                 subPixelMesh,
                 recvcount,
                 MPI_SURROUNDING_PIXELS,
                 0,
                 MPI_COMM_WORLD);

    //Run Cuda function (run Sobel on all pixels)
    //printf("rank: %d\tTEST\n", rank);
    float *out = runSobelOnPixels((size_t)recvcount, subPixelMesh, WIDTH, HEIGHT, rank);

    MPI_Barrier(MPI_COMM_WORLD);

    //Write all calculated data to a new binary
    MPI_Offset offset = (MPI_Offset)displs[rank] * (MPI_Offset)sizeof(float);

    MPI_Barrier(MPI_COMM_WORLD);

    //Does not need MPI_Gather because each rank can write to the file on its own
    MPI_File_open(MPI_COMM_WORLD, outputFilename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &ofh);

    MPI_File_write_at(ofh, offset, out, recvcount, MPI_FLOAT, &status);

    MPI_File_close(&ofh);

    MPI_Barrier(MPI_COMM_WORLD);

    //REPORT TELEMETRY HERE
    if(rank == 0)
    {
        free(pixelMesh);
        free(allPixels);
    }

    free(subPixelMesh);
    free(sendcounts);
    free(displs);
    MPI_Type_free(&MPI_SURROUNDING_PIXELS);
    freeOutArray(out);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();
    return 0;

}
