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
    status status;         // Status do processo
    int is_foreground;     // Flag para indicar se é um processo em foreground
    int is_background;     // Flag para indicar se é um processo em background
    char *command;         // Comando a ser executado
    struct ProcessGroup* parent_group; // Referência ao grupo de processos pai
} Process;

typedef struct ProcessGroup {
    pid_t pgid;             // ID do grupo de processos
    Process* foreground;   // Processo em foreground
    Process** background;  // Array de processos em background
    int num_background;    // Número de processos em background
} ProcessGroup;

/* Cria um novo grupo de processos */
ProcessGroup* create_process_group(pid_t pgid);

/* Cria um novo processo */
Process* create_process(pid_t pid, int is_foreground, int is_background, char* command, ProcessGroup* parent_group);

/* Adiciona um processo a um grupo de processos */
void insert_process_in_group(Process* p, ProcessGroup* pg);

/* Destroi um grupo de processos */
void destroy_process_group(ProcessGroup* pg);

/* Destroi um processo */
void destroy_process(Process* p);

#endif // PROCESS_H


