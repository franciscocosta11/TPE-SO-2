#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

/**
 * @brief Genera una excepción de división por cero desde userland.
 */
void _divzero(void);
/**
 * @brief Genera una excepción de opcode inválido desde userland.
 */
void _invalidopcode(void);

#endif
