// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "./../libc/stdio.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <syscalls.h>

#include <sys.h>
#include <exceptions.h>

#include "cat.h"
#include "wc.h"
#include "filter.h"
#include "echo.h"
#include "mvar.h"

#include "shell_internal.h"

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

#define TEST_MM_MAX_INSTANCES 4

typedef struct
{
    int pid;
    char arg[MAX_BUFFER_SIZE];
    char *argv[1];
} TestMmSlot;

typedef struct
{
    char arg[MAX_BUFFER_SIZE];
    char *argv[1];
} TestPrioContext;

static TestMmSlot testMmSlots[TEST_MM_MAX_INSTANCES];
static uint8_t testMmSlotsInitialized = 0;
static TestPrioContext testPrioContext = {0};
static uint8_t testPrioRunning = 0;

static void test_prio_context_init(void);
static void refreshTestPrioRunning(void);
static void test_mm_slot_init(void);
static void test_mm_cleanup_slots(void);
static TestMmSlot *test_mm_acquire_slot(void);
static TestMmSlot *test_mm_find_slot_by_argv(char **argv);
static void test_mm_entry(uint64_t argc, char **argv);
static void test_prio_entry(uint64_t argc, char **argv);
static void test_process_entry(uint64_t argc, char **argv);
static void sleep2_sleeper(uint64_t argc, char **argv);
static void print3_entry(uint64_t argc, char **argv);
static void ps_entry(uint64_t argc, char **argv);
static void loop_entry(uint64_t argc, char **argv);
static void pipe_producer_entry(uint64_t argc, char **argv);
static void pipe_consumer_entry(uint64_t argc, char **argv);
static void pipe_broken_writer_entry(uint64_t argc, char **argv);
static void pipeSyncWriter(uint64_t argc, char **argv);
static void pipeSyncReader(uint64_t argc, char **argv);
static void pipeStressWriter(uint64_t argc, char **argv);
static void pipeStressReader(uint64_t argc, char **argv);

static int divzero_cmd(void);
static int invop_cmd(void);
static int pipe_demo(void);
static int pipe_eof_cmd(void);
static int pipe_broken_cmd(void);
static int pipe_sync_cmd(void);
static int pipe_stress_cmd(void);

static int block(void);
static int clear(void);
static int shell_exit(void);
static int font(void);
static int help(void);
static int history(void);
static int killcmd(void);
static int man(void);
static int memcmd(void);
static int nice(void);
static int regs(void);
static int test_mm_command(void);
static int test_prio_command(void);
static int test_sync_command(void);
static int time_cmd(void);
static int yield_cmd(void);

static void printSpaces(int count);
static int digitsForInt(int value);
static int digitsForHex(uint64_t value);
static void printIntColumn(int value, int width);
static void printHexColumn(uint64_t value, int width);
static void printStringColumn(const char *value, int width);
static int parsePid(const char *arg, int *pidOut);

