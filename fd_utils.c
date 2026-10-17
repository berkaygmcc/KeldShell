#include "fd_utils.h"


int save_fd(t_fd *backup){ 

    backup->stdout_backup = dup(STDOUT_FILENO);

    if(backup->stdout_backup == -1){
        perror("dup");
        return -1;
    }

    backup->stdin_backup = dup(STDIN_FILENO);

    if(backup->stdin_backup == -1){
        perror("dup");

        if(close(backup->stdout_backup) == -1){
            perror("close");
        }

        return -1;
    }

    backup->stderr_backup = dup(STDERR_FILENO);

    if(backup->stderr_backup == -1){
        perror("dup");

        if(close(backup->stdout_backup) == -1){
            perror("close");
        }
        if(close(backup->stdin_backup) == -1){
            perror("close");
        }

        return -1;       
    }
    
    return 0;
 }

int restore_fd(t_fd *backup){

    if(dup2(backup->stderr_backup, STDERR_FILENO) == -1){
        perror("dup2");

        if(close(backup->stderr_backup) == -1){
            perror("close");
        }
        if(close(backup->stdout_backup) == -1){
            perror("close");
        }
        if(close(backup->stdin_backup) == -1){
            perror("close");
        }
        

        return -1;
    }

    if(dup2(backup->stdout_backup, STDOUT_FILENO) == -1){
        perror("dup2");

        if(close(backup->stderr_backup) == -1){
            perror("close");
        }
        if(close(backup->stdout_backup) == -1){
            perror("close");
        }
        if(close(backup->stdin_backup) == -1){
            perror("close");
        }

        return -1;
    }

    if(dup2(backup->stdin_backup, STDIN_FILENO) == -1){
        perror("dup2");

        if(close(backup->stderr_backup) == -1){
            perror("close");
        }
        if(close(backup->stdout_backup) == -1){
            perror("close");
        }
        if(close(backup->stdin_backup) == -1){
            perror("close");
        }

        return -1;
    }
       
    if(close(backup->stderr_backup) == -1){
        perror("close");
        return -1;
    }
    if(close(backup->stdout_backup) == -1){
        perror("close");
        return -1;
    }

    if(close(backup->stdin_backup) == -1){
        perror("close");
        return -1;
    }

    return 0;
}

int close_fd(int *fd, int index){

    if(close(fd[index]) == -1){
        perror("close");
        return -1;
    }

    return 0;

}