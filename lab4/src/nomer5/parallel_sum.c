#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/time.h>

#include "sum.h"
#include "../../../lab3/src/utils.h"


void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;

    *(sum_args->result) = Sum(sum_args);

    return NULL;
}


int main(int argc, char **argv) {

    uint32_t threads_num = 0;
    uint32_t array_size = 0;
    uint32_t seed = 0;


    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {0, 0, 0, 0}
    };


    int option_index = 0;

    while (1) {

        int c = getopt_long(
            argc,
            argv,
            "",
            options,
            &option_index
        );

        if (c == -1)
            break;


        switch (option_index) {

            case 0:
                threads_num = atoi(optarg);
                break;

            case 1:
                seed = atoi(optarg);
                break;

            case 2:
                array_size = atoi(optarg);
                break;
        }
    }


    if (threads_num == 0 ||
        array_size == 0) {

        printf(
            "Usage: %s --threads_num num "
            "--seed num --array_size num\n",
            argv[0]
        );

        return 1;
    }


    int *array = malloc(sizeof(int) * array_size);

    if (array == NULL) {
        printf("Error: malloc failed!\n");
        return 1;
    }


    GenerateArray(
        array,
        array_size,
        seed
    );


    pthread_t threads[threads_num];


    struct SumArgs args[threads_num];


    int sums[threads_num];


    struct timeval start_time;
    struct timeval finish_time;

    gettimeofday(&start_time, NULL);


    for (uint32_t i = 0; i < threads_num; i++) {

        args[i].array = array;

        args[i].begin =
            i * array_size / threads_num;

        args[i].end =
            (i + 1) * array_size / threads_num;

        args[i].result = &sums[i];


        if (pthread_create(
                &threads[i],
                NULL,
                ThreadSum,
                &args[i]
            ) != 0) {

            printf(
                "Error: pthread_create failed!\n"
            );

            free(array);
            return 1;
        }
    }


    for (uint32_t i = 0; i < threads_num; i++) {

        pthread_join(
            threads[i],
            NULL
        );
    }


    int total_sum = 0;

    for (uint32_t i = 0; i < threads_num; i++) {
        total_sum += sums[i];
    }


    gettimeofday(&finish_time, NULL);


    double elapsed_time =
        (finish_time.tv_sec - start_time.tv_sec)
        * 1000.0;

    elapsed_time +=
        (finish_time.tv_usec - start_time.tv_usec)
        / 1000.0;


    printf("Total: %d\n", total_sum);
    printf("Elapsed time: %f ms\n", elapsed_time);


    free(array);

    return 0;
}
