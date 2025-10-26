#include "syscall.h"
#include "test_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MemoryManager.h>

#define MAX_BLOCKS 128

#ifndef TEST_MM_ITERATIONS
#define TEST_MM_ITERATIONS 128
#endif

#ifndef TEST_MM_POOL_SIZE
#define TEST_MM_POOL_SIZE (1 << 20) // 1 MiB de heap para las pruebas
#endif

static uint8_t test_mm_pool[TEST_MM_POOL_SIZE];

typedef struct MM_rq {
  void *address;
  uint32_t size;
} mm_rq;

uint64_t test_mm(uint64_t argc, char *argv[]) {

  mm_rq mm_rqs[MAX_BLOCKS];
  uint8_t rq;
  uint32_t total;
  uint64_t max_memory;
  uint64_t iterations = 0;

  if (argc != 1)
    return -1;

  if ((max_memory = satoi(argv[0])) <= 0)
    return -1;

  if (max_memory > TEST_MM_POOL_SIZE)
    return -1;

  createMemory(test_mm_pool, TEST_MM_POOL_SIZE);

  while (iterations < TEST_MM_ITERATIONS) {
    rq = 0;
    total = 0;

    // Request as many blocks as we can
    while (rq < MAX_BLOCKS && total < max_memory) {
  mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
  mm_rqs[rq].address = allocMemory(mm_rqs[rq].size);

      if (mm_rqs[rq].address) {
        total += mm_rqs[rq].size;
        rq++;
      }
    }

    // Set
    uint32_t i;
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        memset(mm_rqs[i].address, i, mm_rqs[i].size);

    // Check
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
          printf("test_mm ERROR\n");
          return -1;
        }

    // Free
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
  freeMemory(mm_rqs[i].address);

    iterations++;
  }

  return 0;
}
