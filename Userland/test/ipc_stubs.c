#include <stdint.h>
#include <stdio.h>

void print(const char *string) {
    if (string != NULL) {
        fputs(string, stdout);
    }
}

void printDec(uint64_t value) {
    printf("%llu", (unsigned long long)value);
}

int init_shared_memory(int pid) {
    (void)pid;
    return -1; // no shared memory implemented for tests
}

void release_shared_memory(int shm_id) {
    (void)shm_id;
}

int create_pipe_in(int pid) {
    (void)pid;
    return -1; // no pipes implemented
}

int create_pipe_out(int pid) {
    (void)pid;
    return -1; // no pipes implemented
}

void close_pipe(int fd) {
    (void)fd;
}
