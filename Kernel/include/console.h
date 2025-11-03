#ifndef CONSOLE_H
#define CONSOLE_H

#include "ipc.h"

// Create File objects backed by the console device
// Caller owns the returned pointer and should free it when closing
// (for now consoleClose does nothing; freeing is managed by the owner)
File *createConsoleIn(void);
File *createConsoleOut(void);
File *createConsoleErr(void);

#endif // CONSOLE_H
