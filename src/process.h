#ifndef PROCESS_H
#define PROCESS_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_ARGS 2
#define MAX_PROCESS 5

typedef enum {
    RUNNING, STOPPED, DONE
} status;

typedef struct Process {
    pid_t pid;              // ID do processo
    pid_t pgid;             // ID do grupo de processos
    status status;         // Status do processo
    int is_foreground;     // Flag para indicar se é um processo em foreground
} Process;

typedef struct Session {
    Process* foreground;   // Processo em foreground
    Process** background;  // Array de processos em background
    int num_background;    // Número de processos em background
} Session;

/* Cria uma nova sessao */
Session* create_session();

/* Cria um novo processo */
Process* create_process(pid_t pid, pid_t pgid, int is_foreground);

/* Adiciona um processo a um grupo de processos */
void insert_process_in_session(Process* p, Session* s);

/* Destroi um grupo de processos */
void destroy_session(Session* s);

/* Destroi um processo */
void destroy_process(Process* p);

#endif // PROCESS_H