Command commands[] = {
    {.name = "block",   .isProcess = 0, .builtin = block,      .entry = 0,             .description = "Toggles a process between BLOCKED and READY"},
    {.name = "cat",     .isProcess = 1, .builtin = 0,          .entry = cat_entry,     .description = "Echo stdin to stdout until EOF"},
    {.name = "clear",   .isProcess = 0, .builtin = clear,      .entry = 0,             .description = "Clears the screen"},
    {.name = "divzero", .isProcess = 0, .builtin = divzero_cmd, .entry = 0,            .description = "Generates a division by zero exception"},
    {.name = "echo",    .isProcess = 1, .builtin = 0,          .entry = echo_entry,    .description = "Prints arguments to stdout"},
    {.name = "exit",    .isProcess = 0, .builtin = shell_exit, .entry = 0,             .description = "Command exits w/ the provided exit code or 0"},
    {.name = "filter",  .isProcess = 1, .builtin = 0,          .entry = filter_entry,  .description = "Filter vowels from stdin"},
    {.name = "font",    .isProcess = 0, .builtin = font,       .entry = 0,             .description = "Increase or decrease the font size"},
    {.name = "help",    .isProcess = 0, .builtin = help,       .entry = 0,             .description = "Prints the available commands"},
    {.name = "history", .isProcess = 0, .builtin = history,    .entry = 0,             .description = "Prints the command history"},
    {.name = "invop",   .isProcess = 0, .builtin = invop_cmd,  .entry = 0,             .description = "Generates an invalid Opcode exception"},
    {.name = "kill",    .isProcess = 0, .builtin = killcmd,    .entry = 0,             .description = "Kills a process by PID"},
    {.name = "loop",    .isProcess = 1, .builtin = 0,          .entry = loop_entry,    .description = "Prints PID every N seconds (default: 1)"},
    {.name = "man",     .isProcess = 0, .builtin = man,        .entry = 0,             .description = "Prints the description of the provided command"},
    {.name = "mem",     .isProcess = 0, .builtin = memcmd,     .entry = 0,             .description = "Displays kernel memory usage"},
    {.name = "mvar",    .isProcess = 0, .builtin = mvar_cmd,   .entry = 0,             .description = "Multi-variable synchronization test"},
    {.name = "nice",    .isProcess = 0, .builtin = nice,       .entry = 0,             .description = "Changes a process priority"},
    {.name = "ps",      .isProcess = 1, .builtin = 0,          .entry = ps_entry,      .description = "Prints the process list"},
    {.name = "regs",    .isProcess = 0, .builtin = regs,       .entry = 0,             .description = "Prints the register snapshot, if any"},
    {.name = "test_mm", .isProcess = 0, .builtin = test_mm_command, .entry = 0,       .description = "Stress tests memory manager. Usage: test_mm <max_bytes>"},
    {.name = "test_prio", .isProcess = 0, .builtin = test_prio_command, .entry = 0,   .description = "Tests process priorities. Usage: test_prio <max_iterations>"},
    {.name = "test_process", .isProcess = 1, .builtin = 0,      .entry = test_process_entry, .description = "Creates, blocks and kills processes randomly"},
    {.name = "test_sync", .isProcess = 0, .builtin = test_sync_command, .entry = 0,   .description = "Tests semaphores. Usage: test_sync <n> <use_sem>"},
    {.name = "time",    .isProcess = 0, .builtin = time_cmd,   .entry = 0,             .description = "Prints the current time"},
    {.name = "wc",      .isProcess = 1, .builtin = 0,          .entry = wc_entry,      .description = "Count lines from stdin"},
    {.name = "yield",   .isProcess = 0, .builtin = yield_cmd,  .entry = 0,             .description = "Voluntarily yields the CPU"},
    {.name = "sleep2",  .isProcess = 1, .builtin = 0,          .entry = sleep2_sleeper, .description = "Foreground process that sleeps 2 seconds"},
    {.name = "print3",  .isProcess = 1, .builtin = 0,          .entry = print3_entry,   .description = "Prints three lines and exits"},
    {.name = "pipe_demo",   .isProcess = 0, .builtin = pipe_demo,    .entry = 0,      .description = "Demonstrates a simple pipe between two processes"},
    {.name = "pipe_eof",    .isProcess = 0, .builtin = pipe_eof_cmd, .entry = 0,      .description = "Shows EOF when writer closes"},
    {.name = "pipe_broken", .isProcess = 0, .builtin = pipe_broken_cmd, .entry = 0,   .description = "Shows broken pipe when no readers"},
    {.name = "pipe_sync",   .isProcess = 0, .builtin = pipe_sync_cmd, .entry = 0,     .description = "Tests pipe blocking/synchronization"},
    {.name = "pipe_stress", .isProcess = 0, .builtin = pipe_stress_cmd, .entry = 0,   .description = "Stress test with multiple writers/readers"}
};

const int command_count = (int)(sizeof(commands) / sizeof(commands[0]));

extern uint64_t test_mm(uint64_t argc, char *argv[]);
extern uint64_t test_prio(uint64_t argc, char *argv[]);
extern uint64_t test_sync(uint64_t argc, char *argv[]);
extern int64_t test_processes(uint64_t argc, char *argv[]);

static void test_prio_context_init(void)
{
    if (testPrioContext.argv[0] == NULL)
    {
        testPrioContext.argv[0] = testPrioContext.arg;
    }
}

static void refreshTestPrioRunning(void)
{
    if (!testPrioRunning)
    {
        return;
    }

    ProcessInfo snapshot[PROCESS_SNAPSHOT_CAP];
    int32_t count = getProcesses(snapshot, PROCESS_SNAPSHOT_CAP);
    for (int32_t i = 0; i < count; i++)
    {
        if (strcmp(snapshot[i].name, "test_prio") == 0 && snapshot[i].state != TERMINATED)
        {
            return;
        }
    }

    testPrioRunning = 0;
}

static void test_mm_slot_init(void)
{
    if (testMmSlotsInitialized)
    {
        return;
    }

    for (int i = 0; i < TEST_MM_MAX_INSTANCES; i++)
    {
        testMmSlots[i].pid = 0;
        testMmSlots[i].arg[0] = '\0';
        testMmSlots[i].argv[0] = testMmSlots[i].arg;
    }

    testMmSlotsInitialized = 1;
}

