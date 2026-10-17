#include "builtin.h"
#include "shell.h"
#include "job_control.h"

int run_cd(t_command *command, t_shell *shell){
    char *target = command->args[1].segments[0].value;
    if(target == NULL || strcmp(target, "~") == 0){
        target = getenv("HOME");

        if(target == NULL){
            fprintf(stderr, "keld: HOME not set\n");
            return 1;
        }
    }
    if(chdir(target) == -1){
        perror("keld");
        shell->exit_status = 1;
        return 1;
    }

    return 0;
}

int run_exit(t_command *command, t_shell *shell){

    (void)command;

    shell->exit_status = 0;
    shell->is_exit = 1;

    return 0;
}

int run_help(t_command *command, t_shell *shell){ 
    (void)command;
    (void)shell;
    printf(
        "\n"
        "KeldShell - A Unix shell written in C\n"
        "===========================================\n"
        "\n"
        "USAGE\n"
        "  Enter a command and press Enter to execute it.\n"
        "  KeldShell supports external programs, built-in commands,\n"
        "  pipelines, redirections, logical operators, job control,\n"
        "  variable expansion, wildcards, and command history.\n"
        "\n"
        "BUILT-IN COMMANDS\n"
        "  cd [dir]             Change the current working directory.\n"
        "  pwd                  Print the current working directory.\n"
        "  echo [args]          Print arguments to standard output.\n"
        "  env                  Display environment variables.\n"
        "  export NAME=VALUE    Set or update an environment variable.\n"
        "  unset NAME           Remove an environment variable.\n"
        "  history              Display command history.\n"
        "  jobs                 List active and stopped jobs.\n"
        "  fg [job_id]          Resume a job in the foreground.\n"
        "  bg [job_id]          Resume a stopped job in the background.\n"
        "  clear                Clear the terminal screen.\n"
        "  help                 Display this help message.\n"
        "  exit                 Exit KeldShell.\n"
        "\n"
        "OPERATORS\n"
        "  |                    Pipe stdout to another command.\n"
        "  &&                   Execute the next command on success.\n"
        "  ||                   Execute the next command on failure.\n"
        "  ;                    Execute commands sequentially.\n"
        "  &                    Run a command in the background.\n"
        "  ( command )          Execute commands in a subshell.\n"
        "\n"
        "REDIRECTIONS\n"
        "  < file               Redirect standard input from a file.\n"
        "  > file               Redirect standard output to a file.\n"
        "  >> file              Append standard output to a file.\n"
        "  2> file              Redirect standard error to a file.\n"
        "  2>> file             Append standard error to a file.\n"
        "  << delimiter         Read input using a here-document.\n"
        "\n"
        "EXPANSION\n"
        "  $VAR                 Expand an environment variable.\n"
        "  ${VAR}               Expand an environment variable.\n"
        "  $?                   Expand the last command's exit status.\n"
        "  $$                   Expand the KeldShell process ID.\n"
        "  * and ?              Expand wildcard patterns.\n"
        "\n"
        "QUOTING\n"
        "  'text'               Preserve text literally.\n"
        "  \"text\"               Preserve spaces while allowing expansion.\n"
        "  \\char                Escape a special character.\n"
        "\n"
        "JOB CONTROL\n"
        "  Ctrl-C               Interrupt the foreground job.\n"
        "  Ctrl-Z               Stop the foreground job.\n"
        "  jobs                 Inspect jobs managed by KeldShell.\n"
        "  fg / bg              Resume stopped jobs.\n"
        "\n"
    );
    return 0;
 }

int run_pwd(t_command *command, t_shell *shell){
    (void)command;
    char cwd[1024];

    if(getcwd(cwd, sizeof(cwd)) != NULL){
        printf("%s\n", cwd);
    }
    else{
        perror("pwd");
        shell->exit_status = 1;
        return 1;
    }

    return 0;
}

int run_echo(t_command *command, t_shell *shell){
    (void)shell;

    int i = 1;
    while(command->args[i].segments[0].value != NULL){
        printf("%s", command->args[i].segments[0].value);

        if(command->args[i + 1].segments[0].value != NULL){ printf(" "); }
        i++;
    }

    printf("\n");

    return 0;
}

