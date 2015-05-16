.text
.align 2

.extern putc

.set IO_BASE,  0xC0000
.set TIL311,   IO_BASE + 0x8000

.global asm_test
asm_test:
    move.l #0x40000, %a0
    
/*l:
    move.b (%a0)+, (TIL311)
    cmp.l #0x40100, %a0
    jne l*/
    
    move.l #TIL311, %a1
    
    movem.l (%a0)+, %d1-%d7/%a2-%a6   | 24 + 12 * 16
    movem.l %d1-%d7/%a2-%a6, (%a1)    | 16 + 12 * 16
    
	movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1) 
    
	movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1)
    
	movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1)
    
	movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1)
    
    movem.l (%a0)+, %d1-%d3           | 24 + 3 * 16
    movem.l %d1-%d3, (%a1)            | 16 + 3 * 16
    
    jbra asm_test


.global _strlen
.global _nulltest

#uint16_t _strlen(char* str);
   
   
_strlen:
    move.l 4(%sp), %a1   | dst
	move.w #-1, %d0
	
_cnt:
	tst.b (%a1)+
	dbeq %d0, _cnt
	
	addq.w #1, %d0
	neg.w %d0

    rts
    
_nulltest:
     move.l 4(%sp), %d0   | dst
	clr.l %d0
	
    rts
    
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||
|||||||||||||||||||||||||||||||||||||||||||||||||||||
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
