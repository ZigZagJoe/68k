.text
.align 2

.global qcrc_update
.global memset
.global memcpy
.global strlen
.global strcpy

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint32_t qcrc_update (uint32_t inp, uint8_t v) (return crc_new)
qcrc_update:
	/* get arguments off stack */
	move.l 4(%sp), %d0
	move.b 11(%sp), %d1
	
	eor.b %d1, %d0
	rol.l #1, %d0
	
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void * memset ( void *dst, int value, size_t num ) (return *dst)
memset:
	move.l 4(%sp), %a0   | dst
	move.b 11(%sp), %d1  | val
	
	| assemble a long in %d1 containg val repeated 4 times
	move.b %d1, %d0
	lsl.w #8, %d1
	or.b %d0, %d1
	move.w %d1, %d0
	swap %d1
	move.w %d0, %d1
	
	move.l 12(%sp), %d0  | count
	
	tst.l %d0
	beq _endset
	
	cmp.l #5, %d0        | smaller than 5 will cause below code to die, so just
	blt _small_set       | use the simple byte copy routine
	
	btst #0, 7(%sp)      | test lowest bit of destination
	beq _aligned_set     | if lowest bit not set, we are aligned
	
	move.b %d1, (%a0)+   | copy first byte now so that we are aligned
	subi.l #1, %d0
	
_aligned_set:
    cmp.l #128, %d0
    jge _super_set       | use super set if >= 128 bytes

	move.l %d0, %a1
		
	lsr.l #2, %d0        | number of longs to copy
	sub.w #1, %d0
	
_qlset:
	move.l %d1, (%a0)+
	dbra %d0, _qlset
		
	| copy done, check for odd bits
	
	move.w %a1, %d0      | get count lowest word
	andi.w #3, %d0       | %4 
	
_qlset_done:
    tst.b %d0
	beq _endset          | if nonzero, there are odd bytes to copy

_small_set:	             | byte copy routine for small counts and odd bytes
	sub.w #1, %d0
	
_qbset:
	move.b %d1, (%a0)+   | byte copy
	dbra %d0, _qbset
	
_endset:
	move.l 4(%sp),%d0
	rts
	
_super_set:
    divu #48, %d0        | figure out block count
	swap %d0
	move.w %d0, %a1      | save remainder to clear afterwards
	swap %d0
	
	sub.w #1, %d0        | compensate dbra -1 requirement
	
	movem.l %d2-%d7/%a2-%a6, -(%sp)
	
    | load registers with value
    move.l %d1, %d2
    move.l %d1, %d3
    move.l %d1, %d4
    move.l %d1, %d5
    move.l %d1, %d6
    move.l %d1, %d7
    move.l %d1, %a2
    move.l %d1, %a3
    move.l %d1, %a4
    move.l %d1, %a5
    move.l %d1, %a6
            
_sup_qset:
    movem.l %d1-%d7/%a2-%a6, (%a0)
    add #(12*4), %a0
    dbra %d0, _sup_qset
	
	movem.l (%sp)+, %d2-%d7/%a2-%a6
    
    move.l %a1, %d0
    bra _qlset_done
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void *memcpy(void *dst, const void *src, size_t len) (return *dst)
memcpy:
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
    cmp.l #128, %d0
    jge _super_cpy       | use super copy if >= 128 bytes

    | longcopy routine
    
	move.b %d0, %d1      | save for later
	andi.w #3, %d1       | check for any leftover bytes
	lsr.l #2, %d0        | %d0 / 4 = num longs to copy
	sub.w #1, %d0        | compensate dbra -1 requirement
	
_qcpy:
	move.l (%a0)+,(%a1)+ | do the copy!
	dbra %d0, _qcpy
	
_qcpy_done:
	tst.b %d1            | check for any leftover bytes
	beq _endcpy          | none! leave!

	subi.b #1, %d1       | dbra -1
	
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
	
	| super copy routine: copy 48 byte chunks
_super_cpy:
    divu #48, %d0        | figure out block count
	swap %d0
	move.w %d0, %d1      | save remainder to copy afterwards
	swap %d0
	
	sub.w #1, %d0        | compensate dbra -1 requirement
	
	movem.l %d1-%d7/%a2-%a6, -(%sp)
    
_sup_qcpy:
	movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1)
    add #(12*4), %a1
    dbra %d0, _sup_qcpy
	
	movem.l (%sp)+, %d1-%d7/%a2-%a6
    
    bra _qcpy_done


|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint32_t strlen(uint8_t *src)
strlen:
	move.l 4(%sp), %a1   | dst
	clr.l %d0
	
_cnt:
	tst.b (%a1)+
	beq _memcr
	
	add.l #1, %d0
	bra _cnt
	
_memcr:
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t *strcpy(uint8_t *dst, uint8_t *src) (returns dst)
strcpy:
	move.l 4(%sp), %a1   | dst
	move.l 8(%sp), %a0   | src
	clr.l %d0
	
_scpy:
	move.b (%a0)+, (%a1)+
	bne _scpy
	
	move.l 4(%sp), %d0
	rts
