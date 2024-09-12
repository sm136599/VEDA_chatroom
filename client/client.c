#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>

#define TCP_PORT 5125

volatile sig_atomic_t data_received = 0;

void child_zombie_handler(int signo) {
    while (waitpid(0, NULL, WNOHANG) == 0);
    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
    int ssock;
    struct sockaddr_in serveraddr;
    char mesg[BUFSIZ];
    int child_pid;

    if (argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* child 좀비 문제 해결 */
    struct sigaction sigact;
    sigact.sa_handler = child_zombie_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigact, NULL);

    /* 소켓 할당 */
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket allocated failed");
        exit(EXIT_FAILURE);
    }

    /* 주소 초기화 및 포트 설정 */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    inet_pton(AF_INET, argv[1], &(serveraddr.sin_addr.s_addr));
    serveraddr.sin_port = htons(TCP_PORT);

    // /* 서버 소켓 nonblock 처리 */
    // fcntl(ssock, F_SETFL, O_NONBLOCK);

    /* 연결 */
    if (connect(ssock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0) {
        /* 자식 프로세스 처리 
            * - 키보드에서 입력 받아 부모 프로세스로 데이터 전달 */

        while (1) {
            /* 키보드에서 입력 받아 서버에 전송 */
            memset(mesg, 0, BUFSIZ);
            fgets(mesg, BUFSIZ, stdin);
            mesg[strlen(mesg) - 1] = '\0';

            if (strncmp(mesg, "logout", 6) == 0) {
                break;
            }

            if (send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0) { 
                perror("send failed");
                exit(EXIT_FAILURE);
            }
        }
        exit(EXIT_SUCCESS);
    }
    else {
        /* 부모 프로세스 처리 
         * - 서버에서 데이터를 받아 출력함. */
        int n;
        while (1) {
            /* 서버에서 데이터를 받아 화면에 출력 */
            memset(mesg, 0, BUFSIZ);
            n = read(ssock, mesg, BUFSIZ);
            if (n < 0) {
                perror("read failed");
                exit(EXIT_FAILURE);
            }
            else if (n == 0) {
                printf("server closed\n");
                close(ssock);
                exit(EXIT_FAILURE);
            }
            else {
                if (strcmp(mesg, ""))
                    printf("%s\n", mesg);
            }
        }
    }
    return 0;
}