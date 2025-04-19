#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout = 0;
  
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 't'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "ft:", options, &option_index);
    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case 't':
        timeout = atoi(optarg);
        if (timeout < 0) {
            printf("timeout must be a non-negative number\n");
            return 1;
        }
      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  pid_t *child_pids = malloc(sizeof(pid_t) * pnum); 

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipefd[2];
  if (!with_files) {
    if (pipe(pipefd) == -1) {
      perror("pipe");
      return 1;
    }
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      child_pids[i] = child_pid;
      if (child_pid == 0) {
         // child process
         int start = i * array_size / pnum;
         int end = (i + 1) * array_size / pnum;
         if (i == pnum - 1) {
           end = array_size;
         }
 
         struct MinMax min_max = GetMinMax(array, start, end);
 

         if (with_files) {
          // use files
          char filename[20];
          sprintf(filename, "minmax_%d.txt", i);
          FILE *fp = fopen(filename, "w");
          if (fp == NULL) {
            perror("fopen");
            return 1;
          }
          fprintf(fp, "%d %d", min_max.min, min_max.max);
          fclose(fp);
        } else {
          // use pipe
          close(pipefd[0]); // close reading end in child
          write(pipefd[1], &min_max.min, sizeof(int));
          write(pipefd[1], &min_max.max, sizeof(int));
          close(pipefd[1]);
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }


    if (timeout > 0) {
      sleep(timeout);

      int status;
      pid_t result;
      while ((result = waitpid(-1, &status, WNOHANG))) {
          if (result == -1) {
              break;
          } else if (result > 0) {
              active_child_processes--;
          }
      }

      if (active_child_processes > 0) {
          printf("Timeout reached, killing child processes...\n");
          for (int i = 0; i < pnum; i++) {
              if (kill(child_pids[i], 0) == 0) {
                  kill(child_pids[i], SIGKILL); 
              }
          }
      }
  }

  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // read from files
      char filename[20];
      sprintf(filename, "minmax_%d.txt", i);
      FILE *fp = fopen(filename, "r");
      if (fp == NULL) {
        perror("fopen");
        return 1;
      }
      fscanf(fp, "%d %d", &min, &max);
      fclose(fp);
      remove(filename);
    } else {
      // read from pipes
      close(pipefd[1]); 
      read(pipefd[0], &min, sizeof(int));
      read(pipefd[0], &max, sizeof(int));
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  if (!with_files) {
    close(pipefd[0]);
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
