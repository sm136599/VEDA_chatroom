#ifndef CONNECTION_H
#define CONNECTION_H

#include <sys/socket.h>
#include <arpa/inet.h>

int create_client_socket(const char* ip_address);

#endif