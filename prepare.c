#include "prepare.h"
#include "cleanup.h"

static int has_wildcard(char *value){
    for(int i = 0; value[i] != '\0'; i++){
        if(value[i] == '*' || value[i] == '?'){
            return 1;
        }
    }

    return 0;
}

char *prepare_value(t_token *arg){

    size_t len = 0;

    for(int i = 0; i < arg->segment_count; i++){
        len += strlen(arg->segments[i].value);
    }

    char *ptr = malloc(len + 1);

    if(ptr == NULL){
        return NULL;
    }

    char *dst = ptr; 

    for(int j = 0; j < arg->segment_count; j++){

        size_t s_len = strlen(arg->segments[j].value);

        memcpy(dst, arg->segments[j].value, s_len);

        dst += s_len;

    }

    *dst = '\0';

    return ptr;

}

static int prepare_arg(t_token *arg){

    int is_escaped = 0;

    for(int i = 0; i < arg->segment_count; i++){
        if(has_wildcard(arg->segments[i].value) && (arg->segments[i].quote != QUOTE_NONE || arg->segments[i].escaped)){
            is_escaped = 1;
        }
    }

    char *new_value = prepare_value(arg);

    if(new_value == NULL){
        return -1;
    }

    for(int i = 0; i < arg->segment_count; i++){
        arg->segments[i].quote = QUOTE_NONE;
        arg->segments[i].escaped = 0;
        free(arg->segments[i].value);
        arg->segments[i].value = NULL;
    }

    arg->segment_count = 1;
    arg->segments[0].value = new_value;

    if(arg->segments[0].value == NULL){
        return -1;
    }

    arg->segments[0].escaped = is_escaped;
    arg->segments[0].quote = QUOTE_NONE;

    return 0;
}


static int prepare_command(t_command *command){

    for(int i = 0; i < command->argc; i++){
        if(prepare_arg(&command->args[i]) == -1){
            return -1;
        }
    }

    for(int i = 0; i < command->redirection_count; i++){
        if(command->redirections[i].type != TOKEN_REDIR_HEREDOC){
            if(prepare_arg(&command->redirections[i].target) == -1){
                return -1;
            }
        }
    }

    return 0;
}

int prepare_node(t_ast *root){

    if(root == NULL){
        return 0;
    }

    if(root->type == AST_COMMAND && root->command != NULL){
        if(prepare_command(root->command) == -1){
            return -1;
        }
    }

    if(prepare_node(root->left) == -1){
        return -1;
    }
    if(prepare_node(root->right) == -1){
        return -1;
    }

    return 0;
}