#include <stdlib.h>
#include <stdio.h>

#include <io.h>
#include <interrupts.h>

void store_ch(uint8_t c, ser_buffer *buff);

ser_buffer rec_buffer;

ISR(rec_err)      { HALT_CODE(0xEE,0x89); }

ISR(char_rec) {
    char c = UDR;
    store_ch(c, &rec_buffer);
}

void store_ch(uint8_t c, ser_buffer *buff) {
    uint8_t i = (buff->head + 1) % SER_BUFFER_SZ;
    if (i != buff->tail) {
        buff->buffer[buff->head] = c;
        buff->head = i;
    }
}

int16_t getc() {
    if (rec_buffer.head == rec_buffer.tail) return -1;
    char ch = rec_buffer.buffer[rec_buffer.tail];
    rec_buffer.tail = (rec_buffer.tail + 1) % SER_BUFFER_SZ;
    return ch;
}

void enable_serial() {
    __vectors.user[MFP_INT + MFP_CHAR_RDY - user_isr_start] = &char_rec;
    __vectors.user[MFP_INT + MFP_REC_ERR - user_isr_start] = &rec_err;
    
    VR = MFP_INT;
    IERA |= INT_REC_FULL | INT_REC_ERR;
    IMRA |= INT_REC_FULL | INT_REC_ERR;
    // set interrupt mask to 0
    sei();
}

void putc(char ch) {
    /*__asm volatile(
                   "moveq #6,%%d0\n"
                   "move.b %0,%%d1\n"
                   "trap #15\n"
                   :
                   : "m"(ch)
                   : "d0","d1"
                   );*/
    while (!(TSR & 0x80));
    UDR = ch; 
}

void puts(char *str) {
    while (*str != 0)
        putc(*str++);
}
