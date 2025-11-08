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

#ifdef ANSI_4_BIT_COLOR_SUPPORT
#include <ansiColors.h>
#endif

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10
#define PROCESS_SNAPSHOT_CAP 32
#define PID_COL_WIDTH 3
#define STATE_COL_WIDTH 9
#define FG_COL_WIDTH 4
#define PRIORITY_COL_WIDTH 8
#define NAME_COL_WIDTH 8
#define STACK_COL_WIDTH 10
#define BASE_COL_WIDTH 10
#define COLUMN_PADDING 2

#define FOREGROUND 1

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

int clear(void);
int exit(void);
int fontdec(void);
int font(void);
int help(void);
int history(void);
int block(void);
int man(void);
int memcmd(void);
int killcmd(void);
int regs(void);
int time(void);
int yield_cmd(void);
int ps(void);
int nice(void);
int test_mm_command(void);
int test_prio_command(void);
int test_sync_command(void);
int sleep2(void);
static void sleep2_sleeper(void *arg);
static void print3_entry(void *arg);
static void ps_entry(void *arg);
static void loop_entry(void *arg);
static void pipe_producer_entry(void *arg);
static void pipe_consumer_entry(void *arg);
static int pipe_demo(void);
static int pipe_eof_cmd(void);
static int pipe_broken_cmd(void);
static void pipe_broken_writer_entry(void *arg);
static int pipe_sync_cmd(void);
static void pipeSyncWriter(void *arg);
static void pipeSyncReader(void *arg);
static int pipe_stress_cmd(void);
static void pipeStressWriter(void *arg);
static void pipeStressReader(void *arg);
static int run_pipeline(const char *leftCmd, const char *rightCmd);
static int mvar_cmd(void);
static void mvar_writer_entry(uint64_t argc, char **argv);
static void mvar_reader_entry(uint64_t argc, char **argv);
static void trim(char *s);
static void test_process_entry(void *arg);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static void printSpaces(int count);
static int digitsForInt(int value);
static int digitsForHex(uint64_t value);
static void printIntColumn(int value, int width);
static void printHexColumn(uint64_t value, int width);
static void printStringColumn(const char *value, int width);
static int parsePid(const char *arg, int *pidOut);

#define TEST_MM_MAX_INSTANCES 4

typedef struct
{
    int pid;
    char arg[MAX_BUFFER_SIZE];
    char *argv[1];
} TestMmSlot;

static TestMmSlot testMmSlots[TEST_MM_MAX_INSTANCES];
static uint8_t testMmSlotsInitialized = 0;

static void test_mm_slot_init(void);
static void test_mm_cleanup_slots(void);
static TestMmSlot *test_mm_acquire_slot(void);
static TestMmSlot *test_mm_find_slot_by_argv(char **argv);
static void test_mm_entry(uint64_t argc, char **argv);


static uint8_t last_command_arrowed = 0;
static volatile uint8_t ctrl_c_requested = 0;
// Track foreground processes to allow Ctrl-C to kill them
static volatile int current_fg_pid = 0;
static volatile int current_pipeline_pids[2] = {0, 0};
// Small wrappers to adapt void exception triggers to builtin(int)(void)
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


