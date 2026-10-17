#ifndef UTILS_H
#define UTILS_H

#include "shell.h"


int is_variable_start(char c);
int is_variable(char c);
int copy_token(t_token *dst, t_token *src);
void move_token(t_token *dst, t_token *src);


#endif