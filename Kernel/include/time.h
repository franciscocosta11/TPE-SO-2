#ifndef _TIME_H_
#define _TIME_H_

#include <stdint.h>

#define SECONDS_TO_TICKS 18

/**
 * @brief Handler del IRQ0. Incrementa el contador global de ticks.
 */
void timer_handler(void);

/**
 * @brief Devuelve la cantidad de ticks desde que arrancó el sistema.
 */
int ticks_elapsed(void);

/**
 * @brief Devuelve los segundos transcurridos desde el arranque.
 */
int seconds_elapsed(void);

/**
 * @brief Bloquea el proceso actual durante @p seconds segundos.
 */
void sleep(int seconds);

/**
 * @brief Bloquea el proceso actual durante @p sleep_t ticks.
 */
void sleepTicks(uint64_t sleep_t);

#endif
