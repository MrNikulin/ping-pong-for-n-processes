#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

int n, n1, n2;
pid_t* pid;
int fd[2];

void free_all(void);
void signal_to_kill_son(int s);
void signal_handler(int s);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Wrong format! Expected start, finish numbers and number of processes\n");
		exit(1);
    }
    n1 = atoi(argv[1]);
    if (!n1 && (argv[1][0] != '0')) {
        fprintf(stderr, "Wrong format! Expected number, got %s\n", argv[1]);
		exit(1);
    }
    n2 = atoi(argv[2]);
    if (!n2 && (argv[2][0] != '0')) {
        fprintf(stderr, "Wrong format! Expected number, got %s\n", argv[2]);
		exit(1);
    }
    if (n2 < n1) {
        fprintf(stderr, "Wrong format! Expected start number <= finish number\n");
		exit(1);
    }
    n = atoi(argv[3]);
    if (n < 2) {
        fprintf(stderr, "Wrong format! Number of processes must be >= 2\n");
		exit(1);
    }
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
    }
    signal(SIGUSR1, signal_handler);
    signal(SIGTERM, signal_to_kill_son);
    pid = (pid_t*) malloc(n * sizeof(pid_t));
    for (int i = 0; i < n; ++i) {
        pid[i] = 0;
    }
    pid_t tmp = pid[0] = getpid();
    char c = 'c';
    for (int i = 1; i < n; ++i) {
        if (tmp && ((tmp = fork()) == -1)) {
            perror("fork");
            free_all();
            exit(1);
        }
        if (tmp) {//father
            pid[i] = tmp;
            read(fd[0], &c, 1);
        }
        else {//son
            pid[i] = getpid();
            write(fd[1], &c, 1);
            while (1);
        }
    }
    printf("pid = %d, number = %d\n", pid[0], n1);
    write(fd[1], &n1, sizeof(int));
    kill(pid[n - 1], SIGUSR1);
    while (wait(NULL) != -1);
    free_all();
    return 0;
}

void free_all(void) {
    free(pid);
    close(fd[0]);
    close(fd[1]);
}

void signal_to_kill_son(int s) {
    free_all();
    exit(0);
}

void signal_handler(int s) {
    int my_pid = getpid();
    int k = -1;
    for (int i = n - 1; i >= 0; --i) {
        if (pid[i] == my_pid) {
            k = i;
            break;
        }
    }
    read(fd[0], &n1, sizeof(int));
    if (n1 < n2) {
        printf("pid = %d, number = %d\n", my_pid, ++n1);
        write(fd[1], &n1, sizeof(int));
        kill(pid[(k + n - 1) % n], SIGUSR1);
    }
    else {
        if (k) {
            write(fd[1], &n1, sizeof(int));
            kill(pid[0], SIGUSR1);
        }
        else {//father kills his sons
            for (int i = 1; i < n; ++i) {
                kill(pid[i], SIGTERM);
            }
        }
    }
}