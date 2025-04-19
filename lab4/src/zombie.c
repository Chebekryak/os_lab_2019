#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Создаем зомби-процесс...\n");
    pid_t pid = fork();

    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс (PID: %d) запущен\n", getpid());
        printf("Дочерний процесс завершается\n");
        exit(0);
    } else if (pid > 0) {
        // Родительский процесс
        printf("Родительский процесс (PID: %d) создал дочерний %d\n", getpid(), pid);
        printf("Родительский процесс будет работать 30 секунд...\n");
        sleep(30); // Искусственная задержка
        
        // Теперь ждем завершения дочернего процесса
        wait(NULL);
        printf("Родительский процесс завершается\n");
    } else {
        perror("Ошибка fork");
        return 1;
    }

    return 0;
}