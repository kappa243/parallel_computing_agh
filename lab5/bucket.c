#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

/*

Bucket Sort Algorithm.

1) Read first parameter from command line as the size of the array.
2) Fill array with random numbers from 0 to MAX_INT. Parallelized. Calculated times.
3) Sorting array.
  3.1) Each thread have its own part of array.
  3.2) Each thread sorts its part to its own buckets (number of buckets = number of threads). Parallelized. Calculated times.
  3.3) When all threads finished sorting to bucket, merge all buckets (one bucket is merged by one thread). Parallelized. Calculated times.
  3.4) Each thread rewrites its own sorted bucket to the array. (Threads should inform each other what is size of each bucket
       to be able to rewrite it to the array in correct index position. It must be synchronized). Parallelized. Calculated times.
4) Print time of each part.

*/


struct bucket {
    int *data;
    int size;
} typedef bucket;


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <size>\n", argv[0]);
        exit(1);
    }


    // --- array allocation ---
    int size = atoi(argv[1]);

    int *a = malloc(size * sizeof(int));


    int num_threads = omp_get_max_threads();

    // printf("Threads: %d\n", num_threads);

    // thread buckets - each thread has its own list of buckets)
    bucket ***thread_buckets = malloc(num_threads * sizeof(void *));

    // bucket lists - list of buckets for each thread
    for (int i = 0; i < num_threads; i++) {
        thread_buckets[i] = malloc(num_threads * sizeof(void *));
    }

    // buckets - buckets for each thread
    int bucket_size = size / num_threads + size % num_threads;

    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < num_threads; j++) {
            thread_buckets[i][j] = malloc(sizeof(bucket));
            thread_buckets[i][j]->data = malloc(bucket_size * sizeof(int));
            thread_buckets[i][j]->size = 0;
        }
    } 

    
    bucket **concatenated_buckets = malloc(num_threads * sizeof(void *));


    // random seed
    int tid;
    unsigned short xsubi[3];


    // --- fill array step ---
    double random_start_time = omp_get_wtime();

#pragma omp parallel private(tid, xsubi) shared(a, size)
    {
        tid = omp_get_thread_num();
        xsubi[0] = xsubi[1] = xsubi[2] = tid + 3;

#pragma omp for schedule(guided, 2048)
        for (int i = 0; i < size; i++) {
            a[i] = (int)(nrand48(xsubi));
        }

#pragma omp barrier
    }

    double random_end_time = omp_get_wtime();


    // --- sort into buckets step ---
    double sort_start_time = omp_get_wtime();

#pragma omp parallel private(tid) shared(a, thread_buckets, size)
    {
        tid = omp_get_thread_num();
        
        int base_job_size = size / num_threads;
        int job_size = base_job_size;
        if (omp_get_thread_num() == num_threads - 1)
            job_size += size % num_threads;
        
        for (int i = tid * base_job_size; i < tid * base_job_size + job_size; i++) {
            int bucket_id = MIN((a[i] / (INT_MAX / num_threads)), num_threads - 1); // min to put numbers meeting (INT_MAX % num_threads) to the last bucket
            thread_buckets[tid][bucket_id]->data[thread_buckets[tid][bucket_id]->size++] = a[i]; // put into 'size' index and increment size
        }

    }

    double sort_stop_time = omp_get_wtime();

    // // print buckets
    // for (int i = 0; i < num_threads; i++) {
    //     for (int j = 0; j < num_threads; j++) {
    //         if (thread_buckets[i][j]->size > 0){
    //             printf("Thread %d, Bucket %d: ", i, j);
    //             for (int k = 0; k < thread_buckets[i][j]->size; k++) {
    //                 printf("%d ", thread_buckets[i][j]->data[k]);
    //             }
    //             printf("\n");
    //         }
    //     }
    // }


    // --- merge buckets step ---
    double merge_start_time = omp_get_wtime();

    int compare(const void *a, const void *b) {
        return (*(int *)a - *(int *)b);
    }

#pragma omp parallel private(tid) shared(thread_buckets, concatenated_buckets)
    {
        tid = omp_get_thread_num();

        // create conc array
        int conc_size = 0;
        for (int i = 0; i < num_threads; i++) {
            conc_size += thread_buckets[i][tid]->size;
        }

        // allocate memory for conc array
        concatenated_buckets[tid] = malloc(sizeof(bucket));
        concatenated_buckets[tid]->data = malloc(conc_size * sizeof(int));
        concatenated_buckets[tid]->size = conc_size;
        
        // concatenate buckets
        for (int i = 0, j = 0; i < num_threads; i++) {
            for (int k = 0; k < thread_buckets[i][tid]->size; k++) {
                concatenated_buckets[tid]->data[j++] = thread_buckets[i][tid]->data[k];
            }
        }

        // sort array
        qsort(concatenated_buckets[tid]->data, concatenated_buckets[tid]->size, sizeof(int), compare);
    }

    double merge_stop_time = omp_get_wtime();

    // // print concatenated buckets
    // for (int i = 0; i < num_threads; i++) {
    //     printf("Thread %d, Concatenated: ", i);
    //     for (int j = 0; j < concatenated_buckets[i]->size; j++) {
    //         printf("%d ", concatenated_buckets[i]->data[j]);
    //     }
    //     printf("\n");
    // }

    // --- rewrite buckets step ---

    // synchronized rewrite size calculation
    int rewrite_size = 0;
    for (int i = 0; i < num_threads; i++) {
        rewrite_size += concatenated_buckets[i]->size;
        concatenated_buckets[i]->size = rewrite_size;
    }

    double rewrite_start_time = omp_get_wtime();

#pragma omp parallel private(tid) shared(a, concatenated_buckets)
    {
        tid = omp_get_thread_num();

        int min_index;
        if (tid == 0)
            min_index = 0;
        else
            min_index = concatenated_buckets[tid - 1]->size;

        for (int i = min_index; i < concatenated_buckets[tid]->size; i++) {
            a[i] = concatenated_buckets[tid]->data[i - min_index];
        }
    }

    double rewrite_stop_time = omp_get_wtime();

    // // print array
    // for (int i = 0; i < size; i++) {
    //     printf("%d\n", a[i]);
    // }

    // --- deallocate buckets ---
    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < num_threads; j++) {
            free(thread_buckets[i][j]->data);
            free(thread_buckets[i][j]);

        }
    } 

    for (int i = 0; i < num_threads; i++) {
        free(thread_buckets[i]);
    }

    free(thread_buckets);


    // --- deallocate memory ---
    free(a);

    for (int i = 0; i < num_threads; i++) {
        free(concatenated_buckets[i]->data);
        free(concatenated_buckets[i]);
    }

    free(concatenated_buckets);

    // --- print times --- (fill, sort, merge, rewrite)
    printf("%f, %f, %f, %f\n", random_end_time - random_start_time, sort_stop_time - sort_start_time, merge_stop_time - merge_start_time, rewrite_start_time - rewrite_stop_time);

    return 0;
}