#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {

    if (argc != 4) {
        printf("Usage: %s <size> <chunk_size> <schedule_type>\n", argv[0]);
        exit(1);
    }

    int size = atoi(argv[1]);
    int chunk_size = atoi(argv[2]);
    int schedule_type = atoi(argv[3]);

    switch (schedule_type) {
        case 0:
            omp_set_schedule(omp_sched_static, chunk_size);
            break;
        case 1:
            omp_set_schedule(omp_sched_dynamic, chunk_size);
            break;  
        case 2:
            omp_set_schedule(omp_sched_guided, chunk_size);
            break;
        default:
            printf("Invalid schedule type\n");
            exit(1);
    }


    int *a = malloc(size * sizeof(int));
    int i;

    int tid;

    unsigned short xsubi[3];

    double start_time = omp_get_wtime();

#pragma omp parallel private(tid, xsubi) shared(a, size)
    {
        tid = omp_get_thread_num();
        xsubi[0] = xsubi[1] = xsubi[2] = tid + 3;

#pragma omp for schedule(runtime)
        for (i = 0; i < size; i++) {
            a[i] = (int)(jrand48(xsubi));
        }

#pragma omp barrier
    }

    double end_time = omp_get_wtime();

    printf("%f\n",  end_time - start_time);

    free(a);

    return 0;
}
