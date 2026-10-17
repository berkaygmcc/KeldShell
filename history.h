#ifndef HISTORY_H
#define HISTORY_H

#include "shell.h"

extern char *history[MAX_HISTORY];
extern int history_count;

void load_history(void);
void append_history_to_file(char *input);
void rewrite_history_file(void);
void save_history(char *input);
int run_history(t_command *command, t_shell *shell);
void free_resources(void);


#endif