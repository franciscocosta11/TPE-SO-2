// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "mvar.h"

#include "./../libc/stdio.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys.h>
#include <syscalls.h>
#include <libsys/sys.h>
#include <process_info.h>

#define MVAR_MAX_PROCESSES 10
#define MVAR_BUFFER_SIZE 3
#define MVAR_BUSY_WAIT_BASE 100
#define MVAR_BUSY_WAIT_RAND 200
#define MVAR_NUM_COLORS 10

static const char *mvarReaderColors[] = {
    "\e[0;31m",  // Red
    "\e[0;32m",  // Green
    "\e[0;33m",  // Yellow
    "\e[0;34m",  // Blue
    "\e[0;35m",  // Magenta
    "\e[0;36m",  // Cyan
    "\e[0;91m",  // Bright red
    "\e[0;92m",  // Bright green
    "\e[0;93m",  // Bright yellow
    "\e[0;94m",  // Bright blue
};

static volatile char mvarBuffer[MVAR_BUFFER_SIZE];
static volatile int mvarWritePos = 0;
static volatile int mvarReadPos = 0;

typedef struct {
    char letter;
    int writerNum;
    char semEmptyName[32];
    char semFullName[32];
    char semWriteMutexName[32];
} MvarWriterArg;

typedef struct {
    int readerNum;
    const char *color;
    char semEmptyName[32];
    char semFullName[32];
    char semReadMutexName[32];
} MvarReaderArg;

static int activeWriterPids[MVAR_MAX_PROCESSES];
static int activeReaderPids[MVAR_MAX_PROCESSES];
static int activeWriters = 0;
static int activeReaders = 0;
static int currentSemEmptyId = -1;
static int currentSemFullId = -1;
static int currentSemWriteMutexId = -1;
static int currentSemReadMutexId = -1;

static int mvarStart(int numWriters, int numReaders);

static unsigned int mvarSimpleRand(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed;
}

static int appendNumber(char *dest, int idx, unsigned int value) {
    char tmp[12];
    int len = 0;
    do {
        tmp[len++] = '0' + (value % 10);
        value /= 10;
    } while (value > 0 && len < (int)sizeof(tmp));
    for (int i = len - 1; i >= 0; i--) {
        dest[idx++] = tmp[i];
    }
    return idx;
}

static void buildSemName(char *dest, const char *prefix, int pid, unsigned int uniqueId) {
    int idx = 0;
    dest[idx++] = '/';
    for (const char *p = prefix; *p != '\0'; p++) {
        dest[idx++] = *p;
    }
    dest[idx++] = '_';
    idx = appendNumber(dest, idx, (unsigned int)pid);
    dest[idx++] = '_';
    idx = appendNumber(dest, idx, uniqueId);
    dest[idx] = '\0';
}

static void copySemName(char dest[32], const char *src) {
    for (int i = 0; i < 32; i++) {
        dest[i] = src[i];
        if (src[i] == '\0') {
            break;
        }
    }
}

static int parsePositiveInt(const char *s) {
    if (s == NULL || *s == '\0') return -1;
    int val = 0;
    int idx = 0;
    if (s[0] == '+') idx = 1;
    for (; s[idx] != '\0'; idx++) {
        char c = s[idx];
        if (c < '0' || c > '9') return -1;
        val = val * 10 + (c - '0');
        if (val < 0) return -1;
    }
    return val;
}

static int getMyPriority(void) {
    ProcessInfo procs[32];
    int count = getProcesses(procs, 32);
    int mypid = getPid();
    if (count <= 0) return 0;
    for (int i = 0; i < count; i++) {
        if (procs[i].pid == mypid) {
            return procs[i].priority;
        }
    }
    return 0;
}

static void adaptiveYield(unsigned int *seed) {
    int prio = getMyPriority();
    int maxSleep;

    switch (prio) {
        case 0: maxSleep = 130; break;
        case 1: maxSleep = 70; break;
        case 2: maxSleep = 30; break;
        default: maxSleep = 5; break;
    }

    int minSleep = maxSleep / 4;
    int jitterRange = maxSleep - minSleep;
    unsigned int randVal = mvarSimpleRand(seed);
    int sleepTime = minSleep + (jitterRange > 0 ? (randVal % (jitterRange + 1)) : 0);

    sleep(sleepTime);
}

