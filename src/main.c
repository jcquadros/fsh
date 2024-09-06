#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "forward_list.h"
#include "process.h"
#include "fsh.h"
#include "session.h"
#include "utils.h"

#define MAX_INPUT_SIZE 2000

FSH *fsh;

/* Tratador do sinal SIGINT */
void sigint_handler(int sig);

/* Tratador do sinal SIGSTP */
void sigtstp_handler(int sig);

/* Configura os tratadores de sinal */
void setup_signal_handlers();

/* Executa uma linha de comando */
void launch_session(char *input);

int main(){
    char input[MAX_INPUT_SIZE]; // Buffer para a entrada do usuário
    
    fsh = fsh_create();
    setup_signal_handlers();
    while(1){
        fsh_acquire_terminal();
        printf("fsh> ");
        fflush(stdout); // Força a impressão do texto no terminal

        // Lê a entrada do usuário
        if(fgets(input, MAX_INPUT_SIZE, stdin) == NULL){
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        // Verifica se foi escrito algo
        if(strlen(input) == 1){
            continue;
        }        
       
        input[strcspn(input, "\n")] = 0; // Remove o \n do final da string

        // Verifica se o comando é para finalizar a shell
        if(strcmp(input, "die") == 0){
            fsh_die(fsh);
        }

        // Verifica se o comando é para esperar todos os processos
        if(strcmp(input, "waitall") == 0){
            fsh_waitall();
            continue;
        }

        launch_session(input);
    }
    return EXIT_SUCCESS;
}

void launch_session(char *input){
    // remove_signal_handlers(); // Remove os tratadores de sinal
    Session *s = session_create(input); 
    // setup_signal_handlers(); // Configura os tratadores de sinal

    Process *fg = s->foreground;
    fsh_put_process_in_foreground(fg);   // Coloca o processo em foreground
    fsh_push_session(fsh, s);
    process_wait(fg); // Espera o processo em foreground terminar
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
        char response;
        scanf("%c\n", &response);
        if (response == 's' || response == 'S') {
            fsh_die(fsh);
        }
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
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        if (WIFSIGNALED(status)) {
            printf("Processo %d terminou devido ao sinal %d.\n", pid, WTERMSIG(status));
            Session * s = fsh_session_find(fsh, pid);
            session_notify(s, WTERMSIG(status));
    
        } else if (WIFSTOPPED(status)) {
            printf("Processo %d foi suspenso pelo sinal %d.\n", pid, WSTOPSIG(status));
            Session * s = fsh_session_find(fsh, pid);
            session_notify(s, WSTOPSIG(status));
        }
    }

    // Verifica se houve um erro no waitpid
    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
    }
}


// Configura os tratadores de sinal
void setup_signal_handlers() {
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
}


// Restaura mascaras de sinais
void remove_signal_handlers() {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
}



