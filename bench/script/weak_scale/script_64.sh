#!/bin/bash
#----------------------------------------------------
#SBATCH -J ctf_pp
#SBATCH -o pp.bench.N64.n512.1e-2.o%j.out
#SBATCH -e pp.bench.N64.n512.1e-2.o%j.err
#SBATCH -p normal
#SBATCH -N 64
#SBATCH -n 512
#SBATCH -t 01:00:00
#SBATCH --mail-user=solomon2@illinois.edu
#SBATCH --mail-type=all    # Send email at begin and end of job

module list
pwd
date

export CTF_PPN=8
export OMP_NUM_THREADS=6

# ibrun ./pp_bench -model CP -tensor r -dim 6 -size 60 -rank 5 -maxiter 5 -filename CP_r_nodes=64_dim=6_size=60_rank=5_bench.csv 
ibrun ./pp_bench -model Tucker -tensor r2 -dim 6 -size 60 -rank 4 -maxiter 5 -filename Tucker_r2_nodes=64_dim=6_size=60_rank=4_bench.csv








