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
void bitonic_sort_seq(int start, int length, int *seq, int flag);
void bitonic_sort_par(int start, int length, int *seq, int flag);
void swap(int *a, int *b);


int m;


int main(int argc, char *argv[])  // Arg1: Número de threads; Arg2: Tamanho da sequência
{
    int i, j;
    int size, size_ext;
    int flag;
    int *seq;
    int numThreads;
    double startTime, finishTime, elapsedTime;

    size = atoi(argv[2]);
    seq = generate_random_array(size, size);
    size_ext = get_size_extended(size);

    // start
    startTime = omp_get_wtime();

    numThreads = atoi(argv[1]);
    omp_set_num_threads(numThreads);

    printf("Número de threads: %d\n", numThreads);
    printf("Vetor inicial:\t");
    print_sequence(seq, size, 10, 10);

    // the size of sub part
    m = size_ext / numThreads;

    // make the sequence bitonic - part 1
    for (i = 2; i <= m; i = 2 * i)
    {
#pragma omp parallel for shared(i, seq) private(j, flag)
        for (j = 0; j < size_ext; j += i)
        {
            if ((j / i) % 2 == 0)
                flag = UP;
            else
                flag = DOWN;
            bitonic_sort_seq(j, i, seq, flag);
        }
    }

    // make the sequence bitonic - part 2
    for (i = 2; i <= numThreads; i = 2 * i)
    {
        for (j = 0; j < numThreads; j += i)
        {
            if ((j / i) % 2 == 0)
                flag = UP;
            else
                flag = DOWN;
            bitonic_sort_par(j*m, i*m, seq, flag);
        }
#pragma omp parallel for shared(j)
        for (j = 0; j < numThreads; j++)
        {
            if (j < i)
                flag = UP;
            else
                flag = DOWN;
            bitonic_sort_seq(j*m, m, seq, flag);
        }
    }

    // bitonic sort
    //bitonic_sort_par(0, size_ext, seq, UP);
    //bitonic_sort_seq(0, size_ext, seq, UP);

    //end
    finishTime = omp_get_wtime();
    elapsedTime = finishTime - startTime;

    printf("Vetor final:\t");
    print_sequence(seq, size, 10, 10);
    printf("\n");
    printf("Começo: %f seg\n", startTime);
    printf("Fim: %f seg\n", finishTime);
    printf("Tempo decorrido: %f seg\n", elapsedTime);

    free(seq);

    return 0;
}


int is_power_of_two(int num)
{
    // Verifica se o número é positivo e se tem apenas um bit definido
    return (num > 0) && ((num & (num - 1)) == 0);
}


int get_size_extended(int size)
{
    int size_extended;
    if (is_power_of_two(size)){
        // printf("%d é potência de 2.\n", size);
        size_extended = size;
    } else {
        // printf("%d NÃO é potência de 2.\n", size);
        double exponent = floor(log2(size)) + 1.0;
        size_extended = (int) pow(2, exponent);
        // printf("log2(%d) = %f\n", size, log2(size));
        // printf("Próxima potência de 2 é %d (2^%f)\n", size_extended, exponent);
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


void bitonic_sort_seq(int start, int length, int *seq, int flag)
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

    // bitonic split
    for (i = start; i < start + split_length; i++)
    {
        if (flag == UP)
        {
            if (seq[i] > seq[i + split_length])
                swap(&seq[i], &seq[i + split_length]);
        }
        else
        {
            if (seq[i] < seq[i + split_length])
                swap(&seq[i], &seq[i + split_length]);
        }
    }

    bitonic_sort_seq(start, split_length, seq, flag);
    bitonic_sort_seq(start + split_length, split_length, seq, flag);
}


void bitonic_sort_par(int start, int length, int *seq, int flag)
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


// bitonic split
#pragma omp parallel for shared(seq, flag, start, split_length) private(i)
    for (i = start; i < start + split_length; i++)
    {
        if (flag == UP)
        {
            if (seq[i] > seq[i + split_length])
                swap(&seq[i], &seq[i + split_length]);
        }
        else
        {
            if (seq[i] < seq[i + split_length])
                swap(&seq[i], &seq[i + split_length]);
        }
    }

    if (split_length > m)
    {
        // m is the size of sub part-> n/numThreads
        bitonic_sort_par(start, split_length, seq, flag);
        bitonic_sort_par(start + split_length, split_length, seq, flag);
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