static void cleanupPreviousMvar(void) {
    for (int i = 0; i < activeWriters; i++) {
        int pid = activeWriterPids[i];
        if (pid > 0) {
            killProcess(pid);
            waitProcess(pid);
        }
        activeWriterPids[i] = 0;
    }
    for (int i = 0; i < activeReaders; i++) {
        int pid = activeReaderPids[i];
        if (pid > 0) {
            killProcess(pid);
            waitProcess(pid);
        }
        activeReaderPids[i] = 0;
    }
    activeWriters = 0;
    activeReaders = 0;

    if (currentSemEmptyId >= 0) {
        semClose(currentSemEmptyId);
        currentSemEmptyId = -1;
    }
    if (currentSemFullId >= 0) {
        semClose(currentSemFullId);
        currentSemFullId = -1;
    }
    if (currentSemWriteMutexId >= 0) {
        semClose(currentSemWriteMutexId);
        currentSemWriteMutexId = -1;
    }
    if (currentSemReadMutexId >= 0) {
        semClose(currentSemReadMutexId);
        currentSemReadMutexId = -1;
    }
}

static void mvarWriterEntry(uint64_t argc, char **argv) {
    (void)argc;
    MvarWriterArg *warg = (MvarWriterArg *)argv;
    int sem_empty = semOpen(warg->semEmptyName);
    int sem_full = semOpen(warg->semFullName);
    int sem_write_mutex = semOpen(warg->semWriteMutexName);

    if (sem_empty < 0 || sem_full < 0 || sem_write_mutex < 0) {
        sys_exit(1);
    }

    unsigned int seed = (unsigned int)(getPid() * 2654435761u + warg->writerNum * 97);

    while (1) {
        unsigned int waitTime = MVAR_BUSY_WAIT_BASE + (mvarSimpleRand(&seed) % MVAR_BUSY_WAIT_RAND);
        for (volatile unsigned int i = 0; i < waitTime; i++) {
        }

        if (semWait(sem_empty) < 0) break;
        if (semWait(sem_write_mutex) < 0) break;

        mvarBuffer[mvarWritePos] = warg->letter;
        mvarWritePos = (mvarWritePos + 1) % MVAR_BUFFER_SIZE;

        semPost(sem_write_mutex);
        semPost(sem_full);

        adaptiveYield(&seed);
    }

    semClose(sem_empty);
    semClose(sem_full);
    semClose(sem_write_mutex);
    sys_exit(0);
}

static void mvarReaderEntry(uint64_t argc, char **argv) {
    (void)argc;
    MvarReaderArg *rarg = (MvarReaderArg *)argv;
    const char *myColor = rarg->color;

    int sem_empty = semOpen(rarg->semEmptyName);
    int sem_full = semOpen(rarg->semFullName);
    int sem_read_mutex = semOpen(rarg->semReadMutexName);

    if (sem_empty < 0 || sem_full < 0 || sem_read_mutex < 0) {
        sys_exit(1);
    }

    unsigned int seed = (unsigned int)(getPid() * 9719u + rarg->readerNum * 37 + ((uintptr_t)&seed & 0xFFFF));

    while (1) {
        if (semWait(sem_full) < 0) break;
        if (semWait(sem_read_mutex) < 0) break;

        char val = mvarBuffer[mvarReadPos];
        mvarReadPos = (mvarReadPos + 1) % MVAR_BUFFER_SIZE;

        semPost(sem_read_mutex);
        semPost(sem_empty);

        printf("%s%c\e[0m", myColor, val);

        unsigned int waitTime = MVAR_BUSY_WAIT_BASE + (mvarSimpleRand(&seed) % MVAR_BUSY_WAIT_RAND);
        for (volatile unsigned int i = 0; i < waitTime; i++) {
        }

        adaptiveYield(&seed);
    }

    semClose(sem_empty);
    semClose(sem_full);
    semClose(sem_read_mutex);
    sys_exit(0);
}

