#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

volatile sig_atomic_t data_received = 0;

void run_client(int ssock, const char* username) {
    int child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    else if (child_pid == 0) {
        handle_child_process(ssock, username);
    }
    handle_parent_process(child_pid, ssock, username);
}

void handle_parent_process(int pid, int ssock, const char* username) {
    /* 부모 프로세스 
       키보드에서 입력 받아 서버로 데이터 전송*/
    char mesg[BUFSIZ];
    
    while (1) {
        memset(mesg, 0, BUFSIZ);
        fgets(mesg, BUFSIZ, stdin);
        mesg[strlen(mesg) - 1] = '\0';

        if (strncmp(mesg, "logout", 6) == 0) {
            char logout[BUFSIZ] = "LOGOUT";
            send(ssock, logout, BUFSIZ, 0);
            kill(pid, SIGTERM);
            waitpid(pid, NULL, 0);
            break;
        }

        if (send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0) { 
            perror("send failed");
            exit(EXIT_FAILURE);
        }
    }
    return;
}

void handle_child_process(int ssock, const char* username) {
    /* 자식 프로세스
       서버에서 데이터를 수신해 표준 출력 처리 */
    char mesg[BUFSIZ];
    int n;

    while (1) {
        memset(mesg, 0, BUFSIZ);
        n = read(ssock, mesg, BUFSIZ);
        if (n < 0) {
            perror("read failed");
            exit(EXIT_FAILURE);
        }
        else if (n == 0) {
            printf("server closed\n");
            close(ssock);
            exit(EXIT_FAILURE);
        }
        else {
            if (strcmp(mesg, ""))
                printf("%s\n", mesg);
        }
    }
}