# ParallelizedSobel
This is a repository containing code for a parallelized Sobel algorithm that utilizes MPI and CUDA function calls. This program is meant to run on Power9 system architecture.


This is image edge detection:
-Sobel or Prewitt Filter
-Look at an image file (MPI-IO)
-Output a grayscale image where each pixel represents the strength of a detected edge

Convert file to a binary file (.bin) for reading the image into each node's memory
Use MPI_File_read_at for each ranks chunk of the photo (rank 0 does row 1; rank 1 does row 2)
Then MPI_send recv for calculations
-Need to hard code length and width

MPI_IO reads and outputs image
MPI to send and recv adjacent pixel rows
CUDA to calculate SOBEL Values
MPI_IO outputs new image


Each pixel is dependent on adjacent pixels, but not on those pixels' results

#### 3 BYTES PER PIXEL RGB IN ORDER!!!

Algorithm:
-Measure intensity in its 3x3 area (high intensity)

https://www.youtube.com/watch?v=uihBwtPIBxM


TODO:
-Currently, the number of MPI Ranks must be equal to the width of the image for logic to work. This must be resolved.

-The output file is 1/3 the size of the input, which is perfect. But, each pixel of the output file is equal to the grayscale value of the first pixel of the input file.

-Ensure MPI send/recvs are working

-Finishing writing Kernel
