#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include "utils.h"


struct SumArgs {
  int *array;
  int begin;
  int end;
};

int Sum(const struct SumArgs *args) {
  int sum = 0;
  for (int i = args->begin; i < args->end; i++) {
      sum += args->array[i];
  }
  return sum;
}

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {

  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  static struct option options[] = {{"threads_num", required_argument, 0, 't'},
                                    {"array_size", required_argument, 0, 'a'},
                                    {"seed", required_argument, 0, 's'},
                                    {0, 0, 0, 0}
                                  };

  int opt;
  while ((opt = getopt_long(argc, argv, "t:a:s:", options, NULL)) != -1) {
      switch (opt) {
          case 't':
              threads_num = atoi(optarg);
              break;
          case 'a':
              array_size = atoi(optarg);
              break;
          case 's':
              seed = atoi(optarg);
              break;
          default:
              printf("Usage: %s --threads_num num --array_size num --seed num\n", argv[0]);
              return 1;
      }
  }

  
  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  
  struct timespec start, finish;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int chunk_size = array_size / threads_num;
  for (uint32_t i = 0; i < threads_num; i++) {

    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  clock_gettime(CLOCK_MONOTONIC, &finish);
  double elapsed_time = (finish.tv_sec - start.tv_sec);
  elapsed_time += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

  free(array);
  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %.6f seconds\n", elapsed_time);
  return 0;
}
