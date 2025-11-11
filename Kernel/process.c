// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <fonts.h>

#include "process.h"
#include "MemoryManager.h"
#include "scheduler.h"
#include "interrupts.h"
#include "lib.h"
#include "process_info.h"
#include "semaphore.h"

int currentPid = 0; // el primer proceso current va a ser el primero en inicializarse
int availableProcesses = 0;

Process processTable[MAX_PROCESSES]; // tabla de procesos

void initProcessSystem(void)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        processTable[i].pid = 0;
        processTable[i].parentPid = 0;
        processTable[i].state = TERMINATED;
        processTable[i].entry = NULL;
        processTable[i].Arg = NULL;
        processTable[i].stackBase = NULL;
        processTable[i].stackSize = 0;
        processTable[i].next = NULL;
        processTable[i].waiterPid = -1;
        processTable[i].priority = MIN_PRIORITY;
        processTable[i].baseQuantum = 0;
        processTable[i].quantumRemaining = 0;
        processTable[i].readyTicks = 0;
        processTable[i].ctx = 0;
        for (int j = 0; j < MAX_FD; j++)
        {
            processTable[i].fdTable[j] = NULL;
        }
        for (int j = 0; j < MAX_SEM_PER_PROCESS; j++)
        {
            processTable[i].openSemaphores[j] = -1;
        }
    }
    availableProcesses = MAX_PROCESSES;
    currentPid = 0;
    initScheduler();
}

Process *createProcess(char *name, ProcessEntryPoint Entry, char **Argv, int Argc, void *StackBase, size_t StackSize, int priority, bool isForeground)
{
    if (Entry == NULL)
        return NULL;

    int slot = -1;
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

    Process *p = &processTable[slot];
    p->pid = slot + 1;
    p->state = READY;
    p->entry = Entry;
    p->Arg = Argv;
    p->next = NULL;
    p->priority = priority;
    p->baseQuantum = schedulerQuantumForPriority(p->priority);
    p->quantumRemaining = p->baseQuantum;
    p->readyTicks = 0;
    p->name = name;
    p->isForeground = isForeground;
    for (int j = 0; j < MAX_FD; j++)
    {
        p->fdTable[j] = NULL;
    }
    for (int j = 0; j < MAX_SEM_PER_PROCESS; j++)
    {
        p->openSemaphores[j] = -1;
    }

    size_t sz = (StackSize > 0) ? StackSize : PROCESS_STACK_SIZE;
    void *stk = allocMemory(sz);
    if (stk == NULL)
    {
        p->pid = 0;
        p->state = TERMINATED;
        return NULL;
    }
    p->stackBase = stk;
    p->stackSize = sz;

    {
        Process *parent = getCurrentProcess();
        p->parentPid = parent != NULL ? parent->pid : 0;
        if (parent != NULL)
        {
            for (int j = 0; j < MAX_FD; j++)
            {
                File *f = parent->fdTable[j];
                if (f != NULL)
                {
                    fileRetain(f);
                    p->fdTable[j] = f;
                }
            }
        }
    }

    if (availableProcesses > 0)
        availableProcesses--;
    uint8_t *stackTop = (uint8_t *)p->stackBase + p->stackSize;

    uint8_t *readyRsp = initStack(stackTop, (void *)Entry, Argc, Argv);

    p->ctx = (uint64_t)readyRsp;

    schedulerAddProcess(p);

    return p;
}

void exitCurrentProcess(int exitCode)
{
    (void)exitCode;

    Process *currentProcess = getCurrentProcess();

    if (currentProcess == NULL)
    {
        return;
    }

    semCloseAllForProcess(currentProcess->pid);
    semRemoveProcessFromAllQueues(currentProcess->pid);

    if (currentProcess != NULL)
    {
        for (int j = 0; j < MAX_FD; j++)
        {
            if (currentProcess->fdTable[j] != NULL)
            {
                fileRelease(currentProcess->fdTable[j]);
                currentProcess->fdTable[j] = NULL;
            }
        }
    }

    currentProcess->entry = NULL;
    currentProcess->Arg = NULL;
    currentProcess->state = TERMINATED;
    currentProcess->pid = 0;
    currentProcess->parentPid = 0;
    availableProcesses++;
    currentPid = 0;

    int waiter = currentProcess->waiterPid;
    currentProcess->waiterPid = -1;
    if (waiter > 0)
    {
        unblockProcess(waiter);
    }
    return;
}

