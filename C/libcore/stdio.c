#include <stdlib.h>
#include <stdio.h>

#include <io.h>
#include <interrupts.h>

extern ser_buffer rx_buffer;
extern ser_buffer tx_buffer;
extern void _charRecISR_fast(void);
extern void _charRecISR_safe(void);
extern void _charXmtISR(void);

uint8_t RXERR;
uint8_t TXERR;

__serial_handler *on_rcvr_error = 0;
__serial_handler *on_xmit_error = 0;

ISR(ser_rec_err)  { 
	RXERR = RSR; 
	if (on_rcvr_error)
		(*on_rcvr_error)(RXERR);
}

ISR(ser_xmt_err)  { 
	TXERR = TSR;
	if (on_xmit_error)
		(*on_xmit_error)(TXERR);
}

uint8_t isdigit(char ch) {
	return (ch >= '0') && (ch <= '9');
}

void serial_start(uint8_t fast) {
    __vectors.user[MFP_INT + MFP_CHAR_RDY - USER_ISR_START] =  fast ? (&_charRecISR_fast) : (&_charRecISR_safe);
    //__vectors.user[MFP_INT + MFP_XMIT_EMPTY - USER_ISR_START] =  &_charXmtISR;
    __vectors.user[MFP_INT + MFP_REC_ERR - USER_ISR_START] = &ser_rec_err;
    __vectors.user[MFP_INT + MFP_XMIT_ERR - USER_ISR_START] = &ser_xmt_err;
    
    VR = MFP_INT;
    IERA |= INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR /*| INT_XMIT_EMPTY*/;
    IMRA |= INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR;
  
    // baud rate generation
    TCDR = 1; // baud 28800
    TCDCR &= 0xF; 	 // stop C
    TCDCR |= 1 << 4; // start C with prescaler of 4  
    
    RXERR = 0;
	TXERR = 0;
	
	// set interrupt mask to 0
    // sei();
}

void serial_stop() {
	IERA &= ~(INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR | INT_XMIT_EMPTY);
    IMRA &= ~(INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR | INT_XMIT_EMPTY);
}

bool serial_available() {
	return rx_buffer.head != rx_buffer.tail;
}

int16_t getc() {
    if (rx_buffer.head == rx_buffer.tail) 
    	return -1;
    	
    return rx_buffer.buffer[rx_buffer.tail++];
}

/*
void putc(char ch) {  
    if ((TSR & 0x80) && tx_buffer.head == tx_buffer.tail) {
		IMRA |= INT_XMIT_EMPTY; 
    	UDR = ch;
		return;
	}
	
	while ((uint8_t)(tx_buffer.head + 1) == tx_buffer.tail); // block until buffer not full
	
    tx_buffer.buffer[tx_buffer.head++] = ch;
}*/

// wait for transmitter idle, then send character
void putc(char ch) {
    while (!(TSR & 0x80));
    UDR = ch; 
}

void puts(char *str) {
    while (*str != 0)
        putc(*str++);
}


