#!/bin/bash

echo "size,schedule,chunk,threads,time" > "output.csv"

for size in 0 2 4 6 7 8 9; do
    actual_size=$((10**$size))

    for schedule_type in 0 1 2; do
        for chunk_size in 0 1 64 1024 2048 4096; do
            for threads in 1 2 4 8 16 32 64; do
                echo "run for" $actual_size "size, " $chunk_size "chunk size, " $schedule_type "schedule type and " $threads "threads"
                gcc -Wall rand.c -o rand -fopenmp -DCHUNKSIZE=$chunk_size -DPADDING=0
                printf "%d,%d,%d,%d,%f\n" $actual_size $schedule_type $chunk_size $threads $(OMP_NUM_THREADS=$threads OMP_DYNAMIC=false ./rand $actual_size $chunk_size $schedule_type) >> "output.csv";
            done
        done
    done
done
