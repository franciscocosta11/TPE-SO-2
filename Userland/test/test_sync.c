#include <stdint.h>
#include <stdio.h>
#include <sys.h>
#include "test_util.h"

#define SEM_NAME "test_sync_sem"
#define TOTAL_PAIR_PROCESSES 10
#define BACKGROUND 0
#define ITERATIONS 5

// Proceso que imprime con sincronización
void sync_process(void *arg) {
    (void)arg;
    int32_t sem = semOpen(SEM_NAME);
    if (sem < 0) {
        printf("ERROR: Process %d couldn't open semaphore\n", getPid());
        exitProcess(1);
    }

    for (int i = 0; i < ITERATIONS; i++) {
        semWait(sem);

        // Incrementar contador del kernel (entrando a sección crítica)
        semEnterCriticalTest();

        // Verificar que solo hay 1 proceso en la sección crítica
        int32_t count = semGetCriticalCount();
        if (count > 1) {
            printf("ERROR: %d processes in critical section! (should be 1)\n", count);
        }

        printf("[PID %d] Iteration %d (inside critical section, counter=%d)\n", getPid(), i, count);

        // Decrementar contador del kernel (saliendo de sección crítica)
        semLeaveCriticalTest();

        semPost(sem);
    }

    semClose(sem);
    printf("[PID %d] DONE!\n", getPid());
    exitProcess(0);
}

uint64_t test_sync(uint64_t argc, char *argv[]) {
    if (argc != 1) {
        printf("Usage: test_sync <n_processes>\n");
        printf("  Creates n_processes that print messages\n");
        printf("  WITH semaphore synchronization\n");
        printf("  You should see ordered output (no interleaving)\n");
        return -1;
    }

    char *n_processes_str = argv[0];
    uint64_t n_processes = satoi(n_processes_str);

    if (n_processes == 0) {
        printf("ERROR: n_processes must be > 0\n");
        return -1;
    }

    if (n_processes > TOTAL_PAIR_PROCESSES * 2) {
        printf("ERROR: n_processes exceeds maximum (%d)\n", TOTAL_PAIR_PROCESSES * 2);
        return -1;
    }

    // Crear semáforo binario (mutex)
    int32_t sem_id = semCreate(SEM_NAME, 1);
    if (sem_id < 0) {
        printf("ERROR: Failed to create semaphore\n");
        return -1;
    }

    printf("=== TEST_SYNC ===\n");
    printf("Creating %llu processes\n", n_processes);
    printf("Each will print %d messages\n", ITERATIONS);
    printf("WITH semaphore protection\n\n");

    int64_t pids[TOTAL_PAIR_PROCESSES * 2];
    char *args[] = {"sync_proc"};

    for (uint64_t i = 0; i < n_processes; i++) {
        pids[i] = createProcess("sync", sync_process, args, 1,
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

    printf("\n=== TEST_SYNC RESULTS ===\n");
    printf("All processes completed successfully!\n");
    printf("Notice how output is NOT interleaved (semaphore works)\n");

    semClose(sem_id);
    return 0;
}
