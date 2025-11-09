#ifndef _SYSCALL_DISPATCHER_H_
#define _SYSCALL_DISPATCHER_H_

#include <stdint.h>
#include <stddef.h>
#include <keyboard.h>
#include <process.h>
#include <process_info.h>
#include <string.h>

/**
 * @brief Estado de los registros preservados al invocar una syscall.
 */
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
 * @brief Resuelve la syscall solicitada por el proceso en modo usuario.
 *
 * @param registers Contexto de registros con los parámetros y estado previo.
 * @return Resultado entero entregado al llamador de la syscall.
 */
uint64_t syscallDispatcher(Registers * registers);

// Linux syscall prototypes
/**
 * @brief Escribe datos en un descriptor de archivo.
 *
 * @param fd Descriptor destino.
 * @param __user_buf Búfer con los datos a enviar.
 * @param count Cantidad de bytes a escribir.
 * @return Número de bytes escritos o un valor negativo ante error.
 */
int32_t sys_write(int32_t fd, char * __user_buf, int32_t count);
/**
 * @brief Lee datos desde un descriptor de archivo.
 *
 * @param fd Descriptor origen.
 * @param __user_buf Búfer de destino en espacio de usuario.
 * @param count Capacidad máxima en bytes del búfer.
 * @return Número de bytes leídos o un código de error negativo.
 */
int32_t sys_read(int32_t fd, signed char * __user_buf, int32_t count);
/**
 * @brief Cierra un descriptor de archivo abierto por el proceso.
 *
 * @param fd Descriptor a cerrar.
 * @return 0 en caso de éxito o un código de error negativo.
 */
int32_t sys_close(int32_t fd);
/**
 * @brief Crea un par de extremos de pipe.
 *
 * @param pipefd Arreglo de dos enteros donde se almacenan los descriptores.
 * @return 0 al crearse con éxito o código de error negativo.
 */
int32_t sys_pipe(int32_t pipefd[2]);
/**
 * @brief Duplica un descriptor de archivo sobre otro descriptor.
 *
 * @param oldfd Descriptor original.
 * @param newfd Descriptor destino.
 * @return Descriptor resultante o un código de error negativo.
 */
int32_t sys_dup2(int32_t oldfd, int32_t newfd);

// Custom syscall prototypes
/**
 * @brief Inicia la reproducción de un tono.
 *
 * @param nFrequence Frecuencia del tono en hertz.
 * @return 0 si se inicia correctamente, negativo ante error.
 */
int32_t sys_start_beep(uint32_t nFrequence);
/**
 * @brief Detiene la reproducción de cualquier tono activo.
 *
 * @return 0 al detener correctamente, negativo en caso contrario.
 */
int32_t sys_stop_beep(void);
/**
 * @brief Configura el color del texto de la fuente actual.
 *
 * @param color Valor RGB de 32 bits.
 * @return 0 si se aplicó el color, negativo ante error.
 */
int32_t sys_fonts_text_color(uint32_t color);
/**
 * @brief Configura el color de fondo de la fuente.
 *
 * @param color Valor RGB de 32 bits.
 * @return 0 si la operación fue exitosa, negativo si falló.
 */
int32_t sys_fonts_background_color(uint32_t color);
/**
 * @brief Disminuye en un paso el tamaño de la fuente.
 *
 * @return 0 si se aplicó el nuevo tamaño, negativo ante error.
 */
int32_t sys_fonts_decrease_size(void);
/**
 * @brief Incrementa en un paso el tamaño de la fuente.
 *
 * @return 0 si se aplicó el nuevo tamaño, negativo ante error.
 */
int32_t sys_fonts_increase_size(void);
/**
 * @brief Establece un tamaño específico de fuente.
 *
 * @param size Nuevo tamaño en píxeles.
 * @return 0 si se configuró correctamente, negativo ante error.
 */
int32_t sys_fonts_set_size(uint8_t size);
/**
 * @brief Limpia todo el contenido visible de la pantalla.
 *
 * @return 0 si se limpió correctamente, negativo en caso contrario.
 */
