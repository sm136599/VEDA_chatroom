#ifndef UTILS_H
#define UTILS_H
#include "server.h"

void set_nonblock(int fd);
void set_block(int fd);
void make_daemon();
void load_data();
void save_user_data(user* user);
int login_user(user* user);
int register_user(user* user);
void get_user_from_string(const char* string, user* user);

#endif