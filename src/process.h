#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

#define MAX_ARGS 3 // Um comando pode ter 2 argumentos + 1 para o comando em si

typedef struct Process {
    pid_t pid;              // ID do processo
    pid_t pgid;             // ID do grupo de processos
    int is_foreground;     // Flag para indicar se é um processo em foreground
} Process;

/* Cria um novo processo */
Process* create_process(char *args, pid_t pgid, int is_foreground);
/* Espera um processo terminar */
void process_wait(Process* p);
/* Destroi um processo */
void process_destroy(Process* p);

#endif // PROCESS_H


