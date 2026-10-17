#include "cleanup.h"
#include "ast.h"

void free_command(t_command *command){

    for(int i = 0; i < command->argc; i++){
        for(int j = 0; j < command->args[i].segment_count; j++){
            free(command->args[i].segments[j].value);
            command->args[i].segments[j].value = NULL;
            command->args[i].segments[j].escaped = 0;
        }
        command->args[i].segment_count = 0;
    }

    command->argc = 0;

    for(int i = 0; i < command->redirection_count; i++){

        command->redirections[i].type = TOKEN_END;

        if(command->redirections[i].heredoc_fd != -1){
            if(close(command->redirections[i].heredoc_fd) == -1){
                perror("close");
            }
        }

        for(int j = 0; j < command->redirections[i].target.segment_count; j++){
            free(command->redirections[i].target.segments[j].value);
            command->redirections[i].target.segments[j].value = NULL;
            command->redirections[i].target.segments[j].escaped = 0;
        }

        command->redirections[i].target.segment_count = 0;
    }

    command->redirection_count = 0;
}

void free_token_list(t_token *token_list){

    if(token_list == NULL){
        return;
    }

    for(int i = 0; i < MAX_ARGS; i++){

        for(int j = 0; j < token_list[i].segment_count; j++){
             
            if(token_list[i].segments[j].value != NULL){
                free(token_list[i].segments[j].value);
                token_list[i].segments[j].value = NULL;    
                token_list[i].segments[j].quote = QUOTE_NONE;
                token_list[i].segments[j].escaped = 0;          
            }
        }

        token_list[i].segment_count = 0;
        token_list[i].type = TOKEN_END;
    }
}

int free_argv(char **argv){
   for(int i = 0; argv[i] != NULL; i++){
        free(argv[i]);
   }
   return 0;
}

int free_matches(char **matches, int match_count){
    for(int i = 0; i < match_count; i++){
        free(matches[i]);
    }
    return 0;
}

void cleanup_all(t_ast *root, t_token *token_list){

    ast_destroy(root);
    free_token_list(token_list);
    free(token_list);

}