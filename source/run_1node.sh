#!/bin/bash -x
#SBATCH --job-name=sobel_1node
#SBATCH --partition=el8-rpi
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --time=00:30:00
#SBATCH --output=run_1node.txt

# Strong scaling on 1 node

echo "STRONG SCALING!!!"
mpirun -np 1 ./Sobel-mpi-multi-gpu 18432 18432 BlackMarble_2016_C1_18432x18432.data BlackMarble_2016_C1_18432x18432_1rank_1node_out.data
mpirun -np 2 ./Sobel-mpi-multi-gpu 18432 18432 BlackMarble_2016_C1_18432x18432.data BlackMarble_2016_C1_18432x18432_2rank_1node_out.data
mpirun -np 4 ./Sobel-mpi-multi-gpu 18432 18432 BlackMarble_2016_C1_18432x18432.data BlackMarble_2016_C1_18432x18432_4rank_1node_out.data

# Weak scaling on 1 node
mpirun -np 1 ./Sobel-mpi-multi-gpu 11585 11585 BlackMarble_2016_Full_11585x11585.data BlackMarble_2016_Full_11585x11585_1rank_1node_out.data
mpirun -np 2 ./Sobel-mpi-multi-gpu 16384 16384 BlackMarble_2016_Full_16384x16384.data BlackMarble_2016_Full_16384x16384_2rank_1node_out.data
mpirun -np 4 ./Sobel-mpi-multi-gpu 23170 23170 BlackMarble_2016_Full_23170x23170.data BlackMarble_2016_Full_23170x23170_4rank_1node_out.data
