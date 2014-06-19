#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdint.h>

void puts(char *str);
void putc(char ch);
void enable_serial();

#define SER_BUFFER_SZ 128

typedef struct {
    uint8_t buffer[SER_BUFFER_SZ];
    volatile uint8_t head;
    volatile uint8_t tail;
} ser_buffer;

#define INT_XMIT_ERR    (1<<1)
#define INT_XMIT_EMPTY  (1<<2)
#define INT_REC_ERR     (1<<3)
#define INT_REC_FULL    (1<<4)

#endif