static int mvarStart(int numWriters, int numReaders) {
    if (numWriters <= 0 || numReaders <= 0) {
        printf("mvar: Both writers and readers must be > 0\n");
        return 1;
    }
    if (numWriters > MVAR_MAX_PROCESSES || numReaders > MVAR_MAX_PROCESSES) {
        printf("mvar: Maximum %d processes of each type\n", MVAR_MAX_PROCESSES);
        return 1;
    }

    cleanupPreviousMvar();

    mvarWritePos = 0;
    mvarReadPos = 0;
    for (int i = 0; i < MVAR_BUFFER_SIZE; i++) {
        mvarBuffer[i] = 0;
    }

    int myPid = getPid();
    unsigned int seed = (unsigned int)(myPid * 1664525u + 1013904223u);
    unsigned int uniqueId = mvarSimpleRand(&seed) % 100000u;

    char semEmptyName[32];
    char semFullName[32];
    char semWriteMutexName[32];
    char semReadMutexName[32];
    buildSemName(semEmptyName, "mve", myPid, uniqueId);
    buildSemName(semFullName, "mvf", myPid, uniqueId);
    buildSemName(semWriteMutexName, "mvw", myPid, uniqueId);
    buildSemName(semReadMutexName, "mvr", myPid, uniqueId);

    int sem_empty = semCreate(semEmptyName, MVAR_BUFFER_SIZE);
    int sem_full = semCreate(semFullName, 0);
    int sem_write_mutex = semCreate(semWriteMutexName, 1);
    int sem_read_mutex = semCreate(semReadMutexName, 1);
    if (sem_empty < 0 || sem_full < 0 || sem_write_mutex < 0 || sem_read_mutex < 0) {
        printf("mvar: Failed to create semaphores\n");
        if (sem_empty >= 0) semClose(sem_empty);
        if (sem_full >= 0) semClose(sem_full);
        if (sem_write_mutex >= 0) semClose(sem_write_mutex);
        if (sem_read_mutex >= 0) semClose(sem_read_mutex);
        return 1;
    }
    currentSemEmptyId = sem_empty;
    currentSemFullId = sem_full;
    currentSemWriteMutexId = sem_write_mutex;
    currentSemReadMutexId = sem_read_mutex;

    static MvarWriterArg writerArgs[MVAR_MAX_PROCESSES];
    static MvarReaderArg readerArgs[MVAR_MAX_PROCESSES];

    for (int i = 0; i < numWriters; i++) {
        writerArgs[i].letter = 'A' + i;
        writerArgs[i].writerNum = i;
        copySemName(writerArgs[i].semEmptyName, semEmptyName);
        copySemName(writerArgs[i].semFullName, semFullName);
        copySemName(writerArgs[i].semWriteMutexName, semWriteMutexName);

        static char writerNames[MVAR_MAX_PROCESSES][8];
        writerNames[i][0] = 'w';
        writerNames[i][1] = '_';
        writerNames[i][2] = 'A' + i;
        writerNames[i][3] = '\0';

        int pid = createProcess(writerNames[i], mvarWriterEntry, (char **)(void *)&writerArgs[i], 0, NULL, 0, 0, 0);
        if (pid > 0) {
            activeWriterPids[activeWriters++] = pid;
        } else {
            printf("mvar: Failed to create writer %d\n", i);
        }
    }

    unsigned int colorSeed = seed ^ 0x9e3779b9u;

    for (int i = 0; i < numReaders; i++) {
        readerArgs[i].readerNum = i;
        readerArgs[i].color = mvarReaderColors[mvarSimpleRand(&colorSeed) % MVAR_NUM_COLORS];
        copySemName(readerArgs[i].semEmptyName, semEmptyName);
        copySemName(readerArgs[i].semFullName, semFullName);
        copySemName(readerArgs[i].semReadMutexName, semReadMutexName);

        static char readerNames[MVAR_MAX_PROCESSES][8];
        readerNames[i][0] = 'r';
        readerNames[i][1] = '_';
        readerNames[i][2] = '0' + i;
        readerNames[i][3] = '\0';

        int pid = createProcess(readerNames[i], mvarReaderEntry, (char **)(void *)&readerArgs[i], 0, NULL, 0, 0, 0);
        if (pid > 0) {
            activeReaderPids[activeReaders++] = pid;
        } else {
            printf("mvar: Failed to create reader %d\n", i);
        }
    }

    printf("mvar: Created %d writers and %d readers. Use 'ps' to see PIDs.\n", numWriters, numReaders);
    printf("Writers (w_X where X is the letter): ");
    for (int i = 0; i < numWriters; i++) {
        printf("%c ", 'A' + i);
    }
    printf("\nReaders (r_N with color): ");
    for (int i = 0; i < numReaders; i++) {
        printf("%s%d\e[0m ", readerArgs[i].color, i);
    }
    printf("\nUse 'kill <PID>' or 'nice <PID> <prio>' to control processes.\n\n");

    return 0;
}

int mvar_cmd(void) {
    char *argW = strtok(NULL, " ");
    char *argR = strtok(NULL, " ");
    if (argW == NULL || argR == NULL) {
        fprintf(FD_STDERR, "Usage: mvar <writers> <readers>\n");
        return 1;
    }

    int writers = parsePositiveInt(argW);
    int readers = parsePositiveInt(argR);
    if (writers <= 0 || readers <= 0) {
        fprintf(FD_STDERR, "mvar: invalid arguments\n");
        return 1;
    }

    return mvarStart(writers, readers);
}
