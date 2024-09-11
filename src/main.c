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

/* Tratador do sinal SIGCHLD */
void sigchld_handler(int sig);

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

        char *retorno_fgets;
        do {
            printf("fsh> ");
            fflush(stdout); // Força a impressão do texto no terminal
            retorno_fgets = fgets(input, MAX_INPUT_SIZE, stdin);
        } while (retorno_fgets == NULL && errno == EINTR);

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
    Session *s = session_create(input); 
    if (s == NULL) { // Desalocando memória no erro do execvp e finalizando o filho problemático.
        fsh_deallocate(fsh);
        exit(EXIT_FAILURE);
    }
    Process *fg = s->foreground;
    fsh_put_process_in_foreground(fg);   // Coloca o processo em foreground
    fsh_push_session(fsh, s);
    // process_wait(fg); // Espera o processo em foreground terminar
    fsh_wait_foreground(s);

}


void sigint_handler(int sig) {
    printf("\nRecebido SIGINT (Ctrl+C).\n");
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
        scanf("%c%*c", &response);
        if (response == 's' || response == 'S') {
            fsh_die(fsh);
        }
    } 
    else {
        fsh_die(fsh);
    }

    // Restaura a máscara de sinais
    if (sigprocmask(SIG_SETMASK, &old_mask, NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    };
}

/* Tratador do SIGTSTP - suspende todos os filhos do shell - não conta os netos*/
void sigtstp_handler(int sig) {
    printf("\nRecebido SIGTSTP (Ctrl+Z). O programa não será suspenso. Mas os filhos de shell sim!\n");
    fsh_notify(fsh, SIGTSTP);
}

/* Tratador do SIGCHLD */
void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // Loop para capturar todos os processos filhos que mudaram de estado
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG | WCONTINUED)) > 0) {
        if (WIFSIGNALED(status)) {
            printf("Processo %d terminou devido ao sinal %d.\n", pid, WTERMSIG(status));
            Session * s = fsh_session_find(fsh, pid);
            session_notify(s, WTERMSIG(status), 1);
        }
        else if (WIFSTOPPED(status)) {
            printf("Processo %d foi suspenso pelo sinal %d.\n", pid, WSTOPSIG(status));
            Session * s = fsh_session_find(fsh, pid);
            session_notify(s, WSTOPSIG(status), 1);
        } 
        else if (WIFCONTINUED(status)) {
            printf("Processo %d recebeu sina para continuar.\n", pid);
            fsh_notify(fsh, SIGCONT);
        }
    }

    // Verifica se houve um erro no waitpid
    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
    }
}


// Configura os tratadores de sinal
void setup_signal_handlers() {
    struct sigaction sa;
    
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction_SIGINT");
        exit(EXIT_FAILURE);
    }
    
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction_SIGTSTP");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction_SIGCHLD");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction_SIGQUIT");
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGTTIN, &sa, NULL) == -1) {
        perror("sigaction_SIGTTIN");
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGTTOU, &sa, NULL) == -1) {
        perror("sigaction_SIGTTOU");
        exit(EXIT_FAILURE);
    }
    
    // signal(SIGINT, sigint_handler);
    // signal(SIGTSTP, sigtstp_handler);
    // signal(SIGCHLD, sigchld_handler);
    // signal(SIGQUIT, SIG_IGN);
    // signal(SIGTTIN, SIG_IGN);
    // signal(SIGTTOU, SIG_IGN);
}


// Restaura mascaras de sinais
void remove_signal_handlers() {
    struct sigaction sa;

    sa.sa_handler = SIG_DFL; // Define o manipulador para o comportamento padrão
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction_SIGINT");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction_SIGTSTP");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction_SIGCHLD");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction_SIGQUIT");
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGTTIN, &sa, NULL) == -1) {
        perror("sigaction_SIGTTIN");
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGTTOU, &sa, NULL) == -1) {
        perror("sigaction_SIGTTOU");
        exit(EXIT_FAILURE);
    }

    // signal(SIGINT, SIG_DFL);
    // signal(SIGTSTP, SIG_DFL);
    // signal(SIGCHLD, SIG_DFL);
    // signal(SIGQUIT, SIG_DFL);
    // signal(SIGTTIN, SIG_DFL);
    // signal(SIGTTOU, SIG_DFL);
}



