#ifndef AST_H
#define AST_H

#include "shell.h"

t_ast *ast_create_node(t_ast_type type, t_ast *left, t_ast *right, t_command *command);

void ast_destroy(t_ast *root);

void ast_print(t_ast *root, int depth);
char *get_t_type(t_type type);

#endif