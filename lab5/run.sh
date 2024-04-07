#!/bin/bash

echo "size,bucket,threads,fill,distrib,sort,rewrite,all" > "output.csv"

g++ bucket.cpp -o bucket -fopenmp
for size in 5000000 10000000 15000000; do

    for bucket_size in 0 1 2; do
        bucket_real=$bucket_size
        for threads in 1 2 3 4 5 6 7 8; do
            case $bucket_size in
                0)
                    bucket_real=$size
                    ;;
                1)
                    bucket_real=$threads
                    ;;
                2)
                    bucket_real=$(echo "sqrt($size)" | bc)
                    ;;
            esac

            echo "run for size: " $size ", bucket size: " $bucket_real " and threads: " $threads
            printf "%d,%d,%d,%s\n" $size $bucket_real $threads $(OMP_DYNAMIC=false ./bucket $size $bucket_real $threads) >> "output.csv";
        done
    done
done
