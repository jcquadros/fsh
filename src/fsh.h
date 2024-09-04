#ifndef FSH_H
#define FSH_H

#include "process.h"
#include <sys/types.h>
#include "forward_list.h"

typedef struct FSH {
    ForwardList* process_list;
    ForwardList* session_list;
} FSH;

FSH* fsh_create();
void fsh_destroy(FSH* fsh);
void fsh_push_process(FSH* fsh, Process* process);
void fsh_push_session(FSH* fsh, Session* session);
void fsh_put_process_in_foreground(Process* p);
void fsh_acquire_terminal();
void fsh_waitall();
void fsh_die(FSH * fsh);
int fsh_has_alive_process(FSH* fsh);
void fsh_notify(FSH* fsh, pid_t sig);

Session *session_find(FSH* fsh, pid_t pid);


#endif