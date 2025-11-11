// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <semaphore.h>
#include <process.h>
#include <lib.h>
#include <stdint.h>
#include <string.h>
#include <interrupts.h>

#define MAX_SEMAPHORES 32
#define MAX_SEM_NAME 32
#define MAX_BLOCKED_PROCESSES 64

typedef struct {
    char name[MAX_SEM_NAME];
    int32_t value;
    uint8_t inUse;
    int32_t blockedPids[MAX_BLOCKED_PROCESSES];
    uint32_t blockedCount;
    uint32_t refCount;  // Número de procesos que tienen el semáforo abierto
} Semaphore;

static Semaphore semaphores[MAX_SEMAPHORES];
static uint8_t initialized = 0;
static uint8_t semLock = 0;  // Global spinlock for all semaphore operations
static uint32_t semRandSeed = 12345;

static inline uint8_t interruptsEnabled(void) {
    uint64_t flags;
    __asm__ volatile("pushfq; pop %0" : "=r"(flags));
    return (flags & (1ULL << 9)) != 0;
}

static void acquireSemLock(void) {
    while (_xchg(&semLock, 1) != 0) {
        if (interruptsEnabled()) {
            _hlt();
        }
    }
}

static inline void releaseSemLock(void) {
    semLock = 0;
}

void initSemaphores(void) {
    if (initialized) {
        return;
    }

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        semaphores[i].inUse = 0;
        semaphores[i].value = 0;
        semaphores[i].blockedCount = 0;
        semaphores[i].refCount = 0;
        semaphores[i].name[0] = '\0';
    }

    initialized = 1;
}

static int32_t findSemaphore(const char *name) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].inUse && strcmp(semaphores[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static int32_t findFreeSemaphore(void) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphores[i].inUse) {
            return i;
        }
    }
    return -1;
}

int32_t semCreate(const char *name, uint32_t initialValue) {
    if (!initialized) {
        initSemaphores();
    }

    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    int32_t existingId = findSemaphore(name);
    if (existingId >= 0) {
        return -2;
    }

    int32_t semId = findFreeSemaphore();
    if (semId < 0) {
        return -3;
    }

    strncpy(semaphores[semId].name, name, MAX_SEM_NAME - 1);
    semaphores[semId].name[MAX_SEM_NAME - 1] = '\0';
    semaphores[semId].value = initialValue;
    semaphores[semId].inUse = 1;
    semaphores[semId].blockedCount = 0;
    semaphores[semId].refCount = 1;

    Process *current = getCurrentProcess();
    if (current != NULL) {
        int registered = 0;
        for (int i = 0; i < MAX_SEM_PER_PROCESS; i++) {
            if (current->openSemaphores[i] == -1) {
                current->openSemaphores[i] = semId;
                registered = 1;
                break;
            }
        }
        if (!registered) {
            semaphores[semId].inUse = 0;
            semaphores[semId].refCount = 0;
            return -4;
        }
    }

    return semId;
}

int32_t semOpen(const char *name) {
    if (!initialized) {
        initSemaphores();
    }

    if (name == NULL || name[0] == '\0') {
        return -1;
    }

    int32_t semId = findSemaphore(name);
    if (semId < 0) {
        return -2;
    }

    Process *current = getCurrentProcess();
    if (current != NULL) {
        for (int i = 0; i < MAX_SEM_PER_PROCESS; i++) {
            if (current->openSemaphores[i] == semId) {
                return -3;
            }
        }

        int registered = 0;
        for (int i = 0; i < MAX_SEM_PER_PROCESS; i++) {
            if (current->openSemaphores[i] == -1) {
                current->openSemaphores[i] = semId;
                registered = 1;
                break;
            }
        }
        if (!registered) {
            return -4;
        }
    }

    semaphores[semId].refCount++;
    return semId;
}

int32_t semClose(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;
    }

    if (!semaphores[semId].inUse) {
        return -2;
    }

    semaphores[semId].refCount--;

    Process *current = getCurrentProcess();
    if (current != NULL) {
        for (int i = 0; i < MAX_SEM_PER_PROCESS; i++) {
            if (current->openSemaphores[i] == semId) {
                current->openSemaphores[i] = -1;
                break;
            }
        }
    }

    if (semaphores[semId].refCount == 0) {
        for (uint32_t i = 0; i < semaphores[semId].blockedCount; i++) {
            unblockProcess(semaphores[semId].blockedPids[i]);
        }

        semaphores[semId].inUse = 0;
        semaphores[semId].blockedCount = 0;
    }

    return 0;
}

