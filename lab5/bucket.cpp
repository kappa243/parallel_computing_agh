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

Bucket Sort Algorithm 3.

1) Read first parameter from command line as the size of the array.
2) Fill array with random numbers from 0 to MAX_INT. Parallelized. Calculated times.
3) Sorting array.
  3.1) Each thread have its own part of array.
  3.2) Each thread sorts its part to its own buckets (number of buckets = number of threads). Parallelized. Calculated times.
  3.3) When all threads finished sorting to bucket, merge all buckets (one bucket is merged by one thread). Parallelized. Calculated times.
  3.4) Sort buckets. Parallelized. Calculated times.
  3.5) Synchronized calculation of write index.
  3.6) Each thread rewrites its own sorted bucket to the array. Parallelized. Calculated times.
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
    for (int i = 0; i < n_buckets; i++) {
        concatenated_buckets[i] = new bucket;
    }


    // random seed
    int tid;
    unsigned short xsubi[3];


    double alg_start_time = omp_get_wtime();

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


    // --- distribute into buckets step ---
    double distrb_start_time = omp_get_wtime();

    #pragma omp parallel private(tid) shared(a, thread_buckets, size)
    {
        tid = omp_get_thread_num();
        
        int base_job_size = size / num_threads;
        int job_size = base_job_size;
        if (omp_get_thread_num() == num_threads - 1)
            job_size += size % num_threads;
        
        for (int i = tid * base_job_size; i < tid * base_job_size + job_size; i++) {
            int bucket_id = MIN((a[i] / (INT_MAX / n_buckets)), n_buckets - 1); // min to put numbers meeting (INT_MAX % num_threads) to the last bucket
            thread_buckets[tid][bucket_id]->data.push_back(a[i]); // put into bucket
        }

    }

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

    #pragma omp parallel for shared(thread_buckets, concatenated_buckets)
    for (int bucket_id = 0; bucket_id < n_buckets; bucket_id++){
        
        int conc_size = 0;
        for (int thread_id = 0; thread_id < num_threads; thread_id++){
            conc_size += thread_buckets[thread_id][bucket_id]->data.size();
        }

        concatenated_buckets[bucket_id]->size = conc_size;
        
        // concatenate buckets
        for (int thread_id = 0; thread_id < num_threads; thread_id++) {
            for (int k = 0; k < thread_buckets[thread_id][bucket_id]->data.size(); k++) {
                concatenated_buckets[bucket_id]->data.push_back(thread_buckets[thread_id][bucket_id]->data[k]);
            }
        }

    }

    double distrb_stop_time = omp_get_wtime();


    
   
    
    double sort_start_time = omp_get_wtime();

    // --- sort buckets step ---

    #pragma omp parallel for shared(concatenated_buckets)
    for (int bucket_id = 0; bucket_id < n_buckets; bucket_id++){
        // sort array
        std::sort(concatenated_buckets[bucket_id]->data.begin(), concatenated_buckets[bucket_id]->data.end());

    }

    // // print concatenated buckets
    // for (int i = 0; i < n_buckets; i++) {
    //     printf("Thread %d, Concatenated: ", i);
    //     for (int j = 0; j < concatenated_buckets[i]->data.size(); j++) {
    //         printf("%d ", concatenated_buckets[i]->data[j]);
    //     }
    //     printf("\n");
    // }

    double sort_stop_time = omp_get_wtime();

    // --- rewrite buckets step ---

    double rewrite_start_time = omp_get_wtime();

    // synchronized rewrite size calculation
    int rewrite_size = 0;
    for (int i = 0; i < n_buckets; i++) {
        rewrite_size += concatenated_buckets[i]->data.size();
        concatenated_buckets[i]->size = rewrite_size;
    }


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

    double rewrite_stop_time = omp_get_wtime();

    // // print array
    // for (int i = 0; i < size; i++) {
    //     printf("%d\n", a[i]);
    // }

    double alg_stop_time = omp_get_wtime();


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

    // --- print times --- (fill, distribute, sort, rewrite, all)
    printf("%f,%f,%f,%f,%f", random_end_time - random_start_time, distrb_stop_time - distrb_start_time, sort_stop_time - sort_start_time, rewrite_stop_time - rewrite_start_time, alg_stop_time - alg_start_time);

    return 0;
}