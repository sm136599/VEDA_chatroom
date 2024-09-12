#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>

#define MAX_CLIENT_NO   50  
#define TCP_PORT        5125

typedef struct {
    int pid;
    int from_connecting_to_center_pipe[2];
    int from_center_to_connecting_pipe[2];
} connecting_server;

extern volatile int data_from_connecting;
extern volatile int data_from_center;
extern int client_num;
extern connecting_server servers[MAX_CLIENT_NO];

void init_server();
void run_server();

#endif