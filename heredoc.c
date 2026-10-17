#include "heredoc.h"

#include "fd_utils.h"
#include "prepare.h"
#include "signals.h"
#include "utils.h"
#include "conversion.h"
#include "heredoc_expand.h"

static int process_command(t_command *command, t_shell *shell){

    setup_heredoc_signals();
    interrupted = 0;


    for(int j = 0; j < command->redirection_count; j++){
        if(command->redirections[j].type == TOKEN_REDIR_HEREDOC){
    
            t_token *delimiter_token = &command->redirections[j].target;

            int is_delimiter_expand = 1;

            if(delimiter_token->segments[0].quote != QUOTE_NONE){
                is_delimiter_expand = 0;
            }
                
            char *delimiter = prepare_value(delimiter_token);

            if(delimiter == NULL){
                setup_signals();
                return -1;
            }

            int fd[2];

            if(pipe(fd) == -1){
                free(delimiter);
                perror("pipe");
                setup_signals();
                return -1;
            }

            while(1){

                if(interrupted){
                    break;
                }

                char *line = readline("> ");

                if(line == NULL){
                    fprintf(stderr, "warning: here-document delimited by end-of-file\n");
                    break;
                }

                if(strcmp(line, delimiter) == 0){
                    free(line);
                    break;
                }

                if(is_delimiter_expand){

                    char *new_line = expand_heredoc_line(line, shell);       
                    
                    if(new_line == NULL){
                        free(line);
                        close_fd(fd, 0);
                        close_fd(fd, 1);
                        setup_signals();
                        return -1;
                    }  
                    
                    free(line);

                    line = new_line;
                }

                if(write(fd[1], line, strlen(line)) == -1){
                    free(delimiter);
                    free(line);
                    perror("write");
                    close_fd(fd, 0);
                    close_fd(fd, 1);
                    setup_signals();
                    return -1;
                }   
                    
                free(line); 
                
                if(write(fd[1], "\n", 1) == -1){
                    free(delimiter);
                    perror("write");
                    close_fd(fd, 0);
                    close_fd(fd, 1);
                    setup_signals();
                    return -1;
                } 
            }


            free(delimiter);

            close_fd(fd, 1);

            if(interrupted){
                shell->exit_status = 130;
                interrupted = 0;
                close_fd(fd, 0);
                setup_signals();
                return -1;
            }

            command->redirections[j].heredoc_fd = fd[0];

        }
    }    

    setup_signals();

    

    return 0;
}

int process_heredoc(t_ast *root, t_shell *shell){

    if(root == NULL){
        return 0;
    }

    if(root->type == AST_COMMAND && root->command != NULL){
        if(process_command(root->command, shell) == -1){
            return -1;
        }     
    }

    

    if(process_heredoc(root->left, shell) == -1){
        return -1;
    }
    if(process_heredoc(root->right, shell) == -1){
        return -1;
    }

    return 0;
    
}