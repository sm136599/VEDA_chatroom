#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_CLIENT  50
#define TCP_PORT    5125

void init_server();

int main(int argc, char** argv) {
    init_server();

    int ssock;
    socklen_t clen;
    struct sockaddr_in serveraddr, clientaddr;
    char mesg[BUFSIZ];
    int yes = 1;
    int pipefds[MAX_CLIENT][2][2], client_num = 0;

    /* 서버 소켓 생성 */
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket not created");
        exit(EXIT_FAILURE);
    }

    /* 소켓 즉시 재사용 */
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
    if (listen(ssock, MAX_CLIENT) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    clen = sizeof(clientaddr);

    while (1) {
        /* 클라이언트 접속 시 접속 허용 및 소켓 생성*/
        int n, csock = accept(ssock, (struct sockaddr*)&clientaddr, &clen);
        if (csock == -1) {
            perror("accept failed");
            continue;
        }

        pipe(pipefds[client_num][0]); // 중앙서버 -> 통신서버
        pipe(pipefds[client_num][1]); // 통신서버 -> 중앙서버 
        
        int res;
        if ((res = fork()) < 0) {
            perror("fork failed");
            continue;
        }
        else if (res == 0) {
            /* 자식 프로세스 (통신 서버)
             * - 클라이언트와 직접 통신 
             * - 파이프를 이용한 부모 프로세스와의 통신 */
            int my_num = client_num;
            int n_client, n_center;
            char mesg_client[BUFSIZ], mesg_center[BUFSIZ];
            
            /* 네트워크 주소 출력 */
            inet_ntop(AF_INET, &clientaddr.sin_addr, mesg, BUFSIZ);
            printf("Client is conneted : %s\n", mesg);
            memset(mesg, 0, BUFSIZ);

            /* pipe 처리 */
            close(pipefds[my_num][0][1]); // 중앙서버에서 데이터를 받는 파이프
            close(pipefds[my_num][1][0]); // 중앙서버로 보내는 파이프

            /* 클라이언트에서 입력 받는 소켓 디스크립터와 
             * 중앙서버에서 데이터를 받는 디스크립터를 논블로킹 모드로 변환 */
            fcntl(pipefds[my_num][0][0], F_SETFL, O_NONBLOCK);
            fcntl(csock, F_SETFL, O_NONBLOCK);

            while (1) {
                n_client = read(csock, mesg_client, BUFSIZ);
                n_center = read(pipefds[my_num][0][0], mesg_center, BUFSIZ);

                /* 클라이언트에서 입력 받은 신호 처리 */
                if (n_client < 0) {
                    if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
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
                    printf("%d번 통신 서버 : %s\n", my_num, mesg_client);
                    /* 클라이언트에서 받은 데이터 중앙 서버로 전송 */
                    write(pipefds[my_num][1][1], mesg_client, n_client);
                }

                /* 중앙 서버에서 입력 받은 신호 처리 */
                if (n_center < 0) {
                    if (!(errno == EAGAIN || errno == EWOULDBLOCK))  {
                        perror("read failed");
                        exit(EXIT_FAILURE);
                    }
                }
                else if (n_center > 0) {
                    /* 서버에서 받은 메시지 클라이언트로 전송 */
                    write(csock, mesg_center, n_center);
                }
            }
            exit(EXIT_SUCCESS);
        }
        else if (res > 0) {
            /* 부모 프로세스 (중앙 서버)
             * 파이프를 통해 자식 프로세스에서 메시지를 받아 다른 자식들게 전송한다. */

            /* pipe 처리 */
            close(pipefds[client_num][0][0]);
            close(pipefds[client_num][1][1]);
            fcntl(pipefds[client_num][1][0], F_SETFL, O_NONBLOCK);
            client_num++;
        }
        
        /* 통신 서버의 입력을 받는다. */
        for (int i = 0; i < client_num; i++) {
            n = read(pipefds[i][1][0], mesg, BUFSIZ);
            if (n < 0) {
                if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                    perror("read failed");
                    exit(EXIT_FAILURE);
                }
            }
            else if (n == 0) {
                printf("client server stoped\n");
                // 통신 서버 종료
                // TODO : pipefds 배열 업데이트 
            }
            else if (n > 0) {
                /* 통신 서버에서 전송된 메시지 나머지 통신 서버에 전송 */
                for (int j = 0; j < client_num; j++) {
                    if (i == j) continue;
                    write(pipefds[j][0][1], mesg, n);
                }
            }
        }
    }
    close(ssock);
    return 0;
}

void init_server() {
    /* 데몬 만들기 */
    int pid;
    if ((pid = fork()) < 0) {
        perror("init fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    /* 새 세션 만들기 */
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }
    /* 디렉토리 변경 */
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }
    /* 권한 변경 */
    umask(0);
}