#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>
#include <sys.h>

// Forward declarations
void zero_to_max_wrapper(uint64_t argc, char **argv);
void my_process_inc_wrapper(uint64_t argc, char **argv);
uint64_t my_process_inc(uint64_t argc, char *argv[]);

// Map test syscalls to real implementations
static inline int64_t my_getpid(void) {
    return getPid();
}

static inline int strings_equal(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }

    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static inline int64_t my_create_process(const char *name, uint64_t requested_argc, char **argv) {
    // Determinar qué wrapper usar según el nombre
    void (*wrapper)(uint64_t, char **) = zero_to_max_wrapper;

    // Contar argumentos
    uint64_t argc = requested_argc;
    if (argv != NULL) {
        uint64_t count = 0;
        while (argv[count] != NULL) {
            count++;
        }
        argc = count;
    }

    // Comparar con "my_process_inc"
    const char *target_inc = "my_process_inc";

    if (strings_equal(name, target_inc)) {
        wrapper = my_process_inc_wrapper;
    } else if (strings_equal(name, "zero_to_max")) {
        wrapper = zero_to_max_wrapper;
    }

    return createProcess((char*)name, (void (*)(void *))wrapper, argv, (uint32_t)argc, NULL, 0, 0, 0);
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

static inline void my_yield(void) {
    sleep(1); // force a scheduler tick to improve interleaving
}

// Semaphore wrappers - convert name-based API to ID-based API
static inline int32_t my_sem_open(const char *name, uint32_t initialValue) {
    int32_t semId = semOpen(name);

    if (semId >= 0) {
        return semId;
    }

    semId = semCreate(name, initialValue);
    if (semId >= 0) {
        return semId;
    }

    // If creation failed because it already exists, try to open again
    if (semId == -2) {
        semId = semOpen(name);
    }

    if (semId < 0) {
        semId = semCreate(name, initialValue);
    }

    return semId;
}

static inline int32_t my_sem_wait(int32_t semId) {
    return semWait(semId);
}

static inline int32_t my_sem_post(int32_t semId) {
    return semPost(semId);
}

static inline int32_t my_sem_close(int32_t semId) {
    return semClose(semId);
}

#endif
