#include "ast.h"
#include "cleanup.h"

char *get_t_type(t_type type){

    if(type == TOKEN_WORD){
        return "TOKEN_WORD";
    }
    else if(type == TOKEN_PIPE){
        return "TOKEN_PIPE";
    }
    else if(type == TOKEN_SEMICOLON){
        return "TOKEN_SEMICOLON";
    }
    else if(type == TOKEN_AND){
        return "TOKEN_AND";
    }
    else if(type == TOKEN_OR){
        return "TOKEN_OR";
    }
    else if(type == TOKEN_BACKGROUND){
        return "TOKEN_BACKGROUND";
    }
    else if(type == TOKEN_LPAREN){
        return "TOKEN_LPAREN";
    }
    else if(type == TOKEN_RPAREN){
        return "TOKEN_RPAREN";
    }
    else if(type == TOKEN_DLPAREN){
        return "TOKEN_DLPAREN";
    }
    else if(type == TOKEN_DRPAREN){
        return "TOKEN_DRPAREN";
    }
    else if(type == TOKEN_REDIR_IN){
        return "TOKEN_REDIR_IN";
    }
    else if(type == TOKEN_REDIR_OUT){
        return "TOKEN_REDIR_OUT";
    }
    else if(type == TOKEN_REDIR_ERR_OUT){
        return "TOKEN_REDIR_ERR_OUT";
    }
    else if(type == TOKEN_REDIR_ERR_APPEND){
        return "TOKEN_REDIR_ERR_APPEND";
    }
    else if(type == TOKEN_REDIR_HEREDOC){
        return "TOKEN_REDIR_HEREDOC";
    }
    else if(type == TOKEN_REDIR_APPEND){
        return "TOKEN_REDIR_APPEND";
    }
    else if(type == TOKEN_END){
        return "TOKEN_END";
    }
    
    return NULL;

}

/*static char *get_ast_type(t_ast_type type){

    if(type == AST_COMMAND){
        return "AST_COMMAND";
    }
    else if(type == AST_PIPE){
        return "AST_PIPE";
    }
    else if(type == AST_AND){
        return "AST_AND";
    }
    else if(type == AST_OR){
        return "AST_OR";
    }
    else if(type == AST_SEMICOLON){
        return "AST_SEMICOLON";
    }
    else if(type == AST_SUBSHELL){
        return "AST_SUBSHELL";
    }    

    return NULL;
}*/

t_ast *ast_create_node(t_ast_type type, t_ast *left, t_ast *right, t_command *command){

    t_ast *node = malloc(sizeof(t_ast));

    if(node == NULL){
        return NULL;
    }

    node->type = type;

    node->background = 0; 

    node->left = left;
    node->right = right;

    node->command = command;

    return node;

}


void ast_destroy(t_ast *root){

    if(root == NULL){
        return;
    }

    if(root->left != NULL){
        ast_destroy(root->left);
    }
    if(root->right != NULL){
        ast_destroy(root->right);
    }

    if(root->command != NULL){
        free_command(root->command);
        free(root->command);
    }

    free(root);

}

/*

DEBUG

void ast_print(t_ast *root, int depth){

    if(root == NULL){
        return;
    }

    for(int i = 0; i < depth; i++){
        printf("    ");
    }

    printf("NODE_TYPE: %s (bg: %d)\n", get_ast_type(root->type), root->background);

    printf("\n");

    if(root->command != NULL){
        for(int i = 0; i < root->command->argc; i++){
            for(int i = 0; i < depth + 1; i++){
                printf("    ");
            }
            printf("COMMAND[%d]: \n", i);

            for(int i = 0; i < depth + 2; i++){
                printf("    ");
            }
            printf("SEGMENTS: \n");

            for(int j = 0; j < root->command->args[i].segment_count; j++){
                for(int i = 0; i < depth + 3; i++){
                    printf("    ");
                }
                printf("segment[%d].value: %s\n", j, root->command->args[i].segments[j].value);
                for(int i = 0; i < depth + 3; i++){
                    printf("    ");
                }
                printf("segment[%d].type: %s\n", j, get_t_type(root->command->args[i].type));
            

            }

            printf("\n");
        }

        
        for(int i = 0; i < depth + 1; i++){
            printf("    ");
        }

        printf("REDIRECTIONS: \n");

        for(int k = 0; k < root->command->redirection_count; k++){
            for(int i = 0; i < depth + 3; i++){
                printf("    ");
            }
            printf("redirection[%d].type: %s\n", k, get_t_type(root->command->redirections[k].type));

            for(int y = 0; y < root->command->redirections[k].target.segment_count; y++){
                for(int i = 0; i < depth + 3; i++){
                    printf("    ");
                }
                printf("redirection[%d].value.segment[%d]: %s\n", k, y, root->command->redirections[k].target.segments[y].value);
            }
        }

    }


    if(root->left != NULL){
        ast_print(root->left, depth + 1);
    }

    if(root->right != NULL){
        ast_print(root->right, depth + 1);
    }
    
}*/