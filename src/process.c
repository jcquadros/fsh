#include "process.h"
#include "utils.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

Process* create_process(char *args, pid_t pgid, int is_foreground) {
    char *args_list[MAX_ARGS + 1]; // +1 para receber o NULL necessario na lista de argumentos
    int n_args = 0;
    process_input(args, args_list, &n_args, " ", MAX_ARGS);
    args_list[n_args] = NULL; // Adicionando NULL no final da lista de argumentos

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Processo pai
    if (pid > 0) {
        if (pgid == 0) {
            pgid = pid;
        }
        setpgid(pid, pgid);
        Process * p = (Process*)malloc(sizeof(Process));

        
        p->pid = pid;
        p->pgid = pgid;
        p->is_foreground = is_foreground;
        printf("Processo %d criado e grupo %d\n", pid, pgid);
        return p;
    }

    // configura o tratamento de sinais
    // signal(SIGINT, SIG_IGN); 
    // signal(SIGTSTP, SIG_DFL);
    // signal(SIGCHLD, SIG_DFL);

    // Processo filho

    // Se for um processo em background cria um filho secundário
    if (!is_foreground) {
        
    }

    execvp(args_list[0], args_list);
    perror("execvp");
    exit(EXIT_FAILURE);
}

void process_wait(Process *p) {
    int status;
    waitpid(p->pid, &status, 0);
}

void process_destroy(Process* p){
    free(p);
}