#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <arpa/inet.h>

#define MAX_CLIENT_NO   50  
#define TCP_PORT        5125
#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 32
#define USER_DATA_PATH  "data.txt"

typedef struct {
    int pid;
    int from_connecting_to_center_pipe[2];
    int from_center_to_connecting_pipe[2];
} connecting_server;

typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_USERNAME_LENGTH];
} user;

extern volatile int data_from_connecting;
extern volatile int data_from_center;
extern int client_num;
extern connecting_server servers[MAX_CLIENT_NO];
extern int user_count;
extern user users[1024];

void init_server();
void run_server();
void handle_child_process(int csock, struct sockaddr_in clientaddr);

#endif