typedef struct
{
    char *name;
    uint8_t isProcess;           // 0 = builtin (run inline), 1 = process (createProcess)
    int (*builtin)(void);        // used when isProcess == 0
    void (*entry)(void *);       // used when isProcess == 1
    char *description;
} Command;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    {.name = "block",   .isProcess = 0, .builtin = block,      .entry = 0,             .description = "Toggles a process between BLOCKED and READY"},
    {.name = "cat",     .isProcess = 1, .builtin = 0,          .entry = cat_entry, .description = "Echo stdin to stdout until EOF"},
    {.name = "clear",   .isProcess = 0, .builtin = clear,      .entry = 0,             .description = "Clears the screen"},
    {.name = "divzero", .isProcess = 0, .builtin = divzero_cmd, .entry = 0,             .description = "Generates a division by zero exception"},
    {.name = "echo",    .isProcess = 1, .builtin = 0,          .entry = echo_entry,    .description = "Prints arguments to stdout"},
    {.name = "exit",    .isProcess = 0, .builtin = exit,       .entry = 0,             .description = "Command exits w/ the provided exit code or 0"},
    {.name = "filter",  .isProcess = 1, .builtin = 0,          .entry = filter_entry, .description = "Filter vowels from stdin"},
    {.name = "font",    .isProcess = 0, .builtin = font,       .entry = 0,             .description = "Increases or decreases the font size.\n\t\t\t\tUse:\n\t\t\t\t\t  + font increase\n\t\t\t\t\t  + font decrease"},
    {.name = "help",    .isProcess = 0, .builtin = help,       .entry = 0,             .description = "Prints the available commands"},
    {.name = "history", .isProcess = 0, .builtin = history,    .entry = 0,             .description = "Prints the command history"},
    {.name = "invop",   .isProcess = 0, .builtin = invop_cmd,  .entry = 0,             .description = "Generates an invalid Opcode exception"},
    {.name = "kill",    .isProcess = 0, .builtin = killcmd,    .entry = 0,             .description = "Kills a process by PID"},
    {.name = "loop",    .isProcess = 1, .builtin = 0,          .entry = loop_entry,    .description = "Prints PID with greeting every N seconds (default: 1). Usage: loop [seconds]"},
    {.name = "man",     .isProcess = 0, .builtin = man,        .entry = 0,             .description = "Prints the description of the provided command"},
    {.name = "mem",     .isProcess = 0, .builtin = memcmd,     .entry = 0,             .description = "Displays kernel memory usage"},
    {.name = "mvar",    .isProcess = 0, .builtin = mvar_cmd,   .entry = 0,             .description = "Multi-variable synchronization test. Usage: mvar <writers> <readers>"},
    {.name = "nice",    .isProcess = 0, .builtin = nice,       .entry = 0,             .description = "Changes a process priority"},
    {.name = "ps",      .isProcess = 1, .builtin = 0,          .entry = ps_entry,      .description = "Prints the process list"},
    {.name = "regs",    .isProcess = 0, .builtin = regs,       .entry = 0,             .description = "Prints the register snapshot, if any"},
    {.name = "test_mm", .isProcess = 0, .builtin = test_mm_command, .entry = 0,       .description = "Stress tests memory manager with random blocks. Usage: test_mm <max_bytes>"},
    {.name = "test_prio", .isProcess = 0, .builtin = test_prio_command, .entry = 0,   .description = "Tests process priorities. Usage: test_prio <max_iterations>"},
    {.name = "test_process", .isProcess = 1, .builtin = 0, .entry = test_process_entry, .description = "Creates, blocks and kills processes randomly. Usage: test_process <max_processes>"},
    {.name = "test_sync", .isProcess = 0, .builtin = test_sync_command, .entry = 0,    .description = "Tests semaphores. Usage: test_sync <n> <use_sem> (0=no sync, 1=with sync)"},
    {.name = "time",    .isProcess = 0, .builtin = time,       .entry = 0,             .description = "Prints the current time"},
    {.name = "yield",   .isProcess = 0, .builtin = yield_cmd,  .entry = 0,             .description = "Voluntarily yields the CPU"},
    {.name = "wc",      .isProcess = 1, .builtin = 0,          .entry = wc_entry, .description = "Count lines from stdin"},
    // Process-style command example (entry must call sys_exit)
    {.name = "sleep2",  .isProcess = 1, .builtin = 0,          .entry = sleep2_sleeper, .description = "Runs a foreground process that sleeps 2 seconds"},
    {.name = "print3",  .isProcess = 1, .builtin = 0,          .entry = print3_entry,   .description = "Prints a line 3 times and exits"},
    {.name = "pipe_demo", .isProcess = 0, .builtin = pipe_demo, .entry = 0,             .description = "Demonstrates a simple pipe between two processes"},
    {.name = "pipe_eof", .isProcess = 0, .builtin = pipe_eof_cmd, .entry = 0,           .description = "Shows EOF when writer closes"},
    {.name = "pipe_broken", .isProcess = 0, .builtin = pipe_broken_cmd, .entry = 0,     .description = "Shows broken pipe when no readers"},
    {.name = "pipe_sync", .isProcess = 0, .builtin = pipe_sync_cmd, .entry = 0,         .description = "Tests pipe blocking/sync: writer blocks when full, reader when empty"},
    {.name = "pipe_stress", .isProcess = 0, .builtin = pipe_stress_cmd, .entry = 0,     .description = "Stress test with multiple writers/readers. Usage: pipe_stress [numWriters] [numReaders]"},
};

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

extern uint64_t test_mm(uint64_t argc, char *argv[]);
extern uint64_t test_prio(uint64_t argc, char *argv[]);
extern uint64_t test_sync(uint64_t argc, char *argv[]);
extern int64_t test_processes(uint64_t argc, char *argv[]);

