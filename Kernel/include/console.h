#ifndef CONSOLE_H
#define CONSOLE_H

#include "ipc.h"

/**
 * @brief Crea un File* para leer desde la consola (stdin).
 */
File *createConsoleIn(void);
/**
 * @brief Crea un File* para escribir en la consola (stdout).
 */
File *createConsoleOut(void);
/**
 * @brief Crea un File* para escribir errores en la consola (stderr).
 */
File *createConsoleErr(void);

#endif // CONSOLE_H
