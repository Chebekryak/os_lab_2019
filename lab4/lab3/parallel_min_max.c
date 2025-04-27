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

int timeout_reached = 0;
int killed_children = 0;

void handle_alarm(int sig) {
    timeout_reached = 1;
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    int timeout = 0;
    bool with_files = false;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {"timeout", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

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
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case 't':
                timeout = atoi(optarg);
                if (timeout <= 0) {
                    printf("timeout must be a positive number\n");
                    return 1;
                }
                break;
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
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n",
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
            if (child_pid == 0) {
                // child process
                int start = i * array_size / pnum;
                int end = (i + 1) * array_size / pnum;
                if (i == pnum - 1) {
                    end = array_size;
                }

                struct MinMax min_max = GetMinMax(array, start, end);

                if (with_files) {
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
                    close(pipefd[0]);
                    write(pipefd[1], &min_max.min, sizeof(int));
                    write(pipefd[1], &min_max.max, sizeof(int));
                    close(pipefd[1]);
                }
                return 0;
            } else {
                child_pids[i] = child_pid;
                active_child_processes++;
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    if (timeout > 0) {
        signal(SIGALRM, handle_alarm);
        alarm(timeout);
    }

    while (active_child_processes > 0) {
        if (timeout_reached) {
            printf("Timeout reached! Killing child processes...\n");
            for (int i = 0; i < pnum; i++) {
                if (child_pids[i] > 0) {
                    if (kill(child_pids[i], SIGKILL) == 0) {
                        killed_children++;
                        printf("Killed child process %d\n", child_pids[i]);
                    } else {
                        perror("kill");
                    }
                }
            }
            break;
        }

        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0) {
            active_child_processes--;
            printf("Child process %d finished normally\n", pid);
        } else if (pid == -1) {
            perror("waitpid");
            break;
        }
    }

    if (killed_children > 0) {
        printf("Total killed child processes: %d\n", killed_children);
    }

    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX;
        int max = INT_MIN;

        if (with_files) {
            char filename[20];
            sprintf(filename, "minmax_%d.txt", i);
            FILE *fp = fopen(filename, "r");
            if (fp != NULL) {
                fscanf(fp, "%d %d", &min, &max);
                fclose(fp);
                remove(filename);
            }
        } else {
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
    free(child_pids);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    fflush(NULL);
    return 0;
}