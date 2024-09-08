#include "fsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

FSH* fsh_create(){
    FSH* new_fsh = (FSH*)malloc(sizeof(FSH));
    new_fsh->session_list = forward_list_create();
    return new_fsh;
}


void fsh_push_session(FSH* fsh, Session* session){
    forward_list_push_front(fsh->session_list, session);
}

void fsh_put_process_in_foreground(Process* p){
    if(tcsetpgrp(STDIN_FILENO, p->pid) == -1) {
        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    }
}

void fsh_acquire_terminal(){
    if(tcsetpgrp(STDIN_FILENO, getpid()) == -1){
        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    }
}

void fsh_waitall(){
    printf("Esperando todos os processos terminarem\n");
    pid_t pid;
    while((pid = wait(NULL)) > 0); // Perguntar a roberta se é o suficiente

    if (pid == -1 && errno != ECHILD) {
        perror("Erro ao esperar pelo término dos processos filhos");
    }
}

void fsh_die(FSH * fsh){
    ForwardList * session_list = fsh->session_list;
    while(session_list->size > 0){
        Session * s = (Session*)forward_list_pop_front(session_list);
        session_notify(s, SIGKILL, 1);
        session_destroy(s);
    }
    fsh_destroy(fsh);
    exit(EXIT_SUCCESS);
}

int fsh_has_alive_process(FSH* fsh){
    // percorre a lista de sessões e verifica se há algum processo vivo
    Node * current = fsh->session_list->head;
    while(current != NULL){
        Session * s = (Session*)current->value;

        // Verifica se o processo em foreground está vivo
        if(s->foreground != NULL){
            int status;
            if(waitpid(s->foreground->pid, &status, WNOHANG) == 0){
                return 1;
            }
        }

        // Verifica se algum processo em background está vivo
        for(int i = 0; i < s->num_background; i++){
            int status;
            if(waitpid(s->background[i]->pid, &status, WNOHANG) == 0){
                return 1;
            }
        }
        current = current->next;
    }
    return 0;
}

Session *fsh_session_find(FSH* fsh, pid_t pid){
    return (Session*)forward_list_find(fsh->session_list, &pid, session_pid_cmp);
}

void fsh_notify(FSH* fsh, pid_t sig){
    Node * current = fsh->session_list->head;
    while(current != NULL){
        Session * s = (Session*)current->value;
        session_notify(s, sig, 1);
        current = current->next;
    }
}

void fsh_destroy(FSH* fsh){
    forward_list_destroy(fsh->session_list);
    free(fsh);
}

