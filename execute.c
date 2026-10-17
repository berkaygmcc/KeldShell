#include "execute.h"
#include "redirection.h"
#include "builtin.h"
#include "signals.h"
#include "prepare.h"
#include "fd_utils.h"
#include "cleanup.h"
#include "job_control.h"

static int is_builtin(t_command *command);
static void close_pipe(int *fd);
static int is_logical_node(t_ast *root);
static void collect_pipe(t_ast *node, t_ast **nodes, int *count);
static void cleanup_process(pid_t pid);
static void init_exec(t_exec *exec, t_shell *shell, t_job *job, int interactive, int background, int wait);
static int launch_builtin(t_command *command, t_shell *shell, int index, int in_parent);
static void launch_external(t_command *command);
static int execute_command(t_command *command, t_exec *exec);
static int execute_pipe(t_ast *root, t_exec *exec);
static int execute_subshell(t_ast *root, t_exec *exec);
static int launch_unit(t_ast *root, t_exec *exec);
static int execute_unit(t_ast *root, t_exec *exec);
static int execute_and(t_ast *root, t_exec *exec);
static int execute_or(t_ast *root, t_exec *exec);
static int execute_sequence_node(t_ast *node, t_exec *exec);
static int execute_semicolon(t_ast *root, t_exec *exec);
static int execute_background_ast(t_ast *root, t_shell *shell);
static int execute_ast(t_ast *root, t_exec *exec);


static int is_builtin(t_command *command){

    if(command == NULL || command->argc == 0){
        return -1;
    }

    for(int i = 0; builtins[i].name != NULL; i++){
        if(strcmp(command->args[0].segments[0].value, builtins[i].name) == 0){
            return i;
        }
    }
    return -1;
}

static void close_pipe(int *fd){
    if(close(fd[0]) == -1){
        perror("close");
    }
    if(close(fd[1]) == -1){
        perror("close");
    }  
}


static int is_logical_node(t_ast *root){

    if(root == NULL){
        return -1;
    }

    if(root->type == AST_AND || root->type == AST_OR || root->type == AST_SEMICOLON){
        return 0;
    }

    return -1;
}

static void collect_pipe(t_ast *node, t_ast **nodes, int *count){

    if(node == NULL){
        return;
    }

    if(node->type == AST_PIPE){
        collect_pipe(node->left, nodes, count);
        collect_pipe(node->right, nodes, count);
    }


    if(*count >= MAX_COMMANDS){
        fprintf(stderr, "keld: too many commands in pipeline\n");
        return;
    }


    nodes[*count] = node;
    (*count)++;

}

static void cleanup_process(pid_t pid){

    if(pid == 0){
        return;
    }

    if(kill(pid, SIGKILL) == -1){
        if(errno != ESRCH){
            perror("kill");
        }
    }

    int status;

    while(1){

        pid_t ret = waitpid(pid, &status, 0);

        if(ret == -1){

            if(errno == EINTR){
                continue;
            }

            break;
        }

        if(pid > 0){
            break;
        }

    }

}

static void init_exec(t_exec *exec, t_shell *shell, t_job *job, int interactive, int background, int wait){

    exec->shell = shell;
    exec->job = job;
    exec->interactive = interactive;
    exec->background = background;
    exec->wait = wait;
}

static int launch_builtin(t_command *command, t_shell *shell, int index, int in_parent){

    t_fd backup;

    if(in_parent){

        if(save_fd(&backup) == -1){
            return -1;
        }

    }

    if(apply_redirections(command) == -1){

        if(in_parent){
            restore_fd(&backup);
        }

        shell->exit_status = 1;

        return -1;
    }

    shell->exit_status = builtins[index].func(command, shell);

    if(in_parent){
        if(restore_fd(&backup) == -1){
            return -1;
        }
    }

    return shell->exit_status;
}

static void launch_external(t_command *command){

    if(apply_redirections(command) == -1){
        _exit(EXIT_FAILURE);
    }

    char *argv[MAX_ARGS + 1] = {0};

    for(int i = 0; i < command->argc; i++){
        argv[i] = command->args[i].segments[0].value;
    }

    argv[command->argc] = NULL;

    execvp(argv[0], argv);

    if (errno == ENOENT){
        fprintf(stderr, "%s: command not found\n", argv[0]);
        _exit(127);
    }
    else if (errno == EACCES){
        perror(argv[0]);
        _exit(126);
    }
    else{
        perror(argv[0]);
        _exit(1);
    }
}

