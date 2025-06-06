#include "session.h"
#include <stdlib.h>
#include <signal.h>
#include "utils.h"
#include <stdio.h>

Session* session_create(char *input) {
    char *commands[MAX_COMMANDS];
    int n_commands = 0;
    pid_t pgid = 0;
    process_input(input, commands, &n_commands, "#", MAX_COMMANDS); // Separa os comandos por #

    Session *s = (Session*)malloc(sizeof(Session));
    s->foreground = NULL;
    s->background = NULL;
    s->num_background = 0;
    s->foreground_is_runnig = 0;

    for(int i = 1; i < n_commands; i++){
        Process *p = create_process(commands[i], pgid, 0); // Cria um processo em background
        if (p == NULL) { // Desaloca caso ocorra um erro na execvp
            session_destroy(s);
            return NULL;
        }
        pgid = p->pgid; // Atualiza o pgid para que todos os processos sejam do mesmo grupo
        session_push_process(s,p);
    }
    pgid = 0;
    Process *p = create_process(commands[0], pgid, 1); // Cria um processo em foreground
    if (p == NULL) { // Desaloca caso ocorra um erro na execvp
        session_destroy(s);
        return NULL;
    }
    session_push_process(s, p);
    s->foreground_is_runnig = 1;
    return s;
}

void session_push_process(Session* s, Process* p){
    if(p->is_foreground){
        s->foreground = p;
    }else{
        s->background = (Process**)realloc(s->background, sizeof(Process*) * (s->num_background + 1));
        s->background[s->num_background] = p;
        s->num_background++;
    }
}

void session_notify(Session * s, pid_t sig){
    kill(s->foreground->pid_principal, sig);
    if (s->num_background > 0) {
        kill(-(s->background[0]->pgid), sig);
        for (int i = 0; i < s->num_background; i++)
            kill(-(s->background[i]->pid_secundario), sig);
    }
}

int session_pid_principal_cmp(void *data, void *key){
    Session *s = (Session*)data;
    pid_t pid = *((pid_t*)key);
    if(s->foreground->pid_principal == pid)
        return 0;
    for(int i = 0; i < s->num_background; i++){
        if(s->background[i]->pid_principal == pid)
            return 0;
    }
    return 1;
}

void session_destroy(Session* s){
    if(s->foreground != NULL){
        process_destroy(s->foreground);
    }
    for(int i = 0; i < s->num_background; i++){
        process_destroy(s->background[i]);
    }
    free(s->background);
    free(s);
}