#include "client.h"
#include "connection.h"
#include "signal_handlers.h"
#include "auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //setup_signal_handlers();
    int ssock = create_client_socket(argv[1]);
    /* 화면 처리 */
    int cmd;
    while (1) {
        printf("----------------------------\n");
        printf("1. 로그인\n");
        printf("2. 회원가입\n");
        printf("9. 종료\n");
        printf("> ");
        fscanf(stdin, "%d", &cmd);
        getchar();
        user user;
        switch (cmd)
        {
        case 1:
            get_user_info(&user);
            if (login(ssock, &user)) {
                printf("로그인 성공!\n");
                printf("----------------------------\n");
                run_client(ssock, user.username);
            }
            else {
                printf("아이디와 비밀번호를 확인해주세요.\n");
            }
            break;
        case 2:
            get_user_info(&user);
            if (register_user(ssock, &user)) {
                printf("회원 가입 성공!\n");
                printf("채팅방으로 바로 접속됩니다.\n");
                printf("----------------------------\n");
                run_client(ssock, user.username);
            }
            else {
                printf("다른 아이디로 생성해주세요.\n");
            }
            break;
        case 9:
            close(ssock);
            return 0;
            break;
        default:
            break;
        }
    }

    return 0;
}