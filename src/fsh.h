#ifndef FSH_H
#define FSH_H

#include "process.h"
#include <sys/types.h>
#include "forward_list.h"
#include "session.h"

typedef struct FSH {
    ForwardList* session_list;
} FSH;

FSH* fsh_create();
void fsh_push_session(FSH* fsh, Session* session);
void fsh_put_process_in_foreground(Process* p);
void fsh_acquire_terminal();
void fsh_waitall();
void fsh_die(FSH * fsh);
void fsh_deallocate(FSH * fsh);
int  fsh_has_alive_process(FSH* fsh);
void fsh_notify(FSH* fsh, pid_t sig);
Session *fsh_session_find(FSH* fsh, pid_t pid);
void fsh_destroy(FSH* fsh);


#endif