 /*
 *  Created on: Apr 18, 2010
 *      Author: anizzomc
 */

#ifndef INTERRUPS_H
#define INTERRUPS_H

#include <stdint.h>

extern void (*_irq00Handler) (void);
extern void (*_irq01Handler) (void);
extern void (*_irq80Handler) (void);

extern void (*_exceptionHandler00) (void);
extern void (*_exceptionHandler06) (void);

/**
 * @brief Deshabilita las interrupciones enmascarables (CLI).
 */
void _cli(void);

/**
 * @brief Habilita las interrupciones enmascarables (STI).
 */
void _sti(void);

/**
 * @brief Ejecuta la instrucción HLT para ceder ciclos mientras no haya IRQ.
 */
void _hlt(void);

/**
 * @brief Actualiza la máscara del PIC maestro.
 *
 * @param mask Máscara de interrupciones (bit 0 = IRQ0, etc.).
 */
void picMasterMask(uint8_t mask);

/**
 * @brief Actualiza la máscara del PIC esclavo.
 *
 * @param mask Máscara de interrupciones para IRQ8-IRQ15.
 */
void picSlaveMask(uint8_t mask);

/**
 * @brief Fuerza un cambio de contexto mediante int 0x20.
 */
void contextSwitch(void);

#define TIMER_PIC_MASTER 0xFE
#define KEYBOARD_PIC_MASTER 0xFD
#define NO_INTERRUPTS 0xFF

#endif
