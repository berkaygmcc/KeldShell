#include "job_control.h"

int init_job_control(t_shell *shell){

    if(shell == NULL){
        return -1;
    }

    pid_t shell_pgid = getpid();

    if(setpgid(shell_pgid, shell_pgid) == -1){
        perror("setpgid");
        shell->exit_status = 1;
        return -1;
    }

    shell->pgid = shell_pgid;

    if(tcsetpgrp(STDIN_FILENO, shell->pgid) == -1){
        perror("tcsetpgrp");
        shell->exit_status = 1;
        return -1;
    }

    return 0;
}

void free_job(t_job *job){

    if(job == NULL){
        return;
    }

    t_process *process = job->process;

    t_process *next;

    while(process != NULL){

        next = process->next;

        free(process);

        process = next;

    }

    free(job);
}

t_job *job_create(t_shell *shell){

    t_job *job = malloc(sizeof(t_job));

    if(job == NULL){
        perror("malloc");
        return NULL;
    }

    job->id = 0;
    job->pgid = 0;
    job->last_pid = 0;

    job->last_wait_status = 0;

    job->process = NULL;

    job->command = NULL;

    job->status = STATE_RUNNING;

    job->process_count = 0;
    job->completed_count = 0;
    job->stopped_count = 0;

    job->next = NULL;

    if(shell->jobs == NULL){
        job->id = 1;
        shell->jobs = job;
    }
    else{
        t_job *current_job = shell->jobs;
        while(current_job->next != NULL){
            current_job = current_job->next;
        }
        job->id = current_job->id + 1;
        current_job->next = job;
    }

    return job;
}

static t_process *process_create(pid_t pid, char *command){

    t_process *process = malloc(sizeof(t_process));

    if(process == NULL){
        perror("malloc");
        return NULL;
    }

    process->pid = pid;
    process->status = STATE_RUNNING;
    process->command = command;
    process->next = NULL;

    return process;
}

int job_add_process(t_job *job, pid_t pid, char *command){

    if(job == NULL || command == NULL){
        return -1;
    }

    t_process *process = process_create(pid, command);

    if(process == NULL){
        return -1;
    }

    if(job->process == NULL){
        job->process = process;
    }
    else{
        t_process *current = job->process;

        while(current->next != NULL){
            current = current->next;
        }

        current->next = process;
    }

    job->process_count++;
    job->last_pid = pid;

    return 0;
}

static void update_job_status(t_job *job){

    if(job == NULL){
        return;
    }

    job->completed_count = 0;
    job->stopped_count = 0;

    t_process *process = job->process;

    while(process != NULL){
        if(process->status == STATE_DONE){
            job->completed_count++;
        }
        else if(process->status == STATE_STOPPED){
            job->stopped_count++;
        }

        process = process->next;
    }

    if(job->completed_count == job->process_count){
        job->status = STATE_DONE;
    }
    else if(job->completed_count + job->stopped_count == job->process_count){
        job->status = STATE_STOPPED;
    }
    else{
        job->status = STATE_RUNNING;
    }

}

static t_process *find_process(t_job *job, pid_t pid){

    t_process *process = job->process;

    while(process != NULL){
        if(process->pid == pid){
            return process;
        }

        process = process->next;
    }

    return NULL;
}

t_job *find_job_by_id(t_shell *shell, int id){

    if(shell == NULL || id <= 0){
        return NULL;
    }

    t_job *job;

    job = shell->jobs;

    while(job != NULL){

        if(job->id == id){
            return job;
        }

        job = job->next;

    }

    return NULL;
}

t_job *find_job_by_pid(t_shell *shell, pid_t pid){

    if(shell == NULL){
        return NULL;
    }

    t_job *job;

    job = shell->jobs;

    t_process *process;

    while(job != NULL){

        process = job->process;

        while(process != NULL){
            if(process->pid == pid){
                return job;
            }

            process = process->next;
        }

        job = job->next;

    }

    return NULL;
}

static void update_process_status(t_job *job, pid_t pid, int status){

    t_process *process = find_process(job, pid);

    if(process == NULL){
        return;
    }

    if(WIFEXITED(status) || WIFSIGNALED(status)){
        process->status = STATE_DONE;
    }
    else if(WIFSTOPPED(status)){
        process->status = STATE_STOPPED;
    }
    else if(WIFCONTINUED(status)){
        process->status = STATE_RUNNING;
    }

    update_job_status(job);
}



void remove_job(t_shell *shell, t_job *job){

    if(shell == NULL || job == NULL){
        return;
    }

    t_job *current = shell->jobs;
    t_job *previous = NULL;

    while(current != NULL){
        if(current == job){
            if(previous == NULL){
                shell->jobs = current->next;
            }
            else{
                previous->next = current->next;
            }

            free_job(current);
            return;
        }

        previous = current;
        current = current->next;
    }

}

