#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s seed arraysize\n", argv[0]);
        return 1;
    }

    pid_t child_pid = fork();

    if (child_pid == -1) {
        printf("Fork failed!\n");
        return 1;
    }

    if (child_pid == 0) {
        execl("./sequential_min_max",
              "./sequential_min_max",
              argv[1],
              argv[2],
              NULL);

        printf("Exec failed!\n");
        return 1;
    }

    wait(NULL);

    return 0;
}
