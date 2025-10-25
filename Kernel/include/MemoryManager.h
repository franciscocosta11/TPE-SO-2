#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdlib.h>

typedef struct MemoryManagerCDT *MemoryManagerADT;

#define MEMORY_MANAGER_SIMPLE 0
#define MEMORY_MANAGER_BUDDY  1

#ifndef MEMORY_MANAGER_STRATEGY
#define MEMORY_MANAGER_STRATEGY MEMORY_MANAGER_BUDDY
#endif

void createMemory(void *const restrict startAddress, const size_t size);

void *allocMemory(const size_t size);

void freeMemory(void *blockAddress);

// char *consultMemory(void);
#endif
