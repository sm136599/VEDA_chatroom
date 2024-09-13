#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <arpa/inet.h>

#define MAX_CLIENT_NO       50  
#define TCP_PORT            5125
#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 32

/* 통신(자식) 서버 정보 구조체
   중앙(부모) 서버에서 메시지 송수신 처리를 위한 구조체*/
typedef struct {
    int pid;
    int user_index;
    int from_connecting_to_center_pipe[2];
    int from_center_to_connecting_pipe[2];
} connecting_server;

/* 유저 정보 구조체 */
typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_USERNAME_LENGTH];
} user;

extern volatile int data_from_connecting;           // 통신 서버에서 메시지 송신 시 signal을 통해 변경
extern volatile int data_from_center;               // 중앙 서버에서 메시지 송신 시 signal을 통해 변경
extern int client_num;                              // 현재 접속한 클라이언트 수
extern connecting_server servers[MAX_CLIENT_NO];    // 통신(자식)서버의 정보 배열
extern int user_count;                              // 전체 유저의 수
extern user users[1024];                            // 전체 유저 배열
extern int connected_user[1024];                    // 현재 접속한 유저 1 or 0

void init_server();
void run_server();
void handle_child_process(int csock, struct sockaddr_in clientaddr);

#endif