#include "../dgemm/dgemmPT.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int thread_count;
int size, local_size;
double *a, *b, *c;


void random_init() {

    a = (double *)malloc(sizeof(double) * size * size);
    b = (double *)malloc(sizeof(double) * size * size);
    c = (double *)malloc(sizeof(double) * size * size);

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            *(a + i * size + j) = i + j + 1;
        }
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            *(b + i * size + j) = i + j + 1;
        }
    }
}

void *dgemmPT(void *rank) {
    long my_rank = (long)rank;
    int i, j, k;
    int my_first_row = my_rank * local_size;
    int my_last_row = (my_rank + 1) * local_size - 1;

    for (i = my_first_row; i <= my_last_row; i++) {
        for (j = 0; j < size; j++) {
            for (k = 0; k < size; k++) {
                *(c + i * size + j) +=
                    *(a + i * size + k) * *(b + k * size + j);
            }
        }
    }
}

void matrix_print(double *matrix) {
    printf("\nMatrix:\n");
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            printf("%.2lf ", *(matrix + i * size + j));
        }
        printf("\n");
    }
    printf("\n");
}

double wtime() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1E-6;
}

int getSize(int argc, char *argv[]) {
    int size;
    if (argc == 3 || argc == 5) {
        if (strcmp(argv[1], "--size") == 0) {
            int num_count = 0;
            for (int i = 0; i < strlen(argv[2]); i++) {
                if (argv[2][i] >= '0' && argv[2][i] <= '9') {
                    num_count++;
                }
            }
            if (num_count == strlen(argv[2])) {
                size = atoi(argv[2]);
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        return -1;
    }
    return size;
}

int getCT(int argc, char *argv[]) {
    int CT;
    if (argc == 5) {
        if (strcmp(argv[3], "--count-threads") == 0) {
            int num_count = 0;
            for (int i = 0; i < strlen(argv[4]); i++) {
                if (argv[4][i] >= '0' && argv[4][i] <= '9') {
                    num_count++;
                }
            }
            if (num_count == strlen(argv[4])) {
                CT = atoi(argv[4]);
            } else {
                return -1;
            }
        } else {
            return -1;
        }
    } else if (argc == 3) {
        return 1;
    } else {
        return -1;
    }
    return CT;
}

int main(int argc, char *argv[]) {
    long thread;
    double t_init, t_dgemm;
    size = getSize(argc, argv);
    if (size == -1) {
        return -1;
    }

    thread_count = getCT(argc, argv);
    if (thread_count == -1) {
        return -1;
    }

    pthread_t *thread_handles;

    local_size = size / thread_count;

    thread_handles = calloc(thread_count, sizeof(pthread_t));

    t_init = wtime();
    random_init();
    t_init = wtime() - t_init;

    t_dgemm = wtime();
    for (thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL, dgemmPT, (void *)thread);

    for (thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);
    t_dgemm = wtime() - t_dgemm;

    free(thread_handles);

    FILE *File;
    File = fopen("./results/PT.dat", "a");
    fseek(File, 0, SEEK_END);
    long pos = ftell(File);
    if (pos == 0) {
        fprintf(File, "%s%s\t%s\t%s\t%s\n", "#", "size", "CT", "t_init",
                "t_dgemm");
        fprintf(File, "%d\t%d\t%lf\t%lf\n", size, thread_count, t_init,
                t_dgemm);
    } else {
        fprintf(File, "%d\t%d\t%lf\t%lf\n", size, thread_count, t_init,
                t_dgemm);
    }
    fclose(File);

    return 0;
}