#ifndef AUTH_H
#define AUTH_H

#define MAX_USERNAME_LENGTH 32
#define MAX_PASSWORD_LENGTH 32

typedef struct {
    char username[MAX_USERNAME_LENGTH];
    char password[MAX_USERNAME_LENGTH];
} user;

int login(int ssock, user* user);
int register_user(int ssock, user* user);
void get_user_info(user* user);

#endif