void semCloseAllForProcess(int32_t pid) {
    if (pid <= 0) {
        return;
    }

    Process *process = getProcessByPid(pid);
    if (process == NULL) {
        return;
    }

    for (int i = 0; i < MAX_SEM_PER_PROCESS; i++) {
        int32_t semId = process->openSemaphores[i];
        if (semId >= 0 && semId < MAX_SEMAPHORES && semaphores[semId].inUse) {
            if (semaphores[semId].refCount > 0) {
                semaphores[semId].refCount--;
            }

            if (semaphores[semId].refCount == 0) {
                for (uint32_t j = 0; j < semaphores[semId].blockedCount; j++) {
                    unblockProcess(semaphores[semId].blockedPids[j]);
                }
                semaphores[semId].inUse = 0;
                semaphores[semId].blockedCount = 0;
            }

            process->openSemaphores[i] = -1;
        }
    }
}

void semRemoveProcessFromAllQueues(int32_t pid) {
    if (pid <= 0) {
        return;
    }

    acquireSemLock();

    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphores[i].inUse) {
            continue;
        }

        for (uint32_t j = 0; j < semaphores[i].blockedCount; j++) {
            if (semaphores[i].blockedPids[j] == pid) {
                for (uint32_t k = j; k < semaphores[i].blockedCount - 1; k++) {
                    semaphores[i].blockedPids[k] = semaphores[i].blockedPids[k + 1];
                }
                semaphores[i].blockedCount--;

                break;
            }
        }
    }

    releaseSemLock();
}

int32_t semWait(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;
    }

    if (!semaphores[semId].inUse) {
        return -2;
    }

    acquireSemLock();

    semaphores[semId].value--;

    if (semaphores[semId].value < 0) {
            Process *self = getCurrentProcess();
            if (self == NULL || self->pid <= 0) {
                semaphores[semId].value++;
                releaseSemLock();
                return -4;
            }

            int32_t currentPid = self->pid;

            if (semaphores[semId].blockedCount < MAX_BLOCKED_PROCESSES) {
                semaphores[semId].blockedPids[semaphores[semId].blockedCount++] = currentPid;

                releaseSemLock();

                blockCurrentProcess();

                return 0;
            } else {
            // No hay espacio para más procesos bloqueados
            semaphores[semId].value++;  // Revertir
            releaseSemLock();
            return -3;
        }
    }

    // Liberar el lock
    releaseSemLock();
    return 0;
}

// Operación Post (V) - Incrementa el semáforo, desbloquea si es necesario
int32_t semPost(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;  // ID inválido
    }

    if (!semaphores[semId].inUse) {
        return -2;  // Semáforo no existe
    }

    // Usar xchg como spinlock para proteger la sección crítica
    acquireSemLock();

    // Sección crítica protegida
    semaphores[semId].value++;

    if (semaphores[semId].value <= 0 && semaphores[semId].blockedCount > 0) {
        uint32_t totalWeight = 0;
        for (uint32_t i = 0; i < semaphores[semId].blockedCount; i++) {
            int32_t pid = semaphores[semId].blockedPids[i];
            Process *p = getProcessByPid(pid);
            totalWeight += (p != NULL) ? (p->priority + 1) : 1;
        }

        int32_t pidToUnblock = -1;
        uint32_t selectedIndex = 0;

        if (totalWeight > 0) {
            semRandSeed = semRandSeed * 1103515245 + 12345;
            uint32_t randomValue = semRandSeed % totalWeight;
            uint32_t accumulated = 0;

            for (uint32_t i = 0; i < semaphores[semId].blockedCount; i++) {
                int32_t pid = semaphores[semId].blockedPids[i];
                Process *p = getProcessByPid(pid);
                uint32_t weight = (p != NULL) ? (p->priority + 1) : 1;
                accumulated += weight;
                if (randomValue < accumulated) {
                    pidToUnblock = pid;
                    selectedIndex = i;
                    break;
                }
            }
        }

        if (pidToUnblock < 0) {
            pidToUnblock = semaphores[semId].blockedPids[0];
            selectedIndex = 0;
        }

        for (uint32_t i = selectedIndex; i < semaphores[semId].blockedCount - 1; i++) {
            semaphores[semId].blockedPids[i] = semaphores[semId].blockedPids[i + 1];
        }
        semaphores[semId].blockedCount--;

        releaseSemLock();
        unblockProcess(pidToUnblock);
        return 0;
    }

    releaseSemLock();
    return 0;
}

int32_t semGetValue(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;
    }

    if (!semaphores[semId].inUse) {
        return -2;
    }

    return semaphores[semId].value;
}
