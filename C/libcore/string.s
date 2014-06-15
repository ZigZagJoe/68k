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
	andi.b #1, %d1
	addi.b #1, %d1
	rol.l %d1, %d0
	
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
	
	cmp.l #0, %d0
	beq _endset
	
	cmp.l #5, %d0        | smaller than 5 will cause below code to die, so just
	blt _small_set       | use the simple byte copy routine
	
	btst #0, 7(%sp)      | test lowest bit of destination
	beq _aligned_set     | if lowest bit not set, we are aligned
	
	move.b %d1, (%a0)+   | copy first byte now so that we are aligned
	subi.l #1, %d0
	
_aligned_set:
	move.l %d0, %a1
		
	lsr.l #2, %d0        | number of longs to copy
	sub.w #1, %d0
	
_qlset:
	move.l %d1, (%a0)+
	dbra %d0, _qlset
		
	| copy done, check for odd bits
	
	move.w %a1, %d0      | get count lowest word
	andi.w #3, %d0       | %4 
	beq _endset          | if nonzero, there are odd bytes to copy

_small_set:	             | byte copy routine for small counts and odd bytes
	sub.w #1, %d0
	
_qbset:
	move.b %d1, (%a0)+   | byte copy
	dbra %d0, _qbset
	
_endset:
	move.l 4(%sp),%d0
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void *memcpy(void *dst, const void *src, size_t len) (return *dst)
memcpy:
	move.l 4(%sp), %a1   | dst
	move.l 8(%sp), %a0   | src
	move.l 12(%sp), %d0  | count
	move.w %d2, -(%sp)
	
	cmp.l #0, %d0        | nope
	beq _endcpy
	
	cmp.l #5, %d0		 | long routine does not work below 5 bytes, use byte copy
	blt _bytememc
	
	move.w %a1, %d1      | verify alignment between src and dest
	move.w %a0, %d2
	andi.b #1, %d1
	andi.b #1, %d2
	
	cmp.b %d1, %d2       | if the alignment differs, this /has/ to be a byte copy
	jne _bytememc
	
	cmp.b #0, %d1
	beq _aligned_cpy     | if lowest bit is not set, we are aligned
	
	move.b (%a0)+,(%a1)+ | copy first byte so that we are aligned
	subi.l #1, %d0
	
_aligned_cpy:
	move.b %d0, %d1      | save for later
	lsr.l #2, %d0        | %d0 / 4 = num longs to copy
	sub.w #1, %d0        | compensate dbra -1 requirement
	
_qcpy:
	move.l (%a0)+,(%a1)+ | do the copy!
	dbra %d0, _qcpy
	
	andi.w #3, %d1       | check for any leftover bytes
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


|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint32_t strlen(uint8_t *src)
strlen:
	move.l 4(%sp), %a1   | dst
	clr.l %d0
	
_cnt:
	cmp.b #0, (%a1)+
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
