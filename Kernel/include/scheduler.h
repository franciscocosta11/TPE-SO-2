#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_PRIORITIES 4
#define AGING 20 // cada 20 ticks aplico aging --> Evito inanicion
#include <stdint.h>
#include "process.h"

typedef struct processQueue {
    Process* head;
    Process* tail;
} processQueue;

/**
 * @brief Inicializa las estructuras internas del scheduler.
 *
 * Debe invocarse una vez durante el arranque del kernel, antes de crear o
 * encolar cualquier proceso. Reinicia las colas READY y limpia el puntero al
 * proceso actual.
 */
void initScheduler(void);

/**
 * @brief Inserta un proceso en la cola READY correspondiente a su prioridad.
 *
 * @param process Proceso que pasará a estar listo para ejecutar. Si es NULL se
 *        ignora la petición.
 */
void schedule(Process* process);

/**
 * @brief Elimina un proceso concreto de la cola READY en la que se encuentre.
 *
 * Útil cuando un proceso pasa a estado bloqueado, terminado o cuando se lo
 * reubica manualmente. Si el proceso no estaba encolado, la función no tiene
 * efecto.
 *
 * @param process Proceso a retirar de la cola READY. Se ignora si es NULL.
 */
void unschedule(Process* process);

/**
 * @brief Selecciona el próximo proceso listo a ejecutar.
 *
 * Recorre las colas READY de mayor a menor prioridad y devuelve el primer PCB
 * disponible. Si no hay procesos listos, retorna NULL.
 *
 * @return Puntero al proceso escogido o NULL cuando todas las colas están
 *         vacías.
 */
Process* pickNext(void);

/**
 * @brief Arranca la ejecución del primer proceso planificado.
 *
 * Obtiene un proceso con @ref pickNext y realiza el primer cambio de contexto
 * hacia su stack inicial. Se usa típicamente al finalizar el boot del kernel.
 */
void startFirstProcess(void);

/**
 * @brief Acción periódica al ocurrir un tick del timer.
 *
 * El handler del timer debe llamar a esta rutina para aplicar política de
 * scheduling (aging, rotación, etc.) y reencolar procesos si corresponde.
 */
void schedulerOnTick(void);

/**
 * @brief Gestiona la entrega voluntaria de CPU por parte del proceso actual.
 *
 * Invocada cuando un proceso llama a una syscall de yield; normalmente mueve
 * al proceso RUNNING a READY y selecciona el próximo a ejecutar.
 */
void schedulerOnYield(void);

/**
 * @brief Realiza el salto al contexto indicado.
 *
 * @param ctx Puntero a la estructura/opaco que representa el stack del proceso
 *        destino. Habitualmente obtenido del PCB seleccionado.
 */
void contextSwitchTo(void* ctx);

/**
 * @brief Prepara y agrega un proceso recién creado a las colas READY.
 *
 * Normalmente inicializa su contexto inicial (stack) y reutiliza
 * @ref schedule para encolarlo.
 *
 * @param process Proceso a registrar. Se ignora si es NULL.
 */
void schedulerAddProcess(Process* process);

#endif // SCHEDULER_H
