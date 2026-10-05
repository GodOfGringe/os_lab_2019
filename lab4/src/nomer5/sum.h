#ifndef SUM_H
#define SUM_H

struct SumArgs {
    int *array;
    int begin;
    int end;
    int *result;
};

int Sum(const struct SumArgs *args);

#endif
