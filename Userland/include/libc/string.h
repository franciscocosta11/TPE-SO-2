#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stddef.h>

/**
 * @brief Devuelve la longitud (sin contar el terminador) de la cadena.
 *
 * @param str Cadena terminada en '\0'.
 * @return Cantidad de caracteres previos al terminador.
 */
int strlen(const char *str);

/**
 * @brief Compara dos cadenas con sensibilidad a mayúsculas.
 *
 * @param str1 Primer cadena a comparar.
 * @param str2 Segunda cadena a comparar.
 * @return 0 si son iguales, negativo si str1 < str2, positivo en caso contrario.
 */
int strcmp(const char *str1, const char *str2);

/**
 * @brief Variante insensible a mayúsculas/minúsculas de strcmp.
 *
 * @param str1 Primer cadena a comparar.
 * @param str2 Segunda cadena a comparar.
 * @return Resultado similar a @ref strcmp.
 */
int strcasecmp(const char *str1, const char *str2);

/**
 * @brief Copia `src` (incluido el '\0') dentro de `dest`.
 *
 * @param dest Buffer destino con espacio suficiente.
 * @param src Cadena origen.
 */
void strcpy(char *dest, const char *src);

/**
 * @brief Copia hasta `n` caracteres desde `src` hacia `dest`.
 *
 * No garantiza terminación; es responsabilidad del llamador agregar '\0'
 * cuando sea necesario.
 *
 * @param dest Buffer destino.
 * @param src Cadena origen.
 * @param n Máximo de bytes a copiar.
 */
void strncpy(char *dest, const char *src, int n);

/**
 * @brief Muestra un mensaje de error simple precedido por `s1`.
 *
 * @param s1 Prefijo a imprimir antes del mensaje.
 */
void perror(const char *s1);

/**
 * @brief Divide una cadena en tokens usando delimitadores arbitrarios.
 *
 * @param s1 Cadena a tokenizar (se modifica in place).
 * @param s2 Lista de delimitadores aceptados.
 * @return El siguiente token o NULL si no quedan más.
 */
char *strtok(char *s1, const char *s2);

/**
 * @brief Rellena `length` bytes con `character` a partir de destination.
 *
 * @param destination Dirección inicial a sobrescribir.
 * @param character Valor de 0-255 que se repetirá.
 * @param length Cantidad de bytes a escribir.
 * @return Puntero original a `destination`.
 */
void *memset(void *destination, int character, size_t length);

/**
 * @brief Copia `length` bytes desde `source` hacia `destination`.
 *
 * @param destination Buffer destino.
 * @param source Buffer origen.
 * @param length Número de bytes a copiar.
 * @return Puntero original a `destination`.
 */
void *memcpy(void *destination, const void *source, size_t length);

#endif // LIBC_STRING_H
