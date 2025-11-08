#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

/**
 * @brief Inicializa las estructuras internas del subsistema de semáforos.
 */
void initSemaphores(void);

/**
 * @brief Crea un semáforo identificado por nombre.
 *
 * @param name Nombre lógico (máx. 31 caracteres).
 * @param initialValue Valor inicial del contador.
 * @return ID del semáforo o negativo en caso de error.
 */
int32_t semCreate(const char *name, uint32_t initialValue);

/**
 * @brief Abre un semáforo previamente creado.
 *
 * @return ID del semáforo o negativo si no existe.
 */
int32_t semOpen(const char *name);

/**
 * @brief Cierra un descriptor de semáforo (decrementa refcount).
 */
int32_t semClose(int32_t semId);

/**
 * @brief Realiza la operación P: decrementa y bloquea si el valor es negativo.
 */
int32_t semWait(int32_t semId);

/**
 * @brief Realiza la operación V: incrementa y desbloquea si corresponde.
 */
int32_t semPost(int32_t semId);

/**
 * @brief Devuelve el valor actual del contador (solo para debugging).
 */
int32_t semGetValue(int32_t semId);

/**
 * @brief Reestablece el valor del semáforo y limpiar su cola de bloqueados.
 */
int32_t semReset(int32_t semId, uint32_t newValue);

/**
 * @brief Elimina a @p pid de todas las colas de espera.
 */
void semRemoveProcessFromAllQueues(int32_t pid);

// Funciones de testeo
/** @brief Marca la entrada a una sección crítica artificial. */
void semEnterCriticalTest(void);

/** @brief Marca la salida de la sección crítica artificial. */
void semLeaveCriticalTest(void);

/** @brief Devuelve el contador actual usado durante tests. */
int32_t semGetCriticalCount(void);

#endif // SEMAPHORE_H
