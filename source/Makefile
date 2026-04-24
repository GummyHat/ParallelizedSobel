Sobel: Sobel.c Sobel.cu
	mpixlc -O3 Sobel.c -c -o Sobel-xlc.o
	nvcc -O3 -arch=sm_70 Sobel.cu -c -o Sobel-nvcc.o
	mpixlc -O3 Sobel-xlc.o Sobel-nvcc.o -o Sobel-mpi-multi-gpu -L/usr/local/cuda-11.2/lib64/ -lcudadevrt -lcudart -lstdc++
