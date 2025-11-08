#ifndef _SYSCALL_DISPATCHER_H_
#define _SYSCALL_DISPATCHER_H_

#include <stdint.h>
#include <stddef.h>
#include <keyboard.h>
#include <process.h>
#include <process_info.h>
#include <string.h>

typedef struct {
    int64_t r15;
	int64_t r14;
	int64_t r13;
	int64_t r12;
	int64_t r11;
	int64_t r10;
	int64_t r9;
	int64_t r8;
	int64_t rsi;
	int64_t rdi;
	int64_t rbp;
	int64_t rdx;
	int64_t rcx;
	int64_t rbx;
	int64_t rax;
	int64_t rip;
} Registers;

/**
 * @brief Punto de entrada común para todas las syscalls (int 0x80).
 *
 * Decodifica el número de syscall, valida parámetros básicos y delega en la
 * implementación específica.
 *
 * @param registers Snapshot del contexto al momento de la interrupción.
 * @return Valor devuelto por la syscall solicitada.
 */
uint64_t syscallDispatcher(Registers * registers);

// Linux syscall prototypes
/** @brief Escribe @p count bytes de @p __user_buf en el descriptor @p fd. */
int32_t sys_write(int32_t fd, char * __user_buf, int32_t count);
/** @brief Lee hasta @p count bytes desde @p fd hacia @p __user_buf. */
int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count);
/** @brief Cierra el descriptor indicado. */
int32_t sys_close(int32_t fd);
/** @brief Crea un pipe anónimo con dos extremos de lectura/escritura. */
int32_t sys_pipe(int32_t pipefd[2]);
/** @brief Reasigna @p newfd para que duplique a @p oldfd. */
int32_t sys_dup2(int32_t oldfd, int32_t newfd);

// Custom syscall prototypes
/** @brief Inicia un beep a la frecuencia indicada. */
int32_t sys_start_beep(uint32_t nFrequence);
/** @brief Detiene el beep activo. */
int32_t sys_stop_beep(void);
/** @brief Cambia el color del texto actual. */
int32_t sys_fonts_text_color(uint32_t color);
/** @brief Cambia el color de fondo actual. */
int32_t sys_fonts_background_color(uint32_t color);
/** @brief Reduce el tamaño de fuente en una unidad. */
int32_t sys_fonts_decrease_size(void);
/** @brief Incrementa el tamaño de fuente en una unidad. */
int32_t sys_fonts_increase_size(void);
/** @brief Fija explícitamente el tamaño de fuente. */
int32_t sys_fonts_set_size(uint8_t size);
/** @brief Limpia el contenido de la pantalla. */
int32_t sys_clear_screen(void);
/** @brief Borra el buffer de entrada del teclado. */
int32_t sys_clear_input_buffer(void);
/** @brief Devuelve el ancho de la ventana en píxeles. */
uint16_t sys_window_width(void);
/** @brief Devuelve el alto de la ventana en píxeles. */
uint16_t sys_window_height(void);

// Date syscall prototypes
/** @brief Escribe la hora actual en @p hour. */
int32_t sys_hour(int * hour);
/** @brief Escribe los minutos actuales en @p minute. */
int32_t sys_minute(int * minute);
/** @brief Escribe los segundos actuales en @p second. */
int32_t sys_second(int * second);


/** @brief Dibuja un círculo lleno en coordenadas dadas. */
int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
// Draw rectangle syscall prototype
/** @brief Dibuja un rectángulo lleno en pantalla. */
int32_t sys_rectangle(uint32_t color, uint64_t width_pixels, uint64_t height_pixels, uint64_t initial_pos_x, uint64_t initial_pos_y);
/** @brief Rellena toda la pantalla con un color sólido. */
int32_t sys_fill_video_memory(uint32_t hexColor);

// Custom exec syscall prototype
/** @brief Ejecuta una función del kernel en contexto aislado. */
int32_t sys_exec(int32_t (*fnPtr)(void));

// Custom keyboard syscall prototypes
/** @brief Registra un handler para una tecla especial. */
int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn);
/** @brief Registra un handler para combinaciones con Ctrl. */
int32_t sys_register_ctrl_key(uint8_t scancode, SpecialKeyHandler fn);

// System sleep
/** @brief Suspende el proceso actual la cantidad de milisegundos indicada. */
int32_t sys_sleep_milis(uint32_t milis);

// Register snapshot
/** @brief Copia el último snapshot de registros en @p registers. */
int32_t sys_get_register_snapshot(int64_t * registers);

// Get character without showing
/** @brief Obtiene un caracter del teclado sin eco en pantalla. */
int32_t sys_get_character_without_display(void);

/** @brief Copia hasta @p capacity procesos activos al buffer de usuario. */
int32_t sys_get_processes(ProcessInfo *userBuffer, uint64_t capacity);
/** @brief Mata el proceso indicado. */
int32_t sys_kill_process(int32_t pid);
/** @brief Alterna READY/BLOCKED para el PID dado. */
int32_t sys_toggle_block_process(int32_t pid);
/** @brief Describe el estado del heap del kernel en @p userBuffer. */
int32_t sys_get_memory_state(char *userBuffer, uint64_t capacity);
/** @brief Cambia la prioridad del proceso indicado. */
int32_t sys_set_process_priority(int32_t pid, int32_t priority);
/** @brief Crea un proceso hijo con los parámetros dados. */
int32_t sys_create_process(char* name, ProcessEntryPoint entry, char **argv, uint32_t argc, void *stackBase, uint64_t stackSize, int priority, uint8_t isForeground);
/** @brief Termina el proceso actual con @p status. */
int32_t sys_exit(int32_t status);
/** @brief El proceso actual cede voluntariamente la CPU. */
int32_t sys_yield(void);
/** @brief Bloquea al proceso actual hasta que @p pid finalice. */
int32_t sys_wait_process(int32_t pid);
/** @brief Devuelve el PID del proceso en ejecución. */
int32_t sys_get_pid(void);
/** @brief Desbloquea manualmente al proceso indicado. */
int32_t sys_unblock_process(int32_t pid);
/** @brief Reserva memoria en el heap del kernel. */
void *sys_alloc_memory(size_t size);
/** @brief Libera memoria previamente asignada por @ref sys_alloc_memory. */
int32_t sys_free_memory(void *block);

#endif
