#ifndef BUILTIN_H
#define BUILTIN_H

#include "shell.h"
#include "history.h"

typedef struct{
    char *name;
    int (*func)(t_command *command, t_shell *shell);
} BuiltinCommands;


int run_cd(t_command *command, t_shell *shell);
int run_exit(t_command *command, t_shell *shell);
int run_help(t_command *command, t_shell *shell);
int run_pwd(t_command *command, t_shell *shell);
int run_echo(t_command *command, t_shell *shell);
int run_clear(t_command *command, t_shell *shell);
int run_env(t_command *command, t_shell *shell);
int run_export(t_command *command, t_shell *shell);
int run_unset(t_command *command, t_shell *shell);
int run_jobs(t_command *command, t_shell *shell);
int run_fg(t_command *command, t_shell *shell);
int run_bg(t_command *command, t_shell *shell);

extern BuiltinCommands builtins[];

#endif