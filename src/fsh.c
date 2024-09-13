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

// Coloca a session na head da lista de sessions da fsh
void fsh_push_session(FSH* fsh, Session* session){
    forward_list_push_front(fsh->session_list, session);
}

// Coloca um processo em foreground
void fsh_put_process_in_foreground(Process* p){
    if(tcsetpgrp(STDIN_FILENO, p->pid_principal) == -1) {
        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    }
}

// Espera que um processo em foreground parar de rodar, para assim devolver a fsh voltar a executar em foreground
void fsh_wait_foreground(Session *session) {
    while (session->foreground_is_runnig) {}
}

// Coloca a fsh em foreground 
void fsh_acquire_terminal(){
    if(tcsetpgrp(STDIN_FILENO, getpid()) == -1){
        perror("tcsetpgrp");
        exit(EXIT_FAILURE);
    }
}

// Faz com que a shell libere todos os seus descendentes filhos que estejam no estado “Zombie” antes de exibir um novo prompt.
// O TRATADOR DO SIGCHLD JÁ FAZ ISSO...
void fsh_waitall(){
    pid_t pid;
    int status;
    do {
        do {
            pid = waitpid(-1, &status, WNOHANG);
        } while (pid == -1 && errno == EINTR); // A chamada de sistema foi interrompida e deve ser repetida.
    } while (pid > 0); // Tratar os multiplos filhos "zombies".
}

// Mata a fsh, mas antes mata todos os descendentes vivos
void fsh_die(FSH * fsh){
    ForwardList * session_list = fsh->session_list;
    while(session_list->size > 0){
        Session * s = (Session*)forward_list_pop_front(session_list);
        session_notify(s, SIGKILL);
        session_destroy(s);
    }
    fsh_destroy(fsh);
    exit(EXIT_SUCCESS);
}

// Função usada quando ocorre um erro no execvp.
// Ela desaloca mémoria alocada anteriormente sem enviar sinal para os processos.
void fsh_deallocate(FSH * fsh){
    ForwardList * session_list = fsh->session_list;
    while(session_list->size > 0){
        Session * s = (Session*)forward_list_pop_front(session_list);
        session_destroy(s);
    }
    fsh_destroy(fsh);
    exit(EXIT_SUCCESS);
}

// Verifica se a fsh tem descendentes vivos se não tiver retorna 0 e se tiver retorna 1
int fsh_has_alive_process(FSH* fsh){
    // percorre a lista de sessões e verifica se há algum processo vivo
    Node * current = fsh->session_list->head;
    int status;
    pid_t pid;
    while(current != NULL){
        Session * s = (Session*)current->value;

        // Verifica se o processo em foreground está vivo
        if(s->foreground != NULL){
            do {
                pid = waitpid(s->foreground->pid_principal, &status, WNOHANG); 
                if(pid == 0){
                    return 1; // Retorna 1 se o processo foreground ainda estiver vivo.
                }
            } while (pid == -1 && errno == EINTR); // Se a chamada de sistema for interrompida ela deve ser repetida.  
        }

        // Verifica se algum processo principal em background está vivo
        for(int i = 0; i < s->num_background; i++){
            do {
                pid = waitpid(s->background[i]->pid_principal, &status, WNOHANG);
                if(pid == 0){
                    return 1; // Retorna 1 se um dos processos em background ainda estiver vivo.
                }
            } while (pid == -1 && errno == EINTR); // Se a chamada de sistema for interrompida ela deve ser repetida.
        }
        current = current->next;
    }
    return 0;
}

// Encontra de qual session é um pid
Session *fsh_session_find(FSH* fsh, pid_t pid){
    return (Session*)forward_list_find(fsh->session_list, &pid, session_pid_principal_cmp);
}

// Notifica todos os processos da fsh com sinal sig
void fsh_notify(FSH* fsh, pid_t sig){
    Node * current = fsh->session_list->head;
    while(current != NULL){
        Session * s = (Session*)current->value;
        session_notify(s, sig);
        current = current->next;
    }
}

// Libera memória da fsh e das session
void fsh_destroy(FSH* fsh){
    forward_list_destroy(fsh->session_list);
    free(fsh);
}

