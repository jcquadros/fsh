#include "process.h"
#include "utils.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <errno.h>


// Cria o processo, aloca a memória, configura e realiza o execvp
Process* create_process(char *args, pid_t pgid, int is_foreground) {
    char *args_list[MAX_ARGS + 1]; // +1 para receber o NULL necessario na lista de argumentos
    int n_args = 0;
    process_input(args, args_list, &n_args, " ", MAX_ARGS);
    args_list[n_args] = NULL; // Adicionando NULL no final da lista de argumentos

    // Uso do pipe, não sei se o uso era permitido ;-;
    // O pipe foi utilizado para obter o pid do processo secundário Px'
    // Dessa forma é possivel realizar o controle dele memo depois da morte de Px ser finalizado.
    int fd_pipe[2];
    if (pipe(fd_pipe) == -1) {exit(printf("Erro: Criação do Pipe"));}

    pid_t pid_principal = fork();
    pid_t pid_secundario = - 888;

    if (pid_principal < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Processo pai define o pgid
    else if (pid_principal > 0) {
        if (pgid == 0) {
            pgid = pid_principal;
        } // Para o processo foreground e para o primeiro processo em background da sessão.

        // Fecha a escrita no pipe.
        close(fd_pipe[1]); 

        setpgid(pid_principal, pgid);
        Process * p = (Process*)malloc(sizeof(Process));

        p->pid_principal = pid_principal;
        p->pid_secundario = pid_secundario;
        p->pgid = pgid;
        if(!is_foreground) {
            if (read(fd_pipe[0], &pid_secundario, sizeof(pid_secundario)) == -1) {exit(printf("Erro: Falha na leitura do pipe.\n"));}
            // Fecha a leitura no pipe.
            close(fd_pipe[0]);
        }
        p->is_foreground = is_foreground;
        return p;
    }

    // Processo filho cria processo secundário se for um processo em background
    else {
        close(fd_pipe[0]);
        if (!is_foreground) {
            pid_secundario= fork();

            if (pid_secundario < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            // Processo Px' - Neto
            else if (pid_secundario == 0) {
                close(fd_pipe[0]);
                close(fd_pipe[1]);
            }

            // Processo Px - Filho
            else if (pid_secundario > 0) {
                pgid = getpgid(getpid());
                setpgid(pid_secundario, pgid);
                if (write(fd_pipe[1], &pid_secundario, sizeof(pid_secundario)) == -1) {exit(printf("Erro: Falha na escrita do pipe.\n"));}
                close(fd_pipe[1]);
            }
        }
    }

    // Processo filho e secundário(se for background)
    // configura o tratamento de sinais
    signal(SIGINT, SIG_IGN); 
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);

    execvp(args_list[0], args_list);
    perror("execvp");
    return NULL; // Rertorna NULL quando execvp falha, dessa forma é possível desalocar memória duplicada do pai.
}

// Libera a memória do processo
void process_destroy(Process* p){
    free(p);
}