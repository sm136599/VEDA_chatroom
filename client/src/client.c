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
    else {
        handle_parent_process(ssock, username);
    }
}

void handle_child_process(int ssock, const char* username) {
    char mesg[BUFSIZ];
    
    while (1) {
        memset(mesg, 0, BUFSIZ);
        fgets(mesg, BUFSIZ, stdin);
        mesg[strlen(mesg) - 1] = '\0';

        if (strncmp(mesg, "logout", 6) == 0) {
            break;
        }

        if (send(ssock, mesg, BUFSIZ, MSG_DONTWAIT) <= 0) { 
            perror("send failed");
            exit(EXIT_FAILURE);
        }
    }
    return;
}

void handle_parent_process(int ssock, const char* username) {
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