int main()
{
    clear();

    registerKey(KP_UP_KEY, printPreviousCommand);
    registerKey(KP_DOWN_KEY, printNextCommand);

    while (1)
    {
        printf("\e[0mshell \e[0;32m$\e[0m ");

        signed char c;

        while (buffer_dim < MAX_BUFFER_SIZE && (c = getchar()) != '\n')
        {
            command_history_buffer[buffer_dim] = c;
            buffer[buffer_dim++] = c;
        }

        buffer[buffer_dim] = 0;
        command_history_buffer[buffer_dim] = 0;

        if (buffer_dim == MAX_BUFFER_SIZE)
        {
            perror("\e[0;31mShell buffer overflow\e[0m\n");
            buffer[0] = buffer_dim = 0;
            while (c != '\n')
                c = getchar();
            continue;
        };

        buffer[buffer_dim] = 0;

        char *command = strtok(buffer, " ");
        // Simple pipeline support: cmd1 | cmd2 (no args for now)
        // Check raw input for '|'
        char *pipeSep = NULL;
        for (int k = 0; k < buffer_dim; k++) {
            if (command_history_buffer[k] == '|') { pipeSep = &command_history_buffer[k]; break; }
        }
        if (pipeSep != NULL) {
            // Split the original line into left and right parts
            char left[MAX_BUFFER_SIZE];
            char right[MAX_BUFFER_SIZE];
            int len = 0;
            // copy left
            for (int k = 0; k < buffer_dim && &command_history_buffer[k] < pipeSep && len+1 < MAX_BUFFER_SIZE; k++) {
                left[len++] = command_history_buffer[k];
            }
            left[len] = '\0';
            // copy right
            int rlen = 0;
            for (int k = (int)(pipeSep - command_history_buffer) + 1; k < buffer_dim && rlen+1 < MAX_BUFFER_SIZE; k++) {
                right[rlen++] = command_history_buffer[k];
            }
            right[rlen] = '\0';
            trim(left);
            trim(right);

            // Extract command names (first token of each)
            char leftName[MAX_BUFFER_SIZE];
            char rightName[MAX_BUFFER_SIZE];
            leftName[0] = rightName[0] = '\0';
            // Parse first token from left
            int iL = 0, oL = 0;
            while (left[iL] == ' ' || left[iL] == '\t') iL++;
            while (left[iL] && left[iL] != ' ' && left[iL] != '\t') {
                if (oL+1 < MAX_BUFFER_SIZE) leftName[oL++] = left[iL];
                iL++;
            }
            leftName[oL] = '\0';
            // Parse first token from right
            int iR = 0, oR = 0;
            while (right[iR] == ' ' || right[iR] == '\t') iR++;
            while (right[iR] && right[iR] != ' ' && right[iR] != '\t') {
                if (oR+1 < MAX_BUFFER_SIZE) rightName[oR++] = right[iR];
                iR++;
            }
            rightName[oR] = '\0';

            if (leftName[0] == '\0' || rightName[0] == '\0') {
                fprintf(FD_STDERR, "Invalid pipeline. Usage: cmd1 | cmd2\n");
            } else if (run_pipeline(leftName, rightName) != 0) {
                // Error already printed by run_pipeline
            }

            // Record history and continue
            strncpy(command_history[command_history_last], command_history_buffer, 255);
            command_history[command_history_last][buffer_dim] = '\0';
            INC_MOD(command_history_last, HISTORY_SIZE);
            last_command_arrowed = command_history_last;
            buffer[0] = buffer_dim = 0;
            continue;
        }
        int i = 0;

        for (; i < sizeof(commands) / sizeof(Command); i++)
        {
            if (strcmp(commands[i].name, command) == 0)
            {
                // ¿Se pidió background con '&' al final?
                int runInBackground = 0;
                if (buffer_dim > 0)
                {
                    int j = buffer_dim - 1;
                    while (j >= 0 && (command_history_buffer[j] == ' ' || command_history_buffer[j] == '\t'))
                        j--;
                    if (j >= 0 && command_history_buffer[j] == '&')
                    {
                        runInBackground = 1;
                    }
                }

                if (commands[i].isProcess)
                {
                    // Parse arguments for process commands
                    char *argv[32];  // Max 32 arguments
                    int argc = 0;
                    
                    // First argument is the command name
                    argv[argc++] = commands[i].name;
                    
                    // Parse remaining arguments using strtok
                    char *token;
                    while ((token = strtok(NULL, " ")) != NULL && argc < 31)
                    {
                        // Skip '&' if it's the last token
                        if (strcmp(token, "&") == 0)
                        {
                            break;
                        }
                        argv[argc++] = token;
                    }
                    argv[argc] = NULL;  // Null-terminate the array
                    
                    int pid = createProcess(commands[i].name, (void (*)(void *))commands[i].entry, argv, argc, 0, 0, 0, runInBackground ? 0 : 1);
                    (void)pid;
                    if (!runInBackground && pid > 0)
                    {
                        waitProcess(pid);
                    }
                }
                else
                {
                    last_command_output = commands[i].builtin();
                }
                strncpy(command_history[command_history_last], command_history_buffer, 255);
                command_history[command_history_last][buffer_dim] = '\0';
                INC_MOD(command_history_last, HISTORY_SIZE);
                last_command_arrowed = command_history_last;
                break;
            }
        }

        // If the command is not found, ignore \n
        if (i == sizeof(commands) / sizeof(Command))
        {
            if (command != NULL && *command != '\0')
            {
                fprintf(FD_STDERR, "\e[0;33mCommand not found:\e[0m %s\n", command);
            }
            else if (command == NULL)
            {
                printf("\n");
            }
        }

        buffer[0] = buffer_dim = 0;
    }

    __builtin_unreachable();
    return 0;
}

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode)
{
    clearInputBuffer();
    last_command_arrowed = SUB_MOD(last_command_arrowed, 1, HISTORY_SIZE);
    if (command_history[last_command_arrowed][0] != 0)
    {
        fprintf(FD_STDIN, command_history[last_command_arrowed]);
    }
}

static void printNextCommand(enum REGISTERABLE_KEYS scancode)
{
    clearInputBuffer();
    last_command_arrowed = (last_command_arrowed + 1) % HISTORY_SIZE;
    if (command_history[last_command_arrowed][0] != 0)
    {
        fprintf(FD_STDIN, command_history[last_command_arrowed]);
    }
}

static void handleCtrlC(enum REGISTERABLE_KEYS scancode)
{
    (void)scancode;
    clearInputBuffer();
    printf("\n");  // Just print newline to move to next line
    buffer_dim = 0;
    buffer[0] = 0;
    command_history_buffer[0] = 0;
    ctrl_c_requested = 1;

    // If a foreground process (or simple pipeline) is running, kill it/them
    if (current_fg_pid > 0)
    {
        killProcess(current_fg_pid);
        current_fg_pid = 0;
    }
    
    // For pipelines, only kill the writer (left process)
    // This allows the reader (right process) to receive EOF and finish gracefully
    if (current_pipeline_pids[0] > 0)
    {
        killProcess(current_pipeline_pids[0]);
        current_pipeline_pids[0] = 0;
    }
    // Don't kill the reader - let it finish naturally after EOF
}

uint8_t ctrlCIsPending(void)
{
    return ctrl_c_requested;
}

