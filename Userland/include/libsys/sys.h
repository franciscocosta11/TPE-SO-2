#ifndef LIBSYS_SYS_H
#define LIBSYS_SYS_H

#include <process_info.h>
#include <stdint.h>

/**
 * @brief Scancodes admitidos para registrar callbacks personalizados.
 *
 * No incluye teclas de control como TAB o RETURN que se manejan internamente.
 */
enum REGISTERABLE_KEYS {
    ESCAPE_KEY        = 0x01,
    KEY_1             = 0x02,
    KEY_2             = 0x03,
    KEY_3             = 0x04,
    KEY_4             = 0x05,
    KEY_5             = 0x06,
    KEY_6             = 0x07,
    KEY_7             = 0x08,
    KEY_8             = 0x09,
    KEY_9             = 0x0A,
    KEY_0             = 0x0B,
    MINUS_KEY         = 0x0C,
    EQUALS_KEY        = 0x0D,
    BACKSPACE_KEY     = 0x0E,
    TABULATOR_KEY     = 0x0F,
    Q_KEY             = 0x10,
    W_KEY             = 0x11,
    E_KEY             = 0x12,
    R_KEY             = 0x13,
    T_KEY             = 0x14,
    Y_KEY             = 0x15,
    U_KEY             = 0x16,
    I_KEY             = 0x17,
    O_KEY             = 0x18,
    P_KEY             = 0x19,
    LEFT_BRACKET_KEY  = 0x1A,
    RIGHT_BRACKET_KEY = 0x1B,
    RETURN_KEY        = 0x1C,
    CONTROL_KEY_L     = 0x1D,
    A_KEY             = 0x1E,
    S_KEY             = 0x1F,
    D_KEY             = 0x20,
    F_KEY             = 0x21,
    G_KEY             = 0x22,
    H_KEY             = 0x23,
    J_KEY             = 0x24,
    K_KEY             = 0x25,
    L_KEY             = 0x26,
    SEMICOLON_KEY     = 0x27,
    APOSTROPHE_KEY    = 0x28,
    GRAVE_KEY         = 0x29,
    SHIFT_KEY_L       = 0x2A,
    BACKSLASH_KEY     = 0x2B,
    Z_KEY             = 0x2C,
    X_KEY             = 0x2D,
    C_KEY             = 0x2E,
    V_KEY             = 0x2F,
    B_KEY             = 0x30,
    N_KEY             = 0x31,
    M_KEY             = 0x32,
    COMMA_KEY         = 0x33,
    PERIOD_KEY        = 0x34,
    SLASH_KEY         = 0x35,
    SHIFT_KEY_R       = 0x36,
    KP_ASTERISK_KEY   = 0x37,
    ALT_KEY_L         = 0x38,
    SPACE_KEY         = 0x39,
    CAPS_LOCK_KEY     = 0x3A,
    F1_KEY            = 0x3B,
    F2_KEY            = 0x3C,
    F3_KEY            = 0x3D,
    F4_KEY            = 0x3E,
    F5_KEY            = 0x3F,
    F6_KEY            = 0x40,
    F7_KEY            = 0x41,
    F8_KEY            = 0x42,
    F9_KEY            = 0x43,
    F10_KEY           = 0x44,
    NUM_LOCK_KEY      = 0x45,
    SCROLL_LOCK_KEY   = 0x46,
    KP_HOME_KEY       = 0x47,
    KP_UP_KEY         = 0x48,
    KP_PAGE_UP_KEY    = 0x49,
    KP_MINUS_KEY      = 0x4A,
    KP_LEFT_KEY       = 0x4B,
    KP_BEGIN_KEY      = 0x4C,
    KP_RIGHT_KEY      = 0x4D,
    KP_PLUS_KEY       = 0x4E,
    KP_END_KEY        = 0x4F,
    KP_DOWN_KEY       = 0x50,
    KP_PAGE_DOWN_KEY  = 0x51,
    KP_INSERT_KEY     = 0x52,
    KP_DELETE_KEY     = 0x53,
    F11_KEY           = 0x57,
    F12_KEY           = 0x58
};

/**
 * @brief Inicia un beep en la frecuencia indicada.
 *
 * @param nFrequence Frecuencia en Hertz.
 */
void startBeep(uint32_t nFrequence);

/**
 * @brief Detiene cualquier beep en curso.
 */
void stopBeep(void);

/**
 * @brief Cambia el color de texto usado por consola.
 *
 * @param color Código de color ARGB/ANSI según configuración.
 */
void setTextColor(uint32_t color);

/**
 * @brief Cambia el color de fondo de la consola.
 *
 * @param color Código de color ARGB/ANSI según configuración.
 */
void setBackgroundColor(uint32_t color);

/**
 * @brief Incrementa la fuente en una unidad y devuelve el nuevo tamaño.
 *
 * @return Tamaño de fuente resultante.
 */
uint8_t increaseFontSize(void);

/**
 * @brief Reduce la fuente en una unidad y devuelve el nuevo tamaño.
 *
 * @return Tamaño de fuente resultante.
 */
