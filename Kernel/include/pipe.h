#ifndef PIPE_H
#define PIPE_H

#include "ipc.h"

/**
 * @brief Crea un pipe del kernel y devuelve sus extremos File.
 *
 * @param readFile Extremo de lectura que se asociará a `read()`.
 * @param writeFile Extremo de escritura que se asociará a `write()`.
 * @return 0 si se creó correctamente, valor negativo si ocurrió un error.
 */
int createKernelPipe(File **readFile, File **writeFile);

#endif // PIPE_H