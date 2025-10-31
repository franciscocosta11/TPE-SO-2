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
        processTable[i].waiterPid = -1;
        processTable[i].priority = MIN_PRIORITY;
        processTable[i].ctx = 0;
    }
    availableProcesses = MAX_PROCESSES;
    currentPid = 0;
    initScheduler();
}

Process *createProcess(char *name, void (*Entry)(void *), char **Argv, int Argc, void *StackBase, size_t StackSize, int priority, bool isForeground)
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
    p->priority = priority;
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

    Process *currentProcess = getCurrentProcess();

    if (currentProcess == NULL)
    {
        return;
    }

    // si uso stack, lo libero
    if (currentProcess->stackBase)
    {
        freeMemory(currentProcess->stackBase);
        currentProcess->stackBase = NULL;
        currentProcess->stackSize = 0;
    }
    currentProcess->entry = NULL;
    currentProcess->Arg = NULL;
    currentProcess->state = TERMINATED;
    currentProcess->pid = 0;
    availableProcesses++;
    currentPid = 0;

    // Desbloquear al proceso que estuviera esperando (si aplica)
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
    
    // Si se intenta matar a sí mismo, delegar en exitCurrentProcess
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

            // Si estaba en READY, sacarlo de la ready queue
            if (victim->state == READY)
            {
                unschedule(victim);
            }

            // Despertar al waiter (si hay)
            if (victim->waiterPid > 0)
            {
                unblockProcess(victim->waiterPid);
                victim->waiterPid = -1;
            }

            // Si el proceso a matar es a su vez un waiter de otro proceso,
            // evitamos que quede una referencia colgante al PID muerto
            // (que puede reusarse) limpiando cualquier waiterPid igual a victim->pid.
            for (int j = 0; j < MAX_PROCESSES; j++)
            {
                if (processTable[j].pid != 0 && processTable[j].waiterPid == victim->pid)
                {
                    processTable[j].waiterPid = -1;
                }
            }

            // Liberar stack si corresponde
            if (victim->stackBase)
            {
                freeMemory(victim->stackBase);
                victim->stackBase = NULL;
                victim->stackSize = 0;
            }

            victim->entry = NULL;
            victim->Arg = NULL;
            victim->state = TERMINATED;
            victim->pid = 0;
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

        // Si intentamos bloquear al proceso actual (RUNNING), lo marcamos
        // como BLOCKED y conmutamos inmediatamente. No hace falta unschedule
        // porque el RUNNING no está en la ready queue.
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
    // si no estaba bloqueado, no hacemos nada pero no es error
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
            StackFrame *frame = (StackFrame *)ctx; // ctx es rip
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