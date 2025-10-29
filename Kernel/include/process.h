#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "process_info.h"

extern int currentPid; // el primer proceso current va a ser el primero en inicializarse
extern int availableProcesses;

#define MAX_PROCESSES 16
#define MIN_PRIORITY PROCESS_PRIORITY_MIN
#define MAX_PRIORITY PROCESS_PRIORITY_MAX
#define IDLE_PID PROCESS_IDLE_PID

#define FOREGROUND true
#define BACKGROUND false
// Configuración
#define PROCESS_STACK_SIZE (16 * 1024) // 16 KiB; ajustá si tu kernel lo necesita

// El orden DEBE COINCIDIR con tu macro pushState en interrupts.asm
typedef struct
{
    // --- pushState ---
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    // Stack frame de iretq 
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} StackFrame;

#define KERNEL_CS 0x08
#define KERNEL_SS 0x10
#define INITIAL_RFLAGS 0x202 // Flag de interrupción (IF) = 1

extern int currentPid;

/** @struct Process
 *  @brief PCB mínimo usado por el scheduler.
 *
 *  Campos principales:
 *   - Pid: identificador del proceso.
 *   - State: estado actual del proceso.
 *   - StackBase/StackSize: región de stack reservada para el proceso.
 *   - Ctx: puntero opaco al contexto guardado.
 *   - Next: enlace simple para colas READY.
 *   - Entry/Arg: punto de entrada y argumento inicial del proceso.
 */
typedef struct Process
{
    int pid;            // identificador del proceso
    ProcessState state; // estado actual del proceso (ready running blocked etc)

    void *stackBase;  // base del stack --> para la posterior liberacion
    size_t stackSize; // tamaÑo del stack
    uint64_t ctx;  //! Puntero al contexto --> REVISAR
    int priority;

    char* name;
    bool isForeground;

    struct Process *next; // siguiente en la lista

    void (*entry)(void *); // entry point
    char **Arg;             // argumento inicial
} Process;

extern struct Process processTable[MAX_PROCESSES]; // tabla de procesos

/**
 * @brief Inicializa el subsistema de procesos.
 *
 * Debe dejar la tabla de procesos en estado limpio y preparar cualquier
 * estructura interna (cursor del scheduler, next PID, etc.).
 */
void initProcessSystem(void);

/**
 * @brief Crea un nuevo proceso y lo deja listo para ser scheduleado.
 *
 * @param Entry      Puntero a la función que el proceso ejecutará.
 * @param Arg        Argumento que se pasará a Entry al arrancar.
 * @param StackBase  Dirección de memoria reservada para el stack del proceso.
 * @param StackSize  Tamaño en bytes del stack apuntado por StackBase.
 * @return Puntero al `Process` creado, o NULL en caso de error (p.ej. sin
 *         slots libres o stack inválido).
 */
Process *createProcess(char* name, void (*Entry)(void *), char **Argv, int Argc ,void *StackBase, size_t StackSize, bool isForeground);

/**
 * @brief Termina el proceso actual con el código de salida indicado.
 *
 * @param ExitCode Código numérico de salida del proceso.
 */
void exitCurrentProcess(int ExitCode);

//! Agregar comentario
int killProcess(int pid);
int toggleProcessBlock(int pid);
int setProcessPriority(int pid, int priority);

// ============= HELPERS =============

/**
 * @brief Devuelve el PCB del proceso actualmente en ejecución.
 *
 * Utilidad para debugging y para el dispatcher cuando necesita acceder al
 * proceso activo.
 *
 * @return Puntero al `Process` en ejecución o NULL si no hay ninguno.
 */
Process *getCurrentProcess(void);

/**
 * @brief Devuelve el PID del proceso actualmente en ejecución.
 *
 * @return PID del proceso actual o -1 si no hay proceso en ejecución.
 */
int getCurrentPid(void);

size_t getProcessSnapshot(ProcessInfo *buffer, size_t maxCount);

#endif // PROCESS_H