static void test_mm_cleanup_slots(void)
{
    test_mm_slot_init();

    ProcessInfo processes[PROCESS_SNAPSHOT_CAP] = {0};
    int32_t count = getProcesses(processes, PROCESS_SNAPSHOT_CAP);

    if (count <= 0)
    {
        return;
    }

    for (int i = 0; i < TEST_MM_MAX_INSTANCES; i++)
    {
        if (testMmSlots[i].pid <= 0)
        {
            continue;
        }

        int pid = testMmSlots[i].pid;
        uint8_t found = 0;

        for (int j = 0; j < count; j++)
        {
            if (processes[j].pid == pid)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            testMmSlots[i].pid = 0;
            testMmSlots[i].arg[0] = '\0';
        }
    }
}

static TestMmSlot *test_mm_acquire_slot(void)
{
    test_mm_slot_init();

    for (int i = 0; i < TEST_MM_MAX_INSTANCES; i++)
    {
        if (testMmSlots[i].pid == 0)
        {
            return &testMmSlots[i];
        }
    }

    return NULL;
}

static TestMmSlot *test_mm_find_slot_by_argv(char **argv)
{
    if (argv == NULL)
    {
        return NULL;
    }

    test_mm_slot_init();

    for (int i = 0; i < TEST_MM_MAX_INSTANCES; i++)
    {
        if (testMmSlots[i].argv == argv)
        {
            return &testMmSlots[i];
        }
    }

    return NULL;
}

static int history(void)
{
    uint8_t last = command_history_last;
    DEC_MOD(last, HISTORY_SIZE);
    uint8_t i = 0;
    while (i < HISTORY_SIZE && command_history[last][0] != 0)
    {
        printf("%d. %s\n", i, command_history[last]);
        DEC_MOD(last, HISTORY_SIZE);
        i++;
    }
    return 0;
}

static int time_cmd(void)
{
    int hour, minute, second;
    getDate(&hour, &minute, &second);
    printf("Current time: %xh %xm %xs\n", hour, minute, second);
    return 0;
}

static int yield_cmd(void)
{
    if (yieldProcess() != 0)
    {
        perror("Failed to yield CPU\n");
        return 1;
    }
    return 0;
}

static int test_prio_command(void)
{
    char *arg = NULL;
    char *token = NULL;

    refreshTestPrioRunning();

    while ((token = strtok(NULL, " ")) != NULL)
    {
        if (strcmp(token, "&") == 0)
        {
            continue;
        }

        if (arg == NULL)
        {
            arg = token;
            continue;
        }

        fprintf(FD_STDERR, "test_prio accepts exactly one parameter\n");
        return 1;
    }

    if (arg == NULL)
    {
        fprintf(FD_STDERR, "Usage: test_prio <max_iterations>\n");
        return 1;
    }

    if (testPrioRunning)
    {
        fprintf(FD_STDERR, "test_prio is already running.\n");
        return 1;
    }

    test_prio_context_init();
    strncpy(testPrioContext.arg, arg, MAX_BUFFER_SIZE - 1);
    testPrioContext.arg[MAX_BUFFER_SIZE - 1] = '\0';

    uint8_t runInBackground = getCurrentBuiltinBackground();

    testPrioRunning = 1;
    int pid = createProcess("test_prio", test_prio_entry, testPrioContext.argv, 1, 0, 0, 0, runInBackground ? 0 : 1);
    if (pid <= 0)
    {
        fprintf(FD_STDERR, "Failed to start test_prio process\n");
        testPrioRunning = 0;
        return 1;
    }

    if (!runInBackground)
    {
        waitProcess(pid);
        testPrioRunning = 0;
    }

    return 0;
}

static int test_sync_command(void)
{
    char *arg1 = NULL;
    char *arg2 = NULL;
    char *token = NULL;

    while ((token = strtok(NULL, " ")) != NULL)
    {
        if (strcmp(token, "&") == 0)
        {
            continue;
        }

        if (arg1 == NULL)
        {
            arg1 = token;
            continue;
        }

        if (arg2 == NULL)
        {
            arg2 = token;
            continue;
        }

        fprintf(FD_STDERR, "test_sync accepts exactly two parameters\n");
        return 1;
    }

    if (arg1 == NULL || arg2 == NULL)
    {
        fprintf(FD_STDERR, "Usage: test_sync <n> <use_sem>\n");
        fprintf(FD_STDERR, "  n: number of iterations per process\n");
        fprintf(FD_STDERR, "  use_sem: 0 = no semaphores, 1 = use semaphores\n");
        return 1;
    }

    char *argv[3];
    argv[0] = arg1;
    argv[1] = arg2;
    argv[2] = NULL;

    uint64_t result = test_sync(2, argv);

    if (result != 0)
    {
        fprintf(FD_STDERR, "test_sync failed with code %lld\n", (long long)result);
        return 1;
    }

    return 0;
}

