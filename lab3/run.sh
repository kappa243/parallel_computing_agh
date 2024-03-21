#!/bin/bash -l
#SBATCH --output=/net/people/plgrid/plgidec/lab3.out
#SBATCH --nodes 1
#SBATCH --ntasks 12
#SBATCH --time=01:00:00
#SBATCH --partition=plgrid-testing
#SBATCH --account=plgmpr24-cpu

mpicc -lm -o lab3 lab3.c

sizes=(1000000 140712473 19800000000)


for size in "${sizes[@]}"; do
    echo "Running for:"
    for proc_i in {1..12}; do
        actual_size=$(($size)) # amdahl
        # actual_size=$(($size * $proc_i)) # gustafson
        echo "size=$size, proc_i=$proc_i, actual_size=$actual_size"
        mpirun -np $proc_i ./lab3 $size >> ./results/size_${size}.csv
    done
done