static void consumeCtrlC(void)
{
    ctrl_c_requested = 0;
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

int history(void)
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

int time(void)
{
    int hour, minute, second;
    getDate(&hour, &minute, &second);
    printf("Current time: %xh %xm %xs\n", hour, minute, second);
    return 0;
}

int yield_cmd(void)
{
    if (yieldProcess() != 0)
    {
        perror("Failed to yield CPU\n");
        return 1;
    }
    return 0;
}

int test_prio_command(void)
{
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

        fprintf(FD_STDERR, "test_prio accepts exactly one parameter\n");
        return 1;
    }

    if (arg == NULL)
    {
        fprintf(FD_STDERR, "Usage: test_prio <max_iterations>\n");
        return 1;
    }

    char *argv[2];
    argv[0] = arg;
    argv[1] = NULL;

    uint64_t result = test_prio(1, argv);

    if (result != 0)
    {
        fprintf(FD_STDERR, "test_prio failed with code %lld\n", (long long)result);
        return 1;
    }

    return 0;
}

int test_sync_command(void)
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
        fprintf(FD_STDERR, "  use_sem: 0 = no semaphores (race condition), 1 = use semaphores\n");
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

int test_mm_command(void)
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
    int pid = createProcess("test_mm", (void (*)(void *))test_mm_entry, slot->argv, 1, 0, 0, 0, 0);

    if (pid <= 0)
    {
        fprintf(FD_STDERR, "Failed to start test_mm process\n");
        slot->pid = 0;
        slot->arg[0] = '\0';
        return 1;
    }

    slot->pid = pid;
    printf("test_mm running in background (PID %d).\n", pid);
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

static void test_process_entry(void *arg)
{
    (void)arg;

    char *maxProcArg = NULL;
    char *token = NULL;

    while ((token = strtok(NULL, " ")) != NULL)
    {
        if (strcmp(token, "&") == 0)
        {
            continue;
        }

        if (maxProcArg == NULL)
        {
            maxProcArg = token;
            continue;
        }

        fprintf(FD_STDERR, "test_process accepts exactly one parameter\n");
        exitProcess(1);
    }

    if (maxProcArg == NULL)
    {
        fprintf(FD_STDERR, "Usage: test_process <max_processes>\n");
        exitProcess(1);
    }

    char *argv[] = {maxProcArg, NULL};
    int64_t result = test_processes(1, argv);

    if (result != 0)
    {
        fprintf(FD_STDERR, "test_process failed with code %lld\n", (long long)result);
    }

    exitProcess((int)result);
}

int echo(void)
{
    for (int i = strlen("echo") + 1; i < buffer_dim; i++)
    {
        switch (buffer[i])
        {
        case '\\':
            switch (buffer[i + 1])
            {
            case 'n':
                printf("\n");
                i++;
                break;
            case 'e':
#ifdef ANSI_4_BIT_COLOR_SUPPORT
                i++;
                parseANSI(buffer, &i);
#else
                while (buffer[i] != 'm')
                    i++; // ignores escape code, assumes valid format
                i++;
#endif
                break;
            case 'r':
                printf("\r");
                i++;
                break;
            case '\\':
                i++;
            default:
                putchar(buffer[i]);
                break;
            }
            break;
        case '$':
            if (buffer[i + 1] == '?')
            {
                printf("%d", last_command_output);
                i++;
                break;
            }
        default:
            putchar(buffer[i]);
            break;
        }
    }
    printf("\n");
    return 0;
}

int help(void)
{
    printf("Available commands:\n");
    for (int i = 0; i < sizeof(commands) / sizeof(Command); i++)
    {
        printf("%s%s\t ---\t%s\n", commands[i].name, strlen(commands[i].name) < 4 ? "\t" : "", commands[i].description);
    }
    printf("\n");
    return 0;
}

int clear(void)
{
    clearScreen();
    return 0;
}

int exit(void)
{
    char *buffer = strtok(NULL, " ");
    int aux = 0;
    sscanf(buffer, "%d", &aux);
    return aux;
}

int font(void)
{
    char *arg = strtok(NULL, " ");
    if (strcasecmp(arg, "increase") == 0)
    {
        return increaseFontSize();
    }
    else if (strcasecmp(arg, "decrease") == 0)
    {
        return decreaseFontSize();
    }

    perror("Invalid argument\n");
    return 0;
}

int man(void)
{
    char *command = strtok(NULL, " ");

    if (command == NULL)
    {
        perror("No argument provided\n");
        return 1;
    }

    for (int i = 0; i < sizeof(commands) / sizeof(Command); i++)
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

int killcmd(void)
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

int block(void)
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

int memcmd(void)
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

int ps(void)
{
    ProcessInfo processes[PROCESS_SNAPSHOT_CAP] = {0};
    int32_t count = getProcesses(processes, PROCESS_SNAPSHOT_CAP);

    // bastante raro si entra aca...
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
        char *fg = info->foreground ? "FG" : "BG";
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

int regs(void)
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
        printf("\e[0;34m%s\e[0m: %x\n", register_names[i], registers[i]);
    }

    return 0;
}

static void loop_entry(void *arg)
{
    (void)arg;
    int pid = getPid();

    // Parse seconds from command line if provided, default to 1 second
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

    // When used in a pipeline, we might not have parsed args correctly
    // so just use a fixed interval
    if (milliseconds == 0 || milliseconds > 60000) {
        milliseconds = 2000; // Default to 2 seconds
    }

    while (1)
    {
        printf("Hola, soy el proceso %d\n", pid);
        sleep(milliseconds);
    }

    exitProcess(0);
}

