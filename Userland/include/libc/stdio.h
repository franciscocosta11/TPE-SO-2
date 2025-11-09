#ifndef _LIBC_STDIO_H_
#define _LIBC_STDIO_H_

#include <string.h>
#include <stdarg.h>

#define FD_STDIN  0
#define FD_STDOUT 1
#define FD_STDERR 2

/**
 * @brief Imprime una cadena seguida de salto de línea en stdout.
 */
void puts(const char * str);

/**
 * @brief Versión variádica de printf con lista ya construida.
 */
void vprintf(const char * str, va_list args);

/**
 * @brief Imprime texto formateado en stdout.
 */
void printf(const char * str, ...);

/**
 * @brief Imprime texto formateado en el descriptor indicado.
 */
void fprintf(int fd, const char * str, ...);

/**
 * @brief Versión base de scanf que opera sobre una va_list.
 */
int vscanf(const char * format, va_list args);

/**
 * @brief Variante de scanf que lee desde un buffer en memoria.
 */
int vsscanf(const char * buffer, const char * format, va_list args);

/**
 * @brief Lee datos formateados desde una cadena (stdio clásico).
 */
int sscanf(const char * str, const char * format, ...);

/**
 * @brief Lee datos formateados desde stdin.
 */
int scanf(const char * format, ...);

/**
 * @brief Devuelve el siguiente caracter de stdin (o EOF).
 */
int getchar(void);

/**
 * @brief Escribe un caracter en stdout.
 */
void putchar(const char c);

#endif
