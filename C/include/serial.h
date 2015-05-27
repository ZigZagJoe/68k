#ifndef __SERIAL_H__
#define __SERIAL_H__

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
// 29% cpu at 6.144mhz and 38400 baud on 68008
// 15% cpu at 8mhz and 28800 baud on 68008
#define SERIAL_SAFE 0

// Use an unsafe routine that uses less CPU time than the 
// normal routine, but it does not support soft reboot 
// and will clobber the ENITRE buffer if the buffer is full
// when another character is received. instead, the buffer 
// will then be seen as empty.
// 19% cpu at 6.144mhz and 38400 baud on 68008
// 11% cpu at 8mhz and 28800 baud on 68008
#define SERIAL_FAST 1

// nonzero if an error occurs
extern uint8_t RXERR;
extern uint8_t TXERR;

// handlers called if an error occurs
extern __serial_handler *on_rcvr_error;
extern __serial_handler *on_xmit_error;

void serial_start(uint8_t fast);
void serial_stop();

void serial_wait();

extern volatile uint8_t* serial_redirect_buff;

extern bool serial_available();
extern bool tx_busy();

extern uint8_t getc();
extern void putc(char ch);
extern void putc_sync(char ch);

#endif