int nice(void)
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
    if (parsePid(priorityArg, &priority) != 0) //! esto esta dudoso
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

int backgroundTest(void)
{
    while (1)
    {
        putchar('.');
        sleep(1000);
    }
    return 0;
}

// Foreground sleep process demo
static void sleep2_sleeper(void *arg)
{
    (void)arg;
    sleep(2000);
    putchar('.');
    exitProcess(0);
}

int sleep2(void)
{
    int pid = createProcess("sleep2", sleep2_sleeper, 0, 0, 0, 0, 0, FOREGROUND);
    if (pid <= 0)
    {
        perror("Failed to create sleep2 process\n");
        return 1;
    }

    // Block shell until the foreground process completes
    waitProcess(pid);
    printf("sleep2: done\n");
    return 0;
}

// Simple process that prints 3 lines then exits (no sleep syscall used)
static void print3_entry(void *arg)
{
    (void)arg;
    for (int i = 1; i <= 3; i++)
    {
        printf("print3: line %d\n", i);
    }
    exitProcess(0);
}

// ps as a process entry: reuse the existing ps() implementation and exit
static void ps_entry(void *arg)
{
    (void)arg;
    ps();
    exitProcess(0);
}

// ======================== Pipe demo processes ========================

static void pipe_producer_entry(void *arg)
{
    (void)arg;
    const char *msg = "hello through pipe\nline 2\nline 3\n";
    sys_write(FD_STDOUT, msg, (int)strlen(msg));
    exitProcess(0);
}

static void pipe_consumer_entry(void *arg)
{
    (void)arg;
    char buf[1];  // Read 1 byte at a time for immediate output
    int n;
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        // Echo to stdout immediately
        sys_write(FD_STDOUT, buf, n);
    }
    // Done (EOF or error). Add a marker newline
    const char *done = "[consumer done]\n";
    sys_write(FD_STDOUT, done, (int)strlen(done));
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

    // Save shell stdin/stdout to high-numbered FDs and prepare producer/consumer inheritance
    const int savedIn = 10;
    const int savedOut = 11;
    close(savedIn);
    close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);

    // Producer child: stdout -> pipe write end
    dup2(fds[1], FD_STDOUT);
    int prodPid = createProcess("prod", pipe_producer_entry, 0, 0, 0, 0, 0, 0);
    // Restore shell stdout and close our write end copy so only the producer holds it
    dup2(savedOut, FD_STDOUT);
    close(fds[1]);

    // Consumer child: stdin <- pipe read end
    dup2(fds[0], FD_STDIN);
    int consPid = createProcess("cons", pipe_consumer_entry, 0, 0, 0, 0, 0, 0);
    // Restore shell stdin and close our read end copy so only the consumer holds it
    dup2(savedIn, FD_STDIN);
    close(fds[0]);

    // Cleanup: close saved dupes in shell
    close(savedIn);
    close(savedOut);

    // Wait for both children so we see the full output before new prompt
    if (prodPid > 0) waitProcess(prodPid);
    if (consPid > 0) waitProcess(consPid);
    return 0;
}

// Show EOF: consumer reads from pipe and exits when writer is closed (no writer is created)
static int pipe_eof_cmd(void)
{
    int fds[2];
    if (pipe(fds) < 0) {
        fprintf(FD_STDERR, "pipe_eof: pipe() failed\n");
        return 1;
    }
    const int savedIn = 10;
    close(savedIn);
    dup2(FD_STDIN, savedIn);

    // Close writer end BEFORE creating consumer so it doesn't inherit a writer
    close(fds[1]);

    // Hook consumer stdin to pipe read end
    dup2(fds[0], FD_STDIN);
    int consPid = createProcess("cons", pipe_consumer_entry, 0, 0, 0, 0, 0, 0);
    // Restore shell stdin and close our read end copy
    dup2(savedIn, FD_STDIN);
    close(fds[0]);
    close(savedIn);

    if (consPid > 0) {
        waitProcess(consPid);
    }
    return 0;
}

// Show broken pipe: writer attempts to write with no readers
static void pipe_broken_writer_entry(void *arg)
{
    (void)arg;
    const char *msg = "x"; // single byte is enough
    int rc = sys_write(FD_STDOUT, msg, 1);
    if (rc < 0) {
        const char *note = "[broken pipe detected]\n";
        sys_write(FD_STDERR, note, (int)strlen(note));
    }
    exitProcess(0);
}

static int pipe_broken_cmd(void)
{
    int fds[2];
    if (pipe(fds) < 0) {
        fprintf(FD_STDERR, "pipe_broken: pipe() failed\n");
        return 1;
    }
    const int savedOut = 11;
    close(savedOut);
    dup2(FD_STDOUT, savedOut);

    // Close read end so there are no readers
    close(fds[0]);
    // Writer stdout -> pipe write end
    dup2(fds[1], FD_STDOUT);
    int pid = createProcess("writer", pipe_broken_writer_entry, 0, 0, 0, 0, 0, 0);
    // Restore shell stdout and close our write end copy
    dup2(savedOut, FD_STDOUT);
    close(fds[1]);
    close(savedOut);

    if (pid > 0) waitProcess(pid);
    return 0;
}

