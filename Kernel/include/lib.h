#ifndef LIB_H
#define LIB_H

#include <stdint.h>
#include <fonts.h>

#define EOF -1

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
int strcmp(const char *s1, const char *s2);
char *strncpy(char *dest, const char *src, uint64_t n);
void printf(const char * string);

uint8_t getKeyboardBuffer(void);
uint8_t getKeyboardStatus(void);

uint8_t getSecond(void);
uint8_t getMinute(void);
uint8_t getHour(void);

uint8_t * initStack(void * rsp, void * rip, int argc, char ** argv);

// Operación atómica de intercambio (exchange)
uint8_t _xchg(uint8_t *ptr, uint8_t newValue);

#endif
