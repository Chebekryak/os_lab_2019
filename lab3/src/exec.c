#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        // Дочерний процесс
        char *args[] = {
            "./sequential_min_max", "12345", "1000",
            NULL
        };
        
        execvp(args[0], args);
        perror("execvp failed");
        return 1;
    }
    else {
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Child exited with status: %d\n", WEXITSTATUS(status));
        }
        else {
            printf("Child terminated abnormally\n");
        }
    }
    
    return 0;
}