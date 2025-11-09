#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "process_info.h"
#include "ipc.h"

extern int currentPid; // el primer proceso current va a ser el primero en inicializarse
extern int availableProcesses;

#define MAX_PROCESSES 16
#define MIN_PRIORITY PROCESS_PRIORITY_MIN
#define MAX_PRIORITY PROCESS_PRIORITY_MAX
#define IDLE_PID PROCESS_IDLE_PID
#define SHELL_PID PROCESS_SHELL_PID

#define FOREGROUND true
#define BACKGROUND false
#define SHELL_PROCESS_NAME "shell"
// Configuración
#define PROCESS_STACK_SIZE (16 * 1024) // 16 KiB; ajustá si tu kernel lo necesita

#define MAX_FD 16

/**
 * @brief Frame de pila que se guarda al switchear de contexto.
 *
 * El orden de los registros debe coincidir con la macro `pushState` en
 * `interrupts.asm` para garantizar que el scheduler pueda restaurarlos.
 */
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
    int parentPid;
    ProcessState state; // estado actual del proceso (ready running blocked etc)
    void *stackBase;  // base del stack --> para la posterior liberacion
    size_t stackSize; // tamaÑo del stack
    uint64_t ctx;  //! Puntero al contexto --> REVISAR
    int priority;
    char* name;
    bool isForeground;
    struct Process *next; // siguiente en la lista
    int waiterPid;
    void (*entry)(void *); // entry point
    char **Arg;             // argumento inicial
    File *fdTable[MAX_FD];
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
Process *createProcess(char* name, void (*Entry)(void *), char **Argv, int Argc ,void *StackBase, size_t StackSize, int priority, bool isForeground);

/**
 * @brief Termina el proceso actual con el código de salida indicado.
 *
 * @param ExitCode Código numérico de salida del proceso.
 */
void exitCurrentProcess(int ExitCode);

/**
 * @brief Termina el proceso indicado y libera sus recursos inmediatos.
 *
 * @param pid PID del proceso objetivo.
 * @return 0 si el proceso fue eliminado, negativo si no existe o no puede
 *         finalizarse.
 */
int killProcess(int pid);
/**
 * @brief Alterna el estado de bloqueo del proceso.
 *
 * @param pid PID del proceso objetivo.
 * @return Estado resultante o código negativo ante error.
 */
int toggleProcessBlock(int pid);
/**
 * @brief Actualiza la prioridad de planificación del proceso.
 *
 * @param pid PID del proceso cuyo valor se ajusta.
 * @param priority Nueva prioridad solicitada.
 * @return 0 si se aplicó correctamente, negativo en caso contrario.
 */
int setProcessPriority(int pid, int priority);
/**
 * @brief Desbloquea un proceso previamente bloqueado.
 *
 * @param pid PID del proceso a despertar.
 * @return 0 si fue desbloqueado, negativo si falla.
 */
int unblockProcess(int pid);
/**
 * @brief Bloquea al proceso actual hasta que termine el proceso dado.
 *
 * @param pid PID del proceso a esperar.
 */
void waitProcess(int pid);
/**
 * @brief Marca al proceso actual como bloqueado y cede la CPU.
 */
void blockCurrentProcess(void);

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

/**
 * @brief Copia información resumida de los procesos activos.
 *
 * @param buffer Búfer destino proporcionado por el llamador.
 * @param maxCount Capacidad máxima del búfer en entradas.
 * @return Cantidad de procesos copiados en el búfer.
 */
size_t getProcessSnapshot(ProcessInfo *buffer, size_t maxCount);

/**
 * @brief Busca un proceso por su PID y devuelve su PCB.
 *
 * @param pid Identificador del proceso a buscar.
 * @return Puntero al `Process` si existe; NULL si no se encuentra.
 */
Process *getProcessByPid(int pid);

/**
 * @brief Indica si el proceso corresponde a la shell interactiva.
 *
 * @param process Puntero al PCB evaluado.
 * @return true si el nombre coincide con la shell, false en caso contrario.
 */
bool isShellProcess(const Process *process);
/**
 * @brief Evalúa si el proceso tolera la señal generada por Ctrl+C.
 *
 * @param process Proceso objetivo.
 * @return true si puede manejar la señal, false en caso contrario.
 */
bool processCanHandleCtrlC(const Process *process);
/**
 * @brief Obtiene el proceso de primer plano susceptible de ser terminado.
 *
 * @return Proceso en foreground listo para ser finalizado o NULL.
 */
Process *getKillableForegroundProcess(void);
/**
 * @brief Finaliza un proceso y todos sus descendientes.
 *
 * @param pid Raíz del árbol de eliminación.
 */
void killProcessTree(int pid);

#endif // PROCESS_H