static int test_mm_command(void)
{
    test_mm_slot_init();
    test_mm_cleanup_slots();

    char *arg = NULL;
    char *token = NULL;

    while ((token = strtok(NULL, " ")) != NULL)
    {
        if (strcmp(token, "&") == 0)
        {
            continue;
        }

        if (arg == NULL)
        {
            arg = token;
            continue;
        }

        fprintf(FD_STDERR, "test_mm accepts exactly one parameter\n");
        return 1;
    }

    if (arg == NULL)
    {
        fprintf(FD_STDERR, "Usage: test_mm <max_memory_bytes>\n");
        return 1;
    }

    size_t argLen = strlen(arg);
    if (argLen == 0 || argLen >= MAX_BUFFER_SIZE)
    {
        fprintf(FD_STDERR, "test_mm argument is too long\n");
        return 1;
    }

    TestMmSlot *slot = test_mm_acquire_slot();
    if (slot == NULL)
    {
        fprintf(FD_STDERR, "test_mm is already running (max %d instances).\n", TEST_MM_MAX_INSTANCES);
        return 1;
    }

    strncpy(slot->arg, arg, MAX_BUFFER_SIZE - 1);
    slot->arg[MAX_BUFFER_SIZE - 1] = '\0';

    slot->pid = -1;
    uint8_t runInBackground = getCurrentBuiltinBackground();
    int pid = createProcess("test_mm", test_mm_entry, slot->argv, 1, 0, 0, 0, runInBackground ? 0 : 1);

    if (pid <= 0)
    {
        fprintf(FD_STDERR, "Failed to start test_mm process\n");
        slot->pid = 0;
        slot->arg[0] = '\0';
        return 1;
    }

    slot->pid = pid;

    if (!runInBackground)
    {
        waitProcess(pid);
        test_mm_cleanup_slots();
    }

    return 0;
}

static void test_mm_entry(uint64_t argc, char **argv)
{
    TestMmSlot *slot = test_mm_find_slot_by_argv(argv);
    int exitCode = 0;

    if (argc != 1 || argv == NULL || argv[0] == NULL)
    {
        fprintf(FD_STDERR, "test_mm: invalid arguments\n");
        exitCode = -1;
        goto cleanup;
    }

    while (1)
    {
        uint64_t result = test_mm(argc, argv);
        if (result != 0)
        {
            long long signed_result = (long long)result;
            fprintf(FD_STDERR, "test_mm stopped with code %lld\n", signed_result);
            exitCode = (int)signed_result;
            break;
        }
    }

cleanup:
    if (slot != NULL)
    {
        slot->pid = 0;
        slot->arg[0] = '\0';
    }

    exitProcess(exitCode);
}

static void test_prio_entry(uint64_t argc, char **argv)
{
    uint64_t result = test_prio(argc, argv);
    testPrioRunning = 0;
    if (result != 0)
    {
        fprintf(FD_STDERR, "test_prio failed with code %lld\n", (long long)result);
        exitProcess((int)result);
    }
    exitProcess(0);
}

static void test_process_entry(uint64_t argc, char **argv)
{
    if (argc < 2 || argv == NULL || argv[1] == NULL)
    {
        fprintf(FD_STDERR, "Usage: test_process <max_processes>\n");
        exitProcess(1);
        return;
    }

    if (argc > 2)
    {
        fprintf(FD_STDERR, "test_process accepts exactly one parameter\n");
        exitProcess(1);
        return;
    }

    char *args[] = {argv[1], NULL};
    int64_t result = test_processes(1, args);

    if (result != 0)
    {
        fprintf(FD_STDERR, "test_process failed with code %lld\n", (long long)result);
    }

    exitProcess((int)result);
}

static int help(void)
{
    printf("Available commands:\n");
    for (int i = 0; i < command_count; i++)
    {
        printf("%s%s\t ---\t%s\n", commands[i].name, strlen(commands[i].name) < 4 ? "\t" : "", commands[i].description);
    }
    printf("\n");
    return 0;
}

static int clear(void)
{
    clearScreen();
    return 0;
}

static int shell_exit(void)
{
    char *token = strtok(NULL, " ");
    int aux = 0;
    if (token != NULL)
    {
        sscanf(token, "%d", &aux);
    }
    return aux;
}