// Minimal pipeline runner for two process commands without arguments
static int run_pipeline(const char *leftCmd, const char *rightCmd)
{
    // Resolve commands
    int leftIdx = -1, rightIdx = -1;
    for (int i = 0; i < (int)(sizeof(commands)/sizeof(commands[0])); i++) {
        if (leftIdx == -1 && strcmp(commands[i].name, leftCmd) == 0) leftIdx = i;
        if (rightIdx == -1 && strcmp(commands[i].name, rightCmd) == 0) rightIdx = i;
        if (leftIdx != -1 && rightIdx != -1) break;
    }
    if (leftIdx == -1) {
        fprintf(FD_STDERR, "Command not found: %s\n", leftCmd);
        return 1;
    }
    if (rightIdx == -1) {
        fprintf(FD_STDERR, "Command not found: %s\n", rightCmd);
        return 1;
    }
    if (!commands[leftIdx].isProcess || !commands[rightIdx].isProcess) {
        fprintf(FD_STDERR, "Only process commands can be piped\n");
        return 1;
    }

    int fds[2];
    if (pipe(fds) < 0) {
        fprintf(FD_STDERR, "pipeline: pipe() failed\n");
        return 1;
    }
    const int savedIn = 10;
    const int savedOut = 11;
    close(savedIn); close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);

    // Left: stdout -> write end
    dup2(fds[1], FD_STDOUT);
    int leftPid = createProcess(commands[leftIdx].name, commands[leftIdx].entry, 0, 0, 0, 0, 0, 0);
    dup2(savedOut, FD_STDOUT);
    close(fds[1]); // Close write end in shell so only left holds it

    // Right: stdin <- read end
    dup2(fds[0], FD_STDIN);
    int rightPid = createProcess(commands[rightIdx].name, commands[rightIdx].entry, 0, 0, 0, 0, 0, 0);
    dup2(savedIn, FD_STDIN);
    close(fds[0]); // Close read end in shell so only right holds it

    close(savedIn); close(savedOut);
    current_pipeline_pids[0] = leftPid;
    current_pipeline_pids[1] = rightPid;
    
    // Wait for left process (writer)
    if (leftPid > 0) {
        waitProcess(leftPid);
    }
    
    // If left was killed by Ctrl+C, it's already cleaned up
    // Just wait for right process (reader) to finish
    if (rightPid > 0) {
        waitProcess(rightPid);
    }
    
    current_pipeline_pids[0] = 0;
    current_pipeline_pids[1] = 0;
    return 0;
}

static void trim(char *s)
{
    if (s == NULL) return;
    int n = (int)strlen(s);
    int i = 0, j = n - 1;
    while (i < n && (s[i] == ' ' || s[i] == '\t')) i++;
    while (j >= i && (s[j] == ' ' || s[j] == '\t')) j--;
    int k = 0;
    for (; i <= j; i++) s[k++] = s[i];
    s[k] = '\0';
}

// ======================== Pipe synchronization test ========================
// Simple test: Writer writes MORE than pipe capacity, reader reads slowly.
// This tests that writer blocks when pipe is full, and reader unblocks writer.

static void pipeSyncWriter(void *arg)
{
    (void)arg;
    
    // Write 2000 bytes (pipe capacity is 1024, so this MUST block)
    const char *msg = "W"; // 1 byte
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

static void pipeSyncReader(void *arg)
{
    (void)arg;
    
    // Read from pipe until EOF
    char buf[100];
    int totalRead = 0;
    int n;
    
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        totalRead += n;
    }
    
    // Verify we read the expected amount
    if (totalRead != 2000)
    {
        exitProcess(1);
    }
    
    exitProcess(0);
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
    
    // Save stdin/stdout
    int savedIn = 12;
    int savedOut = 13;
    close(savedIn);
    close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);
    
    // Spawn writer first (stdout -> pipe write end)
    dup2(fds[1], FD_STDOUT);
    int writerPid = createProcess("writer", pipeSyncWriter, 0, 0, 0, 0, 0, 0);
    dup2(savedOut, FD_STDOUT);
    close(fds[1]); // Close write end in shell so only writer holds it
    
    // Spawn reader (stdin <- pipe read end)
    dup2(fds[0], FD_STDIN);
    int readerPid = createProcess("reader", pipeSyncReader, 0, 0, 0, 0, 0, 0);
    dup2(savedIn, FD_STDIN);
    close(fds[0]); // Close read end in shell so only reader holds it
    
    // Cleanup saved FDs
    close(savedIn);
    close(savedOut);
    
    if (writerPid <= 0 || readerPid <= 0)
    {
        fprintf(FD_STDERR, "pipe_sync: failed to spawn processes\n");
        return 1;
    }
    
    // Wait for both
    int writerStatus = 0;
    int readerStatus = 0;
    if (writerPid > 0) writerStatus = waitProcess(writerPid);
    if (readerPid > 0) readerStatus = waitProcess(readerPid);
    
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

