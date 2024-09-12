#include "auth.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

int login(int ssock, user* user) {
    char response[BUFSIZ];
    char auth_text[BUFSIZ];
    memset(response, 0, BUFSIZ);
    memset(auth_text, 0, BUFSIZ);

    /* 로그인 인증 텍스트 만들기 */
    strcat(auth_text, "LOGIN:");
    strcat(auth_text, user->username);
    strcat(auth_text, "&");
    strcat(auth_text, user->password);

    send(ssock, auth_text, BUFSIZ, 0);
    recv(ssock, response, BUFSIZ, 0);
    return (strcmp(response, "SUCCESS") == 0);
}

int register_user(int ssock, user* user) {
    char response[BUFSIZ];
    char auth_text[BUFSIZ];
    memset(response, 0, BUFSIZ);
    memset(auth_text, 0, BUFSIZ);
    
    /* 회원가입 인증 텍스트 만들기 */
    strcat(auth_text, "RESGISTER:");
    strcat(auth_text, user->username);
    strcat(auth_text, "&");
    strcat(auth_text, user->password);

    send(ssock, auth_text, BUFSIZ, 0);
    recv(ssock, response, BUFSIZ, 0);
    return (strcmp(response, "SUCCESS") == 0);
}

void get_user_info(user* user) {
    /* 아이디 입력 */
    printf("ID : ");
    fgets(user->username, MAX_USERNAME_LENGTH, stdin);
    user->username[strlen(user->username) - 1] = '\0';
    
    /* 비밀번호 입력 */
    printf("PW : ");
    // TODO : 입력 시 별표 처리
    fgets(user->password, MAX_PASSWORD_LENGTH, stdin);
    user->password[strlen(user->password) - 1] = '\0';
}
