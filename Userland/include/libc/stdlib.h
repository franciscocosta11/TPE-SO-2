#ifndef _LIBC_STDLIB_H_
#define _LIBC_STDLIB_H_

#include <stddef.h>

/**
 * @brief Genera un entero pseudoaleatorio en el rango [0, RAND_MAX].
 */
int rand(void);

/**
 * @brief Inicializa la semilla usada por @ref rand.
 */
void srand(unsigned int seed);

#endif
