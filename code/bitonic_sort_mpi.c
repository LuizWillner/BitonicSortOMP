#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "mpi.h"

#define LOW 0
#define HIGH 1
#define MAX 100000

int temp_list[MAX];

void Merge_list(
    int list_size,
    int list1[],
    int list2[]
);

void Par_bitonic_sort_incr(
    int list_size,
    int local_list[],
    int proc_set_size,
    MPI_Comm comm
);

void Par_bitonic_sort_decr(
    int list_size,
    int local_list[],
    int proc_set_size,
    MPI_Comm comm
);

int CrescFunc(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int DecrescFunc(const void *a, const void *b) {
    return (*(int *)b - *(int *)a);
}

void printArray(int *arr, int my_rank) {
    printf("Processo %d --- [%d, %d, %d, %d]\n", my_rank, arr[0], arr[1], arr[2], arr[3]);
}

void Merge_split(
    int list_size,
    int local_list[],
    int which_keys,
    int partner,
    MPI_Comm comm
) {
    MPI_Status status;

    // Get your MPI process rank
    int my_rank;
    MPI_Comm_rank(comm, &my_rank);  //TODO: Apagar depois, my_rank é pego só pra printar

    MPI_Sendrecv(local_list, list_size, MPI_INT,
                partner, 0, temp_list, list_size,
                MPI_INT, partner, 0, comm, &status);


    if (which_keys == HIGH) 
        Merge_list(list_size, temp_list, local_list);
    else
        Merge_list(list_size, local_list, temp_list);
    
    printArray(local_list, my_rank);
}

int main(int argc, char* argv[]) {
    int proc_set_size, and_bit, i, list_size, my_rank, num_processes;
    double timer_start, timer_end;
    int big_list[] = {13, 2, 5, 4, 16, 9, 6, 10, 12, 7, 14, 3, 11, 15, 1, 8};

    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &num_processes);
    MPI_Comm_rank(comm, &my_rank);
    list_size = atoi(argv[1]) / num_processes;
    int* local_list = (int *)malloc(list_size * sizeof(int));

    int first_index = my_rank * list_size;
    int last_index = first_index + list_size - 1;

    for (int i = first_index, j = 0; i <= last_index; i++, j++) {
        local_list[j] = big_list[i];
    }

    printf("Processo %d --- BEFORE BARRIER - local_list:\n", my_rank);
    printArray(local_list, my_rank);

    MPI_Barrier(comm);
    MPI_Barrier(comm);
    MPI_Barrier(comm);
    MPI_Barrier(comm);

    if (my_rank == 0) {
        printf("Processes: %d\n", num_processes);
        timer_start = MPI_Wtime();
    }

    if (my_rank % 2 == 0)
        qsort(local_list, list_size, sizeof(int), CrescFunc);
    else
        qsort(local_list, list_size, sizeof(int), DecrescFunc);
    
    printArray(local_list, my_rank);

    for (proc_set_size = 2, and_bit = 2;
        proc_set_size <= num_processes;
        proc_set_size *= 2, and_bit = and_bit << 1
    ){
        if ((my_rank & and_bit) == 0)
            Par_bitonic_sort_incr(list_size, local_list, proc_set_size, comm);
        else
            Par_bitonic_sort_decr(list_size, local_list, proc_set_size, comm);
    }
    
    MPI_Barrier(comm);
    MPI_Barrier(comm);
    MPI_Barrier(comm);
    MPI_Barrier(comm);


    timer_end = MPI_Wtime();
    printArray(local_list, my_rank);
    printf("\nProcesso %d --- Time Elapsed (Sec): %f\n", my_rank, timer_end - timer_start);


    free(local_list);
    MPI_Finalize();
    return 0;
}

void Par_bitonic_sort_incr(
    int list_size,
    int local_list[],
    int proc_set_size,
    MPI_Comm comm
){
    int my_rank, proc_set_dim, eor_bit, stage, partner;

    // Get your MPI process rank
    MPI_Comm_rank(comm, &my_rank);

    proc_set_dim = log2(proc_set_size);
    eor_bit = 1 << (proc_set_dim - 1);

    for (stage = 0; stage < proc_set_dim; stage++) {
        partner = my_rank ^ eor_bit;
        printf("Processo %d --- eor_bit = %d; partner = %d\n", my_rank, eor_bit, partner);

        if (my_rank < partner)
            Merge_split(list_size, local_list, LOW, partner, comm);
        else
            Merge_split(list_size, local_list, HIGH, partner, comm);
        eor_bit = eor_bit >> 1;
    }
}

void Par_bitonic_sort_decr(
    int list_size,
    int local_list[],
    int proc_set_size,
    MPI_Comm comm
){
    int my_rank, proc_set_dim, eor_bit, stage, partner;

    // Get your MPI process rank
    MPI_Comm_rank(comm, &my_rank);

    // Calculate the dimension of the processor set
    proc_set_dim = log2(proc_set_size);

    eor_bit = 1 << (proc_set_dim - 1);
    for (stage = 0; stage < proc_set_dim; stage++) {
        partner = my_rank ^ eor_bit;
        printf("Processo %d --- eor_bit = %d; partner = %d\n", my_rank, eor_bit, partner);

        if (my_rank > partner)
            Merge_split(list_size, local_list, LOW, partner, comm);
        else
            Merge_split(list_size, local_list, HIGH, partner, comm);
        eor_bit = eor_bit >> 1;
    }
}



void Merge_list(int list_size, int list1[], int list2[]) {
    int merged_list[list_size * 2];
    for (int i = 0; i < list_size; i++) {
        merged_list[i] = list1[i];
        merged_list[i+list_size] = list2[i];
    }
    qsort(merged_list, list_size*2, sizeof(int), CrescFunc);
    for (int i = 0; i < list_size; i++) {
        list1[i] = merged_list[i];
        list2[i] = merged_list[i+list_size];
    }

}