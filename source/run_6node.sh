#!/bin/bash -x
#SBATCH --job-name=sobel_6node
#SBATCH --partition=el8-rpi
#SBATCH --nodes=6
#SBATCH --ntasks-per-node=6
#SBATCH --gres=gpu:6
#SBATCH --time=00:30:00
#SBATCH --output=run_6node.txt

# Strong scaling 6 nodes
echo "STRONG SCALING!!!"
mpirun -np 32 ./Sobel-mpi-multi-gpu 18432 18432 BlackMarble_2016_C1_18432x18432.data BlackMarble_2016_C1_18432x18432_32rank_6node_out.data
