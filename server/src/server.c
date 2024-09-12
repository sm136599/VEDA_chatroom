#include "server.h"
#include "connection.h"
#include "signal_handlers.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

volatile int data_from_connecting = 0;
volatile int data_from_center = 0;
int client_num = 0;

connecting_server servers[MAX_CLIENT_NO];

void init_server() {
    /* 데몬 만들기 */
    int pid;
    if ((pid = fork()) < 0) {
        perror("init fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    /* 새 세션 만들기 */
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }
    /* 디렉토리 변경 */
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }
    /* 권한 변경 */
    umask(0);
}

void run_server() {
    int ssock = create_server_socket();
    setup_signal_handlers();

    struct sockaddr_in clientaddr;
    socklen_t clen = sizeof(clientaddr);

    char mesg[BUFSIZ];
    int n;

    while (1) {
        int csock = accept(ssock, (struct sockaddr*)&clientaddr, &clen);

        if (csock == -1) {
            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                perror("accept failed");
                break;
            }
        }
        else if (csock > 0) {
            handle_client_connection(csock, clientaddr);
        }

        /* 통신 서버의 입력을 받는다. */
        for (int i = 0; i < client_num; i++) {
            n = read(servers[i].from_connecting_to_center_pipe[0], mesg, BUFSIZ);
            if (n < 0) {
                if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                    perror("read failed");
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            else if (n == 0) {
                // 통신 서버 종료
                // TODO : servers 배열 업데이트 
                printf("%d client server stoped\n", i);
                for (int j = i; j < client_num - 1; j++) {
                    servers[j] = servers[j+1];
                }
                client_num--;
            }
            else if (data_from_connecting && n > 0) {
                if (strcmp(mesg, "") != 0) {
                    printf("Received from %d connecting server : %s\n", i, mesg);
                    /* 통신 서버에서 전송된 메시지 나머지 통신 서버에 전송 */
                    for (int j = 0; j < client_num; j++) {
                        if (i != j) {
                            kill(servers[j].pid, SIGUSR2);
                            write(servers[j].from_center_to_connecting_pipe[1], mesg, n);
                        }
                    }
                    data_from_connecting = 0;
                }
            }
        }
    }

    close(ssock);
}