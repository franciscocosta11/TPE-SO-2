#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdlib.h>

typedef struct MemoryManagerCDT *MemoryManagerADT;

#define MEMORY_MANAGER_SIMPLE 0
#define MEMORY_MANAGER_BUDDY  1

#ifndef MEMORY_MANAGER_STRATEGY
#define MEMORY_MANAGER_STRATEGY MEMORY_MANAGER_BUDDY
#endif

/**
 * @brief Inicializa el administrador de memoria física.
 *
 * Debe invocarse con la dirección inicial y el tamaño del bloque
 * contiguo disponible durante el arranque para preparar las futuras
 * asignaciones dinámicas.
 *
 * @param startAddress Inicio del rango de memoria administrado.
 * @param size Cantidad de bytes disponibles a partir de startAddress.
 */
void createMemory(void *const restrict startAddress, const size_t size);

/**
 * @brief Reserva un bloque de memoria contiguo.
 *
 * Entrega un puntero alineado de al menos @p size bytes o NULL si no
 * hay suficiente espacio libre.
 *
 * @param size Cantidad de bytes solicitada.
 * @return Puntero al bloque reservado o NULL en caso de error.
 */
void *allocMemory(const size_t size);

/**
 * @brief Libera un bloque previamente asignado.
 *
 * El puntero debe provenir de @ref allocMemory; pasar NULL no tiene
 * efecto.
 *
 * @param blockAddress Dirección devuelta por allocMemory.
 */
void freeMemory(void *blockAddress);

/**
 * @brief Obtiene un resumen textual del estado del heap.
 *
 * @return Cadena estática con estadísticas generales (ocupado/libre).
 */
char *consultMemory(void);
#endif
