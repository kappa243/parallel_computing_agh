#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

const double PI = 3.14159265358979323846;

// random double between -1 and 1
double randOneToOne(){
    return (double)rand() / RAND_MAX * 2.0 - 1.0;
}


int main(int argc, char *argv[]) {
    long N = atol(argv[1]);   
    
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(rank);

    // calculate iteration for current process (last process may have more iterations if N is not divisible by size)
    long K = N / size;
    if (rank == size - 1) K += N % size;

    // double barrier start time to ensure all processes start calculation at the same time
    MPI_Barrier(MPI_COMM_WORLD);

    double start_time = MPI_Wtime();
   
    MPI_Barrier(MPI_COMM_WORLD);

    long sum = 0;
    for (long i = 0; i < K; i++){
        double x = randOneToOne();
        double y = randOneToOne();

        if ((x*x + y*y) <= 1) sum++;
    }

    long global_sum;
    MPI_Reduce(&sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();

    if (rank == 0){
        printf("%ld, %d, %f\n", N, size, end_time - start_time);
        fflush(stdout);
    }


    MPI_Finalize();
    return 0;
}