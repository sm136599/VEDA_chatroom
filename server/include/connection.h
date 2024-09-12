#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include <arpa/inet.h>

int create_server_socket();
void handle_client_connection(int csock, struct sockaddr_in clientaddr);

#endif