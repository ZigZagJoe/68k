.text
.align 2

.extern putc


| put hex long from D0
puthexlong:
	move.l %D0, -(%SP)

	swap %D0
	jsr puthexword
	
	move.l (%SP), %D0
	and.l #0xFFFF, %D0
	jsr puthexword

	move.l (%SP)+, %D0
	rts
	
| put hax word from D0	
puthexword:
	move.w %D0, -(%SP)

	lsr.w #8, %D0
	jsr puthexbyte
	
	move.w (%SP), %D0
	and.w #0xFF, %D0
	jsr puthexbyte

	move.w (%SP)+, %D0
	rts
	
| put hex byte from D0
puthexbyte:
	move.l %A0, -(%SP)
	move.w %D0, -(%SP)
	
	movea.l #_hexchars, %A0
	
	lsr #4, %D0				    | shift top 4 bits over
	and.w #0xF, %D0
	move.b (%A0, %D0.W), %D0    | look up char
	
	jsr _putc
	
	move.w (%SP), %D0			
	and.w #0xF, %D0			    | take bottom 4 bits
	move.b (%A0, %D0.W), %D0	| look up char
	
	jsr _putc
	
	move.w (%SP)+, %D0		    | restore byte
	move.l (%SP)+, %A0		    | restore address
	
	rts
	
| convert to asm abi: dont clobber registers.
_putc:
	move.l %A1, -(%SP)
	move.l %D1, -(%SP)
	move.l %A0, -(%SP)
	move.l %D0, -(%SP)
	jsr putc
	move.l (%SP)+, %D0
	move.l (%SP)+, %A0
	move.l (%SP)+, %D1
	move.l (%SP)+, %A1
	rts

.data 

_hexchars: .ascii "0123456789ABCDEF"
