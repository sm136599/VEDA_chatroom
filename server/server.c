#define _GNU_SOURCE

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

#define MAX_CLIENT_NO  50
#define TCP_PORT    5125

typedef struct {
    int pid;
    int to_center_pipe[2];
    int from_center_pipe[2];
} connecting_server;

volatile sig_atomic_t data_from_connecting = 0; 
volatile sig_atomic_t data_from_center = 0;
int client_num = 0;

void init_server();
void set_nonblock(int fd);

void child_zombie_handler(int signo) {
    while (waitpid(0, NULL, WNOHANG) == 0);
}

void sigusr1_handler(int signo) {
    // TODO : mutex
    data_from_connecting = 1;
}

void sigusr2_handler(int signo) {
    data_from_center = 1;
}

int main(int argc, char** argv) {
    //init_server();

    int ssock;
    socklen_t clen;
    struct sockaddr_in serveraddr, clientaddr;
    char mesg[BUFSIZ];
    connecting_server child_server[MAX_CLIENT_NO];
    
    /* child 좀비 문제 해결 */
    struct sigaction sigact;
    sigact.sa_handler = child_zombie_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sigact, NULL);

    /* 서버간 통신 시 signal 처리*/
    sigact.sa_handler = sigusr1_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sigact, NULL);

    sigact.sa_handler = sigusr2_handler;
    sigfillset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &sigact, NULL);

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

    clen = sizeof(clientaddr);
    while (1) {
        int n, csock;

        /* 클라이언트 접속 시 접속 허용 및 소켓 생성*/
        csock = accept(ssock, (struct sockaddr*)&clientaddr, &clen);

        if (csock == -1) {
            if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
                perror("accept failed");
                break;
            }
        }
        else if (csock > 0) {
            /* 클라이언트가 연결 됬을 때 */

            /* 파이프 열기 */
            pipe2(child_server[client_num].from_center_pipe, O_NONBLOCK); // 중앙서버 -> 통신서버
            pipe2(child_server[client_num].to_center_pipe, O_NONBLOCK); // 통신서버 -> 중앙서버 
            
            int pid = fork();
            if (pid < 0) {
                perror("fork failed");
                continue;
            }
            else if (pid == 0) {
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
                close(child_server[my_num].from_center_pipe[1]); // 중앙서버에서 데이터를 받는 파이프
                close(child_server[my_num].to_center_pipe[0]); // 중앙서버로 보내는 파이프
                
                /* 클라이언트 소켓 nonblock 처리 */
                set_nonblock(csock);

                while (1) {
                    n_client = read(csock, mesg_client, BUFSIZ);

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
                        if (strcmp(mesg_client, "") != 0) {
                            printf("통신 서버 : %s\n", mesg_client);
                            /* 클라이언트에서 받은 데이터 중앙 서버로 전송 */
                            kill(getppid(), SIGUSR1);
                            write(child_server[my_num].to_center_pipe[1], mesg_client, n_client);
                        }
                    }

                    /* 중앙 서버에서 입력 받은 신호 처리 */
                    while (data_from_center) {
                        n_center = read(child_server[my_num].from_center_pipe[0], mesg_center, BUFSIZ);
                        data_from_center = 0;
                        if (n_center < 0) {
                            if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                                perror("read failed");
                                exit(EXIT_FAILURE);
                            }
                            continue;
                        }
                        else if (n_center > 0) {
                            if (strcmp(mesg_center, "") != 0) {
                                printf("Received from center : %s\n", mesg_center);
                                /* 서버에서 받은 메시지 클라이언트로 전송 */
                                write(csock, mesg_center, n_center);
                                data_from_center = 0;
                            }
                        }
                    }
                }
                exit(EXIT_SUCCESS);
            }
            else if (pid > 0) {
                /* 부모 프로세스 (중앙 서버)
                * 파이프를 통해 자식 프로세스에서 메시지를 받아 다른 자식들게 전송한다. */

                /* pipe 처리 */
                child_server[client_num].pid = pid;
                close(child_server[client_num].from_center_pipe[0]);
                close(child_server[client_num].to_center_pipe[1]);
                client_num++;

                //waitpid(pid, NULL, WNOHANG);
            }
        }
        
        /* 통신 서버의 입력을 받는다. */
        for (int i = 0; i < client_num; i++) {
            n = read(child_server[i].to_center_pipe[0], mesg, BUFSIZ);
            if (n < 0) {
                if (!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                    perror("read failed");
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            else if (n == 0) {
                // 통신 서버 종료
                // TODO : child_server 배열 업데이트 
                printf("%d client server stoped\n", i);
                for (int j = i; j < client_num - 1; j++) {
                    child_server[j] = child_server[j+1];
                }
                client_num--;
            }
            else if (data_from_connecting && n > 0) {
                if (strcmp(mesg, "") != 0) {
                    printf("Received from %d connecting server : %s\n", i, mesg);
                    /* 통신 서버에서 전송된 메시지 나머지 통신 서버에 전송 */
                    for (int j = 0; j < client_num; j++) {
                        if (i != j) {
                            kill(child_server[j].pid, SIGUSR2);
                            write(child_server[j].from_center_pipe[1], mesg, n);
                        }
                    }
                    data_from_connecting = 0;
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

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}