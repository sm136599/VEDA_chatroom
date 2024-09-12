#include "connection.h"
#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int create_client_socket(const char* ip_address) {
    int ssock;
    struct sockaddr_in serveraddr;

    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket allocation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &(serveraddr.sin_addr.s_addr));
    serveraddr.sin_port = htons(TCP_PORT);

    if (connect(ssock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    return ssock;
}