static void signal_newline(t_job *job){

    int sig;

    if(job == NULL){
        return;
    }

    if(WIFSIGNALED(job->last_wait_status)){

        sig = WTERMSIG(job->last_wait_status);

        if(sig == SIGINT || sig == SIGQUIT){
            write(STDOUT_FILENO, "\n", 1);
        }
    }
    else if(WIFSTOPPED(job->last_wait_status)){

        sig = WSTOPSIG(job->last_wait_status);

        if(sig == SIGTSTP){
            write(STDOUT_FILENO, "\n", 1);
        }
    }

}

int job_put_in_background(t_job *job, t_shell *shell){ 

    if(job == NULL || job->pgid <= 0 || shell == NULL){
        return -1;
    }

    if(job->status == STATE_STOPPED){
        if(kill(-job->pgid, SIGCONT) == -1){
            perror("kill");
            shell->exit_status = 1;
            return -1;
        }

        t_process *process = job->process;

        while(process != NULL){
            if(process->status == STATE_STOPPED){
                process->status = STATE_RUNNING;
            }
            process = process->next;
        }
    }

    job->status = STATE_RUNNING;

    printf("[%d] pgid: %d\n", job->id, job->pgid);

    return 0;
}

void reap_background_jobs(t_shell *shell){

    if(shell == NULL){
        return;
    }

    pid_t pid;
    int status;

    while(1){
        pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);

        if(pid == 0){
            break;
        }
        else if(pid == -1){
            if(errno == EINTR){
                continue;
            }

            if(errno != ECHILD){
                perror("waitpid");
            }

            break;
        }


        t_job *job = find_job_by_pid(shell, pid);

        if(job == NULL){
            continue;
        }

        update_process_status(job, pid, status);

        if(job->status == STATE_DONE){

            printf("[%d] Done\n", job->id);

            remove_job(shell, job);
        }
    }
}

int wait_for_job_status(t_job *job, t_shell *shell){

    if(job == NULL || job->pgid <= 0){
        return -1;
    }
    
    int status;
    pid_t pid;

    while(job->status == STATE_RUNNING){

        pid = waitpid(-job->pgid, &status, WUNTRACED | WCONTINUED);

        if(pid == -1){
            if(errno == EINTR){
                continue;
            }

            if(errno == ECHILD){
                break;
            }

            perror("waitpid");
            return -1;
        }

        update_process_status(job, pid, status);

        if(pid == job->last_pid){

            job->last_wait_status = status;

            if(WIFEXITED(status)){
                shell->exit_status = WEXITSTATUS(status);
            }
            else if(WIFSIGNALED(status)){
                shell->exit_status = 128 + WTERMSIG(status);
            }
            else if(WIFSTOPPED(status)){
                shell->exit_status = 128 + WSTOPSIG(status);
            }
        }
    }

    return 0;
}

int job_continue_foreground(t_job *job, t_shell *shell){

    if(job == NULL || job->pgid <= 0 || shell == NULL){
        return -1;
    }

    if(tcsetpgrp(STDIN_FILENO, job->pgid) == -1){
        perror("tcsetpgrp");
        shell->exit_status = 1;
        return -1;
    }

    if(job->status == STATE_STOPPED){

        if(kill(-job->pgid, SIGCONT) == -1){
            perror("kill");
            if(tcsetpgrp(STDIN_FILENO, shell->pgid) == -1){
                perror("tcsetpgrp");
            }
            shell->exit_status = 1;
            return -1;
        }

        t_process *process = job->process;

        while(process != NULL){
            if(process->status == STATE_STOPPED){
                process->status = STATE_RUNNING;
            }
            process = process->next;
        }

        job->status = STATE_RUNNING;

    }

    if(wait_for_job_status(job, shell) == -1){
        if(tcsetpgrp(STDIN_FILENO, shell->pgid) == -1){
            perror("tcsetpgrp");
        }
        shell->exit_status = 1;
        return -1;
    }

    if(tcsetpgrp(STDIN_FILENO, shell->pgid) == -1){
        perror("tcsetpgrp");
        shell->exit_status = 1;
        return -1;
    }

    signal_newline(job);

    return 0;

}

int job_put_in_foreground(t_job *job, t_shell *shell){

    if(job == NULL || job->pgid <= 0 || shell == NULL){
        return -1;
    }

    if(job->pgid <= 0){
        shell->exit_status = 1;
        return -1;
    }

    if(tcsetpgrp(STDIN_FILENO, job->pgid) == -1){
        perror("tcsetpgrp");
        shell->exit_status = 1;
        return -1;
    }

    if(wait_for_job_status(job, shell) == -1){
        if(tcsetpgrp(STDIN_FILENO, shell->pgid) == -1){
            perror("tcsetpgrp");
        }
        shell->exit_status = 1;
        return -1;
    }

    if(tcsetpgrp(STDIN_FILENO, shell->pgid) == -1){
        perror("tcsetpgrp");
        shell->exit_status = 1;
        return -1;
    }

    signal_newline(job);

    return 0;
}

int job_wait_no_terminal(t_job *job, t_shell *shell){

    if(job == NULL || shell == NULL){
        return -1;
    }

    if(wait_for_job_status(job, shell) == -1){
        shell->exit_status = 1;
        return -1;
    }

    return 0;
}


