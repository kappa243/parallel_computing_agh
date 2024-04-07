#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <math.h>
#include <vector>
#include <algorithm>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

int compare(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

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
    std::vector<int> data;
    int size;
};


int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Usage: %s <size> <buckets> <threads>\n", argv[0]);
        exit(1);
    }


    int size = atoi(argv[1]);
    int n_buckets = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    // int num_threads = omp_get_max_threads();

    omp_set_num_threads(num_threads);

    // --- array allocation ---
    int *a = (int *) malloc(size * sizeof(int));

    // thread buckets - each thread has its own list of buckets)
    bucket ***thread_buckets = (bucket***) malloc(num_threads * sizeof(void *));

    // bucket lists - list of buckets for each thread
    for (int i = 0; i < num_threads; i++) {
        thread_buckets[i] = (bucket**) malloc(n_buckets * sizeof(void *));
    }

    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < n_buckets; j++) {
            thread_buckets[i][j] = new bucket;
        }
    } 

    
    bucket **concatenated_buckets = (bucket**) malloc(n_buckets * sizeof(void *));


    // random seed
    int tid;
    unsigned short xsubi[3];


    // --- fill array step ---
    double random_start_time = omp_get_wtime();

    #pragma omp parallel private(tid, xsubi) shared(a, size)
    {
        tid = omp_get_thread_num();
        xsubi[0] = xsubi[1] = xsubi[2] = tid + 3;

        #pragma omp for
        for (int i = 0; i < size; i++) {
            a[i] = (int)(nrand48(xsubi));
        }

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
            int bucket_id = MIN((a[i] / (INT_MAX / n_buckets)), n_buckets - 1); // min to put numbers meeting (INT_MAX % num_threads) to the last bucket
            thread_buckets[tid][bucket_id]->data.push_back(a[i]); // put into 'size' index and increment size
        }

    }

    double sort_stop_time = omp_get_wtime();

    // // print buckets
    // for (int i = 0; i < num_threads; i++) {
    //     for (int j = 0; j < n_buckets; j++) {
    //         if (thread_buckets[i][j]->data.size() > 0){
    //             printf("Thread %d, Bucket %d: ", i, j);
    //             for (int k = 0; k < thread_buckets[i][j]->data.size(); k++) {
    //                 printf("%d ", thread_buckets[i][j]->data[k]);
    //             }
    //             printf("\n");
    //         }
    //     }
    // }


    // --- merge buckets step ---
    double merge_start_time = omp_get_wtime();

    #pragma omp parallel for shared(thread_buckets, concatenated_buckets)
    for (int bucket_id = 0; bucket_id < n_buckets; bucket_id++){
        
        int conc_size = 0;
        for (int thread_id = 0; thread_id < num_threads; thread_id++){
            conc_size += thread_buckets[thread_id][bucket_id]->data.size();
        }

        // allocate memory for conc array
        concatenated_buckets[bucket_id] = new bucket;
        concatenated_buckets[bucket_id]->size = conc_size;
        
        // concatenate buckets
        for (int thread_id = 0; thread_id < num_threads; thread_id++) {
            for (int k = 0; k < thread_buckets[thread_id][bucket_id]->data.size(); k++) {
                concatenated_buckets[bucket_id]->data.push_back(thread_buckets[thread_id][bucket_id]->data[k]);
            }
        }

        // sort array
        std::sort(concatenated_buckets[bucket_id]->data.begin(), concatenated_buckets[bucket_id]->data.end());
    }
    

    double merge_stop_time = omp_get_wtime();

    // // print concatenated buckets
    // for (int i = 0; i < n_buckets; i++) {
    //     printf("Thread %d, Concatenated: ", i);
    //     for (int j = 0; j < concatenated_buckets[i]->data.size(); j++) {
    //         printf("%d ", concatenated_buckets[i]->data[j]);
    //     }
    //     printf("\n");
    // }

    // --- rewrite buckets step ---

    // synchronized rewrite size calculation
    int rewrite_size = 0;
    for (int i = 0; i < n_buckets; i++) {
        rewrite_size += concatenated_buckets[i]->data.size();
        concatenated_buckets[i]->size = rewrite_size;
    }

    double rewrite_start_time = omp_get_wtime();

    #pragma omp parallel for shared(a, concatenated_buckets)
    for (int bucket_id = 0; bucket_id < n_buckets; bucket_id++){
        int min_index;
        if (bucket_id == 0)
            min_index = 0;
        else
            min_index = concatenated_buckets[bucket_id - 1]->size;

        for (int i = min_index; i < concatenated_buckets[bucket_id]->size; i++) {
            a[i] = concatenated_buckets[bucket_id]->data[i - min_index];
        }
    }


    // // print array
    // for (int i = 0; i < size; i++) {
    //     printf("%d\n", a[i]);
    // }

    // --- deallocate buckets ---
    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < n_buckets; j++) {
            delete thread_buckets[i][j];
        }
    } 

    for (int i = 0; i < num_threads; i++) {
        free(thread_buckets[i]);
    }

    free(thread_buckets);


    // --- deallocate memory ---
    free(a);

    for (int i = 0; i < n_buckets; i++) {
        delete concatenated_buckets[i];
    }

    free(concatenated_buckets);

    // --- print times --- (fill, sort, merge, rewrite)
    printf("%f, %f, %f, %f\n", random_end_time - random_start_time, sort_stop_time - sort_start_time, merge_stop_time - merge_start_time, rewrite_start_time - merge_stop_time);

    return 0;
}