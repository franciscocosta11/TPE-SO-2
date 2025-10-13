// process.c — Gestión de procesos (PCB + tabla + helpers)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "process.h"
#include "MemoryManager.h"

// -----------------------------------------------------------------------------
// Configuración
// -----------------------------------------------------------------------------
#define MAX_PROCESSES        16
#define PROCESS_STACK_SIZE   (16 * 1024)   // 16 KiB; ajustá si tu kernel lo necesita

// -----------------------------------------------------------------------------
// Estado global del módulo
// -----------------------------------------------------------------------------
static pcb_t process_table[MAX_PROCESSES];
static int   next_pid = 1;                 // PIDs únicos crecientes
static int   rr_cursor = -1;               // cursor p/ round-robin simple
static pcb_t *current_process = NULL;      // proceso en ejecución (si hay)

// -----------------------------------------------------------------------------
// Helpers internos
// -----------------------------------------------------------------------------
static int  find_free_slot(void);
static void clear_pcb(pcb_t *p);
static void setup_initial_context(pcb_t *p, void (*entry_point)(void));

// -----------------------------------------------------------------------------
// Inicialización de la tabla de procesos
// -----------------------------------------------------------------------------
void init_process_table(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid   = -1;
        process_table[i].state = UNUSED;    // mejor que TERMINATED para slot libre
        process_table[i].name[0] = '\0';

        process_table[i].stack_base     = NULL;
        process_table[i].stack_size     = 0;
        process_table[i].stack_owned    = false;

        process_table[i].priority       = 0;
        process_table[i].cpu_time       = 0;
        process_table[i].parent_pid     = 0;
        process_table[i].foreground     = false;

        process_table[i].entry_point    = NULL;

        process_table[i].shared_mem_id  = -1;
        process_table[i].pipe_in_fd     = -1;
        process_table[i].pipe_out_fd    = -1;
        // si tenés semáforos por-proceso, inicializalos acá a valores inválidos
    }

    current_process = NULL;
    rr_cursor = -1;
    next_pid = 1;
}

// -----------------------------------------------------------------------------
// Creación de procesos
// -----------------------------------------------------------------------------
/**
 * Crea un proceso:
 *  - reserva stack (PROCESS_STACK_SIZE) con MemoryManager
 *  - inicializa PCB (PID, nombre, prioridad, fg/bg, estado READY, padre)
 *  - crea recursos IPC (shm, pipes) asociados al PID
 *  - deja el contexto inicial listo para ser despachado
 *
 * Devuelve: PID (>0) o -1 si no hay slot o memoria.
 */
int create_process(const char *name,
                   uint8_t priority,
                   bool foreground,
                   void (*entry_point)(void)) {
    if (entry_point == NULL || name == NULL) return -1;

    int slot = find_free_slot();
    if (slot < 0) {
        fprintf(stderr, "[Kernel] Sin slots de proceso disponibles\n");
        return -1;
    }

    pcb_t *pcb = &process_table[slot];
    clear_pcb(pcb);

    // Asignación de PID y metadatos
    pcb->pid        = next_pid++;
    pcb->state      = READY;
    pcb->priority   = priority;
    pcb->foreground = foreground;
    pcb->parent_pid = (current_process ? current_process->pid : 0);
    pcb->cpu_time   = 0;

    // Copia de nombre segura
    size_t cap = sizeof(pcb->name);
    strncpy(pcb->name, name, cap - 1);
    pcb->name[cap - 1] = '\0';

    // 1) Reservar memoria para el STACK del proceso
    pcb->stack_size  = PROCESS_STACK_SIZE;
    pcb->stack_base  = allocMemory(pcb->stack_size);
    if (pcb->stack_base == NULL) {
        fprintf(stderr, "[Kernel] Sin memoria para stack de PID %d (%s)\n", pcb->pid, pcb->name);
        clear_pcb(pcb);
        return -1;
    }
    pcb->stack_owned = true; // lo reservamos nosotros: lo liberamos al terminar

    // 2) Guardar entry point y setear contexto inicial (SP al tope del stack)
    pcb->entry_point = entry_point;
    setup_initial_context(pcb, entry_point);

    // 3) Inicializar recursos IPC por PID (según tu TPE)
    pcb->shared_mem_id = init_shared_memory(pcb->pid);
    pcb->pipe_in_fd    = create_pipe_in(pcb->pid);
    pcb->pipe_out_fd   = create_pipe_out(pcb->pid);
    // si usás semáforos por proceso, crealos acá

    printf("[Kernel] Proceso %s (PID %d) creado, READY\n", pcb->name, pcb->pid);
    return pcb->pid;
}

