#include "terminal.h"
#include "history.h"

char *get_prompt(void){
    char cwd[1024];

    if(getcwd(cwd, sizeof(cwd)) != NULL){


        int len = snprintf(NULL, 0, "keld>> %s >> ", cwd);

        char *prompt = malloc(len + 1);

        if(prompt == NULL){
            return NULL;
        }

        snprintf(prompt, len + 1, "keld>> %s >> ", cwd);

        return prompt;
    }
    else{
        return strdup("keld>> ");
    }
}

int read_input(char *input, int size){

    char *prompt = get_prompt();

    if(prompt == NULL){
        return -1;
    }

    char *line = readline(prompt);

    free(prompt);

    if(line == NULL){
        return 1;
    }

    strncpy(input, line, size - 1);
    input[size - 1] = '\0';

    if(input[0] != '\0'){
        add_history(input);
        save_history(input);
    }

    free(line);

    return 0;
}