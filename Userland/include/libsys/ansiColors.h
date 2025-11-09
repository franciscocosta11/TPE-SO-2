#ifndef _ANSI_COLORS_H_
#define _ANSI_COLORS_H_

#ifdef ANSI_4_BIT_COLOR_SUPPORT

#include <stdint.h>

/**
 * @brief Parsea una secuencia ANSI y actualiza colores/índices según corresponda.
 *
 * @param string Cadena que contiene la secuencia de escape.
 * @param i Índice actualizado al final del token procesado.
 */
void parseANSI(const char * string, int * i);

#endif

#endif
