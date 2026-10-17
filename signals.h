#ifndef SIGNAL_H
#define SIGNAL_H

#include "shell.h"

extern volatile sig_atomic_t interrupted;

void signal_handler(int signo);
void heredoc_signal_handler(int sig);
void wait_handler(int sig);
void setup_signals();
void setup_wait_signals();
void setup_heredoc_signals();
void reset_signals();


#endif