// Utils de formateo para printear tablas

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
    char buffer[2 + 16 + 1];
    int idx = 0;

    buffer[idx++] = '0';
    buffer[idx++] = 'x';

    int hexDigits = digitsForHex(value);

    for (int i = hexDigits - 1; i >= 0; i--)
    {
        uint8_t nibble = (value >> (i * 4)) & 0xF;
        buffer[idx++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }

    buffer[idx] = 0;

    printf("%s", buffer);

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

    int len = strlen(value);
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

// ======================== Pipe stress test ========================
// Multiple writers and readers to stress the pipe implementation

static void pipeStressWriter(void *arg)
{
    (void)arg;
    int myPid = getPid();
    
    // CRITICAL: Close ALL FDs except stdout (which should be the pipe write end)
    // We inherited many FDs from shell, we only want to keep FD 1 (stdout)
    for (int fd = 0; fd < 20; fd++) {
        if (fd != FD_STDOUT && fd != FD_STDERR) {
            close(fd);
        }
    }
    
    // Write 1000 bytes to the pipe
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
        
        // Report progress every 250 bytes
        if ((i + 1) % 250 == 0)
        {
            fprintf(FD_STDERR, "Writer %d: %d bytes written\n", myPid, i + 1);
        }
    }
    
    fprintf(FD_STDERR, "Writer %d: finished (%d bytes)\n", myPid, total);
    exitProcess(0);
}

static void pipeStressReader(void *arg)
{
    (void)arg;
    int myPid = getPid();
    
    // CRITICAL: Close ALL FDs except stdin (which should be the pipe read end) and stderr
    // We inherited many FDs from shell, we only want to keep FD 0 (stdin) and FD 2 (stderr)
    for (int fd = 1; fd < 20; fd++) {
        if (fd != FD_STDERR) {
            close(fd);
        }
    }
    
    // Read from pipe until EOF
    char buf[100];
    int totalRead = 0;
    int n;
    
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        totalRead += n;
        // Add small delay to let writers queue up
        // sleep(20);
    }
    
    fprintf(FD_STDERR, "Reader %d: EOF detected, read %d bytes total\n", myPid, totalRead);
    exitProcess(0);
}

