#ifndef UTILS_H
#define UTILS_H

/* 
 * Separa a entrada do usuário em argumentos de acordo com o delimitador selecionado
*/
void process_input(char *input, char *args[], int *n_args, char* delimiter, int max_args);

#endif // UTILS_H