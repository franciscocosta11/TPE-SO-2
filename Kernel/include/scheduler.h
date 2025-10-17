#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PRIORITIES 4
#include <stdint.h>

typedef struct procQueue {
    struct process* head;
    struct process* tail;
} processQueue;

void initScheduler(void);

void enqueueReady(Process* process);

Process* pickNext(void);

void startFirstProcess(void);

void schedulerOnTick(void);

schedulerOnYield();

#endif // SCHEDULER_H