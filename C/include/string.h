#ifndef __STRING_H__
#define __STRING_H__

#include <stdint.h>

#define CRC_INITIAL 0xDEADC0DE

extern uint32_t qcrc_update (uint32_t inp, uint8_t v);
extern void *memcpy(void *dst, const void *src, size_t len);
extern void *memset(void *dst, int value, size_t num);
extern uint16_t strlen(char *src);
extern uint32_t strlen_large(char *src);

extern void *strcpy(char *dst, char *src);

#endif
