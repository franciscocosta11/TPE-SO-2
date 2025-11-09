#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdlib.h>

/**
 * @brief Tipo opaco que representa una instancia del administrador de memoria.
 */
typedef struct MemoryManagerCDT *MemoryManagerADT;

#define MEMORY_MANAGER_SIMPLE 0
#define MEMORY_MANAGER_BUDDY  1

#ifndef MEMORY_MANAGER_STRATEGY
#define MEMORY_MANAGER_STRATEGY MEMORY_MANAGER_BUDDY
#endif

/**
 * @brief Inicializa el administrador de memoria sobre una región concreta.
 *
 * @param startAddress Dirección inicial del área administrada.
 * @param size Tamaño total disponible en bytes.
 */
void createMemory(void *const restrict startAddress, const size_t size);

/**
 * @brief Reserva un bloque de memoria dinámica.
 *
 * @param size Cantidad de bytes solicitados.
 * @return Puntero al bloque asignado o NULL si no hay espacio suficiente.
 */
void *allocMemory(const size_t size);

/**
 * @brief Libera un bloque previamente asignado.
 *
 * @param blockAddress Dirección retornada por `allocMemory`.
 */
void freeMemory(void *blockAddress);

/**
 * @brief Obtiene una descripción textual del estado global de la memoria.
 *
 * @return Cadena terminada en cero con el resumen para uso diagnóstico.
 */
char *consultMemory(void);
#endif
