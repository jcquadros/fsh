#define _POSIX_C_SOURCE 200809L

#include "fsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

FSH* fsh_create(){
    FSH* new_fsh = (FSH*)malloc(sizeof(FSH));
    new_fsh->process_list = forward_list_create();
    new_fsh->session_list = forward_list_create();
    return new_fsh;
}

void fsh_destroy(FSH* fsh){
    forward_list_destroy(fsh->process_list);
    forward_list_destroy(fsh->session_list);
    free(fsh);
}

void fsh_push_process(FSH* fsh, Process* process){
    forward_list_push_front(fsh->process_list, process);
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
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

void fsh_die(FSH * fsh){
    Node * current = fsh->session_list->head;
    while(current != NULL){
        Session * s = (Session*)current->value;
        session_notify(s, SIGKILL);
        destroy_session(s);
        current = current->next;
    }
    fsh_destroy(fsh);
    exit(EXIT_SUCCESS);
}

int fsh_has_alive_process(FSH* fsh){
    Node * current = fsh->process_list->head;
    while(current != NULL){
        Process * p = (Process*)current->value;
        if(p->status != DONE)
            return 1;
        current = current->next;
    }
    return 0;
}

Session *session_find(FSH* fsh, pid_t pid){
    Node * current = fsh->session_list->head;
    while(current != NULL){
        Session * s = (Session*)current->value;
        if(s->foreground->pid == pid)
            return s;
        for(int i = 0; i < s->num_background; i++){
            if(s->background[i]->pid == pid)
                return s;
        }
        current = current->next;
    }
    return NULL;
}

void fsh_notify(FSH* fsh, pid_t sig){
    Node * current = fsh->session_list->head;
    while(current != NULL){
        Session * s = (Session*)current->value;
        session_notify(s, sig);
        current = current->next;
    }
}
