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

#define MAX_ARGS 2
#define MAX_INPUT_SIZE 2000
#define MAX_PROCESS 5

/*
Por enquanto temos variaveis globais e varias funcoes, futuramente vamos organizar em varios arquivos
*/

pid_t shell_pgid;
int shell_terminal;
int shell_running = 1;

ForwardList* process_list = NULL; // Lista de processos do shell (nao inclui processos secundarios de cada processo em background - os tais 'Pxs')
ForwardList* session_list = NULL; // Lista de grupos de processos - Basicamente um grupo de processos é uma linha de comando shell

/* Tratador do sinal SIGINT */
void sigint_handler(int sig);

/* Tratador do sinal SIGSTP */
void sigtstp_handler(int sig);

/* Configura os tratadores de sinal */
void setup_signal_handlers();

/* Espera por todos os filhos Zumbis */
void waitall();

/* Mata todos os filhos */
void die();

/* Separa a entrada do usuário em argumentos de acordo com o delimitador selecionado */
void process_input(char *input, char *args[], int *n_args, char* delimiter, int max_args);

/* Limpa o buffer de entrada */
void clean_buffer();

/*Inicializa a shel, cria mascara de sinais e etc */
void init_shell();

/* Cria um novo processo */
void execute_process(char *args, int is_foreground, pid_t *pgid, Session *session);

/* Separa os comandos da entrada do usuário */
void launch_process(char *input);

/* Coloca um processo em primeiro plano */
void put_process_in_foreground(pid_t pid);

void wait_process_in_foreground(Process *p);

/* Coloca um processo em segundo plano */
void put_process_in_background(pid_t pid);

/* Adiciona um grupo de processos à lista */
void session_list_push(Session* session);

/*Adiciona um processo a uma lista de processos*/
void process_list_push(Process* process);

int has_alive_process();


int main(){
    char input[MAX_INPUT_SIZE]; // Buffer para a entrada do usuário
    process_list = forward_list_create();
    session_list = forward_list_create();

    init_shell();

    while(shell_running){
        printf("fsh> ");
        fflush(stdout);

        // Lê a entrada do usuário
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = 0; // Remove o \n do final da string

        launch_process(input);
    }

    // Provavelmente tem vazamento de memoria aqui
    forward_list_destroy(process_list);
    forward_list_destroy(session_list);
    return EXIT_SUCCESS;
}


void init_shell() {
    // Trata alguns sinais
    setup_signal_handlers();
    shell_pgid = getpid();
}

// Le a entrada de usuario e separa os comandos
void launch_process(char *input){
    char *process[MAX_PROCESS];
    int n_process = 0;
    pid_t pgid = 0;
    process_input(input, process, &n_process, "#", MAX_PROCESS);

    Session * new_session = create_session();

    for(int i = 0; i < n_process; i++){
        int is_foreground = (i == 0) ? 1 : 0;
        execute_process(process[i], is_foreground, &pgid, new_session);
    }

    session_list_push(new_session);
    wait_process_in_foreground(new_session->foreground);
}

void execute_process(char *args, int is_foreground, pid_t *pgid, Session *session){
    char *argv[MAX_ARGS + 1]; // +1 para receber o NULL necessario na lista de argumentos
    int n_args = 0;
    process_input(args, argv, &n_args, " ", MAX_ARGS);
    argv[n_args] = NULL; // Adicionando NULL no final da lista de argumentos
    n_args++;

    // Se o comando for waitall ou die, não cria um novo processo e executa a função
    if (strcmp(argv[0], "waitall") == 0) {
        waitall();
        return;
    } else if (strcmp(argv[0], "die") == 0) {
        die();
        return;
    }

    pid_t pid = fork();
    if(pid == 0){ // Processo filho
        signal(SIGINT, SIG_IGN); // Filhos ignoram o sinal SIGINT

        if(is_foreground )
            put_process_in_foreground(getpid());
        else
            put_process_in_background(getpid()); // Nao faz nada

        setpgid(0, *pgid);

        if(execvp(argv[0], argv) == -1)
            perror("execvp");

        exit(EXIT_FAILURE);
    }else if(pid > 0){ // Processo pai
        pid_t this_pgid = 0; // Nao faco getpgid aqui porque pode acontecer uma condição de corrida
        if (*pgid == 0) {
            if (!is_foreground) // Se for um processo em background, o pgid é o pid do primeiro processo em background
                *pgid = pid;
            this_pgid = pid;
        }else{
            this_pgid = *pgid;
        }

        Process *new_process = create_process(pid, this_pgid, is_foreground);
        process_list_push(new_process);
        insert_process_in_session(new_process, session);

    }else{
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void sigint_handler(int sig) {
    if (has_alive_process()) { 
        printf("\nProcessos em execução. Deseja finalizar a shell? (s/n) ");
        char response = getchar();
        if (response == 's' || response == 'S') {
            shell_running = 0;
            die();
        }

        // Limpa o buffer de entrada
        while (getchar() != '\n');

    } else {
        shell_running = 0;
    }
}

/* Tratador do SIGTSTP - suspende todos os filhos do shell - não conta os netos*/
void sigtstp_handler(int sig) {
    printf("\nRecebido SIGTSTP (Ctrl+Z). O programa não será suspenso. Mas os filhos de shell sim!\n");
    Node * current = process_list->head;

    while(current != NULL){
        Process * p = (Process*)current->value;
        kill(p->pid, SIGSTOP);
        p->status = STOPPED;
        current = current->next;
    }
}

// Configura os tratadores de sinal
void setup_signal_handlers() {
    struct sigaction sa_int, sa_tstp;

    // Configura o tratador para SIGINT
    sa_int.sa_handler = sigint_handler;
    sigfillset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("Failed to install SIGINT signal handler");
        exit(EXIT_FAILURE);
    }

    // Configura o tratador para SIGTSTP 
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;

    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("Failed to install SIGTSTP signal handler");
        exit(EXIT_FAILURE);
    }
}

/* Comandos internos (ainda não testados)*/

// TODO: Fazer aquela verificação de erros que a roberta falou. Essa versão é só pra testar o conceito
void waitall(){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

// TODO: Fazer aquela verificação de erros que a roberta falou. Essa versão é só pra testar o conceito
void die(){
    kill(0, SIGTERM);
    while(wait(NULL) > 0);
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

void clean_buffer(){
    while(getchar() != '\n');
}

void session_list_push(Session* session) {
    forward_list_push_front(session_list, session);
}

void process_list_push(Process* process) {
    forward_list_push_front(process_list, process);
}

void put_process_in_foreground(pid_t pid) {
    // Configura o processo como foreground e espera por sua conclusão
    if(tcsetpgrp(shell_terminal, pid) == -1) {
        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    }
}

void wait_process_in_foreground(Process *p) {
    waitpid(p->pid, NULL, 0);
    // Recupera o controle do terminal para o shell
    if(tcsetpgrp(shell_terminal, shell_pgid) == -1){
        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    } 
}

void put_process_in_background(pid_t pid_p){

}

int has_alive_process(){
    Node * current = process_list->head;
    while(current != NULL){
        Process * p = (Process*)current->value;
        if(p->status != DONE)
            return 1;
        current = current->next;
    }
    return 0;

}
