#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <sys.h>

// Forward declaration needed for my_create_process
void zero_to_max_wrapper(void *arg);

// Map test syscalls to real implementations
static inline int64_t my_getpid(void) {
    return getPid();
}

static inline int64_t my_create_process(const char *name, uint64_t priority, char **argv) {
    // test_prio usa priority=0 para todos, luego cambia con my_nice
    // Usamos background (0) para que no bloquee la shell
    return createProcess((char*)name, zero_to_max_wrapper, argv, 0, 0, 0, 0, 0);
}

static inline int64_t my_nice(int64_t pid, int64_t priority) {
    return setProcessPriority(pid, priority);
}

static inline int64_t my_block(int64_t pid) {
    return toggleBlockProcess(pid); // Bloquea si está READY
}

static inline int64_t my_unblock(int64_t pid) {
    return unblockProcess(pid);
}

static inline int64_t my_wait(int64_t pid) {
    return waitProcess(pid);
}

#endif
