#include "process.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

ProcessGroup* create_process_group(pid_t pgid) {
    ProcessGroup* new_group = (ProcessGroup*)malloc(sizeof(ProcessGroup));
    new_group->pgid = pgid;
    new_group->foreground = NULL;
    new_group->background = NULL;
    new_group->num_background = 0;
    return new_group;
}


Process* create_process(pid_t pid, int is_foreground, int is_background, char* command, ProcessGroup* parent_group) {
    Process* new_process = (Process*)malloc(sizeof(Process));
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->is_foreground = is_foreground;
    new_process->is_background = is_background;
    new_process->command = strdup(command);
    new_process->parent_group = parent_group;
    return new_process;
}

void insert_process_in_group(Process* p, ProcessGroup* pg){
    if(p->is_foreground){
        pg->foreground = p;
    }else{
        pg->background = (Process**)realloc(pg->background, sizeof(Process*) * (pg->num_background + 1));
        pg->background[pg->num_background] = p;
        pg->num_background++;
    }
}

void destroy_process_group(ProcessGroup* pg){
    if(pg->foreground != NULL){
        destroy_process(pg->foreground);
    }

    for(int i = 0; i < pg->num_background; i++){
        destroy_process(pg->background[i]);
    }
    
    free(pg);
}

void destroy_process(Process* p){
    // free(p->command);
    free(p);
}
