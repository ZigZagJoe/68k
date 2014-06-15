.align   2
.text
   
.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data register
.set UDR, MFP_BASE + 0x2F | uart data register
.set IMRA, MFP_BASE + 0x13

.set TIL311, 0xC8000

.global wait_for_txbuffer
.global _charRecISR_fast
.global _charRecISR_safe
.global _charXmtISR
.global rx_buffer
.global tx_buffer

| fast routine. no checks, absolute least amount of time spent in ISR
| will use approximately 19% of cpu time at max throughput
| given clock of 6.144mhz and 38400 baud on 68008

| intentional bug: this will clobber the entire buffer, if the buffer 
| contains 255 chars and another character is received. 
| solution: dont let that happen, or use the 'safe' routine.

_charRecISR_fast:
   move.w %D0, -(%SP)
   move.l %A0, -(%SP)
   
   move.l #rx_buffer, %A0
   clr.w %D0
   
   move.b (%A0), %D0
   move.b (UDR), (2, %A0, %D0.W) | buffer[head] = ch
   
   addi.b #1, (%A0)    | head+1

   move.l (%SP)+,%A0
   move.w (%SP)+,%D0

   rte                 | return from interrupt

   
| safe routine: if buffer is full, only new bytes will be discarded
| additionally supports soft-reset by sending 0xCF with MFP GPIO 0 low
| will use approximately 29% cpu time given above same considerations

_charRecISR_safe:
   move.w %D2, -(%SP)
   move.w %D1, -(%SP)
   move.w %D0, -(%SP)
   move.l %A0, -(%SP)
   
   | check char first, so we can reset even if program is not
   | fetching serial characters and buffer has become full
   
   move.b (UDR), %D2    | get character
   
   cmp.b #0xCF, %D2     | does it == 0xCF?
   bne noreb
   
   btst #0, (GPDR)      | is GPIO 0 low?
   bne noreb
   
   jmp 0x80008          | return to bootloader
   
noreb:
   move.l #rx_buffer, %A0
   
   clr.w %D0
   move.b (%A0), %D0
   move.b %D0, %D1
   addi.b #1, %D1
   
   cmp.b (%A0,1), %D1   | would new head == tail?
   jeq drop_byte        | discard this byte, then

   move.b %D2, (2, %A0, %D0.W) | buffer[head] = ch
   
   move.b %D1, (%A0)    | head = head+1

drop_byte:

   move.l (%SP)+,%A0
   move.w (%SP)+,%D0
   move.w (%SP)+,%D1
   move.w (%SP)+,%D2
   
   rte                   | return from interrupt
   

_charXmtISR:
   move.w %D0, -(%SP)
   move.l %A0, -(%SP)
   
   move.b #0xEE, (TIL311)
    
   move.l #tx_buffer, %A0
   clr.w %D0
   
   move.b (1, %A0), %D0   | get tail
   
   cmp.b (%a0), %d0
   beq _end				  | no char, exit now
   
   move.b (2, %A0, %D0.W), (UDR) | UDR = buffer[tail]
   
   addi.b #1, %d0    | tail+1
   move.b %d0, (1, %A0)

_end:
   move.b #0xDD, (TIL311)
    
   move.l (%SP)+,%A0
   move.w (%SP)+,%D0
   rte
   
 /*   char ch = rx_buffer.buffer[tx_buffer.tail];
    rec_buffer.tail = tx_buffer.tail + 1;
    return ch;*/
    
wait_for_txbuffer:
	move.l #tx_buffer, %a0

_dowait:
	move.w (%a0), %d0     		| atomic read of both head and tail
	move.b %d0, %d1				| d1 = tail
	lsr.w #8, %d0               | d0 = head
	add.b #1, %d0
	cmp.b %d0, %d1
	beq _dowait
	
	rts
      
/* ##### EQUIVALENT C #####
// uses about 40% cpu

ISR(ser_char_rec) {
    uint8_t c = UDR;
    
#ifndef NO_SOFT_RESET
    if ((c == 0xCF) && ((GPDR&1) == 0)) 
    	_JMP(0x80008);
#endif
    
    store_ch(c, &rec_buffer);
}

void store_ch(uint8_t c, ser_buffer *buff) {
    uint8_t i = (buff->head + 1) % SER_BUFFER_SZ;
    if (i != buff->tail) {
        buff->buffer[buff->head] = c;
        buff->head = i;
    }
}  */

   .bss                  | ser_buffer structure
rx_buffer: .space 258   | head, tail, 256 bytes of buffer
tx_buffer: .space 258

