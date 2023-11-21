#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define LOW 0
#define HIGH 1
#define MAX 10000000

int temp_list[MAX];

void Merge_list(
    int list_size,
    int list1[],
    int list2[]
);

void Par_bitonic_sort_incr(
    int list_size,
    int local_list[],
    int proc_set_size
);

void Par_bitonic_sort_decr(
    int list_size,
    int local_list[],
    int proc_set_size
);

int CrescFunc(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

int DecrescFunc(const void *a, const void *b) {
    return (*(int *)b - *(int *)a);
}

void printArray(int n, int *arr, int my_rank) {
    int i;

    printf("Thread %d --- [%d", my_rank, arr[0]);
    for (i = 1; i < n; i++) {
        printf(",%d", arr[i]);
    }
    printf("]\n");

}

// Merge_split adapted for OpenMP
void Merge_split(int list_size, int local_list[], int which_keys) {
    // Copy the local_list to temp_list
    for (int i = 0; i < list_size; i++) {
        temp_list[i] = local_list[i];
    }
    
    if (which_keys == HIGH) {
        Merge_list(list_size, temp_list, local_list);
    } else {
        Merge_list(list_size, local_list, temp_list);
    }

    // Copy the sorted temp_list back to local_list
    for (int i = 0; i < list_size; i++) {
        local_list[i] = temp_list[i];
    }
}

// void Merge_split(
//     int list_size,
//     int local_list[],
//     int which_keys,
//     int partner,
//     MPI_Comm comm
// ) {
//     MPI_Status status;

//     MPI_Sendrecv(local_list, list_size, MPI_INT,
//                 partner, 0, temp_list, list_size,
//                 MPI_INT, partner, 0, comm, &status);


//     if (which_keys == HIGH) 
//         Merge_list(list_size, temp_list, local_list);
//     else
//         Merge_list(list_size, local_list, temp_list);
// }

int main(int argc, char *argv[]) {
    int proc_set_size, and_bit, i, list_size, num_threads, itens_per_thread;
    double timer_start, timer_end;

    list_size = atoi(argv[1]);
    int *local_list = (int *)malloc(list_size * sizeof(int));

    srand(time(NULL));

    timer_start = omp_get_wtime();


    #pragma omp parallel
    {
        int my_rank = omp_get_thread_num();
        int num_threads = omp_get_num_threads();
        itens_per_thread = list_size / num_threads;

        for (i = itens_per_thread*my_rank; i < itens_per_thread*(my_rank+1); i++) {
            local_list[i] = (rand() % (atoi(argv[1]))) * (my_rank + 1);
            //printf("%d thread:%d \n", local_list[i], my_rank);
        }
        #pragma omp barrier

        qsort(&local_list[itens_per_thread*my_rank], itens_per_thread, sizeof(int), my_rank % 2 == 0 ? CrescFunc : DecrescFunc);

        #pragma omp barrier

         for (proc_set_size = 2, and_bit = 2; proc_set_size <= omp_get_num_threads(); proc_set_size *= 2, and_bit = and_bit << 1) {
             if ((omp_get_thread_num() & and_bit) == 0){
                Par_bitonic_sort_incr(list_size, local_list, proc_set_size);

        //     printf("incr\n");
             }
             else{
                 Par_bitonic_sort_decr(list_size, local_list, proc_set_size);

        //     printf("derc\n");
             }
         }

    }



    if (omp_get_thread_num() == 0) {
        timer_end = omp_get_wtime();
        printArray(64, local_list, omp_get_thread_num());
        printf("\nThread %d --- Time Elapsed (Sec): %f\n", omp_get_thread_num(), timer_end - timer_start);
    }

    free(local_list);
    return 0;
}

// Par_bitonic_sort_incr adapted for OpenMP
void Par_bitonic_sort_incr(int list_size, int local_list[], int proc_set_size) {
    int and_bit = proc_set_size / 2;
    for (int stage = 0; stage < log2(proc_set_size); stage++) {
        #pragma omp parallel for
        for (int i = 0; i < list_size; i += proc_set_size) {
            if ((i & and_bit) == 0) {
                Merge_split(proc_set_size, &local_list[i], LOW);
            }
        }
        and_bit = and_bit >> 1;
    }
}

// Par_bitonic_sort_decr adapted for OpenMP
void Par_bitonic_sort_decr(int list_size, int local_list[], int proc_set_size) {
    int and_bit = proc_set_size / 2;
    for (int stage = 0; stage < log2(proc_set_size); stage++) {
        #pragma omp parallel for
        for (int i = 0; i < list_size; i += proc_set_size) {
            if ((i & and_bit) != 0) {
                Merge_split(proc_set_size, &local_list[i], HIGH);
            }
        }
        and_bit = and_bit >> 1;
    }
}

// void Par_bitonic_sort_incr(
//     int list_size,
//     int local_list[],
//     int proc_set_size,
//     MPI_Comm comm
// ){
//     int my_rank, proc_set_dim, eor_bit, stage, partner;

//      Get your MPI process rank
//     MPI_Comm_rank(comm, &my_rank);

//     proc_set_dim = log2(proc_set_size);
//     eor_bit = 1 << (proc_set_dim - 1);

//     for (stage = 0; stage < proc_set_dim; stage++) {
//         partner = my_rank ^ eor_bit;

//         if (my_rank < partner)
//             Merge_split(list_size, local_list, LOW, partner, comm);
//         else
//             Merge_split(list_size, local_list, HIGH, partner, comm);
//         eor_bit = eor_bit >> 1;
//     }
// }

// void Par_bitonic_sort_decr(
//     int list_size,
//     int local_list[],
//     int proc_set_size,
//     MPI_Comm comm
// ){
//     int my_rank, proc_set_dim, eor_bit, stage, partner;

//     // Get your MPI process rank
//     MPI_Comm_rank(comm, &my_rank);

//     // Calculate the dimension of the processor set
//     proc_set_dim = log2(proc_set_size);

//     eor_bit = 1 << (proc_set_dim - 1);
//     for (stage = 0; stage < proc_set_dim; stage++) {
//         partner = my_rank ^ eor_bit;
//         if (my_rank > partner)
//             Merge_split(list_size, local_list, LOW, partner, comm);
//         else
//             Merge_split(list_size, local_list, HIGH, partner, comm);
//         eor_bit = eor_bit >> 1;
//     }
// }



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