.text
.align 2

.extern putc

.global do_test1
.global do_test2

| void *memcpy(void *dst, const void *src, size_t len) (return *dst)
.extern memcpy
.extern millis_counter

do_test1:
    movem.l %d2-%d7/%a2-%a6, -(%sp)

    | copy 192 kb
    move.l #131072, -(%sp)
    move.l #0, -(%sp)
    move.l #262144, -(%sp)
    move.l (millis_counter), %d2
    
    jsr memcpy
    jsr memcpy
    jsr memcpy
    jsr memcpy
    jsr memcpy
    jsr memcpy
    jsr memcpy
    jsr memcpy          
    
    move.l (millis_counter), %d3
    
    add.l #12, %sp
    
    sub.l %d2, %d3
    move.l %d3, %d0
    
    movem.l (%sp)+, %d2-%d7/%a2-%a6
    rts
    
do_test2:
    movem.l %d2-%d7/%a2-%a6, -(%sp)

    | copy 192 kb
    move.l #131072, -(%sp)
    move.l #0, -(%sp)
    move.l #262144, -(%sp)
    move.l (millis_counter), %d2
    
    jsr memcpy2
    jsr memcpy2
    jsr memcpy2
    jsr memcpy2
    jsr memcpy2
    jsr memcpy2
    jsr memcpy2
    jsr memcpy2         
    
    move.l (millis_counter), %d3
    
    add.l #12, %sp
    
    sub.l %d2, %d3
    move.l %d3, %d0
    
    movem.l (%sp)+, %d2-%d7/%a2-%a6
    rts
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void *memcpy(void *dst, const void *src, size_t len) (return *dst)
memcpy2:
	move.l 4(%sp), %a1   | dst
	move.l 8(%sp), %a0   | src
	move.l 12(%sp), %d0  | count
	move.w %d2, -(%sp)
	
	tst.l %d0            | nope
	beq _endcpy
	
	cmp.l #5, %d0		 | long routine does not work below 5 bytes, use byte copy
	blt _bytememc
	
	move.w %a1, %d1      | verify alignment between src and dest
	move.w %a0, %d2
	andi.b #1, %d1
	andi.b #1, %d2
	
	cmp.b %d1, %d2       | if the alignment differs, this /has/ to be a byte copy
	jne _bytememc
	
	tst.b %d1
	beq _aligned_cpy     | if lowest bit is not set, we are aligned
	
	move.b (%a0)+,(%a1)+ | copy first byte so that we are aligned
	subi.l #1, %d0
	
_aligned_cpy:
	move.b %d0, %d1      | save for later
	lsr.l #5, %d0        | %d0 / 4 = num longs to copy
	sub.w #1, %d0        | compensate dbra -1 requirement
	
	movem.l %d1-%d7/%a2-%a6, -(%sp)
    
_qcpy:
	movem.l (%a0)+, %d1-%d7/%a2
    movem.l %d1-%d7/%a2, (%a1)
    add #(8*4), %a1
    dbra %d0, _qcpy
	
	movem.l (%sp)+, %d1-%d7/%a2-%a6
    
    
	andi.w #31, %d1       | check for any leftover bytes
	beq _endcpy

	subi.w #1, %d1       | dbra -1
	
_oddcpy:	
	move.b (%a0)+,(%a1)+ | byte copy
	dbra %d1, _oddcpy
	
_endcpy:
	move.w (%sp)+, %d2
	move.l 4(%sp),%d0
	rts
	
	| byte copy routine for unaligned copy and small copy
	| seperate because it may be neccisary to copy in excess of 64k bytes unaligned
_bytememc:
	sub.l #1, %d0

_lbcpy:
	move.b (%a0)+, (%a1)+
	subi.l #1, %d0
	bpl _lbcpy
	
	bra _endcpy



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
