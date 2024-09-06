#include "utils.h"
#include <string.h>
#include <stdio.h>

void process_input(char *input, char *args[], int *n_args, char* delimiter, int max_args){
    *n_args = 0;
    char *token = strtok(input, delimiter);
    while(token != NULL && *n_args < max_args){
        args[*n_args] = token;
        (*n_args)++;
        token = strtok(NULL, delimiter);
    }
}