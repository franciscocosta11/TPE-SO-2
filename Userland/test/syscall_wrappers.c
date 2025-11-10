// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stddef.h>

#include <libsys/sys.h>

#include "include/process_stubs.h"
#include "include/syscall.h"

int64_t my_getpid(void) {
    return getPid();
}

int64_t my_create_process(const char *name, void (*entry)(uint64_t, char **), char **argv, uint32_t argc) {
    if (entry == NULL) {
        return -1;
    }
    return createProcess((char *)name, entry, argv, argc, NULL, 0, 0, 0);
}

int64_t my_wait(int64_t pid) {
    return waitProcess((int32_t)pid);
}

int64_t my_nice(int64_t pid, int64_t priority) {
    return setProcessPriority((int32_t)pid, (int32_t)priority);
}

int64_t my_kill(int64_t pid) {
    return killProcess((int32_t)pid);
}

int64_t my_block(int64_t pid) {
    return toggleBlockProcess((int32_t)pid);
}

int64_t my_unblock(int64_t pid) {
    return toggleBlockProcess((int32_t)pid);
}

int64_t my_yield(void) {
    return yieldProcess();
}
