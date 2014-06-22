.text
.align 2

.extern putc

.global printf 

printf:
	move.l %a2,-(%sp)
	move.l %a3,-(%sp)
	
	move.l 12(%sp), %a2
	lea 16(%sp), %a3
		
_get_ch:
	clr.l %d0
	move.b (%a2)+, %d0
	beq end
	
	cmp.b #'%', %d0
	jeq _perc
	
_put_tch:               | just a normal char
	bsr putc
	
	bra _get_ch
	
end:
	move.l (%sp)+, %a3
	move.l (%sp)+, %a2
	
	rts

| percent was encountered
_perc:
	move.b (%a2)+, %d2
	cmp.b #'%', %d2      | next char is %? this was an escape
	jeq _put_tch
	
	move.l (%a3)+, %d1   | asocciated argument
	
	andi.b #0xDF, %d2    | convert to uppercase
	
	cmp.b #'X', %d2
	jeq _hex
	cmp.b #'B', %d2
	jeq _bin
	cmp.b #'S', %d2
	jeq _str
	cmp.b #'C', %d2
	jeq _ch
	
	move.b #'%', %d0     | not a recognized escape
	sub.l #1, %a2
	bra _put_tch

| hex case
_hex:
	move.l %d1, %d0
	jsr puthexlong
	bra _get_ch
	
| hex case
_ch:
	move.b %d1, %d0
	jsr putc
	bra _get_ch
	
| binary case
_bin:
	move.l #32, %d2
	
put_bin:
	move.b #'0', %D0
	btst %D2, %D1
	jeq not1
	move.b #'1', %D0
not1:
	jsr putc
	
	dbra %D2, put_bin
	
	bra _get_ch

| string case
_str:
	move.l %d1, %a0
_rdc:
	clr.l %d0
	move.b (%a0)+, %d0
	beq _str_end
	bsr putc
	bra _rdc
	
_str_end:
	bra _get_ch

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
	
	jsr putc
	
	move.w (%SP), %D0			
	and.w #0xF, %D0			    | take bottom 4 bits
	move.b (%A0, %D0.W), %D0	| look up char
	
	jsr putc
	
	move.w (%SP)+, %D0		    | restore byte
	move.l (%SP)+, %A0		    | restore address
	
	rts
	
.data 
_hexchars: .ascii "0123456789ABCDEF"
