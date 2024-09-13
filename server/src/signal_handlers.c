#define _GNU_SOURCE

#include "signal_handlers.h"
#include "server.h"
#include <signal.h>
#include <sys/wait.h>

void setup_signal_handlers() {
    /* 서버간 데이터 송수신 및 자식 프로세스 좀비화 방지 시그널 처리 */
    struct sigaction sigact;

    sigact.sa_handler = child_zombie_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigact, NULL);

    /* SIGUSR1 : 통신 서버 -> 중앙 서버 데이터 보낼 시 */    
    sigact.sa_handler = sigusr1_handler;
    sigaction(SIGUSR1, &sigact, NULL);

    /* SIGUSR1 : 중앙 서버 -> 통신 서버 데이터 보낼 시 */    
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
