#ifndef CAT_H
#define CAT_H

#include <stdint.h>

/**
 * @brief Entrada del comando cat: copia stdin -> stdout hasta EOF.
 */
void cat_entry(uint64_t argc, char **argv);

#endif // CAT_H
