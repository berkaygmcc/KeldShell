#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h> 


#define MAX_INPUT 1024
#define MAX_ARGS 124
#define MAX_HISTORY 100
#define MAX_SEGMENTS 64
#define MAX_TOKENS 32
#define MAX_COMMANDS 32
#define MAX_REDIRECTIONS 10
#define MAX_PIPELINES 10
#define MAX_VAR_LEN 256

extern char **environ;


typedef enum{
    TOKEN_END,
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_SEMICOLON,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_BACKGROUND,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_DLPAREN,
    TOKEN_DRPAREN,
    TOKEN_REDIR_IN,
    TOKEN_REDIR_OUT,
    TOKEN_REDIR_APPEND,
    TOKEN_REDIR_ERR_OUT,
    TOKEN_REDIR_ERR_APPEND, 
    TOKEN_REDIR_HEREDOC
} t_type;

typedef enum{
    AST_COMMAND,
    AST_PIPE,
    AST_AND,
    AST_OR,
    AST_SEMICOLON,
    AST_SUBSHELL
} t_ast_type;

typedef enum{
    QUOTE_NONE,
    QUOTE_SINGLE,
    QUOTE_DOUBLE
} t_quote;

typedef struct{
    int stdin_backup;
    int stdout_backup;
    int stderr_backup;
} t_fd;


typedef struct{
    char *value;
    t_quote quote;
    int escaped;
} t_segment;

typedef struct{
    t_segment segments[MAX_SEGMENTS];
    t_type type;
    int segment_count;
} t_token;

typedef struct{
    t_type type;
    t_token target;
    int heredoc_fd;
} t_redirection;

typedef struct{
    t_token args[MAX_ARGS];
    int argc;

    t_redirection redirections[MAX_REDIRECTIONS];
    int redirection_count;
} t_command;

typedef enum{
    STATE_RUNNING,
    STATE_STOPPED,
    STATE_DONE
} t_status;

typedef struct s_process{

    pid_t pid;

    t_status status;

    char *command;

    struct s_process *next;

} t_process;

typedef struct s_job{

    int id;
    pid_t pgid;
    pid_t last_pid;

    int last_wait_status;

    t_process *process;

    char *command;

    t_status status;

    int process_count;
    int completed_count;
    int stopped_count;

    struct s_job *next;

} t_job;

typedef struct{
    int exit_status;
    pid_t pgid;
    t_job *jobs;

    int is_exit;

} t_shell;

typedef struct s_ast{

    t_ast_type type;
    int background;

    struct s_ast *left;
    struct s_ast *right;

    t_command *command;

} t_ast;

typedef struct{

    t_shell *shell;
    t_job *job;

    int interactive;
    int background;
    int wait;

} t_exec;



#endif