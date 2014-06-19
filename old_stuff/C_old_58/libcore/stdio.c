#include <stdlib.h>
#include <stdio.h>

#include <io.h>
#include <interrupts.h>

void store_ch(uint8_t c, ser_buffer *buff);

ser_buffer rec_buffer;

ISR(ser_rec_err)  { HALT_CODE(0xEE,0x89); }

ISR(ser_char_rec) {
    uint8_t c = UDR;
    
#ifndef NO_SOFT_RESET
    if ((c == 0xCF) && ((GPDR&1) == 0)) 
    	__asm volatile("jmp 0x80008\n");
#endif
    
    store_ch(c, &rec_buffer);
}

void store_ch(uint8_t c, ser_buffer *buff) {
    uint8_t i = (buff->head + 1) % SER_BUFFER_SZ;
    if (i != buff->tail) {
        buff->buffer[buff->head] = c;
        buff->head = i;
    }
}

void serial_start() {
    __vectors.user[MFP_INT + MFP_CHAR_RDY - USER_ISR_START] = &ser_char_rec;
    __vectors.user[MFP_INT + MFP_REC_ERR - USER_ISR_START] = &ser_rec_err;
    
    VR = MFP_INT;
    IERA |= INT_REC_FULL | INT_REC_ERR;
    IMRA |= INT_REC_FULL | INT_REC_ERR;
    // set interrupt mask to 0
    sei();
}

void serial_stop() {
	IERA &= ~(INT_REC_FULL | INT_REC_ERR);
    IMRA &= ~(INT_REC_FULL | INT_REC_ERR);
}

bool serial_available() {
	return rec_buffer.head != rec_buffer.tail;
}

int16_t getc() {
    if (rec_buffer.head == rec_buffer.tail) return -1;
    char ch = rec_buffer.buffer[rec_buffer.tail];
    rec_buffer.tail = (rec_buffer.tail + 1) % SER_BUFFER_SZ;
    return ch;
}

void putc(char ch) {
    while (!(TSR & 0x80));
    UDR = ch; 
}

void puts(char *str) {
    while (*str != 0)
        putc(*str++);
}