static int execute_command(t_command *command, t_exec *exec){

    if(command == NULL || command->argc == 0){ 
        exec->shell->exit_status = 1;
        return -1;
    }

    pid_t pid;
    pid_t child_pid;

    int index = is_builtin(command);

    pid = fork();

    if(pid == 0){

        reset_signals();

        child_pid = getpid();

        if(exec->job->pgid == 0){
            exec->job->pgid = child_pid;
        }

        if(setpgid(child_pid, exec->job->pgid) == -1){
            perror("setpgid");
            _exit(1);
        }

        if(index != -1){
            int status = launch_builtin(command, exec->shell, index, 0);

            if(status == -1){
                _exit(1);
            }
            _exit(status);
        }

        launch_external(command);
        _exit(1);
    }
    else if(pid == -1){
        perror("fork");
        exec->shell->exit_status = 1;
        return -1;
    }
    else{
        if(exec->job->pgid == 0){
            exec->job->pgid = pid;
        }

        if(setpgid(pid, exec->job->pgid) == -1){
            if(errno != EACCES && errno != ESRCH){
                perror("setpgid");
                cleanup_process(pid);
                exec->shell->exit_status = 1;
                return -1;
            }
        }

        if(job_add_process(exec->job, pid, command->args[0].segments[0].value) == -1){
            cleanup_process(pid);
            exec->shell->exit_status = 1;
            return -1;
        }

        return 0;
    }
}

static int execute_pipe(t_ast *root, t_exec *exec){

    t_ast *nodes[MAX_COMMANDS];
    int count = 0;

    collect_pipe(root, nodes, &count);

    if(count == 0){
        exec->shell->exit_status = 1;
        return -1;
    }

    int previous_pipe = -1;

    int fd[2];
    pid_t pid;
    pid_t child_pid;
    int index;

    for(int i = 0; i < count; i++){


        if(i < count - 1){
            if(pipe(fd) == -1){
                perror("pipe");
                if(previous_pipe != -1){
                    close(previous_pipe);
                }
                cleanup_process(-exec->job->pgid);
                exec->shell->exit_status = 1;
                return -1;
            }
        }

        pid = fork();

        if(pid == 0){

            reset_signals();

            child_pid = getpid();

            if(exec->job->pgid == 0){
                exec->job->pgid = child_pid;
            }

            if(setpgid(child_pid, exec->job->pgid) == -1){
                perror("setpgid");
                _exit(1);
            }

            if(previous_pipe != -1){
                if(dup2(previous_pipe, STDIN_FILENO) == -1){
                    perror("dup2");
                    _exit(1);
                }
                if(close(previous_pipe) == -1){
                    perror("close");
                    _exit(1);
                }
            }

            if(i < count - 1){
                if(dup2(fd[1], STDOUT_FILENO) == -1){
                    perror("dup2");
                    _exit(1);
                }
                close_pipe(fd);
            }

            if(nodes[i]->type == AST_COMMAND){
                index = is_builtin(nodes[i]->command);

                if(index != -1){
                    int status;
                    
                    status = launch_builtin(nodes[i]->command, exec->shell, index, 0);
                    
                    if(status == -1){
                        _exit(1);
                    }
                    _exit(status);
                }

                launch_external(nodes[i]->command);
                _exit(1);

            }
            else if(nodes[i]->type == AST_SUBSHELL){

                t_exec child_exec;
                init_exec(&child_exec, exec->shell, exec->job, 0, exec->background, 1);

                execute_ast(nodes[i]->left, &child_exec);
                _exit(child_exec.shell->exit_status);
            }
            _exit(1);
        }
        else if(pid == -1){
            perror("fork");
            
            if(previous_pipe != -1){
                close(previous_pipe);
            }

            if(i < count - 1){
                close_pipe(fd);
            }

            if(exec->job->pgid != 0){
                cleanup_process(-exec->job->pgid);
            }
            
            exec->shell->exit_status = 1;
            return -1;
        }
        else{
            if(exec->job->pgid == 0){
                exec->job->pgid = pid;
            }

            if(setpgid(pid, exec->job->pgid) == -1){
                   
                if(errno != EACCES && errno != ESRCH){
                    perror("setpgid");

                    if(previous_pipe != -1){
                        if(close(previous_pipe) == -1){
                            perror("close");
                            return -1;
                        }
                    }

                    if(i < count - 1){
                        close_pipe(fd);
                    }

                    cleanup_process(-exec->job->pgid);
                    exec->shell->exit_status = 1;

                    return -1;
                }
            }

            char *pname = "pipe_command";

            if(nodes[i]->type == AST_COMMAND && nodes[i]->command != NULL && nodes[i]->command->argc > 0){
                pname = nodes[i]->command->args[0].segments[0].value;
            }

            if(job_add_process(exec->job, pid, pname) == -1){
                if(previous_pipe != -1){
                    if(close(previous_pipe) == -1){
                        perror("close");
                        return -1;
                    }
                }

                if(i < count - 1){
                    close_pipe(fd);
                }

                cleanup_process(-exec->job->pgid);
                exec->shell->exit_status = 1;
                return -1;
            }

            if(previous_pipe != -1){
                if(close(previous_pipe) == -1){
                    perror("close");
                    return -1;
                }
                previous_pipe = -1;
            }

            if(i < count - 1){
                if(close(fd[1]) == -1){
                    perror("close");
                    close(fd[0]);

                    exec->shell->exit_status = 1;
                    return -1;
                }
                previous_pipe = fd[0];
            }
        }
    }

    if(previous_pipe != -1){
        close(previous_pipe);
        previous_pipe = -1;
    }

    return 0;
}

