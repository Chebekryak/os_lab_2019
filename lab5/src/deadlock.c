#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_function(void* arg) {
    pthread_mutex_lock(&mutex1);
    printf("Поток 1 захватил mutex1\n");
    sleep(1);  // Искусственная задержка для создания условий deadlock
    
    printf("Поток 1 пытается захватить mutex2...\n");
    pthread_mutex_lock(&mutex2);  // Здесь поток заблокируется
    
    printf("Поток 1 захватил mutex2\n");
    
    // Критическая секция
    printf("Поток 1 выполняет работу\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void* thread2_function(void* arg) {
    pthread_mutex_lock(&mutex2);
    printf("Поток 2 захватил mutex2\n");
    sleep(1);  // Искусственная задержка для создания условий deadlock
    
    printf("Поток 2 пытается захватить mutex1...\n");
    pthread_mutex_lock(&mutex1);  // Здесь поток заблокируется
    
    printf("Поток 2 захватил mutex1\n");
    
    // Критическая секция
    printf("Поток 2 выполняет работу\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    
    printf("Создаем потоки...\n");
    pthread_create(&thread1, NULL, thread1_function, NULL);
    pthread_create(&thread2, NULL, thread2_function, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Программа завершена (это сообщение никогда не будет выведено)\n");
    return 0;
}