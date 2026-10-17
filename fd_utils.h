#ifndef FD_UTILS_H
#define FD_UTILS_H

#include "shell.h"

int save_fd(t_fd *backup);
int restore_fd(t_fd *backup);
int close_fd(int *fd, int index);

#endif