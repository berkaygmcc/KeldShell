#include "parser.h"
#include "redirection.h"
#include "utils.h"
#include "ast.h"

typedef enum{
    PARSER_ERR_EMPTY_COMMAND,
    PARSER_ERR_PIPE_START,
    PARSER_ERR_PIPE_END,
    PARSER_ERR_INVALID_PIPE,
    PARSER_ERR_MISSING_FILENAME,
    PARSER_ERR_TOO_MANY_ARGS,
    PARSER_ERR_TOO_MANY_COMMANDS,
    PARSER_ERR_TOO_MANY_REDIRECTIONS,
    PARSER_ERR_SEPERATOR_START,
    PARSER_ERR_MALLOC,
    PARSER_ERR_MISSING_RPAREN,
    PARSER_ERR_EMPTY_SUBSHELL,
    PARSER_ERR_UNEXPECTED_TOKEN

} t_parser_error;

static t_ast *subshell_parser(t_token *token_list, int *t_index);
static t_ast *primary_parser(t_token *token_list, int *t_index);
static t_ast *pipeline_parser(t_token *token_list, int *t_index);
static t_ast *and_or_parser(t_token *token_list, int *t_index);
t_ast *sequence_parser(t_token *token_list, int *t_index);


static int is_redirection(t_type type){
    if(type == TOKEN_REDIR_IN || type == TOKEN_REDIR_OUT || type == TOKEN_REDIR_APPEND || type == TOKEN_REDIR_HEREDOC || type == TOKEN_REDIR_ERR_OUT || type == TOKEN_REDIR_ERR_APPEND){
        return 0;
    }
    return -1;
}

static int is_command_seperator(t_type type){

    if(type == TOKEN_SEMICOLON){
        return 0;
    }
    else if(type == TOKEN_AND){
        return 0;
    }
    else if(type == TOKEN_OR){
        return 0;
    }
    else if(type == TOKEN_BACKGROUND){
        return 0;
    }

    return -1;
}


static void parser_error(t_parser_error error){
    if(error == PARSER_ERR_EMPTY_COMMAND){
        fprintf(stderr, "shell: empty command\n");
    }
    if(error == PARSER_ERR_PIPE_START){
        fprintf(stderr, "shell: unexpected pipe at beginning\n");
    }
    if(error == PARSER_ERR_PIPE_END){
        fprintf(stderr, "shell: unexpected end of command after pipe\n");
    }
    if(error == PARSER_ERR_INVALID_PIPE){
        fprintf(stderr, "shell: invalid token after pipe\n");
    }
    if(error == PARSER_ERR_MISSING_FILENAME){
        fprintf(stderr, "shell: missing filename for redirection\n");
    }
    if(error == PARSER_ERR_TOO_MANY_ARGS){
        fprintf(stderr, "shell: too many args\n");
    }
    if(error == PARSER_ERR_TOO_MANY_COMMANDS){
        fprintf(stderr, "shell: too many commands\n");
    }
    if(error == PARSER_ERR_TOO_MANY_REDIRECTIONS){
        fprintf(stderr, "shell: too many redirections\n");
    }
    if(error == PARSER_ERR_SEPERATOR_START){
        fprintf(stderr, "shell: unexpected seperator at beginning\n");
    }
    if(error == PARSER_ERR_MALLOC){
        fprintf(stderr, "shell: memory allocation failed\n");
    }
    if(error == PARSER_ERR_MISSING_RPAREN){
        fprintf(stderr, "shell: missing ')'\n");
    }
    if(error == PARSER_ERR_EMPTY_SUBSHELL){
        fprintf(stderr, "shell: empty subshell\n");
    }
    if(error == PARSER_ERR_UNEXPECTED_TOKEN){
        fprintf(stderr, "shell: unexpected token\n");
    }

}

static int command_parser(t_token *token_list, int *t_index, t_command *command){

    t_token *token = &token_list[*t_index];

    command->argc = 0;
    command->redirection_count = 0;


    if(token->type == TOKEN_END){
        parser_error(PARSER_ERR_EMPTY_COMMAND);
        return -1;
    }

    if(token->type == TOKEN_PIPE){
        parser_error(PARSER_ERR_PIPE_START);
        return -1;
    }

    if(is_command_seperator(token->type) == 0){
        parser_error(PARSER_ERR_SEPERATOR_START);
        return -1;
    }

    while (1){

        if(token->type == TOKEN_PIPE){   
            return 0;
        }
        else if(token->type == TOKEN_END){
            return 0;
        }
        else if(token->type == TOKEN_RPAREN){
            return 0;
        }
        else if(token->type == TOKEN_DLPAREN || token->type == TOKEN_DRPAREN){
            parser_error(PARSER_ERR_UNEXPECTED_TOKEN);
            return -1;
        }
        else if(is_command_seperator(token->type) == 0){
            return 0;
        }
        else{
            if(is_redirection(token->type) == 0){
                if(command->redirection_count >= MAX_REDIRECTIONS){
                    parser_error(PARSER_ERR_TOO_MANY_REDIRECTIONS);
                    return -1;
                }
        
                command->redirections[command->redirection_count].type = token->type;
                command->redirections[command->redirection_count].heredoc_fd = -1;

                (*t_index)++;
                token = &token_list[*t_index];

                if(token->type != TOKEN_WORD){
                    parser_error(PARSER_ERR_MISSING_FILENAME);
                    return -1;
                }

                if(copy_token(&command->redirections[command->redirection_count].target, token) != -1){
                    command->redirection_count++;
                }        
            }
            else{
                if(command->argc >= MAX_ARGS){
                    parser_error(PARSER_ERR_TOO_MANY_ARGS);
                    return -1;
                }

                if(copy_token(&command->args[command->argc], token) == -1){
                    return -1;
                } 
                command->argc++;  
            }
        }

        (*t_index)++;
        token = &token_list[*t_index];
    }


    return 0;
}


