#include <vector>
#include <iostream>
#include <mpi.h>
#include <chrono>

#define MASTER_RANK 0

// Function to perform matrix multiplication
void matMul(float *A, float *B, float *C, int m, int n, int k) {
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < k; j++) {
            C[i * k + j] = 0;
            for (int l = 0; l < n; l++) {
                C[i * k + j] += A[i * n + l] * B[l * k + j];
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    int rank, size;
    MPI_Init(&argc, (char ***)&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Status status;

    if (rank == 0) {
        int m, n, k;
        std::cin >> m >> n >> k;
        std::cerr << "Calculating for m = " << m << ", n = " << n << ", k = " << k << std::endl;
        std::cerr << "Running on " << size << " processes\n";

        auto A = new float[m * n];
        auto B = new float[n * k];
        auto C = new float[m * k];
        for (int i = 0; i < m * n; i++) A[i] = (float)rand() / RAND_MAX;
        for (int i = 0; i < n * k; i++) B[i] = (float)rand() / RAND_MAX;

        auto matMulBeginTime = MPI_Wtime();

        int subM = (m + size - 1) / size;
        std::cerr << "subM = " << subM << std::endl;

        // Core 1 ~ size - 1
        for (int i = 1; i < size; ++i) {
            if (subM * i >= m)
                break;

            int rem = std::min(subM, m - subM * i);

            MPI_Send(&subM, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&A[subM * i * n], rem * n, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
            MPI_Send(B, n * k, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
        }

        // Core 0 
        matMul(A, B, C, subM, n, k);

        for (int i = 1; i < size; ++i) {
            if (subM * i >= m)
                break;

            MPI_Recv(&C[subM * i * k], subM * k, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
        }

        auto matMulEndTime = MPI_Wtime();
        std::cout << "matMul time: " << matMulEndTime - matMulBeginTime << " seconds" << std::endl;
    }
    else {
        int subM, n, k;
        MPI_Recv(&subM, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&k, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        auto A = new float[subM * n];
        auto B = new float[n * k];
        auto C = new float[subM * k];

        MPI_Recv(A, subM * n, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(B, n * k, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);

        matMul(A, B, C, subM, n, k);

        MPI_Send(C, subM * k, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
}
