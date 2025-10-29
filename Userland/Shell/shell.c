#include "./../libc/stdio.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <sys.h>
#include <exceptions.h>

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

#define INC_MOD(x, m) x = (((x) + 1) % (m))
#define SUB_MOD(a, b, m) ((a) - (b) < 0 ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

static char buffer[MAX_BUFFER_SIZE];
static int buffer_dim = 0;

int clear(void);
int echo(void);
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
int ps(void);
int nice(void);

static void printPreviousCommand(enum REGISTERABLE_KEYS scancode);
static void printNextCommand(enum REGISTERABLE_KEYS scancode);
static void printSpaces(int count);
static int digitsForInt(int value);
static void printIntColumn(int value, int width);
static void printStringColumn(const char *value, int width);
static int parsePid(const char *arg, int *pidOut);

static uint8_t last_command_arrowed = 0;

typedef struct
{
    char *name;
    int (*function)(void);
    char *description;
} Command;

/* All available commands. Sorted alphabetically by their name */
Command commands[] = {
    {.name = "block", .function = (int (*)(void))(unsigned long long)block, .description = "Toggles a process between BLOCKED and READY"},
    {.name = "clear", .function = (int (*)(void))(unsigned long long)clear, .description = "Clears the screen"},
    {.name = "divzero", .function = (int (*)(void))(unsigned long long)_divzero, .description = "Generates a division by zero exception"},
    {.name = "echo", .function = (int (*)(void))(unsigned long long)echo, .description = "Prints the input string"},
    {.name = "exit", .function = (int (*)(void))(unsigned long long)exit, .description = "Command exits w/ the provided exit code or 0"},
    {.name = "font", .function = (int (*)(void))(unsigned long long)font, .description = "Increases or decreases the font size.\n\t\t\t\tUse:\n\t\t\t\t\t  + font increase\n\t\t\t\t\t  + font decrease"},
    {.name = "help", .function = (int (*)(void))(unsigned long long)help, .description = "Prints the available commands"},
    {.name = "history", .function = (int (*)(void))(unsigned long long)history, .description = "Prints the command history"},
    {.name = "invop", .function = (int (*)(void))(unsigned long long)_invalidopcode, .description = "Generates an invalid Opcode exception"},
    {.name = "kill", .function = (int (*)(void))(unsigned long long)killcmd, .description = "Kills a process by PID"},
    {.name = "man", .function = (int (*)(void))(unsigned long long)man, .description = "Prints the description of the provided command"},
    {.name = "mem", .function = (int (*)(void))(unsigned long long)memcmd, .description = "Displays kernel memory usage"},
    {.name = "nice", .function = (int (*)(void))(unsigned long long)nice, .description = "Changes a process priority"},
    {.name = "ps", .function = (int (*)(void))(unsigned long long)ps, .description = "Prints the process list"},
    {.name = "regs", .function = (int (*)(void))(unsigned long long)regs, .description = "Prints the register snapshot, if any"},
    {.name = "time", .function = (int (*)(void))(unsigned long long)time, .description = "Prints the current time"},
};

char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE] = {0};
char command_history_buffer[MAX_BUFFER_SIZE] = {0};
uint8_t command_history_last = 0;

static uint64_t last_command_output = 0;

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
        int i = 0;

        for (; i < sizeof(commands) / sizeof(Command); i++)
        {
            if (strcmp(commands[i].name, command) == 0)
            {
                last_command_output = commands[i].function();
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
        printIntColumn((int)info->stackPointer, STACK_COL_WIDTH); //! cambiar a hex
        printSpaces(COLUMN_PADDING);
        printIntColumn((int)info->basePointer, BASE_COL_WIDTH); //! cambiar a hex
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

int loop(size_t seconds)
{
    printf("Hola soy el proceso %d", seconds);
    return 0;
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

static void printIntColumn(int value, int width)
{
    printf("%d", value);

    int len = digitsForInt(value);
    if (len < width)
    {
        printSpaces(width - len);
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
