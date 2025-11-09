#include "interrupts.h"
#include "scheduler.h"

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "process.h"
#include "MemoryManager.h"

int countReadyQueue[MAX_PRIORITIES];
processQueue readyQueue[MAX_PRIORITIES];

Process* currentProcess = NULL;
static uint8_t rescheduleRequested = 0;

static const uint8_t priorityQuanta[MAX_PRIORITIES] = {2, 3, 4, 5};

// es para el aging --> Quizas lo podemos obviar si lo manejamos desde el mismo aging
static int normalizePriority(int priority) {
    if (priority < MIN_PRIORITY) {
        return MIN_PRIORITY;
    }
    if (priority >= MAX_PRIORITIES) {
        return MAX_PRIORITIES - 1;
    }
    return priority;
}

static void enqueueReady(processQueue* queue, Process* process) {
    if (process == NULL || queue == NULL) {
        return;
    }

    process->next = NULL;

    if (queue->tail == NULL) {
        queue->head = process;
        queue->tail = process;
        return;
    }

    queue->tail->next = process;
    queue->tail = process;
}

static Process* dequeueReady(processQueue* queue) {
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    Process* toReturn = queue->head;
    queue->head = toReturn->next;

    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    toReturn->next = NULL;
    return toReturn;
}

uint8_t schedulerQuantumForPriority(int priority) {
    int idx = normalizePriority(priority);
    return priorityQuanta[idx];
}

static void resetQuantum(Process *process) {
    if (process == NULL) {
        return;
    }
    process->baseQuantum = schedulerQuantumForPriority(process->priority);
    process->quantumRemaining = process->baseQuantum;
}

static void ageReadyQueues(void) {
    for (int prio = 0; prio < MAX_PRIORITIES; prio++) {
        processQueue *queue = &readyQueue[prio];
        Process *prev = NULL;
        Process *node = queue->head;

        while (node != NULL) {
            Process *next = node->next;
            if (node->readyTicks < UINT16_MAX) {
                node->readyTicks++;
            }

            if (node->readyTicks >= AGING && prio < MAX_PRIORITIES - 1) {
                if (prev == NULL) {
                    queue->head = next;
                } else {
                    prev->next = next;
                }
                if (queue->tail == node) {
                    queue->tail = prev;
                }

                countReadyQueue[prio]--;

                node->priority = prio + 1;
                node->readyTicks = 0;
                resetQuantum(node);
                enqueueReady(&readyQueue[node->priority], node);
                countReadyQueue[node->priority]++;
            } else {
                prev = node;
            }

            node = next;
        }
    }
}

void initScheduler(void) {
    currentProcess = NULL;

    for (int i = 0; i < MAX_PRIORITIES; i++) {
        readyQueue[i].head = NULL;
        readyQueue[i].tail = NULL;
        countReadyQueue[i] = 0;
    }
}

void schedulerAddProcess(Process* process) {
    if (process == NULL) {
        return;
    }

    int priority = normalizePriority(process->priority);
    process->priority = priority;
    if (process->baseQuantum == 0) {
        process->baseQuantum = schedulerQuantumForPriority(priority);
    }
    if (process->quantumRemaining == 0 || process->quantumRemaining > process->baseQuantum) {
        process->quantumRemaining = process->baseQuantum;
    }
    process->readyTicks = 0;

    enqueueReady(&readyQueue[priority], process);
    countReadyQueue[priority]++;
}

uint64_t schedule(uint64_t savedContext) {
    Process* running = currentProcess;

    if (running != NULL && savedContext != 0) {
        running->ctx = savedContext;

        if (running->state == RUNNING && !rescheduleRequested) {
            return savedContext;
        }

        if (running->state == RUNNING && rescheduleRequested) {
            running->state = READY;
            resetQuantum(running);
            schedulerAddProcess(running);
        } else if (running->state == TERMINATED) {
            if (running->stackBase != NULL) {
                freeMemory(running->stackBase);
                running->stackBase = NULL;
                running->stackSize = 0;
            }
            running->ctx = 0;
        }
    }

    rescheduleRequested = 0;

    Process* next = pickNext();
    // pickNext siempre debería garantizar que se devuelva un proceso
    // si no hay procesos que devuelva el idle, pero nunca null
    if (next == NULL) {
        currentProcess = NULL;
        currentPid = 0;
        return savedContext;
    }

    next->state = RUNNING;
    if (next->baseQuantum == 0) {
        resetQuantum(next);
    }
    if (next->quantumRemaining == 0) {
        next->quantumRemaining = next->baseQuantum;
    }
    next->readyTicks = 0;
    currentProcess = next;
    currentPid = next->pid;

    return next->ctx;
}

//! Analizar si doy mas prioridad a 0 que a 3 o viceversa. busca de mayor a menor prioridad. Devuelve el primero en la lista de la primer prioridad no vacia
Process* pickNext(void) {
    for (int i = MAX_PRIORITIES - 1; i >= 0; i--) {
        if (countReadyQueue[i] == 0) {
            continue;
        }

        Process* next = dequeueReady(&readyQueue[i]);
        if (next == NULL) {
            countReadyQueue[i] = 0;
            continue;
        }

        countReadyQueue[i]--;
        currentProcess = next;
        return next;
    }

    currentProcess = NULL;
    return NULL;
}

void unschedule(Process* process) {
    if (process == NULL) {
        return;
    }

    int priority = normalizePriority(process->priority);
    processQueue* queue = &readyQueue[priority];

    Process* prev = NULL;
    Process* node = queue->head;

    while (node != NULL && node != process) {
        prev = node;
        node = node->next;
    }

    if (node == NULL) {
        return;
    }

    if (prev == NULL) {
        queue->head = node->next;
    } else {
        prev->next = node->next;
    }

    if (queue->tail == node) {
        queue->tail = prev;
    }

    node->next = NULL;

    if (countReadyQueue[priority] > 0) {
        countReadyQueue[priority]--;
    }
}

void schedulerOnTick(void) {
    ageReadyQueues();

    if (currentProcess != NULL && currentProcess->state == RUNNING) {
        if (currentProcess->quantumRemaining > 0) {
            currentProcess->quantumRemaining--;
        }

        if (currentProcess->quantumRemaining == 0) {
            rescheduleRequested = 1;
        }
    }
}

void schedulerOnYield(void) {
    rescheduleRequested = 1;
}
