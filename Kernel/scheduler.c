#include "interrupts.h"
#include "scheduler.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "process.h"

int countReadyQueue[MAX_PRIORITIES];
processQueue readyQueue[MAX_PRIORITIES];

Process* currentProcess = NULL;

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
    enqueueReady(&readyQueue[priority], process);
    countReadyQueue[priority]++;
}

void* schedule(void* savedContext) {
    Process* running = currentProcess;

    if (running != NULL && savedContext != NULL) {
        running->ctx = savedContext;

        if (running->state == RUNNING) {
            running->state = READY;
            schedulerAddProcess(running);
        }
    }

    Process* next = pickNext();
    // pickNext siempre debería garantizar que se devuelva un proceso
    // si no hay procesos que devuelva el idle, pero nunca null
    if (next == NULL) {
        currentProcess = NULL;
        currentPid = 0;
        return savedContext;
    }

    next->state = RUNNING;
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

void startFirstProcess(void) {
    Process* current = pickNext();

    // si no hay proceso a ejecutar, me voy
    if (current==NULL)
        return;

    current->state = RUNNING;
    currentProcess = current;
    currentPid = current->pid;
    // Salta al contexto inicial del proceso (rsp=ctx; ret)
    contextSwitchTo((void*)current->ctx);
}
