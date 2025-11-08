// IPC and file abstractions
#ifndef IPC_H
#define IPC_H

#include <stddef.h>

// Forward declaration so FileOps can reference File*
typedef struct File File;

/**
 * @brief Tabla de operaciones que implementa un tipo de archivo.
 */
typedef struct {

    /**
     * @brief Lee 'count' bytes desde el archivo hacia 'buffer'.
     * @param file El puntero "this" al archivo abierto (para obtener private_data).
     * @param buffer El búfer de destino donde se copiarán los datos.
     * @param count El número máximo de bytes a leer.
     * @return El número de bytes leídos. 0 si es EOF. < 0 si hay error.
     */
    int (*read)(File *file, void *buffer, size_t count);

    /**
     * @brief Escribe 'count' bytes desde 'buffer' hacia el archivo.
     * @param file El puntero "this" al archivo abierto (para obtener private_data).
     * @param buffer El búfer de origen desde donde se copiarán los datos.
     * @param count El número de bytes a escribir.
     * @return El número de bytes escritos. < 0 si hay error.
     */
    int (*write)(File *file, const void *buffer, size_t count);

    /**
     * @brief Cierra esta instancia del archivo.
     * @param file El puntero "this" al archivo abierto.
     * @return 0 en caso de éxito, < 0 si hay error.
     */
    int (*close)(File *file);

    // Podrías añadir más operaciones aquí en el futuro (ej. lseek, ioctl)

} FileOps;

/**
 * @brief Descriptor genérico con punteros a operaciones y estado privado.
 */
struct File
{
    FileOps *ops;        /**< Implementación concreta (console, pipe, etc.). */
    void *privateData;   /**< Datos internos específicos del backend. */
    int flags;           /**< Bits de modo de apertura. */
    int refcount;        /**< Referencias activas al descriptor. */
};

/**
 * @brief Incrementa el contador de referencias del File.
 */
void fileRetain(File *file);

/**
 * @brief Decrementa el contador y cierra cuando llega a cero.
 */
void fileRelease(File *file);

#endif // IPC_H