int32_t sys_clear_screen(void);
/**
 * @brief Vacía el búfer de entrada de teclado.
 *
 * @return 0 si se vació correctamente, negativo ante error.
 */
int32_t sys_clear_input_buffer(void);
/**
 * @brief Obtiene el ancho actual de la ventana en píxeles.
 *
 * @return Ancho en píxeles.
 */
uint16_t sys_window_width(void);
/**
 * @brief Obtiene la altura actual de la ventana en píxeles.
 *
 * @return Altura en píxeles.
 */
uint16_t sys_window_height(void);

// Date syscall prototypes
/**
 * @brief Obtiene la hora actual del sistema.
 *
 * @param hour Puntero donde se almacena la hora (0-23).
 * @return 0 en caso de éxito, negativo si falla.
 */
int32_t sys_hour(int * hour);
/**
 * @brief Obtiene los minutos actuales del sistema.
 *
 * @param minute Puntero donde se almacena el minuto (0-59).
 * @return 0 en caso de éxito, negativo si falla.
 */
int32_t sys_minute(int * minute);
/**
 * @brief Obtiene los segundos actuales del sistema.
 *
 * @param second Puntero donde se almacena el segundo (0-59).
 * @return 0 en caso de éxito, negativo si falla.
 */
int32_t sys_second(int * second);


/**
 * @brief Dibuja un círculo sólido en pantalla.
 *
 * @param hexColor Color del círculo en formato hexadecimal.
 * @param topLeftX Coordenada X del cuadro contenedor.
 * @param topLeftY Coordenada Y del cuadro contenedor.
 * @param diameter Diámetro del círculo en píxeles.
 * @return 0 si se dibuja correctamente, negativo ante error.
 */
int32_t sys_circle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
// Draw rectangle syscall prototype
/**
 * @brief Dibuja un rectángulo sólido en pantalla.
 *
 * @param color Color en formato hexadecimal.
 * @param width_pixels Ancho del rectángulo en píxeles.
 * @param height_pixels Alto del rectángulo en píxeles.
 * @param initial_pos_x Coordenada X inicial.
 * @param initial_pos_y Coordenada Y inicial.
 * @return 0 si el dibujo fue exitoso, negativo si se produjo un error.
 */
int32_t sys_rectangle(uint32_t color, uint64_t width_pixels, uint64_t height_pixels, uint64_t initial_pos_x, uint64_t initial_pos_y);
/**
 * @brief Rellena la memoria de video con un color uniforme.
 *
 * @param hexColor Color a aplicar.
 * @return 0 si se completó la operación, negativo si falló.
 */
int32_t sys_fill_video_memory(uint32_t hexColor);

// Custom exec syscall prototype
/**
 * @brief Ejecuta una función en contexto de usuario.
 *
 * @param fnPtr Puntero a la función a ejecutar.
 * @return Código de retorno de la función o negativo si no pudo lanzarse.
 */
int32_t sys_exec(int32_t (*fnPtr)(void));

// Custom keyboard syscall prototypes
/**
 * @brief Registra un manejador para una tecla especial.
 *
 * @param scancode Código de la tecla.
 * @param fn Función que se invocará al presionar la tecla.
 * @return 0 si se registró con éxito, negativo ante error.
 */
int32_t sys_register_key(uint8_t scancode, SpecialKeyHandler fn);
/**
 * @brief Registra un manejador para combinaciones con Ctrl.
 *
 * @param scancode Código de la tecla modificadora.
 * @param fn Función asociada a la combinación.
 * @return 0 si se registró con éxito, negativo ante error.
 */
int32_t sys_register_ctrl_key(uint8_t scancode, SpecialKeyHandler fn);

// System sleep
/**
 * @brief Suspende la ejecución del proceso por una cantidad dada de milisegundos.
 *
 * @param milis Tiempo de suspensión en milisegundos.
 * @return 0 cuando se concreta la suspensión, negativo si falla.
 */
int32_t sys_sleep_milis(uint32_t milis);

// Register snapshot
/**
 * @brief Obtiene una copia de los registros de la última excepción o interrupción.
 *
 * @param registers Búfer donde se almacena el snapshot.
 * @return 0 si hay un snapshot disponible, negativo en caso contrario.
 */
