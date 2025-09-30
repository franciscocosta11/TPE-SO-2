#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

// Dummy syscall implementations for testing
static inline int64_t my_getpid(void) {
    return 1; // Dummy PID
}

#endif
