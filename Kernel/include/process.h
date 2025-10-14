#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// process.h (fragmento mínimo esperado)

typedef enum {
    UNUSED = 0,
    NEW, // el negro no banca
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
} process_state_t;

typedef struct pcb {
    int      pid;
    process_state_t state;

    // Identidad y control
    char     name[32];
    uint8_t  priority;
    bool     foreground;
    int      parent_pid;
    double   cpu_time;

    // Stack del proceso
    void    *stack_base;
    size_t   stack_size;
    bool     stack_owned;   // true si lo reservó el kernel y hay que liberarlo
    void    *sp;            // puntero de pila efectivo para el dispatcher

    // Punto de entrada (función "main" del proceso)
    void   (*entry_point)(void);

    // IPC (ajustá a tu TPE)
    int      shared_mem_id;
    int      pipe_in_fd;
    int      pipe_out_fd;

    // Si tuvieras un contexto de CPU explícito, agregalo:
    // cpu_context_t ctx;
} pcb_t;

// API del módulo
void     init_process_table(void);
int      create_process(const char *name, uint8_t priority, bool foreground, void (*entry_point)(void));
void     set_process_state(int pid, process_state_t new_state);
pcb_t*   get_next_ready_process(void);
void     set_current_process(pcb_t *p);
pcb_t*   get_current_process(void);
void     yield_current_process(void);
void     terminate_process(int pid);
void     print_process_table(void);
const char* state_to_string(process_state_t s);

#endif // PROCESS_H
