#ifndef _LIBC_CTYPE_H_
#define _LIBC_CTYPE_H_

/**
 * @brief Convierte un caracter mayúscula ASCII a minúscula.
 */
#define tolower(a) ((a) >= 'A' && (a) <= 'Z' ? (a) + ('a' - 'A') : (a) )
/**
 * @brief Convierte un caracter minúscula ASCII a mayúscula.
 */
#define toupper(a) ((a) >= 'a' && (a) <= 'z' ? (a) - ('a' - 'A') : (a) )

#endif