uint8_t decreaseFontSize(void);

/**
 * @brief Fija el tamaño de fuente al valor indicado.
 *
 * @param size Nueva altura de caracteres.
 * @return Tamaño efectivo aplicado (puede saturar por límites).
 */
uint8_t setFontSize(uint8_t size);

/**
 * @brief Obtiene la hora actual directamente desde el RTC.
 *
 * @param hour Puntero donde se almacenará la hora.
 * @param minute Puntero para los minutos.
 * @param second Puntero para los segundos.
 */
void getDate(int *hour, int *minute, int *second);

/**
 * @brief Limpia el contenido de la pantalla.
 */
void clearScreen(void);

/**
 * @brief Dibuja un círculo sólido en el buffer de video.
 *
 * @param color Color en formato hexadecimal.
 * @param topleftX Coordenada X del bounding box.
 * @param topLefyY Coordenada Y del bounding box.
 * @param diameter Diámetro del círculo.
 */
void drawCircle(uint32_t color, long long int topleftX, long long int topLefyY, long long int diameter);

/**
 * @brief Dibuja un rectángulo sólido sobre el buffer de video.
 *
 * @param color Color de relleno.
 * @param width_pixels Ancho en píxeles.
 * @param height_pixels Alto en píxeles.
 * @param initial_pos_x Coordenada X del vértice superior izquierdo.
 * @param initial_pos_y Coordenada Y del vértice superior izquierdo.
 */
void drawRectangle(uint32_t color, long long int width_pixels, long long int height_pixels, long long int initial_pos_x, long long int initial_pos_y);

/**
 * @brief Rellena la memoria de video completa con un color plano.
 *
 * @param hexColor Color en formato hexadecimal.
 */
void fillVideoMemory(uint32_t hexColor);

/**
 * @brief Ejecuta una rutina del kernel y devuelve su resultado.
 *
 * @param fnPtr Puntero a función sin argumentos.
 * @return Resultado de la ejecución.
 */
int32_t exec(int32_t (*fnPtr)(void));

/**
 * @brief Alias histórico de @ref exec mantenido por compatibilidad.
 */
int32_t execProgram(int32_t (*fnPtr)(void));

/**
 * @brief Registra un handler para una tecla especial.
 *
 * @param scancode Tecla a monitorear.
 * @param fn Callback a invocar cuando se presione la tecla.
 */
void registerKey(enum REGISTERABLE_KEYS scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));

/**
 * @brief Registra un handler para combinaciones con Ctrl.
 *
 * @param scancode Tecla a monitorear junto con Ctrl.
 * @param fn Callback ejecutado cuando la combinación se dispara.
 */
void registerControlKey(enum REGISTERABLE_KEYS scancode, void (*fn)(enum REGISTERABLE_KEYS scancode));

/**
 * @brief Vacía el buffer circular de entrada de teclado.
 */
void clearInputBuffer(void);

/**
 * @brief Devuelve el ancho actual de la ventana gráfica.
 *
 * @return Ancho en píxeles.
 */
int getWindowWidth(void);

/**
 * @brief Devuelve la altura actual de la ventana gráfica.
 *
 * @return Alto en píxeles.
 */
int getWindowHeight(void);

/**
 * @brief Suspende el proceso actual cierta cantidad de milisegundos.
 *
 * @param milliseconds Tiempo a dormir.
 */
void sleep(uint32_t milliseconds);

/**
 * @brief Copia el último snapshot de registros tomado por el kernel.
 *
 * @param registers Buffer destino con espacio para todos los registros.
 * @return 0 en éxito o negativo si no hay snapshot disponible.
 */
int32_t getRegisterSnapshot(int64_t *registers);

/**
 * @brief Obtiene un carácter de teclado sin eco en pantalla.
 *
 * @return Código ASCII leído o negativo en error.
 */
int32_t getCharacterWithoutDisplay(void);

/**
 * @brief Llena @p buffer con información de procesos activos.
 *
 * @param buffer Arreglo de @ref ProcessInfo.
 * @param capacity Cantidad máxima de entradas.
 * @return Número de procesos copiados o negativo si falló.
 */
int32_t getProcesses(ProcessInfo *buffer, uint64_t capacity);

/**
 * @brief Solicita la terminación del proceso indicado.
 *
 * @param pid Identificador del proceso objetivo.
 * @return 0 en éxito o código de error.
 */
int32_t killProcess(int32_t pid);

/**
 * @brief Alterna el estado READY/BLOCKED del PID dado.
 *
 * @param pid Identificador del proceso.
 * @return Nuevo estado o negativo si no existe.
 */
int32_t toggleBlockProcess(int32_t pid);

/**
 * @brief Obtiene un resumen textual del uso de memoria.
 *
 * @param buffer Destino donde se escribe la descripción.
 * @param capacity Tamanio máximo del buffer.
 * @return Bytes escritos o negativo si no alcanza el espacio.
 */
int32_t getMemoryState(char *buffer, uint64_t capacity);

