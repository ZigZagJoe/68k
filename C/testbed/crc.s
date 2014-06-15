.global crc_update_asm
.global crc_update_null

crc_update_asm:
	/* get arguments off stack */
	move.l 4(%sp), %d0
	move.b 11(%sp), %d1
	
	eor.b %d1, %d0
	andi.b #1, %d1
	addi.b #1, %d1
	rol.l %d1, %d0
	
	rts


/*
crc_update_null:
	rts

crc_update_asm:
	move.l 4(%sp), %d0
	move.b 11(%sp), %d1
	eor.b %d1, %d0
	
	btst.b #0, %d1
	beq z
	
	rol.l #2, %d0
	rts
z:
	rol.l #1, %d0
	rts


crc_update_asm:
	move.l 4(%sp), %d0
	move.b 11(%sp), %d1
	eor.b %d1, %d0
	and.b #3, %d1
	
	rol.l %d1, %d0
	rts
	
	
	*/
