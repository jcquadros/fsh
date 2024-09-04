#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include "forward_list.h"
#include "process.h"
#include "fsh.h"

#define MAX_ARGS 2
#define MAX_INPUT_SIZE 2000
#define MAX_PROCESS 5

FSH *fsh;
/* Tratador do sinal SIGINT */
void sigint_handler(int sig);

/* Tratador do sinal SIGSTP */
void sigtstp_handler(int sig);

/* Configura os tratadores de sinal */
void setup_signal_handlers();

/* Separa a entrada do usuário em argumentos de acordo com o delimitador selecionado */
void process_input(char *input, char *args[], int *n_args, char* delimiter, int max_args);

/* Cria um processo */
Process *launch_process(char *input, int is_foreground, pid_t pgid);

/* Cria uma sessão */
Session *launch_session(char *input);

int main(){
    char input[MAX_INPUT_SIZE]; // Buffer para a entrada do usuário
    setup_signal_handlers();
    
    fsh = fsh_create();
    while(1){
        fsh_acquire_terminal();
        printf("fsh> ");
        fflush(stdout);

        // Lê a entrada do usuário
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = 0; // Remove o \n do final da string

        Session *s = launch_session(input);
        process_wait(s->foreground);

    }
    return EXIT_SUCCESS;
}

Session *launch_session(char *input){
    char *commands[MAX_PROCESS];
    int n_commands = 0;
    pid_t pgid = 0;
    process_input(input, commands, &n_commands, "#", MAX_PROCESS);

    Session *s = create_session();
    for(int i = 1; i < n_commands; i++){
        Process *p = launch_process(commands[i], 0, pgid);
        pgid = p->pgid;
        insert_process_in_session(p, s);
        fsh_push_process(fsh, p);
    }

    Process *p = launch_process(commands[0], 1, 0);
    fsh_put_process_in_foreground(p);
    insert_process_in_session(p, s);
    fsh_push_process(fsh, p);
    fsh_push_session(fsh, s);
    return s;
}


Process *launch_process(char *input, int is_foreground, pid_t pgid){
    char *args[MAX_ARGS + 1]; // +1 para receber o NULL necessario na lista de argumentos
    int n_args = 0;
    process_input(input, args, &n_args, " ", MAX_ARGS);
    args[n_args] = NULL; // Adicionando NULL no final da lista de argumentos

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Processo pai
        if (pgid == 0) {
            pgid = pid;
        }
        setpgid(pid, pgid);
        Process * p = create_process(pid, pgid, is_foreground);
        return p;
    } 

    // Processo filho
    execvp(args[0], args);
    perror("execvp");
    exit(EXIT_FAILURE);
}


void sigint_handler(int sig) {
    printf("\nRecebido SIGINT (Ctrl+C). ");
    fflush(stdout);
    sigset_t all_signals;
    sigset_t old_mask;
    sigfillset(&all_signals); // Preenche o conjunto de sinais com todos os sinais

    // Bloqueia todos os sinais enquanto o manipulador é executado
    if (sigprocmask(SIG_BLOCK, &all_signals, &old_mask) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    if (fsh_has_alive_process(fsh)) { 
        printf("\nProcessos em execução. Deseja finalizar a shell? (s/n) ");
        char response = getchar();
        if (response == 's' || response == 'S') {
            fsh_die(fsh);
        }

        // Limpa o buffer de entrada
        while (getchar() != '\n');

    } else {
        fsh_die(fsh);
    }

    // Restaura a máscara de sinais
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

}

/* Tratador do SIGTSTP - suspende todos os filhos do shell - não conta os netos*/
void sigtstp_handler(int sig) {
    printf("\nRecebido SIGTSTP (Ctrl+Z). O programa não será suspenso. Mas os filhos de shell sim!\n");
    fsh_notify(fsh, SIGTSTP);
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // Loop para capturar todos os processos filhos que mudaram de estado
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG | WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            printf("Processo %d terminou normalmente com status %d.\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Processo %d terminou devido ao sinal %d.\n", pid, WTERMSIG(status));
            Session * s = session_find(fsh, pid);
            session_notify(s, WTERMSIG(status));
            session_mark_as_done(s);

        } else if (WIFSTOPPED(status)) {
            printf("Processo %d foi suspenso pelo sinal %d.\n", pid, WSTOPSIG(status));
            Session * s = session_find(fsh, pid);
            session_notify(s, WSTOPSIG(status));
            session_mark_as_stopped(s);


        } else if (WIFCONTINUED(status)) {
            printf("Processo %d foi continuado.\n", pid);
        }
    }

    // Verifica se houve um erro no waitpid
    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
    }
}


void signal_handler(int sig) {
    switch (sig) {
        case SIGINT:
            sigint_handler(sig);
            break;
        case SIGTSTP:
            sigtstp_handler(sig);
            break;

        case SIGCHLD:
            sigchld_handler(sig);
            break;
        default:
            break;
    }
}

// Configura os tratadores de sinal
void setup_signal_handlers() {
    struct sigaction sa;

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTSTP, &sa, NULL) == -1 || sigaction(SIGCHLD, &sa, NULL) == -1 ) {
        exit(EXIT_FAILURE);
    }
    // Investigar qual desses sinais permite que a shell nao se bloqueie
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}

void process_input(char *input, char *args[], int *n_args, char* delimiter, int max_args){
    *n_args = 0;
    char *token = strtok(input, delimiter);
    while(token != NULL && *n_args < max_args){
        args[*n_args] = token;
        (*n_args)++;
        token = strtok(NULL, delimiter);
    }
}