static int font(void)
{
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
        perror("Invalid argument\n");
        return 1;
    }

    if (strcasecmp(arg, "increase") == 0)
    {
        return increaseFontSize();
    }
    if (strcasecmp(arg, "decrease") == 0)
    {
        return decreaseFontSize();
    }

    perror("Invalid argument\n");
    return 1;
}

static int man(void)
{
    char *command = strtok(NULL, " ");

    if (command == NULL)
    {
        perror("No argument provided\n");
        return 1;
    }

    for (int i = 0; i < command_count; i++)
    {
        if (strcasecmp(commands[i].name, command) == 0)
        {
            printf("Command: %s\nInformation: %s\n", commands[i].name, commands[i].description);
            return 0;
        }
    }

    perror("Command not found\n");
    return 1;
}

static int killcmd(void)
{
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
        perror("Missing PID\n");
        return 1;
    }

    int pid = 0;
    if (parsePid(arg, &pid) != 0)
    {
        perror("Invalid PID\n");
        return 1;
    }
    int32_t res = killProcess(pid);
    if (res == 0)
    {
        printf("Process %d killed\n", pid);
        return 0;
    }

    perror("Failed to kill process\n");
    return 1;
}

static int block(void)
{
    char *arg = strtok(NULL, " ");
    if (arg == NULL)
    {
        perror("Missing PID\n");
        return 1;
    }

    int pid = 0;
    if (parsePid(arg, &pid) != 0)
    {
        perror("Invalid PID\n");
        return 1;
    }
    int32_t state = toggleBlockProcess(pid);

    if (state == BLOCKED)
    {
        printf("Process %d blocked\n", pid);
        return 0;
    }

    if (state == READY)
    {
        printf("Process %d ready\n", pid);
        return 0;
    }

    perror("Failed to toggle process\n");
    return 1;
}

static int memcmd(void)
{
    char info[160] = {0};
    int32_t written = getMemoryState(info, sizeof(info));

    if (written <= 0)
    {
        perror("Failed to read memory state\n");
        return 1;
    }

    printf("%s\n", info);
    return 0;
}

static int ps(void)
{
    ProcessInfo processes[PROCESS_SNAPSHOT_CAP] = {0};
    int32_t count = getProcesses(processes, PROCESS_SNAPSHOT_CAP);

    if (count <= 0)
    {
        printf("No active processes\n");
        return 0;
    }

    const char *stateNames[] = {
        [READY] = "READY",
        [RUNNING] = "RUNNING",
        [TERMINATED] = "TERMINATED",
        [BLOCKED] = "BLOCKED"};

    printStringColumn("PID", PID_COL_WIDTH);
    printSpaces(COLUMN_PADDING);
    printStringColumn("NAME", NAME_COL_WIDTH);
    printSpaces(COLUMN_PADDING);
    printStringColumn("STATE", STATE_COL_WIDTH);
    printSpaces(COLUMN_PADDING);
    printStringColumn("PRIORITY", PRIORITY_COL_WIDTH);
    printSpaces(COLUMN_PADDING);
    printStringColumn("FG/BG", FG_COL_WIDTH);
    printSpaces(COLUMN_PADDING);
    printStringColumn("STACK", STACK_COL_WIDTH);
    printSpaces(COLUMN_PADDING);
    printStringColumn("BASE", BASE_COL_WIDTH);
    printf("\n");

    for (int i = 0; i < count; i++)
    {
        const ProcessInfo *info = &processes[i];
        const char *state = "UNKNOWN";

        if (info->state >= READY && info->state <= BLOCKED && stateNames[info->state] != NULL)
        {
            state = stateNames[info->state];
        }

        printIntColumn(info->pid, PID_COL_WIDTH);
        printSpaces(COLUMN_PADDING);
        printStringColumn(info->name, NAME_COL_WIDTH);
        printSpaces(COLUMN_PADDING);
        printStringColumn(state, STATE_COL_WIDTH);
        printSpaces(COLUMN_PADDING);
        printIntColumn(info->priority, PRIORITY_COL_WIDTH);
        const char *fg = info->foreground ? "FG" : "BG";
        printSpaces(COLUMN_PADDING);
        printStringColumn(fg, FG_COL_WIDTH);
        printSpaces(COLUMN_PADDING);
        printHexColumn((uint64_t)info->stackPointer, STACK_COL_WIDTH);
        printSpaces(COLUMN_PADDING);
        printHexColumn((uint64_t)info->basePointer, BASE_COL_WIDTH);
        printf("\n");
    }

    return 0;
}

