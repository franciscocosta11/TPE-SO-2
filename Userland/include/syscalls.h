#ifndef _LIBC_SYSCALLS_H_
#define _LIBC_SYSCALLS_H_

#include <stdint.h>
#include <sys.h>
#include <process_info.h>

// Linux syscall prototypes
/** @brief Escribe @p count bytes del buffer @p buf al descriptor @p fd. */
int32_t sys_write(int64_t fd, const void *buf, int64_t count);
/** @brief Lee hasta @p count bytes desde @p fd hacia @p buf. */
int32_t sys_read(int64_t fd, void *buf, int64_t count);
/** @brief Cierra el descriptor @p fd. */
int32_t sys_close(int32_t fd);
/** @brief Crea un pipe anónimo y devuelve ambos extremos. */
int32_t sys_pipe(int32_t pipefd[2]);
/** @brief Duplica @p oldfd sobre @p newfd. */
int32_t sys_dup2(int32_t oldfd, int32_t newfd);

// Custom syscall prototypes
int32_t sys_start_beep(uint32_t nFrequence);
int32_t sys_stop_beep(void);
int32_t sys_fonts_text_color(uint32_t color);
int32_t sys_fonts_background_color(uint32_t color);
int32_t sys_fonts_decrease_size(void);
int32_t sys_fonts_increase_size(void);
int32_t sys_fonts_set_size(uint8_t size);
int32_t sys_clear_screen(void);
int32_t sys_clear_input_buffer(void);

// Date syscall prototypes
int32_t sys_hour(int *hour);
int32_t sys_minute(int *minute);
int32_t sys_second(int *second);

/** @brief Dibuja un círculo sólido en pantalla. */
int32_t sys_circle(int color, long long int topleftX, long long int topLefyY, long long int diameter);
/** @brief Dibuja un rectángulo sólido. */
int32_t sys_rectangle(int color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y);
/** @brief Rellena la pantalla completa con el color dado. */
int32_t sys_fill_video_memory(uint32_t hexColor);

int32_t sys_exec(int32_t (*fnPtr)(void));
int32_t sys_exit(int32_t status);
int32_t sys_yield(void);

int32_t sys_register_key(uint8_t scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));
int32_t sys_register_ctrl_key(uint8_t scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));

int32_t sys_window_width(void);
int32_t sys_window_height(void);

int32_t sys_sleep_milis(uint32_t milis);

int32_t sys_get_register_snapshot(int64_t *registers);

int32_t sys_get_character_without_display(void);

int32_t sys_get_processes(ProcessInfo *buffer, uint64_t capacity);
int32_t sys_kill_process(int32_t pid);
int32_t sys_toggle_block_process(int32_t pid);
int32_t sys_get_memory_state(char *buffer, uint64_t capacity);
int32_t sys_set_process_priority(int32_t pid, int32_t priority);
int32_t sys_create_process(char* name, void (*entry)(uint64_t, char **), char **argv, uint32_t argc, void *stackBase, uint64_t stackSize, uint8_t priority, uint8_t isForeground);
int32_t sys_wait_process(int32_t pid);
int32_t sys_get_pid(void);
int32_t sys_unblock_process(int32_t pid);
uint64_t sys_alloc_memory(uint64_t size);
int32_t sys_free_memory(void *block);

// Semaphore syscalls
int32_t sys_sem_create(const char *name, uint32_t initialValue);
int32_t sys_sem_open(const char *name);
int32_t sys_sem_close(int32_t semId);
int32_t sys_sem_wait(int32_t semId);
int32_t sys_sem_post(int32_t semId);
int32_t sys_sem_get_value(int32_t semId);
int32_t sys_sem_reset(int32_t semId, uint32_t newValue);

/* Critical-test semaphore helpers */
void sys_sem_enter_critical_test(void);
void sys_sem_leave_critical_test(void);
int32_t sys_sem_get_critical_count(void);

#endif
