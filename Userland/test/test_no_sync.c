#include <stdint.h>
#include <stdio.h>
#include <sys.h>
#include "test_util.h"

#define TOTAL_PROCESSES 20
#define BACKGROUND 0
#define ITERATIONS 5

// Proceso que imprime SIN sincronización
void nosync_process(void *arg) {
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        // Incrementar contador (SIN protección de semáforo)
        semEnterCriticalTest();

        // Leer el contador - debería ser > 1 a veces (múltiples procesos simultáneos)
        int32_t count = semGetCriticalCount();

        printf("[PID %d] Iteration %d (NO synchronization, counter=%d)\n", getPid(), i, count);

        // Decrementar contador
        semLeaveCriticalTest();
    }

    printf("[PID %d] DONE!\n", getPid());
    exitProcess(0);
}

uint64_t test_no_sync(uint64_t argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: test_no_sync <n_processes>\n");
        printf("  Creates n_processes that print messages\n");
        printf("  WITHOUT synchronization (demonstrates race conditions)\n");
        printf("  You should see interleaved/garbled output\n");
        return -1;
    }

    char *n_processes_str = argv[0];
    uint64_t n_processes = satoi(n_processes_str);

    if (n_processes == 0) {
        printf("ERROR: n_processes must be > 0\n");
        return -1;
    }

    if (n_processes > TOTAL_PROCESSES) {
        printf("ERROR: n_processes exceeds maximum (%d)\n", TOTAL_PROCESSES);
        return -1;
    }

    printf("=== TEST_NO_SYNC ===\n");
    printf("Creating %llu processes\n", n_processes);
    printf("Each will print %d messages\n", ITERATIONS);
    printf("WITHOUT semaphore protection\n\n");

    int64_t pids[TOTAL_PROCESSES];
    char *args[] = {"nosync_proc"};

    for (uint64_t i = 0; i < n_processes; i++) {
        pids[i] = createProcess("nosync", nosync_process, args, 1,
                                 NULL, 0, 0, BACKGROUND);
        if (pids[i] < 0) {
            printf("ERROR: Failed to create process %llu\n", i);
        }
    }

    printf("Processes created, waiting for completion...\n\n");

    // Esperar a que todos terminen
    for (uint64_t i = 0; i < n_processes; i++) {
        waitProcess(pids[i]);
    }

    printf("\n=== TEST_NO_SYNC RESULTS ===\n");
    printf("All processes completed!\n");
    printf("Notice how output IS interleaved (no synchronization)\n");
    printf("This demonstrates why semaphores are needed!\n");

    return 0;
}
