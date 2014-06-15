#ifndef __STDIO_H__
#define __STDIO_H__

#include <stdint.h>
#include <printf.h>

typedef void(*__serial_handler)(uint8_t code);

typedef struct __attribute__((packed)) {
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t buffer[256];
} ser_buffer;

#define INT_XMIT_ERR    (1<<1)
#define INT_XMIT_EMPTY  (1<<2)
#define INT_REC_ERR     (1<<3)
#define INT_REC_FULL    (1<<4)

// SERIAL_SAFE allows reboot from serial port, and 
// will not clobber the entire buffer if a character is
// received and the buffer contains 255 bytes already 
// instead, the new character is discarded
// will use around 30% CPU at 38400 baud (maxed out)
#define SERIAL_SAFE 0

// Use an unsafe routine that uses less CPU time than the 
// normal routine, but it does not support soft reboot 
// and will clobber the ENITRE buffer if the buffer is full
// when another character is received. instead, the buffer 
// will then be seen as empty.
#define SERIAL_FAST 1

extern uint8_t RXERR;
extern uint8_t TXERR;

extern __serial_handler *on_rcvr_error;
extern __serial_handler *on_xmit_error;

void serial_start(uint8_t fast);
void serial_stop();

bool serial_available();

void puts(char *str);
//void putc(char ch);
void putc(char ch);
int16_t getc();

uint8_t isdigit(char ch);


#endif