static int regs(void)
{
    const static char *register_names[] = {
        "rax", "rbx", "rcx", "rdx", "rbp", "rdi", "rsi", "r8 ", "r9 ", "r10", "r11", "r12", "r13", "r14", "r15", "rsp", "rip", "rflags"};

    int64_t registers[18];

    uint8_t aux = getRegisterSnapshot(registers);

    if (aux == 0)
    {
        perror("No register snapshot available\n");
        return 1;
    }

    printf("Latest register snapshot:\n");

    for (int i = 0; i < 18; i++)
    {
        printf("\e[0;34m%s\e[0m: %llx\n", register_names[i], (unsigned long long)registers[i]);
    }

    return 0;
}

static void loop_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    int pid = getPid();

    char *secondsArg = strtok(NULL, " ");
    uint32_t seconds = 1;

    if (secondsArg != NULL)
    {
        int parsed = 0;
        if (parsePid(secondsArg, &parsed) == 0 && parsed > 0)
        {
            seconds = (uint32_t)parsed;
        }
    }

    uint32_t milliseconds = seconds * 1000;

    if (milliseconds == 0 || milliseconds > 60000)
    {
        milliseconds = 2000;
    }

    while (1) //-V776
    {
        printf("Hola, soy el proceso %d\n", pid);
        sleep(milliseconds);
    }
}

static int nice(void)
{
    char *pidArg = strtok(NULL, " ");
    char *priorityArg = strtok(NULL, " ");
    if (pidArg == NULL || priorityArg == NULL)
    {
        perror("Uso: nice <pid> <prioridad>\n");
        return 1;
    }

    int pid = 0;
    if (parsePid(pidArg, &pid) != 0)
    {
        perror("Invalid PID\n");
        return 1;
    }

    int priority = 0;
    if (parsePid(priorityArg, &priority) != 0)
    {
        perror("Invalid priority\n");
        return 1;
    }

    if (pid == PROCESS_IDLE_PID)
    {
        perror("No se puede cambiar la prioridad del proceso idle\n");
        return 1;
    }

    if (priority < PROCESS_PRIORITY_MIN || priority > PROCESS_PRIORITY_MAX)
    {
        perror("Prioridad fuera de rango\n");
        return 1;
    }

    uint32_t res = setProcessPriority(pid, priority);
    if (res == 0)
    {
        printf("PID %d ahora tiene prioridad %d\n", pid, priority);
        return 0;
    }

    perror("No se pudo actualizar la prioridad\n");
    return 1;
}

static void sleep2_sleeper(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    sleep(2000);
    putchar('.');
    exitProcess(0);
}

static void print3_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    for (int i = 1; i <= 3; i++)
    {
        printf("print3: line %d\n", i);
    }
    exitProcess(0);
}

static void ps_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    ps();
    exitProcess(0);
}

static int pipe_demo(void)
{
    int fds[2];
    if (pipe(fds) < 0)
    {
        fprintf(FD_STDERR, "pipe_demo: pipe() failed\n");
        return 1;
    }

    const int savedIn = 10;
    const int savedOut = 11;
    close(savedIn);
    close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);

    dup2(fds[1], FD_STDOUT);
    int prodPid = createProcess("prod", pipe_producer_entry, 0, 0, 0, 0, 0, 0);
    dup2(savedOut, FD_STDOUT);
    close(fds[1]);

    dup2(fds[0], FD_STDIN);
    int consPid = createProcess("cons", pipe_consumer_entry, 0, 0, 0, 0, 0, 0);
    dup2(savedIn, FD_STDIN);
    close(fds[0]);

    close(savedIn);
    close(savedOut);

    if (prodPid > 0)
        waitProcess(prodPid);
    if (consPid > 0)
        waitProcess(consPid);
    return 0;
}

static int pipe_eof_cmd(void)
{
    int fds[2];
    if (pipe(fds) < 0)
    {
        fprintf(FD_STDERR, "pipe_eof: pipe() failed\n");
        return 1;
    }
    const int savedIn = 10;
    close(savedIn);
    dup2(FD_STDIN, savedIn);

    close(fds[1]);

    dup2(fds[0], FD_STDIN);
    int consPid = createProcess("cons", pipe_consumer_entry, 0, 0, 0, 0, 0, 0);
    dup2(savedIn, FD_STDIN);
    close(fds[0]);
    close(savedIn);

    if (consPid > 0)
    {
        waitProcess(consPid);
    }
    return 0;
}

