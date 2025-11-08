#ifndef PIPE_H
#define PIPE_H

#include "ipc.h"

/**
 * @brief Crea un pipe en espacio kernel y entrega ambos extremos.
 *
 * @param readFile Devuelve el File* para operaciones de lectura.
 * @param writeFile Devuelve el File* para operaciones de escritura.
 * @return 0 si tuvo éxito, <0 si no hay recursos disponibles.
 */
int createKernelPipe(File **readFile, File **writeFile);

#endif // PIPE_H
