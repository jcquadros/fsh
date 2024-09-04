#include "process.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

Session* create_session() {
    Session* new_group = (Session*)malloc(sizeof(Session));
    new_group->foreground = NULL;
    new_group->background = NULL;
    new_group->num_background = 0;
    return new_group;
}


Process* create_process(pid_t pid, pid_t pgid, int is_foreground) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    new_process->pid = pid;
    new_process->pgid = pgid;
    new_process->status = RUNNING;
    new_process->is_foreground = is_foreground;

    printf("Processo criado com pid %d e pgid %d e foreground=%d\n", new_process->pid, new_process->pgid, new_process->is_foreground);

    return new_process;
}

void insert_process_in_session(Process* p, Session* s){
    if(p->is_foreground){
        s->foreground = p;
    }else{
        s->background = (Process**)realloc(s->background, sizeof(Process*) * (s->num_background + 1));
        s->background[s->num_background] = p;
        s->num_background++;
    }
}

void destroy_session(Session* s){
    if(s->foreground != NULL){
        destroy_process(s->foreground);
    }

    for(int i = 0; i < s->num_background; i++){
        destroy_process(s->background[i]);
    }
    
    free(s);
}

void destroy_process(Process* p){
    free(p);
}

void process_wait(Process *p) {
    int status;
    waitpid(p->pid, &status, 0);
}

void session_notify(Session * s, pid_t sig){
    kill(s->foreground->pid, sig);
    if (s->num_background > 0)
        kill(s->background[0]->pgid, sig);
}

void session_mark_as_done(Session * s){
    s->foreground->status = DONE;
    if (s->num_background > 0)
        s->background[0]->status = DONE;
}

void session_mark_as_stopped(Session * s){
    s->foreground->status = STOPPED;
    if (s->num_background > 0)
        s->background[0]->status = STOPPED;
}