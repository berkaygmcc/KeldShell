#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include "shell.h"

t_job *job_create(t_shell *shell);
t_job *find_job_by_id(t_shell *shell, int id);
t_job *find_job_by_pid(t_shell *shell, pid_t pid);
int init_job_control(t_shell *shell);
int job_add_process(t_job *job, pid_t pid, char *command_name);
int job_continue_foreground(t_job *job, t_shell *shell);
int job_put_in_background(t_job *job, t_shell *shell);
int job_put_in_foreground(t_job *job, t_shell *shell);
int job_wait_no_terminal(t_job *job, t_shell *shell);
void free_job(t_job *job);
void remove_job(t_shell *shell, t_job *job);
void reap_background_jobs(t_shell *shell);

#endif