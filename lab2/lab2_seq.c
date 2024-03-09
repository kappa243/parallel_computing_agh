#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
// #include <mpi.h>

const double PI = 3.14159265358979323846;

double randOneToOne(){
    return (double)rand() / RAND_MAX * 2.0 - 1.0;
}

const long N = 1e7 * 4;

int main(int argc, char *argv[]) {
    clock_t start_time = clock();

    long sum = 0;
    for (long i = 0; i < N; i++){
        // rand double between -1 and 1
        double x = randOneToOne();
        double y = randOneToOne();

        if ((x*x + y*y) <= 1) sum++;

        // print progress every 10e5 iterations
        // if (i % (long)10e1 == 0){
        //     // progress in percent but dont print new line instead overwrite previous line
        //     double progress = (double)i / K * 100;
        //     printf("Progress: %.2f%%\r", progress);
        //     fflush(stdout);
        // }
    }

    clock_t end_time = clock();

    // double aprox_pi = 4.0 * sum / N;
    // double error = fabs(aprox_pi - PI);

    // printf("Time: %f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);
    // printf("Pi is approximately %.16f, Error is %.16f\n", aprox_pi, error);
    
    printf("%ld, %d, %f\n", N, 0, (double)(end_time - start_time) / CLOCKS_PER_SEC);
    fflush(stdout);

    return 0;
}