#include "expansion.h"
#include "conversion.h"
#include "utils.h"


static int expand_segment(t_segment *segment, t_shell *shell){

    if(segment->quote == QUOTE_SINGLE || segment->escaped){
        return 0;
    }

    if(segment->value[0] == '$'){

        if(strcmp(segment->value, "$?") == 0){
            free(segment->value);
            segment->value = itoa(shell->exit_status);
            if(segment->value == NULL){
                return -1;
            }
        }
        else if(strcmp(segment->value, "$$") == 0){
            free(segment->value);
            segment->value = itoa(getpid());
            if(segment->value == NULL){
                return -1;
            }
        }
        else{
            if(is_variable_start(segment->value[1]) != 0){
                return 0;
            }

            char *value = getenv(segment->value + 1);

            free(segment->value);


            if(value != NULL){
                segment->value = strdup(value);
            }
            else{
                segment->value = strdup("");
            }

            if(segment->value == NULL){
                return -1;
            }
        }

    }

    return 0;

}

static int expand_token(t_token *arg, t_shell *shell){

    for(int i = 0; i < arg->segment_count; i++){
        
        if(expand_segment(&arg->segments[i], shell) == -1){
            return -1;
        }
    }

    return 0;

}

static int expand_command(t_command *command, t_shell *shell){

    for(int i = 0; i < command->argc; i++){
        if(expand_token(&command->args[i], shell) == -1){
            return -1;
        }
    }

    for(int j = 0; j < command->redirection_count; j++){
        if(command->redirections[j].type != TOKEN_REDIR_HEREDOC){
            if(expand_token(&command->redirections[j].target, shell) == -1){
                return -1;
            }
        }
    }

    return 0;

}

int expand_ast(t_ast *ast, t_shell *shell){

    if(ast == NULL){
        return 0;
    }

    if(ast->type == AST_COMMAND && ast->command != NULL){
        if(expand_command(ast->command, shell) == -1){
            return -1;
        }
    }

    if(expand_ast(ast->left, shell) == -1){
        return -1;
    }

    if(expand_ast(ast->right, shell) == -1){
        return -1;
    }

    return 0;

}
