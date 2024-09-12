#define _GNU_SOURCE
#include "connection.h"
#include "server.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

int create_server_socket() {
    int ssock;
    struct sockaddr_in serveraddr;

    /* 서버 소켓 생성 */
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket not created");
        exit(EXIT_FAILURE);
    }

    /* 소켓 즉시 재사용 */
    int yes = 1;
    if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("set socket option failed");
        exit(EXIT_FAILURE);
    }

    /* 주소 구조체 주소 지정 */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(TCP_PORT);

    /* 바인딩 */
    if (bind(ssock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    /* 동시 접속 클라이언트 처리 */
    if (listen(ssock, MAX_CLIENT_NO) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    /* 서버 소켓 nonblock 처리 */
    set_nonblock(ssock);

    return ssock;
}

void handle_client_connection(int csock, struct sockaddr_in clientaddr) {
    /* 클라이언트 초과 접속 처리 */
    if (client_num >= MAX_CLIENT_NO) {
        fprintf(stderr, "Maximum number of clients reached\n");
        close(csock);
        return;
    }

    /* 클라이언트가 연결 됬을 때 */

    /* 파이프 열기 */
    pipe2(servers[client_num].from_center_to_connecting_pipe, O_NONBLOCK); // 중앙서버 -> 통신서버
    pipe2(servers[client_num].from_connecting_to_center_pipe, O_NONBLOCK); // 통신서버 -> 중앙서버 
    
    int pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    else if (pid == 0) {
        /* 자식 프로세스 (통신 서버)
        * - 클라이언트와 직접 통신 
        * - 파이프를 이용한 부모 프로세스와의 통신 */
        int my_num = client_num;
        int n_client, n_center;
        char mesg_client[BUFSIZ], mesg_center[BUFSIZ];
        char mesg[BUFSIZ];
        
        /* 네트워크 주소 출력 */
        inet_ntop(AF_INET, &clientaddr.sin_addr, mesg, BUFSIZ);
        printf("Client is conneted : %s\n", mesg);
        memset(mesg, 0, BUFSIZ);

        /* pipe 처리 */
        close(servers[my_num].from_center_to_connecting_pipe[1]); // 중앙서버에서 데이터를 받는 파이프
        close(servers[my_num].from_connecting_to_center_pipe[0]); // 중앙서버로 보내는 파이프
        
        /* 클라이언트 소켓 nonblock 처리 */
        set_nonblock(csock);

        while (1) {
            n_client = read(csock, mesg_client, BUFSIZ);

            /* 클라이언트에서 입력 받은 신호 처리 */
            if (n_client < 0) {
                if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                    perror("read failed");
                    exit(EXIT_FAILURE);
                }
            }
            else if (n_client == 0) {
                // TODO : 클라이언트 소켓 닫고, pipe 없애고 종료
                close(csock);
                break;
            }
            else {
                if (strcmp(mesg_client, "") != 0) {
                    printf("통신 서버 : %s\n", mesg_client);
                    /* 클라이언트에서 받은 데이터 중앙 서버로 전송 */
                    kill(getppid(), SIGUSR1);
                    write(servers[my_num].from_connecting_to_center_pipe[1], mesg_client, n_client);
                }
            }

            /* 중앙 서버에서 입력 받은 신호 처리 */
            while (data_from_center) {
                n_center = read(servers[my_num].from_center_to_connecting_pipe[0], mesg_center, BUFSIZ);
                data_from_center = 0;
                if (n_center < 0) {
                    if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                        perror("read failed");
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }
                else if (n_center > 0) {
                    if (strcmp(mesg_center, "") != 0) {
                        printf("Received from center : %s\n", mesg_center);
                        /* 서버에서 받은 메시지 클라이언트로 전송 */
                        write(csock, mesg_center, n_center);
                        data_from_center = 0;
                    }
                }
            }
        }
        exit(EXIT_SUCCESS);
    }
    else if (pid > 0) {
        /* 부모 프로세스 (중앙 서버)
        * 파이프를 통해 자식 프로세스에서 메시지를 받아 다른 자식들게 전송한다. */

        /* pipe 처리 */
        servers[client_num].pid = pid;
        close(servers[client_num].from_center_to_connecting_pipe[0]);
        close(servers[client_num].from_connecting_to_center_pipe[1]);
        client_num++;

        //waitpid(pid, NULL, WNOHANG);
    }
}
