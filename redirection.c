#include "redirection.h"
#include "prepare.h"
#include "fd_utils.h"


static int open_redirection_file(t_redirection *redirection, char *target){

    if(redirection->type == TOKEN_REDIR_OUT){
        return open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    if(redirection->type == TOKEN_REDIR_APPEND){
        return open(target, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    if(redirection->type == TOKEN_REDIR_IN){
        return open(target, O_RDONLY);
    }
    if(redirection->type == TOKEN_REDIR_ERR_OUT){
        return open(target, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    if(redirection->type == TOKEN_REDIR_ERR_APPEND){
        return open(target, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    return -1;
}

static int get_fileno(t_type type){
    if(type == TOKEN_REDIR_OUT || type == TOKEN_REDIR_APPEND){
        return STDOUT_FILENO;
    }
    if(type == TOKEN_REDIR_IN || type == TOKEN_REDIR_HEREDOC){
        return STDIN_FILENO;
    }
    if(type == TOKEN_REDIR_ERR_OUT || type == TOKEN_REDIR_ERR_APPEND){
        return STDERR_FILENO;
    }

    return -1;

}

int apply_redirections(t_command *command){
    for(int i = 0; i < command->redirection_count; i++){


        char *target = command->redirections[i].target.segments[0].value;

        int fd;

        if(command->redirections[i].type == TOKEN_REDIR_HEREDOC){
            fd = command->redirections[i].heredoc_fd;
        }
        else{
            fd = open_redirection_file(&command->redirections[i], target);
        }
 

        if(fd == -1){
            return -1;
        }

        if(dup2(fd, get_fileno(command->redirections[i].type)) == -1){
            perror("dup2");

            if(close(fd) == -1){
                perror("close");
            }

            if(command->redirections[i].type ==TOKEN_REDIR_HEREDOC){
                command->redirections[i].heredoc_fd = -1;
            }

            return -1;
        }

        if(close(fd) == -1){
            perror("close");

            if(command->redirections[i].type ==TOKEN_REDIR_HEREDOC){
                command->redirections[i].heredoc_fd = -1;
            }

            return -1;
        }

        if(command->redirections[i].type ==TOKEN_REDIR_HEREDOC){
            command->redirections[i].heredoc_fd = -1;
        }
    }
    return 0;
}