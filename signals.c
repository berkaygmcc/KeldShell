#include "signals.h"

volatile sig_atomic_t interrupted = 0;

void signal_handler(int sig){
    (void)sig;
    interrupted = 1;
}

int prompt_event_hook(void){
    if(interrupted){
        rl_replace_line("", 0);
        rl_done = 1;
    }

    return 0;
}

int heredoc_event_hook(void){
    if(interrupted){
        rl_done = 1;
        rl_clear_pending_input();
    }

    return 0;
}

void heredoc_signal_handler(int sig){
    (void)sig;
    interrupted = 1;
}

void setup_signals(){

    rl_catch_signals = 0;
    rl_event_hook = prompt_event_hook;

    struct sigaction signal_sigint = {0};

    signal_sigint.sa_handler = signal_handler;
    sigemptyset(&signal_sigint.sa_mask);
    signal_sigint.sa_flags = 0;

    if(sigaction(SIGINT, &signal_sigint, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction signal_sigquit = {0};

    signal_sigquit.sa_handler = SIG_IGN;
    sigemptyset(&signal_sigquit.sa_mask);
    signal_sigquit.sa_flags = 0;

    if(sigaction(SIGQUIT, &signal_sigquit, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction signal_sigtstp = {0};

    signal_sigtstp.sa_handler = SIG_IGN;
    sigemptyset(&signal_sigtstp.sa_mask);
    signal_sigtstp.sa_flags = 0;

    if(sigaction(SIGTSTP, &signal_sigtstp, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction signal_sigttin = {0};

    signal_sigttin.sa_handler = SIG_IGN;
    sigemptyset(&signal_sigttin.sa_mask);
    signal_sigttin.sa_flags = 0;

    if(sigaction(SIGTTIN, &signal_sigttin, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction signal_sigttou = {0};

    signal_sigttou.sa_handler = SIG_IGN;
    sigemptyset(&signal_sigttou.sa_mask);
    signal_sigttou.sa_flags = 0;

    if(sigaction(SIGTTOU, &signal_sigttou, NULL) == -1){
        perror("sigaction");
    }
}


void setup_heredoc_signals(){

    rl_catch_signals = 0; 
    rl_event_hook = heredoc_event_hook;

    struct sigaction heredoc_sigint = {0};
    heredoc_sigint.sa_handler = heredoc_signal_handler;
    sigemptyset(&heredoc_sigint.sa_mask);
    heredoc_sigint.sa_flags = 0;

    if(sigaction(SIGINT, &heredoc_sigint, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction heredoc_sigquit = {0};

    heredoc_sigquit.sa_handler = SIG_IGN;
    sigemptyset(&heredoc_sigquit.sa_mask);
    heredoc_sigquit.sa_flags = 0;

    if(sigaction(SIGQUIT, &heredoc_sigquit, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction heredoc_sigtstp = {0};

    heredoc_sigtstp.sa_handler = SIG_IGN;
    sigemptyset(&heredoc_sigtstp.sa_mask);
    heredoc_sigtstp.sa_flags = 0;

    if(sigaction(SIGTSTP, &heredoc_sigtstp, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction heredoc_sigttin = {0};

    heredoc_sigttin.sa_handler = SIG_IGN;
    sigemptyset(&heredoc_sigttin.sa_mask);
    heredoc_sigttin.sa_flags = 0;

    if(sigaction(SIGTTIN, &heredoc_sigttin, NULL) == -1){
        perror("sigaction");
    }

    struct sigaction heredoc_sigttou = {0};

    heredoc_sigttou.sa_handler = SIG_IGN;
    sigemptyset(&heredoc_sigttou.sa_mask);
    heredoc_sigttou.sa_flags = 0;

    if(sigaction(SIGTTOU, &heredoc_sigttou, NULL) == -1){
        perror("sigaction");
    }
}

void reset_signals(){
    struct sigaction signal = {0};

    signal.sa_handler = SIG_DFL;
    sigemptyset(&signal.sa_mask);
    signal.sa_flags = 0;

    if(sigaction(SIGINT, &signal, NULL) == -1){
        perror("sigaction");
    } 
    if(sigaction(SIGQUIT, &signal, NULL) == -1){
        perror("sigaction");
    } 
    if(sigaction(SIGTSTP, &signal, NULL) == -1){
        perror("sigaction");
    } 
    if(sigaction(SIGTTIN, &signal, NULL) == -1){
        perror("sigaction");
    } 
    if(sigaction(SIGTTOU, &signal, NULL) == -1){
        perror("sigaction");
    } 
}
