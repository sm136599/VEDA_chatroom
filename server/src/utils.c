#include "utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

user users[1024];

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void make_daemon() {
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

void load_data() {
    FILE* user_data = fopen("data.txt", "r");
    if (user_data == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }
    else {
        while(fscanf(stdin, "%s %s", users[user_count].username, users[user_count].password) != EOF) user_count++;
    }
    fclose(user_data);
}

void save_user_data(user* user) {
    FILE* user_data = fopen("data.txt", "a");
    if (user_data == NULL) {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    fprintf(user_data, "%s %s", user->username, user->password);
}

int login_user(user* user) {
    /* 유저 찾기 */
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user->username, users[i].username) == 0 && 
            strcmp(user->password, users[i].password) == 0) {
            return 1;   // success
        }
    }
    return 0;           // fail
}

int register_user(user* user) {
    /* 같은 아이디 유저 찾기 */
    for (int i = 0; i < user_count; i++) {
        if (strcmp(user->username, users[i].username) == 0) {
            return 0;   // fail
        }
    }
    strcpy(users[user_count].username, user->username);
    strcpy(users[user_count].password, user->password);
    return 1;           // success
}

void get_user_from_string(const char* string, user* user) {
    char* sep = strchr(string, ':');
    char* ampersand_pos = strchr(string, '&');
    strncpy(user->username, sep + 1, ampersand_pos - sep);
    strcpy(user->password, ampersand_pos + 1);
}
