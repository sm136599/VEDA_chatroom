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

