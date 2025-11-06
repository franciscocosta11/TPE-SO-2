// Kernel pipe implementation with simple blocking semantics
#include "ipc.h"
#include "pipe.h"
#include "MemoryManager.h"
#include "interrupts.h"
#include "process.h"
#include <stddef.h>
#include <stdint.h>

#define PIPE_CAPACITY 1024
#define PIPE_READ_END  1
#define PIPE_WRITE_END 2

// Simple wait queues without semaphores
// Allow more blocked waiters to avoid starvation in stress tests
#define PIPE_MAX_WAITERS 32
typedef struct WaitQueue {
    int pids[PIPE_MAX_WAITERS];
    int count;
} WaitQueue;

static void wqAdd(WaitQueue *q, int pid) {
    if (q == NULL || pid <= 0) return;
    // avoid duplicates
    for (int i = 0; i < q->count; i++) {
        if (q->pids[i] == pid) return;
    }
    if (q->count < PIPE_MAX_WAITERS) {
        q->pids[q->count++] = pid;
    }
}

static int wqPop(WaitQueue *q) {
    if (q == NULL || q->count == 0) return -1;
    int pid = q->pids[0];
    for (int i = 1; i < q->count; i++) q->pids[i-1] = q->pids[i];
    q->count--;
    return pid;
}

static void wqWakeOne(WaitQueue *q) {
    int pid = wqPop(q);
    if (pid > 0) {
        unblockProcess(pid);
    }
}

static void wqWakeAll(WaitQueue *q) {
    while (q && q->count > 0) {
        int pid = wqPop(q);
        if (pid > 0) unblockProcess(pid);
    }
}

typedef struct Pipe {
    uint8_t buffer[PIPE_CAPACITY];
    size_t r;
    size_t w;
    size_t count;
    int readers;
    int writers;
    WaitQueue readersQ;
    WaitQueue writersQ;
} Pipe;

static int pipe_read(File *file, void *buffer, size_t count) {
    if (file == NULL || buffer == NULL || count == 0) return -1;
    if (file->flags != PIPE_READ_END) return -1;
    Pipe *p = (Pipe *)file->privateData;
    if (p == NULL) return -1;

    size_t i = 0;
    int wasFull = 0;
    _cli();
    while (i < count) {        
        // Esperar datos si está vacío
        while (p->count == 0) {
            if (p->writers == 0) {
                // EOF si no hay escritores
                _sti();
                return (int)i;
            }
            // Bloquear lector actual en la cola de lectores
            int self = getCurrentPid();
            if (self > 0) {
                wqAdd(&p->readersQ, self);
                Process *proc = getCurrentProcess();
                if (proc && proc->state == RUNNING) {
                    proc->state = BLOCKED;
                }
            }

            _sti();
            contextSwitch();
            _cli();
        }

        // CRITICAL SECTION: Consume one byte atomically
        wasFull = (p->count == PIPE_CAPACITY);
        ((uint8_t *)buffer)[i++] = p->buffer[p->r];
        p->r = (p->r + 1) % PIPE_CAPACITY;
        p->count--;
        
        // Si estaba lleno y ahora hay espacio, despertar a un escritor
        if (wasFull) {
            wqWakeOne(&p->writersQ);
        }
    }
    _sti();
    return (int)i;
}

static int pipe_write(File *file, const void *buffer, size_t count) {
    if (file == NULL || buffer == NULL || count == 0) return -1;
    if (file->flags != PIPE_WRITE_END) return -1;
    Pipe *p = (Pipe *)file->privateData;
    if (p == NULL) return -1;

    size_t i = 0;
    int wasEmpty = 0;
    // Check for broken pipe
    _cli();
    
    if (p->readers == 0) {
        // Broken pipe: no readers
        _sti();
        return -1;
    }
    
    while (i < count) {
        // Esperar espacio si está lleno
        while (p->count == PIPE_CAPACITY) {
            if (p->readers == 0) {
                // Sin lectores; reportar error si no pudimos escribir nada
                _sti();
                return (i > 0) ? (int)i : -1;
            }
            // Bloquear escritor actual en la cola de escritores
            int self = getCurrentPid();
            if (self > 0) {
                wqAdd(&p->writersQ, self);
                Process *proc = getCurrentProcess();
                if (proc && proc->state == RUNNING) {
                    proc->state = BLOCKED;
                }
            }
            // Re-check after waking up
            _sti();
            contextSwitch();
            _cli();
        }

        wasEmpty = (p->count == 0);
        p->buffer[p->w] = ((const uint8_t *)buffer)[i++];
        p->w = (p->w + 1) % PIPE_CAPACITY;
        p->count++;
        
        // Si estaba vacío y ahora hay datos, despertar a un lector
        if (wasEmpty) {
            wqWakeOne(&p->readersQ);
        }
    }
    _sti();
    return (int)i;
}

static int pipe_close(File *file) {
    if (file == NULL) return 0;
    Pipe *p = (Pipe *)file->privateData;
    if (p == NULL) return 0;

    int shouldFree = 0;

    _cli();

    if (file->flags == PIPE_READ_END) {
        p->readers--;
        
        // Si no quedan lectores, despertar a todos los escritores para que vean broken pipe
        if (p->readers <= 0) {
            wqWakeAll(&p->writersQ);
        }
    } else if (file->flags == PIPE_WRITE_END) {
        p->writers--;

        // Si no quedan escritores, despertar a todos los lectores para que lean EOF
        if (p->writers <= 0) {
            wqWakeAll(&p->readersQ);
        }
    }

    // CRITICAL SECTION: Check if pipe should be freed
    shouldFree = (p->readers <= 0 && p->writers <= 0);

    _sti();
    
    // Si no quedan extremos, liberar el Pipe
    if (shouldFree) {
        freeMemory(p);
        file->privateData = NULL;
    }

    return 0;
}

static FileOps pipeOps = {
    .read = pipe_read,
    .write = pipe_write,
    .close = pipe_close
};

int createKernelPipe(File **readFile, File **writeFile) {
    if (readFile == NULL || writeFile == NULL) return -1;

    Pipe *p = (Pipe *)allocMemory(sizeof(Pipe));
    if (p == NULL) return -1;
    p->r = p->w = p->count = 0;
    p->readers = 1;
    p->writers = 1;

    File *fr = (File *)allocMemory(sizeof(File));
    File *fw = (File *)allocMemory(sizeof(File));
    if (fr == NULL || fw == NULL) {
        if (fr) freeMemory(fr);
        if (fw) freeMemory(fw);
        freeMemory(p);
        return -1;
    }

    fr->ops = &pipeOps;
    fr->privateData = p;
    fr->flags = PIPE_READ_END;
    fr->refcount = 1;

    fw->ops = &pipeOps;
    fw->privateData = p;
    fw->flags = PIPE_WRITE_END;
    fw->refcount = 1;

    *readFile = fr;
    *writeFile = fw;
    return 0;
}