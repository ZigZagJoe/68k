.align 2
.text
.global main

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data reg
.set UDR, MFP_BASE + 0x2F | uart data register
.set TSR, MFP_BASE + 0x2D | transmitter status reg
.set RSR, MFP_BASE + 0x2B | receiver status reg
.set TIL311, 0xC8000      | til311 displays

test_funct:
    move.l (4, %sp), %d0
    jsr puthexlong
    
    move.l (8, %sp), %d0
    jsr puthexlong
    
    move.l (12, %sp), %d0
    jsr puthexlong
    
    rts


main:
    
    move.l #0xF1A6F1A6, %d5
    move.l #0x1E71E71E, %d7
    move.l #0xDEADC0DE, %a5
    
    movem.l %a5/%d7/%d5,-(%sp) 
    
    jsr test_funct
  
    

spin: bra spin


puthexlong:
	move.l %D0, -(%SP)

	swap %D0
	jsr puthexword
	
	move.l (%SP), %D0
	and.l #0xFFFF, %D0
	jsr puthexword

	move.l (%SP)+, %D0
	rts
		
puthexword:
	move.w %D0, -(%SP)

	lsr.w #8, %D0
	jsr puthexbyte
	
	move.w (%SP), %D0
	and.w #0xFF, %D0
	jsr puthexbyte

	move.w (%SP)+, %D0
	rts

puthexbyte:
	movem.l %A0/%D0, -(%SP)     | save regs
	
	movea.l #_hexchars, %A0
	
	lsr #4, %D0				    | shift top 4 bits over
	and.w #0xF, %D0
	move.b (%A0, %D0.W), %D0    | look up char
	jsr putc
	
	move.w (2, %SP), %D0		| restore D0 from stack	
	and.w #0xF, %D0			    | take bottom 4 bits
	move.b (%A0, %D0.W), %D0	| look up char
	jsr putc
	
	movem.l (%SP)+, %A0/%D0		| restore regs

	rts
	
putc:
	btst #7, (TSR)              | test buffer empty (bit 7)
    beq putc                    | Z=1 (bit = 0) ? branch
	move.b %D0, (UDR)		    | write char to buffer
	rts

.section .rodata 

_hexchars: .ascii "0123456789ABCDEF"