// -----------------------------------------------------------------------------
// Cambio de estado (con búsqueda por PID)
// -----------------------------------------------------------------------------
void set_process_state(int pid, process_state_t new_state) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            process_table[i].state = new_state;
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Planificador: próximo READY (Round-Robin básico)
// -----------------------------------------------------------------------------
pcb_t* get_next_ready_process(void) {
    if (MAX_PROCESSES == 0) return NULL;

    int start = (rr_cursor + 1) % MAX_PROCESSES;
    int i = start;

    do {
        if (process_table[i].state == READY) {
            rr_cursor = i;
            return &process_table[i];
        }
        i = (i + 1) % MAX_PROCESSES;
    } while (i != start);

    return NULL; // nadie en READY
}

// Setea proceso actual como RUNNING (útil para el dispatcher)
void set_current_process(pcb_t *p) {
    current_process = p;
    if (p) p->state = RUNNING;
}

pcb_t* get_current_process(void) {
    return current_process;
}

// Cede CPU: RUNNING -> READY y que el scheduler elija otro
void yield_current_process(void) {
    if (current_process && current_process->state == RUNNING) {
        current_process->state = READY;
    }
}

// -----------------------------------------------------------------------------
// Terminación de procesos (libera IPC + stack y marca UNUSED)
// -----------------------------------------------------------------------------
void terminate_process(int pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *pcb = &process_table[i];
        if (pcb->pid == pid) {
            // liberar IPC asociados
            if (pcb->shared_mem_id >= 0) {
                release_shared_memory(pcb->shared_mem_id);
                pcb->shared_mem_id = -1;
            }
            if (pcb->pipe_in_fd >= 0) {
                close_pipe(pcb->pipe_in_fd);
                pcb->pipe_in_fd = -1;
            }
            if (pcb->pipe_out_fd >= 0) {
                close_pipe(pcb->pipe_out_fd);
                pcb->pipe_out_fd = -1;
            }
            // liberar stack si lo reservamos nosotros
            if (pcb->stack_owned && pcb->stack_base) {
                freeMemory(pcb->stack_base);
            }

            printf("[Kernel] Proceso PID %d (%s) finalizado\n", pcb->pid, pcb->name);
            clear_pcb(pcb);
            return;
        }
    }
}

// -----------------------------------------------------------------------------
// Utilidades de depuración
// -----------------------------------------------------------------------------
const char* state_to_string(process_state_t s) {
    switch (s) {
        case UNUSED:     return "UNUSED";
        case NEW:        return "NEW";
        case READY:      return "READY";
        case RUNNING:    return "RUNNING";
        case BLOCKED:    return "BLOCKED";
        case TERMINATED: return "TERMINATED";
        default:         return "???";
    }
}

void print_process_table(void) {
    printf(" PID | STATE      | PRIO | FG | PARENT |   SHM  | IN  | OUT | NAME\n");
    printf("-----+------------+------+----+--------+--------+-----+-----+-----------------\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *p = &process_table[i];
        if (p->state == UNUSED) continue;
        printf("%4d | %-10s | %4u | %2u | %6d | %6d | %3d | %3d | %s\n",
               p->pid, state_to_string(p->state),
               p->priority, p->foreground ? 1 : 0, p->parent_pid,
               p->shared_mem_id, p->pipe_in_fd, p->pipe_out_fd, p->name);
    }
}

// -----------------------------------------------------------------------------
// Internos
// -----------------------------------------------------------------------------
static int find_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == UNUSED && process_table[i].pid == -1) {
            return i;
        }
    }
    return -1;
}

static void clear_pcb(pcb_t *p) {
    p->pid = -1;
    p->state = UNUSED;
    p->name[0] = '\0';

    p->stack_base  = NULL;
    p->stack_size  = 0;
    p->stack_owned = false;

    p->priority    = 0;
    p->cpu_time    = 0;
    p->parent_pid  = 0;
    p->foreground  = false;

    p->entry_point = NULL;

    p->shared_mem_id = -1;
    p->pipe_in_fd    = -1;
    p->pipe_out_fd   = -1;
}

// Setea el SP al tope del stack y deja el entry listo para el primer dispatch.
// Si tenés un contexto más rico (registros, flags), completalo acá.
static void setup_initial_context(pcb_t *p, void (*entry_point)(void)) {
    p->entry_point = entry_point;

    // Si usás un contexto explícito, por ejemplo:
    // p->ctx.rip = (uint64_t)entry_point;
    // p->ctx.rsp = (uint64_t)((uint8_t*)p->stack_base + p->stack_size);
    //
    // Como mínimo, guardamos el puntero de pila "lógico" para el dispatcher:
    p->sp = (uint8_t*)p->stack_base + p->stack_size; // top of stack
}
