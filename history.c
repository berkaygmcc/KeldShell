#include "history.h"

char *history[MAX_HISTORY];
int history_count = 0;

static void shift_history(void){

    free(history[0]);

    int i = 0;

    while(i < history_count - 1){

        history[i] = history[i+1];
        i++;

    }


    history_count--;
    history[i] = NULL;
}


void load_history(void){

    stifle_history(MAX_HISTORY);

    FILE *fp = fopen("history.txt", "r");

    if(fp == NULL){
        return;
    }

    char line[MAX_INPUT];

    while(fgets(line, sizeof(line), fp) != NULL){
        line[strcspn(line, "\n")] = '\0';

        if(history_count >= MAX_HISTORY){
            shift_history();
        }

        history[history_count] = strdup(line);

        if(history[history_count] == NULL){
            perror("strdup");
            break;
        }

        add_history(line);
        history_count++;
    }

    fclose(fp);
}

void append_history_to_file(char *input){
    FILE *fp = fopen("history.txt", "a");

    if(fp == NULL){
        perror("history");
        return;
    }

    fprintf(fp, "%s\n", input);

    fclose(fp);
}

void rewrite_history_file(void){
    FILE *fp = fopen("history.txt", "w");

    if(fp == NULL){
        return;
    }

    for(int i = 0; i < history_count; i++){
        fprintf(fp, "%s\n", history[i]);
    }

    fclose(fp);
}


void save_history(char *input){
    

    if(history_count < MAX_HISTORY){
        history[history_count] = strdup(input);

        if(history[history_count] == NULL){
            perror("strdup");
        }

        history_count++;
        append_history_to_file(input);

    }
    else{
        shift_history();
        history[history_count] = strdup(input);

        if(history[history_count] == NULL){
            perror("strdup");
        }

        history_count++;
        rewrite_history_file();
    }


}

int run_history(t_command *command, t_shell *shell){
    (void)command;
    (void)shell;
    for(int i = 0; i < history_count; i++){
        printf("minishell>> %s \n", history[i]);
    }

    return 0;
}

void free_resources(void){
    for(int i = 0; i < history_count; i++){
        free(history[i]);
        history[i] = NULL;
    }

    history_count = 0;

    clear_history();
}