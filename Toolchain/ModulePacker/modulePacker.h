#ifndef MODULE_PACKER_H
#define MODULE_PACKER_H

#include <argp.h>
#include <stdio.h>

#define FALSE 0
#define TRUE !FALSE

#define BUFFER_SIZE 128
#define OUTPUT_FILE "packedKernel.bin"
#define MAX_FILES 128

/**
 * @brief Lista simple de archivos a empaquetar.
 *
 * @var array_t::array
 * Rutas absolutas o relativas a cada módulo que debe incluirse.
 *
 * @var array_t::length
 * Cantidad de posiciones válidas dentro del arreglo.
 */
typedef struct {
    char **array;
    int length;
} array_t;

/**
 * @brief Parámetros globales utilizados por `argp_parse`.
 *
 * @var arguments::args
 * Copia de los argumentos posicionales recibidos.
 *
 * @var arguments::silent
 * Si es no cero, suprime toda salida estándar.
 *
 * @var arguments::verbose
 * Si es no cero, habilita mensajes adicionales de progreso.
 *
 * @var arguments::output_file
 * Ruta del archivo binario resultante.
 *
 * @var arguments::count
 * Cantidad de archivos válidos detectados en @ref arguments::args.
 */
struct arguments {
    char *args[MAX_FILES];
    int silent;
    int verbose;
    char *output_file;
    int count;
};

/**
 * @brief Construye la imagen final empaquetando todos los módulos.
 *
 * @param fileArray Colección de rutas que se insertarán en el binario.
 * @param output_file Camino del archivo resultado.
 * @return 0 en éxito o código de error negativo.
 */
int buildImage(array_t fileArray, char *output_file);

/**
 * @brief Escribe el tamaño de un archivo en el flujo objetivo.
 *
 * @param target Archivo ya abierto donde se deja el tamaño (little endian).
 * @param filename Ruta del archivo cuya longitud debe serializarse.
 * @return 0 cuando se pudo escribir, negativo en error.
 */
int write_size(FILE *target, char *filename);

/**
 * @brief Copia el contenido de un archivo fuente al destino abierto.
 *
 * @param target Flujo ya abierto en modo binario de escritura.
 * @param source Flujo origen posicionado al inicio.
 * @return Cantidad de bytes copiados o negativo si falló.
 */
int write_file(FILE *target, FILE *source);

/**
 * @brief Verifica la existencia y accesibilidad de los archivos listados.
 *
 * @param fileArray Lista de rutas que deben chequearse.
 * @return 0 si todos son válidos, negativo si alguno falta o es ilegible.
 */
int checkFiles(array_t fileArray);

/**
 * @brief Handler de opciones utilizado por `argp_parse`.
 *
 * @param key Código de opción interpretado por argp.
 * @param arg Valor asociado a la opción, si corresponde.
 * @param state Contexto acumulado provisto por la librería.
 * @return 0 para continuar el parsing o `ARGP_ERR_UNKNOWN`.
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state);

#endif // MODULE_PACKER_H