int run_clear(t_command *command, t_shell *shell){
    (void)command;
    (void)shell;
    printf("\033[3J\033[2J\033[H");
    return 0;
}

int run_env(t_command *command, t_shell *shell){
    (void)command;
    (void)shell;
    for(int i = 0; environ[i] != NULL; i++){
        printf("%s\n", environ[i]);
    } 
    return 0;
}

int run_export(t_command *command, t_shell *shell){
    if(command->args[1].segments[0].value == NULL){
        run_env(command, shell);
    }
    else{
        char *equal = strchr(command->args[1].segments[0].value, '=');

        if(equal == command->args[1].segments[0].value){
            fprintf(stderr, "keld: invalid identifier\n");
            return 1;
        }

        if(equal == NULL){
            fprintf(stderr, "keld: invalid format\n");
            return 1;
        }

        *equal = '\0';

        char *key = command->args[1].segments[0].value;
        char *value = equal + 1;

        if (setenv(key, value, 1) == -1) {
            perror("export");
            shell->exit_status = 1;
            return 1;
        }
    }

    return 0;
}

int run_unset(t_command *command, t_shell *shell){
    
    if(command->args[1].segments[0].value == NULL){
        printf("invalid usage!");
        return 1;
    }
    else{
        if(unsetenv(command->args[1].segments[0].value) == -1){
            perror("keld: unsetenv");
            shell->exit_status = 1;
            return 1;
        }
    }

    return 0;
}

static char *get_job_status(t_status status){
    if(status == STATE_RUNNING){
        return "Running";
    }
    else if(status == STATE_STOPPED){
        return "Stopped";
    }
    else if(status == STATE_DONE){
        return "Done";
    }
    return NULL;
}

int run_jobs(t_command *command, t_shell *shell){

    reap_background_jobs(shell);

    if(command == NULL || shell == NULL){
        return -1;
    }

    t_job *job = shell->jobs;

    while(job != NULL){

        printf("[%d] %-8s pgid: %ld\n", job->id, get_job_status(job->status), (long)job->pgid);
        job = job->next;
    }

    return 0;
}

int run_fg(t_command *command, t_shell *shell){

    if(command->argc != 2){
        fprintf(stderr, "fg: usage: fg <job_id>\n");
        return -1;
    }

    int id = atoi(command->args[1].segments[0].value);

    if(id <= 0){
        fprintf(stderr, "fg: invalid job id\n");
        return 1;
    }

    t_job *job = find_job_by_id(shell, id);

    if(job == NULL){
        fprintf(stderr, "fg: job not found: %d\n", id);
        return -1;
    }

    if(job->status == STATE_DONE){
        fprintf(stderr, "fg: job has already terminated\n");
        return -1;
    }

    if(job_continue_foreground(job, shell) == -1){
        return -1;
    }

    if(job->status == STATE_DONE){
        remove_job(shell, job);
    }

    return shell->exit_status;

}

int run_bg(t_command *command, t_shell *shell){

    if(command->argc != 2){
        fprintf(stderr, "bg: usage: fg <job_id>\n");
        return -1;
    }
    
    int id = atoi(command->args[1].segments[0].value);

    if(id <= 0){
        fprintf(stderr, "bg: invalid job id\n");
        return 1;
    }

    t_job *job = find_job_by_id(shell, id);

    if(job == NULL){
        fprintf(stderr, "bg: job not found: %d\n", id);
        return -1;
    }

    if(job->status == STATE_DONE){
        fprintf(stderr, "bg: job has already terminated\n");
        return -1;
    }

    if(job_put_in_background(job, shell) == -1){
        return -1;
    }

    return shell->exit_status;

}




BuiltinCommands builtins[] = {
    {"cd", &run_cd},
    {"exit", &run_exit},
    {"help", &run_help},
    {"pwd", &run_pwd},
    {"echo", &run_echo},
    {"clear", &run_clear},
    {"env", &run_env},
    {"export", &run_export},
    {"unset", &run_unset},
    {"jobs", &run_jobs},
    {"fg", &run_fg},
    {"bg", &run_bg},
    {"history", &run_history},
    {NULL, NULL} 
};