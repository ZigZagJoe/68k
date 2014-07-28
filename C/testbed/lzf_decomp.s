.text
.align 2
.global lzf_decompress

lzf_decompress:
| init %fp & save regs
	link.w %fp, #0
	movem.l %d2-%d3/%a2-%a4, -(%sp)
	
| arguments
	move.l 16(%fp), %d2
	move.l 8(%fp), %a1          | a1 = in_ptr
	move.l %d2, %a3             | a3 = out_ptr
	move.l %a1, %a4       
	add.l 12(%fp), %a4          | a4 = in_end
	
loop:
    moveq #0, %d0
	move.b (%a1)+, %d0          | read ctrl byte
	
	cmp.b #31, %d0
	jbhi backref                | if any of b7-b5 is set, then backref
	
	| copy (ctrl+1) literal bytes
literal_cpy:
	move.b (%a1)+, (%a3)+
	dbra %d0, literal_cpy
	
	jbra loop_chk
	
backref:
	move.w %d0, %d1
	lsr.b #5, %d1
	cmp.b #7, %d1               | %d1 = len 
	jbne not_long_fmt 
	
	| if upper 3 bytes are all 1, then read another byte of length
	
	move.b (%a1)+, %d1          | %d1 = (*in_ptr++)
	addq.w #7, %d1
	
not_long_fmt:
	lsl.b #3, %d0               | %d0 = -------- AAAAA000
	lsl.w #5, %d0               | %d0 = 000AAAAA 00000000
	move.b (%a1)+, %d0          | %d0 = 000AAAAA BBBBBBBB
	
	lea (-1, %a3), %a2          | %a2 = out_ptr - 1
	
	sub.w %d0, %a2              | %a2 -= backref_dist (%d0)
	                            | %a2 is now valid backref address
	
	addq.w #1, %d1              | len++
	
backref_copy:
	move.b (%a2)+, (%a3)+       | *out_ptr++ = *backref_ptr++
	dbra %d1, backref_copy
	
loop_chk:
	cmp.l %a1, %a4              | while (in_ptr != in_end)
	jbhi loop

| return difference in output pointer	
	move.l %a3, %d0
	sub.l %d2, %d0

| cleanup	
	movm.l (%sp)+, %d2-%d3/%a2-%a4
	unlk %fp
	rts