static int pipe_stress_cmd(void)
{
    // Parse arguments: numWriters numReaders (default 3 writers, 2 readers)
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
    
    // Save stdin/stdout
    int savedIn = 12;
    int savedOut = 13;
    close(savedIn);
    close(savedOut);
    dup2(FD_STDIN, savedIn);
    dup2(FD_STDOUT, savedOut);
    
    // Spawn writers first (stdout -> pipe write end)
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
    
    // CRITICAL: Close write end in shell BEFORE spawning readers
    // This ensures readers can detect EOF when all writers finish
    close(fds[1]);
    
    // Spawn readers (stdin <- pipe read end)
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
    
    // Close read end in shell
    close(fds[0]);
    
    // Cleanup saved FDs
    close(savedIn);
    close(savedOut);
    
    // Wait for all writers
    for (int i = 0; i < numWriters; i++)
    {
        if (writerPids[i] > 0)
        {
            waitProcess(writerPids[i]);
        }
    }
    
    // Wait for all readers
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

// ============================================================================
// MVar Implementation - Multiple readers/writers with synchronization
// ============================================================================

#define MVAR_SEM_MUTEX "/mvar_mutex"
#define MVAR_SEM_EMPTY "/mvar_empty"
#define MVAR_SEM_FULL  "/mvar_full"
#define MVAR_MAX_PROCESSES 10
#define MVAR_BUSY_WAIT_BASE 100
#define MVAR_BUSY_WAIT_RAND 200

// ANSI colors for readers
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

// MVar shared state (global variables are shared across all shell processes)
static volatile char mvar_value = 0;
static volatile int mvar_filled = 0;

typedef struct {
    char letter;
    int writerNum;
} MvarWriterArg;

typedef struct {
    int readerNum;
    const char *color;
} MvarReaderArg;

// Simple pseudo-random number generator using ticks
static unsigned int mvar_rand_seed = 0;
static unsigned int mvar_simple_rand(void) {
    mvar_rand_seed = mvar_rand_seed * 1103515245 + 12345;
    return mvar_rand_seed;
}

static void mvar_busy_wait_random(void) {
    unsigned int wait_time = MVAR_BUSY_WAIT_BASE + (mvar_simple_rand() % MVAR_BUSY_WAIT_RAND);
    for (unsigned int i = 0; i < wait_time; i++) {
        // Busy wait
        __asm__ volatile("nop");
    }
    sleep(10); // yield to allow other processes to run
}

static void mvar_writer_entry(uint64_t argc, char **argv) {
    // argv points to writerArgs[i]
    MvarWriterArg *warg = (MvarWriterArg *)argv;
    char myLetter = warg->letter;

    // Open semaphores
    int sem_empty = semOpen(MVAR_SEM_EMPTY);
    int sem_full = semOpen(MVAR_SEM_FULL);

    if (sem_empty < 0 || sem_full < 0) {
        sys_exit(1);
    }

    // Writer loop: wait for empty, write value, signal full
    while (1) {
        // Random busy wait to simulate some work
        mvar_busy_wait_random();

        // Wait until MVar is empty
        if (semWait(sem_empty) < 0) {
            break; // Probably killed
        }

        // Critical section: write value to SHARED global variable
        mvar_value = myLetter;
        mvar_filled = 1;

        // Signal that MVar is now full
        semPost(sem_full);
    }

    semClose(sem_empty);
    semClose(sem_full);
    sys_exit(0);
}

static void mvar_reader_entry(uint64_t argc, char **argv) {
    // argv points to readerArgs[i]
    MvarReaderArg *rarg = (MvarReaderArg *)argv;
    const char *myColor = rarg->color;

    // Open semaphores
    int sem_empty = semOpen(MVAR_SEM_EMPTY);
    int sem_full = semOpen(MVAR_SEM_FULL);

    if (sem_empty < 0 || sem_full < 0) {
        sys_exit(1);
    }

    // Reader loop: wait for full, read value, signal empty
    while (1) {
        // Random busy wait to simulate some work
        mvar_busy_wait_random();

        // Wait until MVar is full
        if (semWait(sem_full) < 0) {
            break; // Probably killed
        }

        // Critical section: read and consume value from SHARED global variable
        char val = mvar_value;
        mvar_filled = 0;

        // Signal that MVar is now empty
        semPost(sem_empty);

        // Print with color (outside critical section)
        printf("%s%c\e[0m", myColor, val);
    }

    semClose(sem_empty);
    semClose(sem_full);
    sys_exit(0);
}

static int mvar_cmd(void) {
    int numWriters = 0;
    int numReaders = 0;

    // Parse arguments
    if (buffer_dim < 6) { // "mvar X Y" minimum
        printf("Usage: mvar <num_writers> <num_readers>\n");
        printf("Example: mvar 2 2\n");
        return 1;
    }

    // Skip "mvar " to get to arguments
    char *args = buffer + 5; // Skip "mvar "

    // Parse num_writers
    while (*args == ' ') args++;
    numWriters = 0;
    while (*args >= '0' && *args <= '9') {
        numWriters = numWriters * 10 + (*args - '0');
        args++;
    }

    // Parse num_readers
    while (*args == ' ') args++;
    numReaders = 0;
    while (*args >= '0' && *args <= '9') {
        numReaders = numReaders * 10 + (*args - '0');
        args++;
    }

    // Validate arguments
    if (numWriters <= 0 || numReaders <= 0) {
        printf("mvar: Both writers and readers must be > 0\n");
        return 1;
    }

    if (numWriters > MVAR_MAX_PROCESSES || numReaders > MVAR_MAX_PROCESSES) {
        printf("mvar: Maximum %d processes of each type\n", MVAR_MAX_PROCESSES);
        return 1;
    }

    // Initialize random seed with a simple mix of values
    int h, m, s;
    getDate(&h, &m, &s);
    mvar_rand_seed = (unsigned int)(h * 3600 + m * 60 + s + numWriters * 7 + numReaders * 13);

    // Reset MVar global state (shared across all processes)
    mvar_value = 0;
    mvar_filled = 0;

    // Create or open semaphores
    // empty: initially 1 (MVar starts empty, so writers can proceed)
    // full: initially 0 (MVar has no value, so readers must wait)

    // Try to open first (in case they already exist from previous run)
    int sem_empty = semOpen(MVAR_SEM_EMPTY);
    int sem_full = semOpen(MVAR_SEM_FULL);

    // If they don't exist, create them
    if (sem_empty < 0) {
        sem_empty = semCreate(MVAR_SEM_EMPTY, 1);
    }
    if (sem_full < 0) {
        sem_full = semCreate(MVAR_SEM_FULL, 0);
    }

    if (sem_empty < 0 || sem_full < 0) {
        printf("mvar: Failed to create/open semaphores\n");
        if (sem_empty >= 0) semClose(sem_empty);
        if (sem_full >= 0) semClose(sem_full);
        return 1;
    }

    // Reset semaphores to their initial values to ensure clean state
    // This is critical after killing processes from a previous mvar run
    semReset(sem_empty, 1);  // empty: 1 (MVar starts empty)
    semReset(sem_full, 0);   // full: 0 (no value yet)

    // Allocate memory for process arguments (must persist)
    static MvarWriterArg writerArgs[MVAR_MAX_PROCESSES];
    static MvarReaderArg readerArgs[MVAR_MAX_PROCESSES];

    int writerPids[MVAR_MAX_PROCESSES];
    int readerPids[MVAR_MAX_PROCESSES];

    // Create writer processes
    for (int i = 0; i < numWriters; i++) {
        writerArgs[i].letter = 'A' + i;
        writerArgs[i].writerNum = i;

        // Create descriptive name: "w_A", "w_B", "w_C", etc.
        static char writerNames[MVAR_MAX_PROCESSES][8];
        writerNames[i][0] = 'w';
        writerNames[i][1] = '_';
        writerNames[i][2] = 'A' + i;
        writerNames[i][3] = '\0';

        // Cast function pointer and pass struct pointer as argv
        writerPids[i] = createProcess(writerNames[i], (void (*)(void *))mvar_writer_entry, (char **)&writerArgs[i], 0, NULL, 0, 0, 0);
        if (writerPids[i] < 0) {
            printf("mvar: Failed to create writer %d\n", i);
        }
    }

    // Create reader processes
    for (int i = 0; i < numReaders; i++) {
        readerArgs[i].readerNum = i;
        readerArgs[i].color = mvar_reader_colors[i % 10];

        // Create descriptive name: "r_0", "r_1", "r_2", etc.
        static char readerNames[MVAR_MAX_PROCESSES][8];
        readerNames[i][0] = 'r';
        readerNames[i][1] = '_';
        readerNames[i][2] = '0' + i;
        readerNames[i][3] = '\0';

        // Cast function pointer and pass struct pointer as argv
        readerPids[i] = createProcess(readerNames[i], (void (*)(void *))mvar_reader_entry, (char **)&readerArgs[i], 0, NULL, 0, 0, 0);
        if (readerPids[i] < 0) {
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
    printf("\nUse 'kill <PID>' to stop individual processes.\n");
    printf("Note: Processes run in background. Use Ctrl+C or kill to stop them.\n\n");

    // DON'T close semaphores here - processes still need them!
    // They will be cleaned up when all processes exit

    return 0;
}
