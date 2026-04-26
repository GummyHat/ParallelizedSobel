#!/bin/bash -x
#SBATCH --job-name=sobel_2node
#SBATCH --partition=el8-rpi
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=4
#SBATCH --gres=gpu:4
#SBATCH --time=00:30:00
#SBATCH --output=run_2node.txt


# Strong scaling 2 nodes
echo "STRONG SCALING!!!"
mpirun -np 8 ./Sobel-mpi-multi-gpu 18432 18432 BlackMarble_2016_C1_18432x18432.data BlackMarble_2016_C1_18432x18432_8rank_2node_out.data

# Weak scaling 2 nodes
echo "WEAK SCALING!!!"
mpirun -np 8 ./Sobel-mpi-multi-gpu 32768 32768 BlackMarble_2016_Full_32768x32768.data BlackMarble_2016_Full_32768x32768_8rank_2node_out.data
