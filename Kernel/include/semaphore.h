#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

/**
 * @brief Inicializa todas las estructuras del sistema de semáforos.
 */
void initSemaphores(void);

/**
 * @brief Crea un nuevo semáforo identificado por nombre.
 *
 * @param name Nombre simbólico del semáforo.
 * @param initialValue Valor inicial que tendrá el contador.
 * @return Identificador del semáforo o negativo si falla.
 */
int32_t semCreate(const char *name, uint32_t initialValue);

/**
 * @brief Abre un semáforo existente utilizando su nombre.
 *
 * @param name Nombre con el que fue creado.
 * @return Identificador del semáforo o negativo si no existe.
 */
int32_t semOpen(const char *name);

/**
 * @brief Cierra un semáforo y reduce su contador de referencias.
 *
 * @param semId Identificador devuelto por `semCreate`/`semOpen`.
 * @return 0 si se cerró correctamente, negativo en caso contrario.
 */
int32_t semClose(int32_t semId);

/**
 * @brief Realiza la operación de espera (P) sobre el semáforo.
 *
 * @param semId Identificador del semáforo.
 * @return 0 si decreció, negativo si no fue posible.
 */
int32_t semWait(int32_t semId);

/**
 * @brief Realiza la operación de señalización (V) sobre el semáforo.
 *
 * @param semId Identificador del semáforo.
 * @return 0 si incrementó correctamente, negativo si falló.
 */
int32_t semPost(int32_t semId);

/**
 * @brief Obtiene el valor actual del contador de un semáforo.
 *
 * @param semId Identificador del semáforo.
 * @return Valor del contador o negativo en caso de error.
 */
int32_t semGetValue(int32_t semId);

/**
 * @brief Restablece el semáforo a un nuevo valor y libera a los procesos bloqueados.
 *
 * @param semId Identificador del semáforo.
 * @param newValue Nuevo valor inicial.
 * @return 0 si la operación fue exitosa, negativo en caso contrario.
 */
int32_t semReset(int32_t semId, uint32_t newValue);

/**
 * @brief Quita a un proceso de todas las colas de espera de semáforos.
 *
 * Se invoca al terminar un proceso para evitar deadlocks.
 *
 * @param pid PID del proceso removido.
 */
void semRemoveProcessFromAllQueues(int32_t pid);

/**
 * @brief Marca la entrada a una sección crítica para pruebas.
 */
void semEnterCriticalTest(void);

/**
 * @brief Marca la salida de la sección crítica utilizada en pruebas.
 */
void semLeaveCriticalTest(void);

/**
 * @brief Devuelve la cantidad de procesos en la sección crítica de prueba.
 *
 * @return Número de procesos concurrentes registrados.
 */
int32_t semGetCriticalCount(void);

#endif // SEMAPHORE_H
