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
    make_daemon();
    load_data();
}

void run_server() {
    /* 서버 소켓 열기 */
    int ssock = create_server_socket();
    /* 시그널 핸들러 초기 설정 */
    setup_signal_handlers();

    struct sockaddr_in clientaddr;
    socklen_t clen = sizeof(clientaddr);

    char mesg[BUFSIZ];
    int n;

    while (1) {
        /* 부모 프로세스 (중앙 서버)
         * 파이프를 통해 자식 프로세스에서 메시지를 받아 다른 자식들게 전송한다. */

        /* 클라이언트 소켓 기다리기 */
        int csock = accept(ssock, (struct sockaddr*)&clientaddr, &clen);
        usleep(100);

        if (csock == -1) {
            if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                perror("accept failed");
                break;
            }
        }
        else if (csock > 0) {
            /* 클라이언트 연결 성공 시 */
            handle_child_process(csock, clientaddr);
        }

        /* 클라이언트가 연결 신호가 없을 때
           통신 서버로 부터의 입력 신호를 체크한다. */
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
                /* 통신 서버 종료 */
                /* 유저 로그아웃 처리하기 */
                if (servers[i].user_index >= 0) {
                    connected_user[servers[i].user_index] = 0;
                    servers[i].user_index = -1;
                }

                /* 현재 접속한 클라이언트 처리 */
                for (int j = i; j < client_num - 1; j++) {
                    servers[j] = servers[j+1];
                }
                client_num--;
                close(csock);
            }
            else if (data_from_connecting && n > 0) {
                /* 데이터가 통신 서버로 부터 올 때 */
                if (strcmp(mesg, "") != 0) {

                    /* 로그인, 회원가입 처리 */
                    if (strncmp(mesg, "LOGIN", 5) == 0) {
                        /* 로그인 처리 */
                        user user;
                        char result[BUFSIZ];
                        get_user_from_string(mesg, &user);

                        int user_index = login_user(&user);
                        if (user_index >= 0) {
                            strcpy(result, "SUCCESS");
                            servers[i].user_index = user_index;
                        }
                        else {
                            strcpy(result, "FAIL");
                        }
                        write(servers[i].from_center_to_connecting_pipe[1], result, BUFSIZ);
                    }
                    else if (strncmp(mesg, "REGISTER", 8) == 0) {
                        /* 회원가입 처리 */
                        user user;
                        char result[BUFSIZ];
                        get_user_from_string(mesg, &user);

                        int user_index = register_user(&user);
                        if (user_index >= 0) {
                            save_user_data(&user);
                            strcpy(result, "SUCCESS");
                            servers[i].user_index = user_index;
                        }
                        else {
                            strcpy(result, "FAIL");
                        }
                        write(servers[i].from_center_to_connecting_pipe[1], result, BUFSIZ);
                    }
                    else if (strncmp(mesg, "LOGOUT", 6) == 0) {
                        /* 로그아웃 처리 */
                        connected_user[servers[i].user_index] = 0;
                        servers[i].user_index = -1;
                    }
                    else {
                        /* 나머지 신호 처리 */
                        /* 통신 서버에서 전송된 메시지 나머지 통신 서버에 전송 */
                        for (int j = 0; j < client_num; j++) {
                            if (i != j) {
                                kill(servers[j].pid, SIGUSR2);
                                write(servers[j].from_center_to_connecting_pipe[1], mesg, n);
                            }
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
        exit(EXIT_FAILURE);
    }

    /* 클라이언트가 연결 됬을 때 */

    /* 파이프 열기 */
    pipe2(servers[client_num].from_center_to_connecting_pipe, O_NONBLOCK);
    pipe2(servers[client_num].from_connecting_to_center_pipe, O_NONBLOCK);
    
    int pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return;
    }
    else if (pid == 0) {
        /* 자식 프로세스 (통신 서버)
         * - 클라이언트와 직접 통신 
         * - 파이프를 이용한 부모 프로세스와의 통신 */
        int my_num = client_num;                        // 자신의 정보 인덱스 저장
        int n_client, n_center;                         // 클라이언트와 중앙 서버에서 받은 데이터 길이
        char mesg_client[BUFSIZ], mesg_center[BUFSIZ];  // 클라이언트와 중앙 서버에서 받은 데이터
        char mesg[BUFSIZ];                              // 일반적인 메시지 처리
        
        /* 네트워크 주소 출력 */
        inet_ntop(AF_INET, &clientaddr.sin_addr, mesg, BUFSIZ);
        printf("Client is conneted : %s\n", mesg);
        memset(mesg, 0, BUFSIZ);

        /* pipe 처리 */
        close(servers[my_num].from_center_to_connecting_pipe[1]); 
        close(servers[my_num].from_connecting_to_center_pipe[0]);

        /* 로그인 및 회원가입을 위한 block 처리 */
        set_block(csock);
        set_block(servers[my_num].from_center_to_connecting_pipe[0]);

        while (1) {
            /* 클라이언트로 부터 메시지 수신 (최초엔 블록 처리)*/
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
                /* 클라이언트에서 메시지 수신 */
                if (strcmp(mesg_client, "") != 0) {
                    /* 클라이언트에서 받은 데이터 중앙 서버로 전송 */
                    if (strncmp(mesg_client, "LOGIN", 5) == 0 ||
                        strncmp(mesg_client, "REGISTER", 8) == 0) {
                        /* 로그인과 회원가입 메시지 처리 */
                        /* 입력 버퍼 비우기 */
                        while (read(servers[my_num].from_center_to_connecting_pipe[0], mesg_center, BUFSIZ) > 0);
                        /* 중앙 서버로 메시지 전송*/
                        kill(getppid(), SIGUSR1);
                        write(servers[my_num].from_connecting_to_center_pipe[1], mesg_client, BUFSIZ);
                        usleep(100000);
                        /* 중앙 서버로 부터 성공/실패 메시지 수신*/
                        read(servers[my_num].from_center_to_connecting_pipe[0], mesg_center, BUFSIZ);
                        /* 클라이언트로 성공/실패 메시지 전송 */
                        write(csock, mesg_center, BUFSIZ);
                        
                        /* 로그인과 회원가입이 끝난 후 파이프와 클라이언트 소켓 nonblock 처리 */
                        set_nonblock(servers[my_num].from_center_to_connecting_pipe[0]);
                        set_nonblock(csock);
                    }
                    else if (strncmp(mesg_client, "LOGOUT", 6) == 0) {
                        /* 로그아웃 신호 시 */
                        kill(getppid(), SIGUSR1);
                        write(servers[my_num].from_connecting_to_center_pipe[1], mesg_client, BUFSIZ);

                        /* 다시 로그인과 회원가입 처리를 위한 block 처리*/
                        set_block(servers[my_num].from_center_to_connecting_pipe[0]);
                        set_block(csock);
                    }
                    else {
                        /* 일반 메시지 처리 */
                        kill(getppid(), SIGUSR1);
                        write(servers[my_num].from_connecting_to_center_pipe[1], mesg_client, n_client);
                    }
                }
            }

            /* 중앙 서버에서 입력 받은 메시지 처리 */
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
                    /* 중앙 서버에서 받은 메시지는 클라이언트로 전송 */
                    if (strcmp(mesg_center, "") != 0) {
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
