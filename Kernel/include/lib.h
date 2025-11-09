#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <fonts.h>

#define EOF -1

/** @brief Rellena @p length bytes con @p character a partir de destination. */
void * memset(void * destination, int32_t character, uint64_t length);
/** @brief Copia @p length bytes de source a destination. */
void * memcpy(void * destination, const void * source, uint64_t length);
/** @brief Compara dos cadenas terminadas en NULL. */
int strcmp(const char *s1, const char *s2);
/** @brief Copia hasta @p n caracteres de @p src en @p dest. */
char *strncpy(char *dest, const char *src, uint64_t n);
/** @brief Imprime una cadena formateada simple (sin placeholders). */
void printf(const char * string);

/** @brief Devuelve el último carácter leído del buffer de teclado. */
uint8_t getKeyboardBuffer(void);
/** @brief Devuelve el estado actual del controlador de teclado. */
uint8_t getKeyboardStatus(void);

/** @brief Obtiene los segundos actuales del RTC. */
uint8_t getSecond(void);
/** @brief Obtiene los minutos actuales del RTC. */
uint8_t getMinute(void);
/** @brief Obtiene la hora actual del RTC. */
uint8_t getHour(void);

/**
 * @brief Inicializa un stack de usuario con argc/argv y rip dados.
 *
 * @param rsp Tope de stack disponible.
 * @param rip Dirección de retorno al entrar en modo usuario.
 * @param argc Cantidad de argumentos iniciales.
 * @param argv Vector de argumentos.
 * @return Puntero al nuevo contexto listo para cargar en rsp.
 */
uint8_t * initStack(void * rsp, void * rip, int argc, char ** argv);

/**
 * @brief Operación atómica XCHG.
 *
 * @param ptr Dirección a modificar.
 * @param newValue Valor que se escribirá.
 * @return Valor previo almacenado en *ptr.
 */
uint8_t _xchg(uint8_t *ptr, uint8_t newValue);

#endif