static int execute_subshell(t_ast *root, t_exec *exec){

    pid_t pid = fork();

    if(pid == 0){

        reset_signals();

        pid_t child_pid = getpid();

        if(exec->job->pgid == 0){
            exec->job->pgid = child_pid;
        }

        if(setpgid(child_pid, exec->job->pgid) == -1){
            perror("setpgid");
            _exit(1);
        }

        t_exec child_exec;
        init_exec(&child_exec, exec->shell, NULL, 0, 0, 1);

        execute_ast(root->left, &child_exec);
        _exit(child_exec.shell->exit_status);
    }
    else if(pid == -1){
        perror("fork");
        exec->shell->exit_status = 1;
        return -1;
    }
    else{
        if(exec->job->pgid == 0){
            exec->job->pgid = pid;
        }

        if(setpgid(pid, exec->job->pgid) == -1){

            if(errno != EACCES && errno != ESRCH){
                perror("setpgid");
                cleanup_process(pid);
                exec->shell->exit_status = 1;
                return -1;
            }

        }

        if(job_add_process(exec->job, pid, "(subshell)") == -1){
            cleanup_process(pid);
            exec->shell->exit_status = 1;
            return -1;
        }
    }

    return 0; 
}

static int launch_unit(t_ast *root, t_exec *exec){

    if(root == NULL){
        return -1;
    }

    if(root->type == AST_COMMAND){
        return execute_command(root->command, exec);
    }
    else if(root->type == AST_PIPE){
        return execute_pipe(root, exec);
    }
    else if(root->type == AST_SUBSHELL){
        return execute_subshell(root, exec);
    }

    return 0;
}

static int execute_unit(t_ast *root, t_exec *exec){

    if(root == NULL){
        exec->shell->exit_status = 1;
        return -1;
    }

    int index = is_builtin(root->command);

    if(root->type == AST_COMMAND && exec->background != 1){
        if(index != -1){
            return launch_builtin(root->command, exec->shell, index, 1);
        }
    }

    if(exec->job == NULL){

        exec->job = job_create(exec->shell);

        if(exec->job == NULL){
            exec->shell->exit_status = 1;
            return -1;
        }

    }

    int launch_status = launch_unit(root, exec);

    if(launch_status != 0){

        exec->shell->exit_status = 1;
        remove_job(exec->shell, exec->job);
        exec->job = NULL;


        return -1;
    }

    if(exec->wait){
        if(exec->interactive){
            if(job_put_in_foreground(exec->job, exec->shell) == -1){
                exec->shell->exit_status = 1;
                return -1;
            }
        }
        else{
            if(job_wait_no_terminal(exec->job, exec->shell) == -1){
                exec->shell->exit_status = 1;
                exec->job = NULL;
                return -1;
            }
        }

        if(exec->job->status == STATE_DONE){
            remove_job(exec->shell, exec->job);
        }
    }
    else{
        if(exec->background){
            job_put_in_background(exec->job, exec->shell);

            exec->shell->exit_status = 0;
            return 0;
        }
    }

    exec->job = NULL;

    return exec->shell->exit_status;
}

