#ifndef _SOUND_H_
#define _SOUND_H_

#include <stdint.h>

/**
 * @brief Genera un tono en el speaker interno a la frecuencia indicada.
 */
void play_sound(uint32_t nFrequence);

enum SPEAKER {
    SPEAKER_ON = 0xFF,
    SPEAKER_OFF = 0x00,
};

/** @brief Configura el modo de operación del PIT canal 2. */
void setPITMode(uint8_t mode);
/** @brief Configura la frecuencia base del PIT canal 2. */
void setPITFrequency(uint16_t freq);
/** @brief Enciende o apaga el speaker AT. */
void setSpeaker(enum SPEAKER status);

#endif
