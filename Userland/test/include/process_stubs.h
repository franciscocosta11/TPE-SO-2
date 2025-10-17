#ifndef PROCESS_STUBS_H
#define PROCESS_STUBS_H

#include <stdint.h>

int my_create_process(const char *name, int flags, char *argv[]);
int my_kill(int pid);
int my_block(int pid);
int my_unblock(int pid);

#endif /* PROCESS_STUBS_H */
