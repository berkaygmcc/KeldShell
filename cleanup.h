#ifndef CLEANUP_H
#define CLEANUP_H

#include "shell.h"

void free_command(t_command *command);
void free_token_list(t_token *token_list);
int free_argv(char **argv);
int free_matches(char **matches, int match_count);
void cleanup_all(t_ast *root, t_token *token_list);


#endif