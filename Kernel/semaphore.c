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

// Contador global para verificar exclusión mutua (para testing)
static volatile int32_t criticalSectionCounter = 0;

// Inicializa el sistema de semáforos
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

// Encuentra un semáforo por nombre
static int32_t findSemaphore(const char *name) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (semaphores[i].inUse && strcmp(semaphores[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Encuentra un slot libre para un nuevo semáforo
static int32_t findFreeSemaphore(void) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!semaphores[i].inUse) {
            return i;
        }
    }
    return -1;
}

// Crea un nuevo semáforo con el nombre y valor inicial dados
// Retorna el ID del semáforo o un valor negativo en caso de error
int32_t semCreate(const char *name, uint32_t initialValue) {
    if (!initialized) {
        initSemaphores();
    }

    if (name == NULL || name[0] == '\0') {
        return -1;  // Nombre inválido
    }

    // Verificar si ya existe
    int32_t existingId = findSemaphore(name);
    if (existingId >= 0) {
        return -2;  // Ya existe
    }

    // Buscar slot libre
    int32_t semId = findFreeSemaphore();
    if (semId < 0) {
        return -3;  // No hay espacio
    }

    // Inicializar el semáforo
    strncpy(semaphores[semId].name, name, MAX_SEM_NAME - 1);
    semaphores[semId].name[MAX_SEM_NAME - 1] = '\0';
    semaphores[semId].value = initialValue;
    semaphores[semId].inUse = 1;
    semaphores[semId].blockedCount = 0;
    semaphores[semId].refCount = 1;

    return semId;
}

// Abre un semáforo existente por nombre
// Retorna el ID del semáforo o un valor negativo en caso de error
int32_t semOpen(const char *name) {
    if (!initialized) {
        initSemaphores();
    }

    if (name == NULL || name[0] == '\0') {
        return -1;  // Nombre inválido
    }

    int32_t semId = findSemaphore(name);
    if (semId < 0) {
        return -2;  // No existe
    }

    semaphores[semId].refCount++;
    return semId;
}

// Cierra un semáforo (decrementa el contador de referencias)
// Si refCount llega a 0, el semáforo se elimina
int32_t semClose(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;  // ID inválido
    }

    if (!semaphores[semId].inUse) {
        return -2;  // Semáforo no existe
    }

    semaphores[semId].refCount--;

    // Si no hay más referencias, liberar el semáforo
    if (semaphores[semId].refCount == 0) {
        // Desbloquear todos los procesos bloqueados (con error)
        for (uint32_t i = 0; i < semaphores[semId].blockedCount; i++) {
            unblockProcess(semaphores[semId].blockedPids[i]);
        }

        semaphores[semId].inUse = 0;
        semaphores[semId].blockedCount = 0;
    }

    return 0;
}

// Operación Wait (P) - Decrementa el semáforo, bloquea si es necesario
// Esta función usa una instrucción atómica para garantizar exclusión mutua
int32_t semWait(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;  // ID inválido
    }

    if (!semaphores[semId].inUse) {
        return -2;  // Semáforo no existe
    }

    // Usar xchg como spinlock para proteger la sección crítica
    while (_xchg(&semLock, 1) != 0) {
        _hlt();  // Yield mientras esperamos el lock
    }

    // Sección crítica protegida
    semaphores[semId].value--;

    if (semaphores[semId].value < 0) {
        // Necesitamos bloquear el proceso actual
        int32_t currentPid = getCurrentPid();

        // Agregar a la lista de bloqueados
        if (semaphores[semId].blockedCount < MAX_BLOCKED_PROCESSES) {
            semaphores[semId].blockedPids[semaphores[semId].blockedCount++] = currentPid;

            // Liberar el lock antes de bloquear
            semLock = 0;

            // Bloquear el proceso actual
            blockCurrentProcess();

            return 0;
        } else {
            // No hay espacio para más procesos bloqueados
            semaphores[semId].value++;  // Revertir
            semLock = 0;
            return -3;
        }
    }

    // Liberar el lock
    semLock = 0;
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
    while (_xchg(&semLock, 1) != 0) {
        _hlt();  // Yield mientras esperamos el lock
    }

    // Sección crítica protegida
    semaphores[semId].value++;

    if (semaphores[semId].value <= 0 && semaphores[semId].blockedCount > 0) {
        // Hay procesos bloqueados, despertar el primero (FIFO)
        int32_t pidToUnblock = semaphores[semId].blockedPids[0];

        // Mover todos los demás hacia adelante
        for (uint32_t i = 0; i < semaphores[semId].blockedCount - 1; i++) {
            semaphores[semId].blockedPids[i] = semaphores[semId].blockedPids[i + 1];
        }
        semaphores[semId].blockedCount--;

        // Liberar el lock antes de desbloquear
        semLock = 0;

        // Desbloquear el proceso
        unblockProcess(pidToUnblock);

        return 0;
    }

    // Liberar el lock
    semLock = 0;
    return 0;
}

// Obtiene información de un semáforo (para debugging)
int32_t semGetValue(int32_t semId) {
    if (semId < 0 || semId >= MAX_SEMAPHORES) {
        return -1;
    }

    if (!semaphores[semId].inUse) {
        return -2;
    }

    return semaphores[semId].value;
}

// Funciones para testing de exclusión mutua
void semEnterCriticalTest(void) {
    criticalSectionCounter++;
}

void semLeaveCriticalTest(void) {
    criticalSectionCounter--;
}

int32_t semGetCriticalCount(void) {
    return criticalSectionCounter;
}
