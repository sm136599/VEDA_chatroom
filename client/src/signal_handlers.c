#include "signal_handlers.h"
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>

void setup_signal_handlers() {
    struct sigaction sigact;
    sigact.sa_handler = child_zombie_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigact, NULL);
}

void child_zombie_handler(int signo) {
    while (waitpid(0, NULL, WNOHANG) == 0);
    exit(EXIT_SUCCESS);
}