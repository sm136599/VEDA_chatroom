#define _GNU_SOURCE

#include "signal_handlers.h"
#include "server.h"
#include <signal.h>
#include <sys/wait.h>

void setup_signal_handlers() {
    struct sigaction sigact;

    sigact.sa_handler = child_zombie_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigact, NULL);

    
    sigact.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sigact, NULL);

    
    sigact.sa_handler = sigusr2_handler;
    sigaction(SIGUSR2, &sigact, NULL);
}
void child_zombie_handler(int signo) {
    while (waitpid(0, NULL, WNOHANG) == 0);
}
void sigusr1_handler(int signo) {
    data_from_connecting = 1;
}
void sigusr2_handler(int signo) {
    data_from_center = 1;
}