static int pipe_broken_cmd(void)
{
    int fds[2];
    if (pipe(fds) < 0)
    {
        fprintf(FD_STDERR, "pipe_broken: pipe() failed\n");
        return 1;
    }
    const int savedOut = 11;
    close(savedOut);
    dup2(FD_STDOUT, savedOut);

    close(fds[0]);
    dup2(fds[1], FD_STDOUT);
    int pid = createProcess("writer", pipe_broken_writer_entry, 0, 0, 0, 0, 0, 0);
    dup2(savedOut, FD_STDOUT);
    close(fds[1]);
    close(savedOut);

    if (pid > 0)
        waitProcess(pid);
    return 0;
}

static int pipe_sync_cmd(void)
{
    printf("pipe_sync: Testing pipe blocking/EOF behavior...\n");

    int fds[2];
    if (pipe(fds) < 0)
    {
        fprintf(FD_STDERR, "pipe_sync: pipe() failed\n");
        return 1;
    }

    int savedIn = 12;
    int savedOut = 13;
    close(savedIn);
    close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);

    dup2(fds[1], FD_STDOUT);
    int writerPid = createProcess("writer", pipeSyncWriter, 0, 0, 0, 0, 0, 0);
    dup2(savedOut, FD_STDOUT);
    close(fds[1]);

    dup2(fds[0], FD_STDIN);
    int readerPid = createProcess("reader", pipeSyncReader, 0, 0, 0, 0, 0, 0);
    dup2(savedIn, FD_STDIN);
    close(fds[0]);

    close(savedIn);
    close(savedOut);

    if (writerPid <= 0 || readerPid <= 0)
    {
        fprintf(FD_STDERR, "pipe_sync: failed to spawn processes\n");
        return 1;
    }

    int writerStatus = waitProcess(writerPid);
    int readerStatus = waitProcess(readerPid);

    if (writerStatus == 0 && readerStatus == 0)
    {
        printf("pipe_sync: OK - Writer sent 2000 bytes, reader received all and detected EOF\n");
    }
    else
    {
        printf("pipe_sync: FAILED\n");
    }

    return 0;
}

static int pipe_stress_cmd(void)
{
    int numWriters = 3;
    int numReaders = 2;

    char *arg1 = strtok(NULL, " ");
    char *arg2 = strtok(NULL, " ");

    if (arg1 != NULL && strcmp(arg1, "&") != 0)
    {
        int val = 0;
        if (parsePid(arg1, &val) == 0 && val > 0 && val <= 10)
        {
            numWriters = val;
        }
    }

    if (arg2 != NULL && strcmp(arg2, "&") != 0)
    {
        int val = 0;
        if (parsePid(arg2, &val) == 0 && val > 0 && val <= 10)
        {
            numReaders = val;
        }
    }

    printf("pipe_stress: Starting with %d writers and %d readers\n", numWriters, numReaders);

    int fds[2];
    if (pipe(fds) < 0)
    {
        fprintf(FD_STDERR, "pipe_stress: pipe() failed\n");
        return 1;
    }

    int savedIn = 12;
    int savedOut = 13;
    close(savedIn);
    close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);

    int writerPids[10];
    for (int i = 0; i < numWriters; i++)
    {
        dup2(fds[1], FD_STDOUT);
        writerPids[i] = createProcess("writer", pipeStressWriter, 0, 0, 0, 0, 0, 0);
        dup2(savedOut, FD_STDOUT);

        if (writerPids[i] <= 0)
        {
            fprintf(FD_STDERR, "pipe_stress: failed to spawn writer %d\n", i);
        }
    }

    close(fds[1]);

    int readerPids[10];
    for (int i = 0; i < numReaders; i++)
    {
        dup2(fds[0], FD_STDIN);
        readerPids[i] = createProcess("reader", pipeStressReader, 0, 0, 0, 0, 0, 0);
        dup2(savedIn, FD_STDIN);

        if (readerPids[i] <= 0)
        {
            fprintf(FD_STDERR, "pipe_stress: failed to spawn reader %d\n", i);
        }
    }

    close(fds[0]);

    close(savedIn);
    close(savedOut);

    for (int i = 0; i < numWriters; i++)
    {
        if (writerPids[i] > 0)
        {
            waitProcess(writerPids[i]);
        }
    }

    for (int i = 0; i < numReaders; i++)
    {
        if (readerPids[i] > 0)
        {
            waitProcess(readerPids[i]);
        }
    }

    printf("pipe_stress: All processes finished\n");
    return 0;
}

static int divzero_cmd(void)
{
    _divzero();
    return 0;
}

static int invop_cmd(void)
{
    _invalidopcode();
    return 0;
}

static void pipe_producer_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char *msg = "hello through pipe\nline 2\nline 3\n";
    sys_write(FD_STDOUT, msg, (int)strlen(msg));
    exitProcess(0);
}

