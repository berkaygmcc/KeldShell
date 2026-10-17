#include "shell.h"
#include "builtin.h"
#include "redirection.h"
#include "history.h"
#include "parser.h"
#include "terminal.h"
#include "signals.h"
#include "lexer.h"
#include "expansion.h"
#include "heredoc.h"
#include "cleanup.h"
#include "wildcard.h"
#include "ast.h"
#include "prepare.h"
#include "execute.h"
#include "job_control.h"


int main(){

    char input[MAX_INPUT];
    load_history();
    setup_signals();

    t_shell shell = {0};

    if(init_job_control(&shell) == -1){
        return -1;
    }

    while (1)
    {        
        t_token *token_list = calloc(MAX_ARGS + 1, sizeof(t_token));
        t_ast *root = NULL;

        if(token_list == NULL){
            perror("malloc");
            break;
        }
        

        int code = read_input(input, MAX_INPUT);

        if(interrupted){



            shell.exit_status = 130;
            interrupted = 0;
            cleanup_all(root, token_list);
            continue;
        }

        reap_background_jobs(&shell);

        if(code == 0){

            if(tokenize_input(input, token_list) == -1){
                free(token_list);
                continue;
            }

            root = parser(token_list);

            if(root == NULL){
                cleanup_all(root, token_list);
                shell.exit_status = 2;
                continue;
            }

            if(expand_ast(root, &shell) == -1){
                cleanup_all(root, token_list);
                break;   
            }

            if(prepare_node(root) == -1){
                cleanup_all(root, token_list);
                break;  
            }
            
            if(wildcard_node(root) == -1){
                cleanup_all(root, token_list);
                break;  
            }

            if(process_heredoc(root, &shell) == -1){
                cleanup_all(root, token_list);
                continue;  
            }

            int status = execute_top_level(root, &shell);

            cleanup_all(root, token_list);

            if(shell.is_exit){
                break;
            }

            if(status == -1){
                continue;
            }

            
        }
        else if(code == 1){    
            cleanup_all(root, token_list);
            break;
        }
        else{
            cleanup_all(root, token_list);  
            perror("read_input");    
            break;
        }
    }
    
    free_resources();

    return 0;
}



