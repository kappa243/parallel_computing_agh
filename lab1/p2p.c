#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

const int N = 100;

const int N_POWERS = 31;

int main(int argc, char *argv[]) {
    int rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    double timesBlocking[N_POWERS];
    double timesNonBlocking[N_POWERS];

    for (int exp = 0; exp < N_POWERS; exp++) {
        // const int DATA_SIZE = 16384 * (exp + 1);
        const int DATA_SIZE = pow(2, exp);

        // define buffer
        void *sendBuffer = malloc(DATA_SIZE);
        void *recvBuffer = malloc(DATA_SIZE);

        MPI_Request send_request = MPI_REQUEST_NULL;
        MPI_Request recv_request = MPI_REQUEST_NULL;

        if (rank == 0) {
            // === blocking ===
            // sync processes
            MPI_Barrier(MPI_COMM_WORLD);

            // start timer
            double start = MPI_Wtime();

            for (int i = 0; i < N; i++) {
                MPI_Send(sendBuffer, DATA_SIZE, MPI_BYTE,
                       1, 0, MPI_COMM_WORLD);
                MPI_Recv(sendBuffer, DATA_SIZE, MPI_BYTE, 
                       1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            // stop timer
            double end = MPI_Wtime();
            timesBlocking[exp] = (end - start) / N;
            
            // === nonblocking ===
            // sync processes
            MPI_Barrier(MPI_COMM_WORLD);

            // start timer
            start = MPI_Wtime();
            
            for (int i = 0; i < N; i++) {
                MPI_Wait(&send_request, MPI_STATUS_IGNORE);
                MPI_Isend(sendBuffer, DATA_SIZE, MPI_BYTE, 
                        1, 0, MPI_COMM_WORLD, &send_request);

                MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
                MPI_Irecv(recvBuffer, DATA_SIZE, MPI_BYTE,
                        1, 0, MPI_COMM_WORLD, &recv_request);
            }

            // wait before exit
            MPI_Wait(&send_request, MPI_STATUS_IGNORE);
            MPI_Wait(&recv_request, MPI_STATUS_IGNORE);

            // stop timer
            end = MPI_Wtime();
            timesNonBlocking[exp] = (end - start) / N;
        } else {
            // === blocking ===
            // sync processes
            MPI_Barrier(MPI_COMM_WORLD);

            for (int i = 0; i < N; i++) {
                MPI_Recv(sendBuffer, DATA_SIZE, MPI_BYTE,
                       0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(sendBuffer, DATA_SIZE, MPI_BYTE,
                       0, 0, MPI_COMM_WORLD);
            }

            // === nonblocking ===
            // sync processes
            MPI_Barrier(MPI_COMM_WORLD);

            for (int i = 0; i < N; i++) {
                MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
                MPI_Irecv(recvBuffer, DATA_SIZE, MPI_BYTE,
                        0, 0, MPI_COMM_WORLD, &recv_request);

                MPI_Wait(&send_request, MPI_STATUS_IGNORE);
                MPI_Isend(sendBuffer, DATA_SIZE, MPI_BYTE,
                        0, 0, MPI_COMM_WORLD, &send_request);
            }

            // wait before exit
            MPI_Wait(&send_request, MPI_STATUS_IGNORE);
            MPI_Wait(&recv_request, MPI_STATUS_IGNORE);
        }


        // free buffer
        free(sendBuffer);
        free(recvBuffer);
    }

    if (rank == 0) {
        for (int exp = 0; exp < N_POWERS; exp++) {
            printf("%d, %f, %f\n", exp, timesBlocking[exp], timesNonBlocking[exp]);
        }
    }

    MPI_Finalize();
    return 0;
}