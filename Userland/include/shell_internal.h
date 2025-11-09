#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

#include <stdint.h>

#define MAX_BUFFER_SIZE 1024
#define HISTORY_SIZE 10
#define PROCESS_SNAPSHOT_CAP 32
#define PID_COL_WIDTH 3
#define STATE_COL_WIDTH 9
#define FG_COL_WIDTH 4
#define PRIORITY_COL_WIDTH 8
#define NAME_COL_WIDTH 8
#define STACK_COL_WIDTH 10
#define BASE_COL_WIDTH 10
#define COLUMN_PADDING 2

#define FOREGROUND 1

#define INC_MOD(x, m) ((x) = (((x) + 1) % (m)))
#define SUB_MOD(a, b, m) (((a) - (b) < 0) ? (m) - (b) + (a) : (a) - (b))
#define DEC_MOD(x, m) ((x) = SUB_MOD(x, 1, m))

/**
 * @brief Describe un comando disponible en la shell.
 *
 * @param name Nombre que el usuario debe escribir.
 * @param isProcess Indica si el comando se ejecuta como proceso (1) o builtin (0).
 * @param builtin Puntero a la rutina asociada cuando es un builtin.
 * @param entry Punto de entrada del proceso cuando corresponde.
 * @param description Texto descriptivo que se muestra en la ayuda.
 */
typedef struct
{
    char *name;
    uint8_t isProcess;           /* 0 = builtin, 1 = process */
    int (*builtin)(void);
    void (*entry)(void *);
    char *description;
} Command;

/**
 * @brief Tabla con todos los comandos registrados en la shell.
 */
extern Command commands[];

/**
 * @brief Cantidad de comandos disponibles en la tabla.
 */
extern const int command_count;

/**
 * @brief Historial circular de comandos ingresados por el usuario.
 */
extern char command_history[HISTORY_SIZE][MAX_BUFFER_SIZE];

/**
 * @brief Índice del próximo slot libre dentro del historial de comandos.
 */
extern uint8_t command_history_last;

/**
 * @brief Obtiene el flag que indica si el builtin actual debe correr en segundo plano.
 *
 * @return Valor distinto de cero cuando el builtin fue solicitado en background.
 */
uint8_t getCurrentBuiltinBackground(void);

/**
 * @brief Actualiza el flag que define si el builtin actual se ejecuta en segundo plano.
 *
 * @param value Nuevo valor del flag de background (0 para foreground).
 */
void setCurrentBuiltinBackground(uint8_t value);

#endif /* SHELL_INTERNAL_H */
