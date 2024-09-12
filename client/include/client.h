#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <arpa/inet.h>

#define TCP_PORT 5125

extern volatile int data_received;

void run_client(int ssock);
void handle_child_process(int ssock);
void handle_parent_process(int ssock);

#endif