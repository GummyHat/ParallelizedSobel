#!/bin/bash -x
#SBATCH --job-name=sobel_3node
#SBATCH --partition=el8-rpi
#SBATCH --nodes=3
#SBATCH --ntasks-per-node=6
#SBATCH --gres=gpu:6
#SBATCH --time=00:30:00
#SBATCH --output=run_3node.txt

# Strong scaling 3 nodes
echo "STRONG SCALING!!!"
mpirun -np 16 ./Sobel-mpi-multi-gpu 18432 18432 BlackMarble_2016_C1_18432x18432.data BlackMarble_2016_C1_18432x18432_16rank_3node_out.data

# Weak scaling 3 nodes
echo "WEAK SCALING!!!"
mpirun -np 16 ./Sobel-mpi-multi-gpu 46341 46341 BlackMarble_2016_Full_46341x46341.data BlackMarble_2016_Full_46341x46341_16rank_3node_out.data

