#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
// #include "handlers.h"

#define MAX_COMMANDS 5
#define MAX_ARGS 2
#define MAX_INPUT_SIZE 2000

/*
Por enquanto temos variaveis globais e varias funcoes, futuramente vamos organizar em varios arquivos
*/


pid_t pid_filhos[MAX_COMMANDS];
int num_filhos = 0;
int filhos_vivos = 1;
int shell_running = 1;


/* 
* Relativo aos handlers 
*/

// Tratador de sinal SIGINT
void sigint_handler(int sig) {
    if (filhos_vivos > 0) {
        printf("\nProcessos em execução. Deseja finalizar a shell? (s/n) ");
        char response = getchar();
        if (response == 's' || response == 'S') {
            shell_running = 0;
        }

        // Limpa o buffer de entrada
        while (getchar() != '\n');

    } else {
        shell_running = 0;
    }
}


// Tratador de sinal SIGTSTP
void sigtstp_handler(int sig) {
    printf("\nRecebido SIGTSTP (Ctrl+Z). O programa não será suspenso.\n");
    // for(int i = 0; i < num_filhos; i++){
    //     kill(pid_filhos[i], SIGSTOP);
    // }
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

// Espera por todos os processos zumbis
void waitall(){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

// Mata todos os processos filhos vivos
void die(){
    kill(0, SIGTERM);
    while(wait(NULL) > 0);
}


/* Execução de comandos */
 
// processa a entrada do usuário
void process_input(char *input, char *args[], int *n_args, char* delimiter, int max_args){
    *n_args = 0;
    char *token = strtok(input, delimiter);
    while(token != NULL && *n_args < max_args){
        args[*n_args] = token;
        (*n_args)++;
        token = strtok(NULL, delimiter);
    }


}
// Cria os processos
void create_session(char *commands[], int n_commands){
    for(int i = 0; i < n_commands; i++){
        char *args[MAX_ARGS];
        int n_args = 0;
        process_input(commands[i], args, &n_args, " ", MAX_ARGS);

        // se o comando for waitall ou exit entao nao cria um novo processo e executa a funcao
        if(strcmp(args[0], "waitall") == 0){
            waitall();
            continue;
        }else if(strcmp(args[0], "die") == 0){
            die();
            continue;
        }

        pid_t pid = fork();
        if(pid == 0){
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }else if(pid > 0){
            pid_filhos[num_filhos] = pid;
            num_filhos++;
        }else{
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
}




/* Uteis*/
void clean_buffer(){
    while(getchar() != '\n');
}




int main(){
    char input[MAX_INPUT_SIZE]; // Buffer para a entrada do usuário
    char *commands[MAX_COMMANDS]; // Buffer para os comandos
    int n_commands = 0; // Número de comandos

    setup_signal_handlers();

    while(shell_running){
        fflush(stdout);
        printf("fsh> ");

        // Lê a entrada do usuário
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = 0; // Remove o \n do final da string

        // Separa os comandos da entrada
        process_input(input, commands, &n_commands, "#", MAX_COMMANDS);

        // Executa os comandos
        create_proccesses(commands, n_commands);
        


        // limpa o buffer 
        for(int i = 0; i < n_commands; i++){
            commands[i] = NULL;
        }



        
    }
    return EXIT_SUCCESS;
}