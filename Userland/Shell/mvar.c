/*
 * Standalone MVar test command.
 * - Each invocation creates unique semaphores so multiple runs don't clash.
 * - Writers push letters into a bounded buffer protected by mutex semaphores.
 * - Readers keep a fixed ANSI color each and consume letters in random order
 *   thanks to the shared buffer plus priority bias/yielding.
 * - Process priority influences how aggressively each participant yields,
 *   so `nice` changes take effect without starving the rest.
 */

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

static const char *mvar_reader_colors[] = {
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

// Bounded buffer that all shell processes can access.
static volatile char mvar_buffer[MVAR_BUFFER_SIZE];
static volatile int mvar_write_pos = 0;
static volatile int mvar_read_pos = 0;

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

static int mvar_start(int numWriters, int numReaders);

static unsigned int mvar_simple_rand(unsigned int *seed) {
    *seed = *seed * 1103515245 + 12345;
    return *seed;
}

static int append_number(char *dest, int idx, unsigned int value) {
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

static void build_sem_name(char *dest, const char *prefix, int pid, unsigned int uniqueId) {
    int idx = 0;
    dest[idx++] = '/';
    for (const char *p = prefix; *p != '\0'; p++) {
        dest[idx++] = *p;
    }
    dest[idx++] = '_';
    idx = append_number(dest, idx, (unsigned int)pid);
    dest[idx++] = '_';
    idx = append_number(dest, idx, uniqueId);
    dest[idx] = '\0';
}

static void copy_sem_name(char dest[32], const char *src) {
    for (int i = 0; i < 32; i++) {
        dest[i] = src[i];
        if (src[i] == '\0') {
            break;
        }
    }
}

static int parse_positive_int(const char *s) {
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

static int get_my_priority(void) {
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

static void adaptive_yield(unsigned int *seed) {
    int prio = get_my_priority();
    int maxSleep;

    switch (prio) {
        case 0: maxSleep = 130; break;
        case 1: maxSleep = 70; break;
        case 2: maxSleep = 30; break;
        default: maxSleep = 5; break;
    }

    if (maxSleep <= 0) {
        return;
    }

    int minSleep = maxSleep / 4;
    int jitterRange = maxSleep - minSleep;
    unsigned int randVal = mvar_simple_rand(seed);
    int sleepTime = minSleep + (jitterRange > 0 ? (randVal % (jitterRange + 1)) : 0);

    if (sleepTime > 0) {
        sleep(sleepTime);
    }
}

static void cleanup_previous_mvar(void) {
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

static void mvar_writer_entry(uint64_t argc, char **argv) {
    MvarWriterArg *warg = (MvarWriterArg *)argv;
    int sem_empty = semOpen(warg->semEmptyName);
    int sem_full = semOpen(warg->semFullName);
    int sem_write_mutex = semOpen(warg->semWriteMutexName);

    if (sem_empty < 0 || sem_full < 0 || sem_write_mutex < 0) {
        sys_exit(1);
    }

    unsigned int seed = (unsigned int)(getPid() * 2654435761u + warg->writerNum * 97);

    while (1) {
        unsigned int wait_time = MVAR_BUSY_WAIT_BASE + (mvar_simple_rand(&seed) % MVAR_BUSY_WAIT_RAND);
        for (unsigned int i = 0; i < wait_time; i++) {
            __asm__ volatile("nop");
        }

        if (semWait(sem_empty) < 0) break;
        if (semWait(sem_write_mutex) < 0) break;

        mvar_buffer[mvar_write_pos] = warg->letter;
        mvar_write_pos = (mvar_write_pos + 1) % MVAR_BUFFER_SIZE;

        semPost(sem_write_mutex);
        semPost(sem_full);

        adaptive_yield(&seed);
    }

    semClose(sem_empty);
    semClose(sem_full);
    semClose(sem_write_mutex);
    sys_exit(0);
}

static void mvar_reader_entry(uint64_t argc, char **argv) {
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

        char val = mvar_buffer[mvar_read_pos];
        mvar_read_pos = (mvar_read_pos + 1) % MVAR_BUFFER_SIZE;

        semPost(sem_read_mutex);
        semPost(sem_empty);

        printf("%s%c\e[0m", myColor, val);

        unsigned int wait_time = MVAR_BUSY_WAIT_BASE + (mvar_simple_rand(&seed) % MVAR_BUSY_WAIT_RAND);
        for (unsigned int i = 0; i < wait_time; i++) {
            __asm__ volatile("nop");
        }

        adaptive_yield(&seed);
    }

    semClose(sem_empty);
    semClose(sem_full);
    semClose(sem_read_mutex);
    sys_exit(0);
}

static int mvar_start(int numWriters, int numReaders) {
    if (numWriters <= 0 || numReaders <= 0) {
        printf("mvar: Both writers and readers must be > 0\n");
        return 1;
    }
    if (numWriters > MVAR_MAX_PROCESSES || numReaders > MVAR_MAX_PROCESSES) {
        printf("mvar: Maximum %d processes of each type\n", MVAR_MAX_PROCESSES);
        return 1;
    }

    cleanup_previous_mvar();

    mvar_write_pos = 0;
    mvar_read_pos = 0;
    for (int i = 0; i < MVAR_BUFFER_SIZE; i++) {
        mvar_buffer[i] = 0;
    }

    int myPid = getPid();
    unsigned int seed = (unsigned int)(myPid * 1664525u + 1013904223u);
    unsigned int uniqueId = mvar_simple_rand(&seed) % 100000u;

    char semEmptyName[32];
    char semFullName[32];
    char semWriteMutexName[32];
    char semReadMutexName[32];
    build_sem_name(semEmptyName, "mve", myPid, uniqueId);
    build_sem_name(semFullName, "mvf", myPid, uniqueId);
    build_sem_name(semWriteMutexName, "mvw", myPid, uniqueId);
    build_sem_name(semReadMutexName, "mvr", myPid, uniqueId);

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
        copy_sem_name(writerArgs[i].semEmptyName, semEmptyName);
        copy_sem_name(writerArgs[i].semFullName, semFullName);
        copy_sem_name(writerArgs[i].semWriteMutexName, semWriteMutexName);

        static char writerNames[MVAR_MAX_PROCESSES][8];
        writerNames[i][0] = 'w';
        writerNames[i][1] = '_';
        writerNames[i][2] = 'A' + i;
        writerNames[i][3] = '\0';

        int pid = createProcess(writerNames[i], (void (*)(void *))mvar_writer_entry, (char **)&writerArgs[i], 0, NULL, 0, 0, 0);
        if (pid > 0) {
            activeWriterPids[activeWriters++] = pid;
        } else {
            printf("mvar: Failed to create writer %d\n", i);
        }
    }

    unsigned int colorSeed = seed ^ 0x9e3779b9u;

    for (int i = 0; i < numReaders; i++) {
        readerArgs[i].readerNum = i;
        readerArgs[i].color = mvar_reader_colors[mvar_simple_rand(&colorSeed) % MVAR_NUM_COLORS];
        copy_sem_name(readerArgs[i].semEmptyName, semEmptyName);
        copy_sem_name(readerArgs[i].semFullName, semFullName);
        copy_sem_name(readerArgs[i].semReadMutexName, semReadMutexName);

        static char readerNames[MVAR_MAX_PROCESSES][8];
        readerNames[i][0] = 'r';
        readerNames[i][1] = '_';
        readerNames[i][2] = '0' + i;
        readerNames[i][3] = '\0';

        int pid = createProcess(readerNames[i], (void (*)(void *))mvar_reader_entry, (char **)&readerArgs[i], 0, NULL, 0, 0, 0);
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

    int writers = parse_positive_int(argW);
    int readers = parse_positive_int(argR);
    if (writers <= 0 || readers <= 0) {
        fprintf(FD_STDERR, "mvar: invalid arguments\n");
        return 1;
    }

    return mvar_start(writers, readers);
}
