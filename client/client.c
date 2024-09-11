#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#define TCP_PORT 5125

int main(int argc, char** argv) {
    int ssock;
    struct sockaddr_in serveraddr;
    char mesg[BUFSIZ];

    if (argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* 소켓 할당 */
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket allocated failed");
        exit(EXIT_FAILURE);
    }

    /* 주소 초기화 및 포트 설정 */
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;

    inet_pton(AF_INET, argv[1], &(serveraddr.sin_addr.s_addr));
    serveraddr.sin_port = htons(TCP_PORT);

    /* 연결 */
    if (connect(ssock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(mesg, 0, BUFSIZ);
        fgets(mesg, BUFSIZ, stdin);
        mesg[strlen(mesg) - 1] = '\0';
        /* 서버로 보내기 */
        if (strncmp(mesg, "logout", 6) == 0) break;
        if (send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0) {
            perror("send failed");
            exit(EXIT_FAILURE);
        }
    }
    close(ssock); 
    return 0;
}