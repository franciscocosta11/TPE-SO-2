#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <fonts.h>

#include "process.h"
#include "MemoryManager.h"
#include "scheduler.h"
#include "interrupts.h"
#include "lib.h"
#include "process_info.h"

int currentPid = 0; // el primer proceso current va a ser el primero en inicializarse
int availableProcesses = 0;

Process processTable[MAX_PROCESSES]; // tabla de procesos

void initProcessSystem(void)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processTable[i].pid = 0; /* pid 0 = libre */
        processTable[i].state = TERMINATED;
        processTable[i].entry = NULL;
        processTable[i].Arg = NULL;
        processTable[i].stackBase = NULL;
        processTable[i].stackSize = 0;
        processTable[i].next = NULL;
        processTable[i].priority = MIN_PRIORITY;
        processTable[i].ctx = 0;
    }
    availableProcesses = MAX_PROCESSES;
    currentPid = 0;
    initScheduler();
}

Process *createProcess(char *name, void (*Entry)(void *), char **Argv, int Argc, void *StackBase, size_t StackSize, bool isForeground)
{
    if (Entry == NULL)
        return NULL;

    // busco slot libre
    int slot = -1;
    //! despues habria que crear la funcion getAvailableSlot... dbpp
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].state == TERMINATED || processTable[i].pid == 0)
        {
            slot = i;
            break;
        }
    }

    if (slot < 0)
        return NULL;

    // creo PCB
    Process *p = &processTable[slot];
    p->pid = slot + 1; /* pid simple: índice+1 */
    p->state = READY;
    p->entry = Entry;
    p->Arg = Argv;
    p->next = NULL;
    p->priority = MIN_PRIORITY;
    p->name = name;
    p->isForeground = isForeground;

    size_t sz = (StackSize > 0) ? StackSize : PROCESS_STACK_SIZE;
    void *stk = allocMemory(sz);
    if (stk == NULL)
    {
        // osea digamos no funciono
        p->pid = 0;
        p->state = TERMINATED;
        return NULL;
    }
    p->stackBase = stk;
    p->stackSize = sz;

    if (availableProcesses > 0)
        availableProcesses--;
    // Contexto inicial: usamos contextSwitchTo (mov rsp, ctx; ret).
    // Por lo tanto, ctx debe apuntar a una pila cuyo tope contenga la
    // dirección de retorno. Esa dirección será nuestro trampolín.
    uint8_t *stackTop = (uint8_t *)p->stackBase + p->stackSize;
    // stackTop = (uint8_t *)(((uintptr_t)stackTop) & ~((uintptr_t)0xF));
    // StackFrame *frame = (StackFrame *)(stackTop - sizeof(StackFrame));

    // memset(frame, 0, sizeof(StackFrame));

    uint8_t *readyRsp = stackInit(stackTop, (void *)Entry, Argc, Argv);

    // frame->rip = (uint64_t)&processBootstrap;
    // frame->cs = KERNEL_CS;
    // frame->rflags = INITIAL_RFLAGS;
    // frame->rsp = (uint64_t)stackTop;
    // frame->ss = KERNEL_SS;

    p->ctx = (uint64_t)readyRsp;

    schedulerAddProcess(p);

    return p;
}

void exitCurrentProcess(int exitCode)
{
    (void)exitCode;
    int pid = getCurrentPid();
    if (pid <= 0)
        return;

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].pid == pid)
        {
            // si uso stack, lo libero
            if (processTable[i].stackBase)
            {
                freeMemory(processTable[i].stackBase);
                processTable[i].stackBase = NULL;
                processTable[i].stackSize = 0;
            }
            processTable[i].entry = NULL;
            processTable[i].Arg = NULL;
            processTable[i].state = TERMINATED;
            processTable[i].pid = 0;
            availableProcesses++;
            currentPid = 0;
            return;
        }
    }
}

int killProcess(int pid)
{
    if (pid <= 0)
        return -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].pid == pid)
        {
            if (processTable[i].state == READY)
            {
                unschedule(&processTable[i]);
            }
            if (processTable[i].stackBase)
            {
                freeMemory(processTable[i].stackBase);
                processTable[i].stackBase = NULL;
                processTable[i].stackSize = 0;
            }
            processTable[i].entry = NULL;
            processTable[i].Arg = NULL;
            processTable[i].state = TERMINATED;
            processTable[i].pid = 0;
            availableProcesses++;
            if (currentPid == pid)
            {
                currentPid = 0;
            }
            return 0;
        }
    }
    return -1;
}

int toggleProcessBlock(int pid)
{
    if (pid <= 0)
        return -1;

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        Process *process = &processTable[i];

        if (process->pid != pid)
        {
            continue;
        }

        if (process->state == READY)
        {
            unschedule(process);
            process->state = BLOCKED;
            return BLOCKED;
        }

        if (process->state == BLOCKED)
        {
            process->state = READY;
            schedulerAddProcess(process);
            return READY;
        }

        return -1;
    }

    return -1;
}

int setProcessPriority(int pid, int priority)
{
    if (priority < MIN_PRIORITY || priority >= MAX_PRIORITIES)
    {
        return -1;
    }

    if (pid == IDLE_PID)
    {
        return -1;
    }

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        Process *p = &processTable[i];

        if (p->pid != pid || p->state == TERMINATED)
        {
            continue;
        }

        if (p->priority == priority)
        {
            return 0;
        }

        bool wasReady = (p->state == READY);

        if (wasReady)
        {
            unschedule(p);
        }

        p->priority = priority;

        if (wasReady)
        {
            schedulerAddProcess(p);
        }

        return 0;
    }

    return -1;
}

Process *getCurrentProcess()
{
    if (currentPid <= 0 || currentPid > MAX_PROCESSES)
        return NULL;
    return &processTable[currentPid - 1];
}

int getCurrentPid(void) { return currentPid; }

int getAvailableProcesses(void) { return availableProcesses; }

size_t getProcessSnapshot(ProcessInfo *buffer, size_t maxCount)
{
    if (buffer == NULL || maxCount == 0)
    {
        return 0;
    }

    size_t written = 0;

    for (int i = 0; i < MAX_PROCESSES && written < maxCount; i++)
    {
        Process *process = &processTable[i];

        if (process->pid == 0)
        {
            continue;
        }

        buffer[written].pid = process->pid;
        buffer[written].state = process->state;
        buffer[written].priority = process->priority;
        buffer[written].name = process->name;
        buffer[written].foreground = process->isForeground;

        uint64_t ctx = process->ctx;
        buffer[written].stackPointer = ctx;

        uint64_t basePointer = 0;
        if (ctx != 0)
        {
            StackFrame *frame = (StackFrame *)ctx;
            basePointer = frame->rbp;
        }
        buffer[written].basePointer = basePointer;

        written++;
    }

    return written;
}