/**
 * @brief Modifica la prioridad de planificación del proceso.
 *
 * @param pid Proceso a ajustar.
 * @param priority Nuevo nivel de prioridad.
 * @return Prioridad previa o negativo en error.
 */
int32_t setProcessPriority(int32_t pid, int32_t priority);

/**
 * @brief Crea un nuevo proceso con la configuración indicada.
 *
 * @param name Nombre lógico mostrado en herramientas como `ps`.
 * @param entry Entrada principal del proceso.
 * @param argv Vector de argumentos.
 * @param argc Cantidad de argumentos válidos.
 * @param stackBase Stack opcional provisto por el llamador.
 * @param stackSize Tamaño del stack personalizado.
 * @param priority Prioridad inicial.
 * @param isForeground Si es distinto de cero, se trata como proceso en foreground.
 * @return PID del nuevo proceso o negativo en caso de error.
 */
int32_t createProcess(char *name, void (*entry)(uint64_t, char **), char **argv, uint32_t argc, void *stackBase, uint64_t stackSize, uint8_t priority, uint8_t isForeground);

/**
 * @brief Bloquea hasta que el proceso indicado finalice.
 *
 * @param pid Identificador del proceso hijo.
 * @return Código de retorno del proceso esperado o negativo si falló.
 */
int32_t waitProcess(int32_t pid);

/**
 * @brief Termina el proceso actual con código de salida.
 *
 * @param code Valor que verán los que esperen con @ref waitProcess.
 */
void exitProcess(int32_t code);

/**
 * @brief Obtiene el PID del proceso actualmente en ejecución.
 *
 * @return Identificador único del proceso.
 */
int32_t getPid(void);

/**
 * @brief Desbloquea manualmente al proceso indicado.
 *
 * @param pid Proceso objetivo.
 * @return 0 si fue desbloqueado o negativo si no estaba bloqueado.
 */
int32_t unblockProcess(int32_t pid);

/**
 * @brief Solicita al scheduler ceder voluntariamente la CPU.
 *
 * @return 0 cuando el yield se procesa correctamente.
 */
int32_t yieldProcess(void);

/**
 * @brief Cierra el descriptor indicado.
 *
 * @param fd File descriptor válido.
 * @return 0 en éxito o negativo si el descriptor es inválido.
 */
int32_t close(int32_t fd);

/**
 * @brief Crea un pipe y devuelve sus extremos.
 *
 * @param pipefd Arreglo de dos posiciones donde se escribirán [lectura, escritura].
 * @return 0 en éxito o código de error.
 */
int32_t pipe(int32_t pipefd[2]);

/**
 * @brief Redirecciona `newfd` al mismo recurso que `oldfd`.
 *
 * @param oldfd Descriptor original.
 * @param newfd Descriptor que pasará a apuntar al mismo recurso.
 * @return Descriptor resultante o negativo en error.
 */
int32_t dup2(int32_t oldfd, int32_t newfd);

/**
 * @brief Crea un semáforo identificado por nombre.
 *
 * @param name Etiqueta simbólica del semáforo.
 * @param initialValue Valor inicial del contador.
 * @return ID del semáforo o negativo si falló.
 */
int32_t semCreate(const char *name, uint32_t initialValue);

/**
 * @brief Abre un semáforo ya existente.
 *
 * @param name Nombre lógico del semáforo.
 * @return ID válido o negativo si no existe.
 */
int32_t semOpen(const char *name);

/**
 * @brief Cierra (decrementa la refcount) del semáforo indicado.
 *
 * @param semId Identificador retornado por create/open.
 * @return 0 en éxito o negativo si la ID no es válida.
 */
int32_t semClose(int32_t semId);

/**
 * @brief Operación P: decrementa y bloquea si el valor es negativo.
 *
 * @param semId Identificador del semáforo.
 * @return Valor restante o negativo en error.
 */
int32_t semWait(int32_t semId);

/**
 * @brief Operación V: incrementa y desbloquea si corresponde.
 *
 * @param semId Identificador del semáforo.
 * @return Valor actualizado o negativo en error.
 */
int32_t semPost(int32_t semId);

/**
 * @brief Obtiene el valor actual del semáforo.
 *
 * @param semId Identificador del semáforo.
 * @return Contador vigente o negativo si la ID no es válida.
 */
int32_t semGetValue(int32_t semId);

/**
 * @brief Restablece el semáforo a un nuevo valor y limpia bloqueados.
 *
 * @param semId Identificador objetivo.
 * @param newValue Valor a asignar.
 * @return 0 en éxito o negativo si la ID no existe.
 */
int32_t semReset(int32_t semId, uint32_t newValue);

/**
 * @brief Marca que se ingresó a una sección crítica de test.
 */
void semEnterCriticalTest(void);

/**
 * @brief Marca la salida de la sección crítica usada en tests.
 */
void semLeaveCriticalTest(void);

/**
 * @brief Devuelve cuántos procesos se reportaron dentro del test crítico.
 *
 * @return Número de procesos simultáneos registrados.
 */
int32_t semGetCriticalCount(void);
#endif // LIBSYS_SYS_H
