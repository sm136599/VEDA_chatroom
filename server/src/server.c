#define _GNU_SOURCE

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
#include <sys/socket.h>

volatile int data_from_connecting = 0;
volatile int data_from_center = 0;
int client_num = 0;
int user_count = 0;
connecting_server servers[MAX_CLIENT_NO];
user users[1024];
int connected_user[1024];

void init_server() {
    // make_daemon();
    load_data();
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
        usleep(100);

        if (csock == -1) {
            if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                perror("accept failed");
                break;
            }
        }
        else if (csock > 0) {
            handle_child_process(csock, clientaddr);
        }

        /* 통신 서버의 입력을 받는다. */
        for (int i = 0; i < client_num; i++) {
            n = read(servers[i].from_connecting_to_center_pipe[0], mesg, BUFSIZ);
            usleep(100);
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

void handle_child_process(int csock, struct sockaddr_in clientaddr) {
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
        int user_index;
        
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
            usleep(100);

            /* 클라이언트에서 입력 받은 신호 처리 */
            if (n_client < 0) {
                if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
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
                    if (strncmp(mesg_client, "LOGIN", 5) == 0) {
                        user user;
                        char result[BUFSIZ];
                        get_user_from_string(mesg_client, &user);

                        user_index = login_user(&user);
                        printf("%d, %s %s\n", user_index, user.username, user.password);
                        if (user_index >= 0) {
                            strcpy(result, "SUCCESS");
                        }
                        else {
                            strcpy(result, "FAIL");
                        }
                        write(csock, result, BUFSIZ);
                    }
                    else if (strncmp(mesg_client, "REGISTER", 8) == 0) {
                        user user;
                        char result[BUFSIZ];
                        get_user_from_string(mesg_client, &user);

                        user_index = register_user(&user);
                        if (user_index >= 0) {
                            save_user_data(&user);
                            strcpy(result, "SUCCESS");
                        }
                        else {
                            strcpy(result, "FAIL");
                        }
                        write(csock, result, BUFSIZ);
                    }
                    else if (strncmp(mesg_client, "LOGOUT", 6) == 0) {
                        connected_user[user_index] = 0;
                        user_index = -1;
                    }
                    else {
                        kill(getppid(), SIGUSR1);
                        write(servers[my_num].from_connecting_to_center_pipe[1], mesg_client, n_client);
                    }
                }
            }

            /* 중앙 서버에서 입력 받은 신호 처리 */
            while (data_from_center) {
                n_center = read(servers[my_num].from_center_to_connecting_pipe[0], mesg_center, BUFSIZ);
                usleep(100);
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
