.align   2
.text
   
.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data register
.set UDR, MFP_BASE + 0x2F | uart data register
.set IMRA, MFP_BASE + 0x13
.set TSR, MFP_BASE + 0x2D | transmitter status reg

.global _charRecISR_fast
.global _charRecISR_safe
.global _charXmtISR
.global getc
.global putc
.global putc_sync
.global serial_available

.extern soft_reset

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| fast serial ISR: read a byte from UART, save in rx_buffer
| 19% cpu at 6.144mhz and 38400 baud on 68008
| 11% cpu at 8mhz and 28800 baud on 68008
| intentional bug: it will clobber the entire buffer, if the buffer 
| contains 255 chars and another character is received. 
| solution: dont let that happen, or use the 'safe' routine.

_charRecISR_fast:
   movem.l %d0/%a0, -(%SP)
   
   move.l #rx_buffer, %A0
   clr.w %D0
   
   move.b (%A0), %D0
   move.b (UDR), (2, %A0, %D0.W)  | buffer[head] = ch
   
   addi.b #1, (%A0)               | head+1

   movem.l (%SP)+,%d0/%a0

   rte                            | return from interrupt

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| safe serial ISR: read a byte from UART, save in rx_buffer
| additionally supports soft-reset by sending 0xCF with MFP GPIO 0 low
| 29% cpu at 6.144mhz and 38400 baud on 68008
| 15% cpu at 8mhz and 28800 baud on 68008

_charRecISR_safe:
   movem.l %d0-%d2/%a0, -(%SP)
   
   | check char first, so we can reset even if program is not
   | fetching serial characters and buffer has become full
   
   move.b (UDR), %D2            | get character
   
   cmp.b #0xCF, %D2             | does it == 0xCF?
   bne _not_reset
   
   btst #0, (GPDR)              | is GPIO 0 low?
   bne _not_reset
   
   jmp soft_reset               | return to bootloader
   
_not_reset:
   move.l #rx_buffer, %A0
   
   clr.w %D0
   move.b (%A0), %D0
   move.b %D0, %D1
   addi.b #1, %D1
   
   cmp.b (%A0,1), %D1           | would new head == tail?
   jeq drop_byte                | discard this byte, then

   move.b %D2, (2, %A0, %D0.W)  | buffer[head] = ch
   
   move.b %D1, (%A0)            | head = head+1

drop_byte:

   movem.l (%SP)+, %d0-%d2/%a0
   
   rte                          | return from interrupt
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| transmit a char from the tx buffer (ISR)
_charXmtISR:
   movem.l %d0/%a0, -(%SP)
   
   move.l #tx_buffer, %A0
   clr.w %D0
   
   move.b (1, %A0), %D0          | get tail
   move.b (2, %A0, %D0.W), (UDR) | UDR = buffer[tail]
   
   addi.b #1, %d0                | tail+1
   move.b %d0, (1, %A0)
       
   cmp.b (%a0), %d0
   jne _end				         | no char, exit now
   
   andi.b #~4, (IMRA)            | mask next interrupt as there is no data to send
   
_end:
   movem.l (%SP)+, %d0/%a0
   rte
   
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| enter a character into the tx buffer      
putc:
	move.l #tx_buffer, %a0
	
_dowait:
	move.w (%a0), %d0     		 | atomic read of head and tail
	move.b %d0, %d1				 | d1 = tail
	lsr.w #8, %d0                | d0 = head
	subi.b #1, %d1
	cmp.b %d0, %d1
	jne _rdy                     | if head == tail-1, buffer is full, spin while waiting for tx
	/* yield */					 | note that this is algebraically equivilent to head+1 == tail
	jra _dowait                  | but it is more efficient because we can use %d0 (head) later
	
_rdy:
	move.b 7(%sp), (2, %a0, %d0.w)| write char to buffer[head]
	addi.b #1, (%a0)             | head++
	
	ori.b #4, (IMRA)             | unmask interrupt (which will fire immediately)
	
	rts

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| synchronous putc
putc_sync:
	btst #7, (TSR)               | test buffer empty (bit 7)
    jeq putc_sync                | Z=1 (bit = 0) ? branch
	move.b (7,%sp), (UDR)		 | write char to buffer
	rts	
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| does the rx buffer have a char ready?	
serial_available:
	clr.l %d0
	move.w (rx_buffer), %d0		 | atomic read of head and tail
	move.b %d0, %d1				 | d1 = tail
	lsr.w #8, %d0                | d0 = head
	cmp.b %d0, %d1
	sne.b %d0
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| block until char available then return char
getc:
	bsr serial_available
	tst.b %d0
	beq getc
	
	clr.w %d0
	move.l #rx_buffer, %a0
	move.b (1, %a0), %d0         | get tail
	move.b (2, %a0, %d0.w), %d0  | %d0 = buffer[tail]
	addi.b #1, (1, %a0)
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| Variables

   .bss                          | ser_buffer structure
rx_buffer: .space 258            | head, tail, 256 bytes of buffer
tx_buffer: .space 258

