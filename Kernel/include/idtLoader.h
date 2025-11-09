#ifndef _IDT_LOADER_H_
#define _IDT_LOADER_H_

#include <stdint.h>

#include <defs.h>
#include <interrupts.h>

/**
 * @brief Carga la tabla IDT configurada en memoria mediante lidt.
 */
void load_idt(void);

#endif
