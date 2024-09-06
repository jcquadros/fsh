#ifndef SESSION_H
#define SESSION_H

#include "process.h"
#include <sys/types.h>

#define MAX_COMMANDS 5

typedef struct Session {
    Process* foreground;   // Processo em foreground
    Process** background;  // Array de processos em background
    int num_background;    // Número de processos em background
} Session;

/* Cria uma nova sessao */
Session* session_create(char *input);
/* Adiciona um processo a uma sessao */
void session_push_process(Session* s, Process* p);
/* Notifica todos os processos de uma sessao */
void session_notify(Session * s, pid_t sig);
/* Compara o pid de um processo com o pid dos processos de uma sessao */
int session_pid_cmp(void *data, void *key);
/* Destroi uma sessao */
void session_destroy(Session* s);


#endif// SESSION_H