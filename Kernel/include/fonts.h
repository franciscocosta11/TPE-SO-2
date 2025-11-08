#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>
#include <video.h>

#define DEFAULT_TEXT_COLOR 0x00FFFFFF
#define DEFAULT_ERROR_COLOR 0x00DE382B
#define DEFAULT_BACKGROUND_COLOR 0x00000000

#define DEFAULT_GLYPH_SIZE_X 8
#define DEFAULT_GLYPH_SIZE_Y 8

#define NEW_LINE_CHAR '\n'
#define TABULATOR_CHAR '\t'
#define CARRIAGE_RETURN_CHAR '\r'
#define ESCAPE_CHAR '\e'
#define TAB_SIZE 4

/** @brief Imprime un caracter ASCII en la posición actual. */
void putChar(char ascii);
/** @brief Escribe una cadena terminada en NULL en pantalla. */
void print(const char * string);
/** @brief Escribe @p count bytes de @p string en el descriptor @p fd. */
int32_t printToFd(int32_t fd, const char * string, int32_t count);
/** @brief Avanza manualmente a la siguiente línea. */
void newLine(void);
/** @brief Imprime un número decimal sin signo. */
void printDec(uint64_t value);
/** @brief Imprime un número hexadecimal sin signo (prefijo 0x). */
void printHex(uint64_t value);
/** @brief Imprime un número en binario. */
void printBin(uint64_t value);
/** @brief Limpia la pantalla y reposiciona el cursor en (0,0). */
void clear(void);

/** @brief Muestra el cursor textual. */
void showCursor(void);
/** @brief Oculta el cursor textual. */
void hideCursor(void);
/** @brief Retrocede una posición el cursor (sin borrar el caracter). */
void retractPosition(void);
/** @brief Borra el caracter previo y mueve el cursor atrás. */
void clearPreviousCharacter(void);
/** @brief Devuelve la posición X actual en el buffer. */
uint16_t getXBufferPosition(void);

/** @brief Incrementa el tamaño de fuente en un paso. */
uint8_t increaseFontSize(void);
/** @brief Reduce el tamaño de fuente en un paso. */
uint8_t decreaseFontSize(void);
/** @brief Fija el tamaño de fuente al valor solicitado. */
uint8_t setFontSize(int8_t size);
/** @brief Devuelve el tamaño de fuente vigente. */
uint8_t getFontSize(void);
/** @brief Cambia el color del texto para futuras escrituras. */
void setTextColor(uint32_t color);
/** @brief Cambia el color de fondo para futuras escrituras. */
void setBackgroundColor(uint32_t color);
/** @brief Devuelve el color de texto actual. */
uint32_t getTextColor(void);
/** @brief Devuelve el color de fondo actual. */
uint32_t getBackgroundColor(void);

#endif
