#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

long long result = 1;
pthread_mutex_t mutex;

typedef struct {
    int start;
    int end;
} Data;

void *factorial(void *arg)
{
    Data *data = (Data *)arg;

    long long local = 1;

    for (int i = data->start; i <= data->end; i++)
        local *= i;

    pthread_mutex_lock(&mutex);
    result *= local;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char *argv[])
{
    int k = 0;
    int pnum = 1;
    int mod = 1;

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-k") == 0)
            k = atoi(argv[++i]);

        else if (strncmp(argv[i], "--pnum=", 7) == 0)
            pnum = atoi(argv[i] + 7);

        else if (strncmp(argv[i], "--mod=", 6) == 0)
            mod = atoi(argv[i] + 6);
    }

    pthread_t threads[pnum];
    Data data[pnum];

    pthread_mutex_init(&mutex, NULL);

    int part = k / pnum;

    for (int i = 0; i < pnum; i++) {

        data[i].start = i * part + 1;

        if (i == pnum - 1)
            data[i].end = k;
        else
            data[i].end = (i + 1) * part;

        pthread_create(
            &threads[i],
            NULL,
            factorial,
            &data[i]
        );
    }

    for (int i = 0; i < pnum; i++)
        pthread_join(threads[i], NULL);

    printf("%d! mod %d = %lld\n", k, mod, result % mod);

    pthread_mutex_destroy(&mutex);

    return 0;
}
