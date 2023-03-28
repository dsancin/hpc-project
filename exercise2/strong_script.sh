#!/bin/bash

#SBATCH --job-name="script"
#SBATCH --output=log.out
#SBATCH --partition=EPYC
#SBATCH	--nodes=1
#SBATCH --exclusive
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=64
#SBATCH --time=02:00:00
#SBATCH --nodelist=epyc[005]

module load architecture/AMD
module load openBLAS/0.3.21-omp
module load mkl

rm gemm_oblas.x
rm gemm_mkl.x

srun -n 1 make cpu


for i in 1 2 4 6 8 12 16 24 32 48 64
do
   export OMP_NUM_THREADS=$i
   export OMP_PLACES=sockets
   export OMP_PROC_BIND=close
   for j in {1..5}
   do
       srun ./gemm_oblas.x 15000 15000 15000 >> strong_scalability/single/sockets_close_oblas.csv
       srun ./gemm_mkl.x 15000 15000 15000 >> strong_scalability/single/sockets_close_mkl.csv
       echo
   done
   echo
done