static void pipe_consumer_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[1];
    int n;
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        sys_write(FD_STDOUT, buf, n);
    }
    const char *done = "[consumer done]\n";
    sys_write(FD_STDOUT, done, (int)strlen(done));
    exitProcess(0);
}

static void pipe_broken_writer_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char *msg = "x";
    int rc = sys_write(FD_STDOUT, msg, 1);
    if (rc < 0)
    {
        const char *note = "[broken pipe detected]\n";
        sys_write(FD_STDERR, note, (int)strlen(note));
    }
    exitProcess(0);
}

static void pipeSyncWriter(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char *msg = "W";
    int total = 2000;

    for (int i = 0; i < total; i++)
    {
        int w = sys_write(FD_STDOUT, msg, 1);
        if (w < 0)
        {
            exitProcess(1);
        }
    }

    exitProcess(0);
}

static void pipeSyncReader(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[100];
    int totalRead = 0;
    int n;

    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        totalRead += n;
    }

    if (totalRead != 2000)
    {
        exitProcess(1);
    }

    exitProcess(0);
}

static void pipeStressWriter(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    int myPid = getPid();
    srand(myPid);

    for (int fd = 0; fd < 20; fd++)
    {
        if (fd != FD_STDOUT && fd != FD_STDERR)
        {
            close(fd);
        }
    }

    const char *msg = "X";
    int total = 1000;

    for (int i = 0; i < total; i++)
    {
        int w = sys_write(FD_STDOUT, msg, 1);
        if (w < 0)
        {
            fprintf(FD_STDERR, "Writer %d: write failed\n", myPid);
            exitProcess(1);
        }

        if ((rand() % 10) == 0)
        {
            sys_yield();
        }

        if ((i + 1) % 250 == 0)
        {
            fprintf(FD_STDERR, "Writer %d: %d bytes written\n", myPid, i + 1);
        }
    }

    fprintf(FD_STDERR, "Writer %d: finished (%d bytes)\n", myPid, total);
    exitProcess(0);
}

static void pipeStressReader(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    int myPid = getPid();
    srand(myPid);

    for (int fd = 1; fd < 20; fd++)
    {
        if (fd != FD_STDERR)
        {
            close(fd);
        }
    }

    char buf[100];
    int totalRead = 0;
    int n;

    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        totalRead += n;

        if ((rand() % 10) == 0)
        {
            sys_yield();
        }
    }

    fprintf(FD_STDERR, "Reader %d: EOF detected, read %d bytes total\n", myPid, totalRead);
    exitProcess(0);
}

static void printSpaces(int count)
{
    while (count-- > 0)
    {
        putchar(' ');
    }
}

static int digitsForInt(int value)
{
    int len = 0;
    int aux = value;

    if (aux <= 0)
    {
        len = 1;
        aux = -aux;
    }

    while (aux > 0)
    {
        len++;
        aux /= 10;
    }

    return len;
}

static int digitsForHex(uint64_t value)
{
    int len = 0;

    if (value == 0)
    {
        return 1;
    }

    while (value > 0)
    {
        len++;
        value >>= 4;
    }

    return len;
}

static void printIntColumn(int value, int width)
{
    printf("%d", value);

    int len = digitsForInt(value);
    if (len < width)
    {
        printSpaces(width - len);
    }
}

static void printHexColumn(uint64_t value, int width)
{
    char buf[2 + 16 + 1];
    int idx = 0;

    buf[idx++] = '0';
    buf[idx++] = 'x';

    int hexDigits = digitsForHex(value);

    for (int i = hexDigits - 1; i >= 0; i--)
    {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        buf[idx++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }

    buf[idx] = 0;

    printf("%s", buf);

    if (idx < width)
    {
        printSpaces(width - idx);
    }
}

static void printStringColumn(const char *value, int width)
{
    if (value == NULL)
    {
        value = "";
    }

    int len = (int)strlen(value);
    printf("%s", value);

    if (len < width)
    {
        printSpaces(width - len);
    }
}

static int parsePid(const char *arg, int *pidOut)
{
    if (arg == NULL || pidOut == NULL || *arg == '\0')
    {
        return -1;
    }

    int sign = 1;
    size_t index = 0;

    if (arg[index] == '+')
    {
        index++;
    }
    else if (arg[index] == '-')
    {
        sign = -1;
        index++;
    }

    int value = 0;
    int digits = 0;

    for (; arg[index] != '\0'; index++)
    {
        char c = arg[index];
        if (c < '0' || c > '9')
        {
            return -1;
        }
        value = value * 10 + (c - '0');
        digits++;
    }

    if (digits == 0)
    {
        return -1;
    }

    *pidOut = sign * value;
    return 0;
}
