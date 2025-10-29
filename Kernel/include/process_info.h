#ifndef PROCESS_INFO_H
#define PROCESS_INFO_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    READY,
    RUNNING,
    TERMINATED,
    BLOCKED
} ProcessState;

typedef struct {
    int pid;
    ProcessState state;
    int priority;
    char* name;
    bool foreground;
    uint64_t stackPointer;
    uint64_t basePointer;
} ProcessInfo;

#endif // PROCESS_INFO_H