static int execute_and(t_ast *root, t_exec *exec){

    execute_ast(root->left, exec);

    if(exec->shell->is_exit){
        return exec->shell->exit_status;
    }

    if(exec->shell->exit_status == 0){
        execute_ast(root->right, exec);
    }

    return exec->shell->exit_status;
}

static int execute_or(t_ast *root, t_exec *exec){

    execute_ast(root->left, exec);

    if(exec->shell->is_exit){
        return exec->shell->exit_status;
    }

    if(exec->shell->exit_status != 0){
        execute_ast(root->right, exec);
    }

    return exec->shell->exit_status; 
}

static int execute_sequence_node(t_ast *node, t_exec *exec){

    if(node == NULL){
        exec->shell->exit_status = 1;
        return -1;
    }

    if(node->background){
        if(is_logical_node(node) == 0){
            return execute_background_ast(node, exec->shell);
        }

        t_exec bg_exec;

        init_exec(&bg_exec, exec->shell, NULL, exec->interactive, 1, 0);

        return execute_ast(node, &bg_exec);
    }

    return execute_ast(node, exec);
}

static int execute_semicolon(t_ast *root, t_exec *exec){

    execute_sequence_node(root->left, exec);

    if(exec->shell->is_exit){
        return exec->shell->exit_status;
    }

    execute_sequence_node(root->right, exec);

    return exec->shell->exit_status;
}

static int execute_background_ast(t_ast *root, t_shell *shell){

    if(root == NULL){
        shell->exit_status = 1;
        return -1;
    }

    t_job *job = job_create(shell);

    if(job == NULL){
        shell->exit_status = 1;
        return -1;
    }

    pid_t pid = fork();

    if(pid == 0){

        reset_signals();

        pid_t child_pid = getpid();

        job->pgid = child_pid;

        if(setpgid(child_pid, job->pgid) == -1){
            perror("setpgid");
            _exit(1);
        }

        t_exec runner;
        init_exec(&runner, shell, NULL, 0, 0, 1);

        execute_ast(root, &runner);

        _exit(shell->exit_status);
    }
    else if(pid == -1){
        perror("fork");
        shell->exit_status = 1;
        return -1;
    }
    else{

        job->pgid = pid;

        if(setpgid(pid, job->pgid) == -1){
            if(errno != EACCES && errno != ESRCH){
                perror("setpgid");
                shell->exit_status = 1;
                return -1;
            }
        }

        if(job_add_process(job, pid, "background-runner") == -1){
            shell->exit_status = 1;
            return -1;
        }

        job_put_in_background(job, shell);

        shell->exit_status = 0;
        return 0;

    }
    
}

static int execute_ast(t_ast *root, t_exec *exec){

    if(root == NULL){
        exec->shell->exit_status = 1;
        return -1;
    }

    if(root->type == AST_COMMAND || root->type == AST_PIPE || root->type == AST_SUBSHELL){
        return execute_unit(root, exec);
    }
    else if(root->type == AST_AND){
        return execute_and(root, exec);
    }
    else if(root->type == AST_OR){
        return execute_or(root, exec);
    }
    else if(root->type == AST_SEMICOLON){
        return execute_semicolon(root, exec);
    }

    exec->shell->exit_status = 1;
    return -1;
}

int execute_top_level(t_ast *root, t_shell *shell){

    t_exec exec;

    if(root == NULL){
        shell->exit_status = 1;
        return -1;
    }

    if(root->background && is_logical_node(root) == 0){
        return execute_background_ast(root, shell);
    }

    init_exec(&exec, shell, NULL, 1, root->background, !root->background);

    return execute_ast(root, &exec);
}

