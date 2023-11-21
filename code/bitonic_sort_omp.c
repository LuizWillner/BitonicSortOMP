#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include <limits.h>


#define UP 0
#define DOWN 1


int is_power_of_two(int num);
int get_size_extended(int size);
int* generate_random_array(int size, int mod_factor);
void print_sequence(int* sequence, int sequence_size, int first_n, int last_n);
void bitonic_sort_seq(int start, int length, int *list, int flag);
void bitonic_sort_par(int start, int length, int *list, int flag);
void swap(int *a, int *b);


int m;


int main(int argc, char *argv[]) 
{
    int i, j;
    int size, size_ext;
    int flag;
    int *list;
    int n_threads;
    double time_start, time_end, time_total;

    size = atoi(argv[2]);
    list = generate_random_array(size, size);
    size_ext = get_size_extended(size);


    time_start = omp_get_wtime();

    n_threads = atoi(argv[1]);
    omp_set_num_threads(n_threads);

    printf("Número de threads: %d\n", n_threads);
    printf("Vetor inicial:\t");
    print_sequence(list, size, 10, 10);

    m = size_ext / n_threads;

    for (i = 2; i <= m; i = 2 * i)
    {
#pragma omp parallel for shared(i, list) private(j, flag)
        for (j = 0; j < size_ext; j += i)
        {
            if ((j / i) % 2 == 0)
                flag = UP;
            else
                flag = DOWN;
            bitonic_sort_seq(j, i, list, flag);
        }
    }

    for (i = 2; i <= n_threads; i = 2 * i)
    {
        for (j = 0; j < n_threads; j += i)
        {
            if ((j / i) % 2 == 0)
                flag = UP;
            else
                flag = DOWN;
            bitonic_sort_par(j*m, i*m, list, flag);
        }
#pragma omp parallel for shared(j)
        for (j = 0; j < n_threads; j++)
        {
            if (j < i)
                flag = UP;
            else
                flag = DOWN;
            bitonic_sort_seq(j*m, m, list, flag);
        }
    }

    time_end = omp_get_wtime();
    time_total = time_end - time_start;

    printf("Vetor final:\t");
    print_sequence(list, size, 10, 10);
    printf("\n");
    printf("Começo: %f seg\n", time_start);
    printf("Fim: %f seg\n", time_end);
    printf("Tempo decorrido: %f seg\n", time_total);

    free(list);

    return 0;
}


int is_power_of_two(int num)
{
    return (num > 0) && ((num & (num - 1)) == 0);
}


int get_size_extended(int size)
{
    int size_extended;
    if (is_power_of_two(size)){
        size_extended = size;
    } else {
        double exponent = floor(log2(size)) + 1.0;
        size_extended = (int) pow(2, exponent);
    }
    return size_extended;
}


int* generate_random_array(int size, int mod_factor) 
{
    int *array;
    srand(time(NULL));
    
    if (is_power_of_two(size)) {
        array = (int *) malloc(size * sizeof(int));
        for (int i = 0; i < size; i++) {
            array[i] = rand() % mod_factor;
        }

    } else {
        double exponent = floor(log2(size)) + 1.0;
        int size_extended = (int) pow(2, exponent);
        array = (int *) malloc(size_extended * sizeof(int));
        for (int i = 0; i < size; i++) {
            array[i] = rand() % mod_factor;
        }
        for (int i = size; i < size_extended; i++) {
            array[i] = INT_MAX;
        }
    }

    return array;
}


void print_sequence(int* sequence, int sequence_size, int first_n, int last_n)
{
    printf("[");

    if (first_n != 0)
    {
        for (int i = 0; i < first_n - 1; i++)
        {
            printf("%d, ", sequence[i]);
        }
        printf("%d", sequence[first_n - 1]);
    }
    
    if (first_n != sequence_size && last_n != sequence_size)
    {
        printf(", ... , ");
    }

    if (last_n != 0)
    {
        for (int i = sequence_size - last_n; i < sequence_size - 1; i++)
        {
            printf("%d, ", sequence[i]);
        }
        printf("%d", sequence[sequence_size-1]);
    }
    
    printf("]\n");
}


void bitonic_sort_seq(int start, int length, int *list, int flag)
{
    int i;
    int split_length;

    if (length == 1)
        return;

    if (length % 2 !=0 )
    {
        printf("error\n");
        exit(0);
    }

    split_length = length / 2;

    for (i = start; i < start + split_length; i++)
    {
        if (flag == UP)
        {
            if (list[i] > list[i + split_length])
                swap(&list[i], &list[i + split_length]);
        }
        else
        {
            if (list[i] < list[i + split_length])
                swap(&list[i], &list[i + split_length]);
        }
    }

    bitonic_sort_seq(start, split_length, list, flag);
    bitonic_sort_seq(start + split_length, split_length, list, flag);
}


void bitonic_sort_par(int start, int length, int *list, int flag)
{
    int i;
    int split_length;

    if (length == 1)
        return;

    if (length % 2 !=0 )
    {
        printf("The length of a (sub)sequence is not divided by 2.\n");
        exit(0);
    }

    split_length = length / 2;


#pragma omp parallel for shared(list, flag, start, split_length) private(i)
    for (i = start; i < start + split_length; i++)
    {
        if (flag == UP)
        {
            if (list[i] > list[i + split_length])
                swap(&list[i], &list[i + split_length]);
        }
        else
        {
            if (list[i] < list[i + split_length])
                swap(&list[i], &list[i + split_length]);
        }
    }

    if (split_length > m)
    {
        // m is the size of sub part-> n/n_threads
        bitonic_sort_par(start, split_length, list, flag);
        bitonic_sort_par(start + split_length, split_length, list, flag);
    }

    return;
}


void swap(int *a, int *b)
{
    int t;
    t = *a;
    *a = *b;
    *b = t;
}
