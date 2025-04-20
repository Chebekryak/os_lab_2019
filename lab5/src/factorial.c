#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

// Глобальные переменные
unsigned long long result = 1;  // Результат (k! mod mod)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int mod = 1;

// Структура для передачи аргументов в поток
struct ThreadArgs {
    int start;
    int end;
};

// Функция потока
void* compute_factorial_part(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    unsigned long long partial_result = 1;

    // Вычисляем часть факториала
    for (int i = args->start; i <= args->end; i++) {
        partial_result = (partial_result * i) % mod;
    }

    // Защищаем доступ к общему результату с помощью мьютекса
    pthread_mutex_lock(&mutex);
    result = (result * partial_result) % mod;
    pthread_mutex_unlock(&mutex);

    free(args);
    return NULL;
}

int main(int argc, char* argv[]) {
    int k = 0;
    int pnum = 1;
    mod = 1;  // По умолчанию mod=1

    // Обработка аргументов командной строки
    static struct option long_options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s -k <number> --pnum=<threads> --mod=<modulus>\n", argv[0]);
                return 1;
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "All parameters must be positive integers\n");
        return 1;
    }

    // Создаем потоки
    pthread_t threads[pnum];
    int elements_per_thread = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;

    for (int i = 0; i < pnum; i++) {
        // Вычисляем диапазон для текущего потока
        int current_end = current_start + elements_per_thread - 1;
        if (i < remainder) {
            current_end++;
        }

        // Создаем аргументы для потока
        struct ThreadArgs* args = malloc(sizeof(struct ThreadArgs));
        args->start = current_start;
        args->end = current_end;

        // Создаем поток
        if (pthread_create(&threads[i], NULL, compute_factorial_part, args) != 0) {
            perror("pthread_create");
            return 1;
        }

        current_start = current_end + 1;
    }

    // Ожидаем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }

    // Выводим результат
    printf("%d! mod %d = %llu\n", k, mod, result);

    // Уничтожаем мьютекс
    pthread_mutex_destroy(&mutex);

    return 0;
}