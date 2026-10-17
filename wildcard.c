#include "wildcard.h"
#include "cleanup.h"
#include "utils.h"
#include <dirent.h>

static int is_wildcard(char *value){

    for(int i = 0; value[i] != '\0'; i++){
        if(value[i] == '*' || value[i] == '?'){
            return 0;
        }
    }

    return -1;
}

static char *pattern_parser(char *pattern){

    char *new = malloc(strlen(pattern) + 1);

    if(new == NULL){
        return NULL;
    }

    int i = 0;
    int j = 0;
    int added_wildcard = 0;
    
    while(pattern[i] != '\0'){

        if(pattern[i] == '*'){
            if(added_wildcard == 0){
                new[j] = pattern[i];
                added_wildcard = 1;
                j++;
            }
        }
        else{
            new[j] = pattern[i];
            added_wildcard = 0;
            j++;
        }

        i++;

    }

    new[j] = '\0';

    return new;
}


static int expand_matches(char **matches, int match_count, t_command *command, int arg_index){

    int shift_count = match_count - 1;

    if(command->argc + shift_count > MAX_ARGS){
        free_matches(matches, match_count);
        return -1;
    }

    for(int i = command->argc - 1; i > arg_index; i--){
        move_token(&command->args[i + shift_count], &command->args[i]);
    }


    command->argc += shift_count;

    int j = arg_index;

    free(command->args[j].segments[0].value);

    for(int i = 0; i < match_count; i++){

        command->args[j].segments[0].value = strdup(matches[i]);

        if(command->args[j].segments[0].value == NULL){
            free_matches(matches, match_count);
            return -1;
        }

        command->args[j].segments[0].quote = QUOTE_NONE;

        command->args[j].type = TOKEN_WORD;
        command->args[j].segment_count = 1;

        j++;

    }

    if(free_matches(matches, match_count) == -1){
        return -1;
    }

    return 0;
}

static int wildcard_match(char *f_name, char *pattern){

    if(*f_name == '\0' && *pattern == '\0'){
        return 1;
    }
    else if(*f_name == *pattern){
        return wildcard_match(f_name + 1, pattern + 1);
    }
    else if(*pattern == '*'){

        if(*(pattern + 1) == '\0'){
            return 1;
        }

        if(wildcard_match(f_name, pattern + 1) == 1){
            return 1;
        }
        if(*f_name != '\0'){
            return wildcard_match(f_name + 1, pattern);
        }

        return 0;
    }
    else if(*pattern == '?'){
        if(*f_name != '\0'){
            return wildcard_match(f_name + 1, pattern + 1);
        }
        return 0;  
    }
    else{
        return 0;
    }

    return 0;
}

static int expand_wildcard(t_command *command, int arg_index, char *value){


    char *matches[MAX_ARGS] = {0};
    int match_count = 0;

    DIR *dir = opendir(".");

    if(dir == NULL){
        return -1;
    }

    struct dirent *rdir= readdir(dir);

    while(rdir != NULL){

        char *f_name = rdir->d_name;

        if (f_name[0] == '.' && value[0] != '.'){
            rdir = readdir(dir);
            continue;
        }


        if(strcmp(f_name, ".") == 0 || strcmp(f_name, "..") == 0){
            rdir= readdir(dir);
            continue;
        }

        if(wildcard_match(f_name, value) != 0 && match_count < MAX_ARGS){

            matches[match_count] = strdup(f_name);
            if(matches[match_count] == NULL){
                closedir(dir);
                free_matches(matches, match_count);
                return -1;
            }
            match_count++;
        }

        rdir= readdir(dir);

    }

    closedir(dir);



    if(match_count == 0){
        return 0;
    }

    if(expand_matches(matches, match_count, command, arg_index) == -1){
        return -1;
    }

    return 0;
}

int wildcard_node(t_ast *root){

    if(root == NULL){
        return 0;
    }
    
    if(root->type == AST_COMMAND && root->command != NULL){

        for(int i = 0; i < root->command->argc; i++){

            if(is_wildcard(root->command->args[i].segments[0].value) != -1){

                    if(root->command->args[i].segments[0].escaped){
                        continue;
                    }

                    char *new_pattern = pattern_parser(root->command->args[i].segments[0].value);

                    if(new_pattern == NULL){
                        return -1;
                    }

                    free(root->command->args[i].segments[0].value);

                    root->command->args[i].segments[0].value = new_pattern;

                    if(expand_wildcard(root->command, i, root->command->args[i].segments[0].value) == -1){
                        return -1;
                    }
            }
        }

    }

    if(wildcard_node(root->left) == -1){
        return -1;
    }
    if(wildcard_node(root->right) == -1){
        return -1;
    }

    return 0;
}