int32_t sys_get_register_snapshot(int64_t * registers);

// Get character without showing
/**
 * @brief Lee un carácter del teclado sin eco en pantalla.
 *
 * @return Código ASCII leído o negativo si no hay datos disponibles.
 */
int32_t sys_get_character_without_display(void);

/**
 * @brief Obtiene un listado de procesos activos.
 *
 * @param userBuffer Arreglo destino en espacio de usuario.
 * @param capacity Cantidad máxima de entradas disponibles.
 * @return Número de procesos copiados o código de error negativo.
 */
int32_t sys_get_processes(ProcessInfo *userBuffer, uint64_t capacity);
/**
 * @brief Solicita la finalización de un proceso dado.
 *
 * @param pid Identificador del proceso.
 * @return 0 si se envió la señal exitosamente, negativo en caso contrario.
 */
int32_t sys_kill_process(int32_t pid);
/**
 * @brief Alterna el estado de bloqueo de un proceso.
 *
 * @param pid Identificador del proceso.
 * @return Estado resultante o código de error negativo.
 */
int32_t sys_toggle_block_process(int32_t pid);
/**
 * @brief Recupera información global del estado de la memoria.
 *
 * @param userBuffer Búfer de salida en espacio de usuario.
 * @param capacity Tamaño máximo del búfer.
 * @return 0 si se copió la información, negativo si falló.
 */
int32_t sys_get_memory_state(char *userBuffer, uint64_t capacity);
/**
 * @brief Cambia la prioridad de planificación de un proceso.
 *
 * @param pid Identificador del proceso.
 * @param priority Nueva prioridad solicitada.
 * @return 0 al actualizarla, negativo ante error.
 */
int32_t sys_set_process_priority(int32_t pid, int32_t priority);
/**
 * @brief Crea un nuevo proceso en el sistema.
 *
 * @param name Nombre identificatorio.
 * @param entry Punto de entrada del proceso (tipo ProcessEntryPoint).
 * @param argv Vector de argumentos.
 * @param argc Cantidad de argumentos.
 * @param stackBase Dirección base del stack.
 * @param stackSize Tamaño del stack en bytes.
 * @param priority Prioridad inicial.
 * @param isForeground Indica si es un proceso de primer plano.
 * @return PID del nuevo proceso o un código negativo si falla.
 */
int32_t sys_create_process(char* name, ProcessEntryPoint entry, char **argv, uint32_t argc, void *stackBase, uint64_t stackSize, int priority, uint8_t isForeground);
/**
 * @brief Termina la ejecución del proceso actual.
 *
 * @param status Código de salida a reportar al padre.
 * @return No retorna; el valor se entrega al proceso que hace wait.
 */
int32_t sys_exit(int32_t status);
/**
 * @brief Cede voluntariamente la CPU al planificador.
 *
 * @return 0 si se realizó el yield, negativo ante error.
 */
int32_t sys_yield(void);
/**
 * @brief Bloquea al proceso llamador hasta que finalice el proceso indicado.
 *
 * @param pid Proceso objetivo.
 * @return Código de finalización del proceso esperado o negativo ante error.
 */
int32_t sys_wait_process(int32_t pid);
/**
 * @brief Obtiene el PID del proceso en ejecución.
 *
 * @return PID actual.
 */
int32_t sys_get_pid(void);
/**
 * @brief Desbloquea un proceso previamente bloqueado.
 *
 * @param pid Proceso a desbloquear.
 * @return 0 si se desbloqueó, negativo si falla.
 */
int32_t sys_unblock_process(int32_t pid);
/**
 * @brief Reserva memoria dinámica para el proceso llamador.
 *
 * @param size Cantidad de bytes solicitados.
 * @return Puntero al bloque reservado o NULL si no se pudo asignar.
 */
void *sys_alloc_memory(size_t size);
/**
 * @brief Libera un bloque de memoria previamente asignado.
 *
 * @param block Dirección devuelta por sys_alloc_memory.
 * @return 0 si se liberó, negativo si ocurrió un error.
 */
int32_t sys_free_memory(void *block);

#endif
