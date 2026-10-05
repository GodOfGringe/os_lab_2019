#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        printf("Fork failed!\n");
        return 1;
    }

    if (pid == 0) {

        printf("Hello world child, PID = %d\n", getpid());

        return 0;
    }

    printf("Hello worldparent, Child PID = %d\n", pid);

    //wait(NULL);
    sleep(10); //

    return 0;
}