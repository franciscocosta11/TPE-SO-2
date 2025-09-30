#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdlib.h>

typedef struct MemoryManagerCDT *MemoryManagerADT;

void createMemory(void *const restrict startAddress, const size_t size);

void *allocMemory(const size_t size);

void freeMemory(void *blockAddress);

// char *consultMemory(void);

#endif
