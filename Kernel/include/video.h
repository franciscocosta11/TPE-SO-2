#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include <stdint.h>

/** @brief Dibuja un pixel en las coordenadas (x,y). */
void putPixel(uint32_t hexColor, uint64_t x, uint64_t y);
/** @brief Dibuja un círculo lleno tomando el rectángulo delimitador. */
void drawCircle(uint32_t hexColor, uint64_t topLeftX, uint64_t topLeftY, uint64_t diameter);
/** @brief Dibuja un rectángulo lleno del tamaño indicado. */
void drawRectangle(uint32_t hexColor, uint64_t width, uint64_t height, uint64_t initial_pos_x, uint64_t initial_pos_y);
/** @brief Rellena todo el buffer de video con un color sólido. */
void fillVideoMemory(uint32_t hexColor);

/** @brief Devuelve el ancho actual de la ventana. */
uint16_t getWindowWidth(void);
/** @brief Devuelve el alto actual de la ventana. */
uint16_t getWindowHeight(void);

/** @brief Realiza un scroll vertical hacia arriba y completa con un color. */
void scrollVideoMemoryUp(uint16_t scroll, uint32_t fillColor);

#endif
