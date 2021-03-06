#include <stdlib.h>
#include <stdio.h>

#include <io.h>
#include <interrupts.h>

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

void serial_wait() {
    while(tx_busy());
}

void serial_start(uint8_t fast) {
	serial_redirect_buff = 0; // disable redirect
	
	 __vectors.user[MFP_INT + MFP_CHAR_RDY - USER_ISR_START] =  fast ? (&_charRecISR_fast) : (&_charRecISR_safe);
    __vectors.user[MFP_INT + MFP_XMIT_EMPTY - USER_ISR_START] =  &_charXmtISR;
    __vectors.user[MFP_INT + MFP_REC_ERR - USER_ISR_START] = &ser_rec_err;
    __vectors.user[MFP_INT + MFP_XMIT_ERR - USER_ISR_START] = &ser_xmt_err;
    
    VR = MFP_INT;
    IERA |= INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR | INT_XMIT_EMPTY;
    IMRA |= INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR;
  
    // baud rate generation
    /*TCDR = 1;        // must be calculated against your crystal
    TCDCR &= 0xF; 	 // stop timer C
    TCDCR |= 1 << 4; // start timer C with prescaler of 4 */ 
    
    RXERR = 0;
	TXERR = 0;
	
	UDR = '\n';  	 // prime the transmitter so that the XMIT_EMPTY interrupt will be pending

	// set interrupt mask to 0
    // sei();
}

void serial_stop() {
	//IERA &= ~(INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR | INT_XMIT_EMPTY);
    IMRA &= ~(INT_REC_FULL | INT_REC_ERR | INT_XMIT_ERR | INT_XMIT_EMPTY);
}

const char btohex[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void puthexN(uint8_t x) {
    putc(btohex[x]);
}

void puthexB(uint8_t x) {
    puthexN((x >> 4) & 0xF);
    puthexN(x & 0xF);
}

void puthexW(uint16_t x) {
    puthexB((x>>8) & 0xFF);
    puthexB(x & 0xFF);
}   
 
void puthexL(uint32_t x) {
    puthexW((x>>16) & 0xFFFF);
    puthexW(x & 0xFFFF);
}   
