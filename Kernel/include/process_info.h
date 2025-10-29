#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <stdint.h>
#include <stdbool.h>

// Shared representation of process state exposed to userland for inspection
typedef enum {
    READY,
    RUNNING,
    TERMINATED,
    BLOCKED
} ProcessState;

// Lightweight view of a process used when sharing state with userland
typedef struct {
    int pid;
    ProcessState state;
    int priority;
    char* name;
    bool foreground;
} ProcessInfo;

#endif // PROCESS_INFO_H
