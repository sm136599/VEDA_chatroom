#ifndef SIGNAL_HANDLERS_H 
#define SIGNAL_HANDLERS_H 

void setup_signal_handlers();
void child_zombie_handler(int signo);
void sigusr1_handler(int signo);
void sigusr2_handler(int signo);

#endif