int killProcess(int pid)
{
    if (pid <= 0)
        return -1;

    if (pid == currentPid)
    {
        exitCurrentProcess(0);
        contextSwitch();
        return 0;
    }

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].pid == pid)
        {
            Process *victim = &processTable[i];

            semCloseAllForProcess(pid);
            semRemoveProcessFromAllQueues(pid);

            // Si estaba en READY, sacarlo de la ready queue
            if (victim->state == READY)
            {
                unschedule(victim);
            }

            if (victim->waiterPid > 0)
            {
                unblockProcess(victim->waiterPid);
                victim->waiterPid = -1;
            }

            for (int j = 0; j < MAX_PROCESSES; j++)
            {
                if (processTable[j].pid != 0 && processTable[j].waiterPid == victim->pid)
                {
                    processTable[j].waiterPid = -1;
                }
            }

            if (victim->stackBase)
            {
                freeMemory(victim->stackBase);
                victim->stackBase = NULL;
                victim->stackSize = 0;
            }

            for (int j = 0; j < MAX_FD; j++)
            {
                if (victim->fdTable[j] != NULL)
                {
                    fileRelease(victim->fdTable[j]);
                    victim->fdTable[j] = NULL;
                }
            }

            victim->entry = NULL;
            victim->Arg = NULL;
            victim->state = TERMINATED;
            victim->pid = 0;
            victim->parentPid = 0;
            availableProcesses++;

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

        if (pid == currentPid && process->state == RUNNING)
        {
            process->state = BLOCKED;
            contextSwitch();
            return BLOCKED;
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

int unblockProcess(int pid)
{
    if (pid <= 0)
        return -1;

    Process *process = getProcessByPid(pid);
    if (process == NULL)
    {
        return -1;
    }

    if (process->state == BLOCKED)
    {
        process->state = READY;
        schedulerAddProcess(process);
    }
    return 0;
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
        p->baseQuantum = schedulerQuantumForPriority(p->priority);
        p->quantumRemaining = p->baseQuantum;

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

void waitProcess(int pid)
{
    if (pid <= 0 || pid == currentPid)
    {
        return;
    }

    Process *self = getCurrentProcess();
    Process *processToWait = getProcessByPid(pid);

    if (self == NULL || self->state == TERMINATED || processToWait == NULL || processToWait->state == TERMINATED)
    {
        return;
    }

    processToWait->waiterPid = currentPid;

    self->state = BLOCKED; // no uso toggleBlock porque esta RUNNING
    contextSwitch();
}

Process *getProcessByPid(int pid)
{
    if (pid <= 0)
    {
        return NULL;
    }

    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processTable[i].pid == pid)
        {
            return &processTable[i];
        }
    }

    return NULL;
}

bool isShellProcess(const Process *process)
{
    return process != NULL && process->name != NULL && strcmp(process->name, SHELL_PROCESS_NAME) == 0;
}

bool processCanHandleCtrlC(const Process *process)
{
    if (process == NULL)
    {
        return false;
    }

    if (process->pid == IDLE_PID || isShellProcess(process))
    {
        return false;
    }

    if (process->waiterPid > 0)
    {
        return true;
    }

    return process->isForeground;
}

Process *getKillableForegroundProcess(void)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        Process *candidate = &processTable[i];
        if (candidate->pid != 0 && candidate->state != TERMINATED && processCanHandleCtrlC(candidate))
        {
            return candidate;
        }
    }
    return NULL;
}

static void killProcessChildren(int pid)
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        Process *child = &processTable[i];
        if (child->pid != 0 && child->parentPid == pid)
        {
            int childPid = child->pid;
            killProcessChildren(childPid);
            killProcess(childPid);
        }
    }
}

void killProcessTree(int pid)
{
    killProcessChildren(pid);
    killProcess(pid);
}

void blockCurrentProcess(void)
{
    Process *self = getCurrentProcess();
    if (self == NULL || self->state == TERMINATED)
    {
        return;
    }

    self->state = BLOCKED;
    contextSwitch();
}
