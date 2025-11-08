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
typedef void (*ProcessEntryPoint)(uint64_t argc, char **argv);

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
    ProcessEntryPoint entry; // entry point
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
 * @brief Crea un nuevo proceso y lo agrega a la cola READY.
 *
 * Se reserva un stack (si StackBase es NULL se toma del heap), se inicializa
 * el PCB y se heredan los descriptores del proceso padre.
 *
 * @param name Nombre descriptivo usado por ps.
 * @param Entry Punto de entrada que ejecutará el proceso.
 * @param Argv Vector de argumentos terminado en NULL.
 * @param Argc Cantidad de argumentos de Argv.
 * @param StackBase Stack preasignado o NULL para que lo reserve el kernel.
 * @param StackSize Tamaño del stack si StackBase no es NULL.
 * @param priority Prioridad inicial (0..3).
 * @param isForeground Indica si bloquea a la shell al ejecutarse.
 * @return Puntero al PCB creado o NULL si no hay recursos.
 */
Process *createProcess(char* name, ProcessEntryPoint Entry, char **Argv, int Argc ,void *StackBase, size_t StackSize, int priority, bool isForeground);

/**
 * @brief Termina el proceso actual con el código indicado.
 *
 * Libera recursos asociados, despierta a su waiter (si existe) y marca al
 * proceso como TERMINATED.
 *
 * @param ExitCode Código de salida reportado al proceso que espera.
 */
void exitCurrentProcess(int ExitCode);

/**
 * @brief Mata el proceso identificado por @p pid.
 *
 * Si el PID corresponde al proceso en ejecución, delega en
 * @ref exitCurrentProcess. En otros casos limpia recursos y lo remueve de
 * las colas del scheduler.
 *
 * @param pid Proceso a terminar.
 * @return 0 si tuvo éxito, -1 ante errores (PID inválido).
 */
int killProcess(int pid);

/**
 * @brief Termina un proceso y toda su descendencia.
 *
 * Recorre recursivamente los hijos del PID dado invocando @ref killProcess
 * sobre cada uno de ellos.
 */
void killProcessTree(int pid);

/**
 * @brief Alterna el estado READY/BLOCKED de un proceso dado.
 *
 * Se usa principalmente desde la consola para forzar bloqueos o desbloqueos
 * manuales.
 *
 * @param pid Proceso objetivo.
 * @return Nuevo estado (BLOCKED o READY) o -1 si falló.
 */
int toggleProcessBlock(int pid);

/**
 * @brief Ajusta la prioridad de scheduling de un proceso.
 *
 * Si el proceso estaba READY se reencola según la nueva prioridad.
 *
 * @param pid Proceso a modificar.
 * @param priority Valor entre MIN_PRIORITY y MAX_PRIORITY-1.
 * @return 0 si se aplicó, -1 si los parámetros son inválidos.
 */
int setProcessPriority(int pid, int priority);

/**
 * @brief Reincorpora a READY un proceso previamente bloqueado.
 *
 * @param pid Proceso que se quiere despertar.
 * @return 0 si se agregó correctamente a READY, -1 si el PID es inválido.
 */
int unblockProcess(int pid);

/**
 * @brief Suspende al proceso actual hasta que el PID indicado termine.
 */
void waitProcess(int pid);

/**
 * @brief Marca al proceso actual como BLOCKED y cede la CPU.
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
 * @brief Copia una instantánea de la tabla de procesos.
 *
 * @param buffer Arreglo destino donde escribir la información.
 * @param maxCount Cantidad máxima de entradas que puede almacenar buffer.
 * @return Número de procesos escritos en buffer.
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
 * @brief Indica si el PCB corresponde a la shell interactiva.
 */
bool isShellProcess(const Process *process);

/**
 * @brief Determina si el proceso está suscrito al manejo de Ctrl+C.
 */
bool processCanHandleCtrlC(const Process *process);

/**
 * @brief Devuelve el proceso en foreground que puede ser terminado por Ctrl+C.
 */
Process *getKillableForegroundProcess(void);

#endif // PROCESS_H
