#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

// Inicializa el sistema de semáforos
void initSemaphores(void);

// Crea un nuevo semáforo con el nombre y valor inicial dados
// Retorna el ID del semáforo o un valor negativo en caso de error
int32_t semCreate(const char *name, uint32_t initialValue);

// Abre un semáforo existente por nombre
// Retorna el ID del semáforo o un valor negativo en caso de error
int32_t semOpen(const char *name);

// Cierra un semáforo (decrementa el contador de referencias)
int32_t semClose(int32_t semId);

// Operación Wait (P) - Decrementa el semáforo, bloquea si es necesario
int32_t semWait(int32_t semId);

// Operación Post (V) - Incrementa el semáforo, desbloquea si es necesario
int32_t semPost(int32_t semId);

// Obtiene el valor actual de un semáforo (para debugging)
int32_t semGetValue(int32_t semId);

// Resetea un semáforo a un nuevo valor y limpia todos los procesos bloqueados
int32_t semReset(int32_t semId, uint32_t newValue);

// Remueve un proceso de todas las colas de espera de semáforos
// Se llama cuando un proceso es terminado/killed para evitar deadlocks
void semRemoveProcessFromAllQueues(int32_t pid);

// Funciones para testing de exclusión mutua
void semEnterCriticalTest(void);

void semLeaveCriticalTest(void);

int32_t semGetCriticalCount(void);

#endif // SEMAPHORE_H