static t_ast *primary_parser(t_token *token_list, int *t_index){

    t_token *token = &token_list[*t_index];

    if(token->type == TOKEN_LPAREN){
        return subshell_parser(token_list, t_index);
    }

    t_ast *node = ast_create_node(AST_COMMAND, NULL, NULL, NULL);

    if(node == NULL){
        parser_error(PARSER_ERR_MALLOC);
        return NULL;
    }

    t_command *command = malloc(sizeof(t_command));

    if(command == NULL){
        parser_error(PARSER_ERR_MALLOC);
        ast_destroy(node);
        return NULL;
    }

    memset(command, 0, sizeof(t_command));

    node->command = command;

    if(command_parser(token_list, t_index, command) == -1){
        ast_destroy(node);
        return NULL;
    }

    return node;
}

static t_ast *pipeline_parser(t_token *token_list, int *t_index){

    t_ast *root = primary_parser(token_list, t_index);

    if(root == NULL){
        return NULL;
    }

    t_token *token = &token_list[*t_index];

    while(token->type == TOKEN_PIPE){

        (*t_index)++;

        t_ast *right = primary_parser(token_list, t_index);

        if(right == NULL){
            ast_destroy(root);
            return NULL;
        }

        t_ast *new_root = ast_create_node(AST_PIPE, root, right, NULL);

        if(new_root == NULL){
            parser_error(PARSER_ERR_MALLOC);
            ast_destroy(root);
            ast_destroy(right);
            return NULL;
        }

        token = &token_list[*t_index];
        
        root = new_root;
    }

    return root;
}

static t_ast *and_or_parser(t_token *token_list, int *t_index){

    t_ast *root = pipeline_parser(token_list, t_index);

    if(root == NULL){
        return NULL;
    }

    t_token *token = &token_list[*t_index];

    while(token->type == TOKEN_AND || token->type == TOKEN_OR){

        t_ast_type type;

        if(token->type == TOKEN_AND){
            type = AST_AND;
        }
        else{
            type = AST_OR;
        }

        (*t_index)++;

        t_ast *right = pipeline_parser(token_list, t_index);

        if(right == NULL){
            ast_destroy(root);
            return NULL;
        }

        t_ast *new_root = ast_create_node(type, root, right, NULL);

        if(new_root == NULL){
            parser_error(PARSER_ERR_MALLOC);
            ast_destroy(root);
            ast_destroy(right);
            return NULL;
        }

        token = &token_list[*t_index];

        root = new_root;
    }

    return root;
}


t_ast *sequence_parser(t_token *token_list, int *t_index){

    t_ast *root = and_or_parser(token_list, t_index);

    if(root == NULL){
        return NULL;
    }

    t_token *token = &token_list[*t_index];

    if(token->type == TOKEN_BACKGROUND){
        root->background = 1;
    }

    while(token->type == TOKEN_SEMICOLON || token->type == TOKEN_BACKGROUND){

        (*t_index)++;
        token = &token_list[*t_index];

        if(token->type == TOKEN_END || token->type == TOKEN_RPAREN){
            break;
        }

        t_ast *right = and_or_parser(token_list, t_index);

        if(right == NULL){
            ast_destroy(root);
            return NULL;
        }

        token = &token_list[*t_index];
        
        if (token->type == TOKEN_BACKGROUND) {
            right->background = 1;
        }

        t_ast *new_root = ast_create_node(AST_SEMICOLON, root, right, NULL);

        if(new_root == NULL){
            parser_error(PARSER_ERR_MALLOC);
            ast_destroy(root);
            ast_destroy(right);
            return NULL;
        }

        token = &token_list[*t_index];

        root = new_root;

    }

    return root;
}

static t_ast *subshell_parser(t_token *token_list, int *t_index){

    (*t_index)++;

    t_token *token = &token_list[*t_index];

    if(token->type == TOKEN_RPAREN){
        parser_error(PARSER_ERR_EMPTY_SUBSHELL);
        return NULL;
    }

    if(token->type == TOKEN_DLPAREN || token->type == TOKEN_DRPAREN){
        parser_error(PARSER_ERR_UNEXPECTED_TOKEN);
        return NULL;
    }

    t_ast *inside = sequence_parser(token_list, t_index);

    if(inside == NULL){
        return NULL;
    }

    token = &token_list[*t_index];

    if(token->type != TOKEN_RPAREN){
        parser_error(PARSER_ERR_MISSING_RPAREN);
        ast_destroy(inside);
        return NULL;
    }

    (*t_index)++;

    t_ast *node = ast_create_node(AST_SUBSHELL, inside, NULL, NULL);

    if(node == NULL){
        parser_error(PARSER_ERR_MALLOC);
        ast_destroy(inside);
        return NULL;
    }
    
    return node;
}

t_ast *parser(t_token *token_list){

    int t_index = 0;

    t_ast *root = sequence_parser(token_list, &t_index);

    if(root == NULL){
        return NULL;
    }

    t_token *token = &token_list[t_index];

    if(token->type != TOKEN_END){
        parser_error(PARSER_ERR_UNEXPECTED_TOKEN);
        ast_destroy(root);
        return NULL;
    }

    return root;
}

