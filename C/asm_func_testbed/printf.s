.text
.align 2

.set IO_BASE,  0xC0000
.set MFP,      IO_BASE
.set TSR,      MFP+0x2D | transmitter status reg
.set UDR,      MFP+0x2F | uart data register

.global simp_printf 

simp_printf:
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
	beq _hex
	cmp.b #'B', %d2
	beq _bin
	cmp.b #'S', %d2
	beq _str
	cmp.b #'C', %d2
	beq _ch
	cmp.b #'U', %d2
	beq _usdec
	cmp.b #'D', %d2
	beq _dec
	
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

_usdec:
    move.l %d1, %d0
    cmp.l #655359, %d0
    bhi _dec_oor
    jsr print_dec
    bra _get_ch
    	
_dec:
    tst.l %d1
    bpl not_minus
    
    move.b #'-', %d0
    jsr putc
    not %d1
    addi.l #1, %d1
    
not_minus: 
    cmp.l #327689, %d1
    bgt _dec_oor
    
    cmp.l #-327689, %d1
    ble _dec_oor
    
    move.l %d1, %d0
    jsr print_dec
    bra _get_ch

    
_dec_oor:
    move.b #'$', %d0
    jsr putc
    bra _hex
	
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

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| print number in %d0, return character count in %d0
| %d0 max value: 655359    
print_dec:
    move.w %d1, -(%sp)     | save %d1
    clr.b %d1              | character count
    
    tst.l %d0              | check for zero
    beq rz
    
    jsr pr_dec_rec         | begin recursive print
    
dec_r:
    move.b %d1, %d0        | set up return value
    move.w (%sp)+, %d1     | restore %d1
    rts
    
rz: 
    jsr ret_zero
    bra dec_r

| recursive decimal print routine
pr_dec_rec:
    tst.l %d0
    beq pr_ret
    
    divu #10, %d0
    swap %d0
    move.w %d0, -(%sp)    | save remainder
    clr.w %d0             | clear remainder
    swap %d0              | back to normal
    jsr pr_dec_rec        | get next digit
    move.w (%sp)+, %d0    | restore remainder
ret_zero:
    addi.b #'0', %d0      | turn it into a character
    jsr putc              | print
    addi.b #1, %d1        | increment char count
pr_ret:
    rts
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
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
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||	
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
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||	
| put hex byte from D0
puthexbyte:
    movem.l %A0/%D0, -(%SP)        | save regs
    
    movea.l #_hexchars, %A0
    
    lsr #4, %D0                    | shift top 4 bits over
    and.w #0xF, %D0
    move.b (%A0, %D0.W), %D0       | look up char
    jsr putc
    
    move.w (2, %SP), %D0           | restore D0 from stack    
    and.w #0xF, %D0                | take bottom 4 bits
    move.b (%A0, %D0.W), %D0       | look up char
    jsr putc
    
    movem.l (%SP)+, %A0/%D0        | restore regs

    rts
	
putc: 
    btst #7, (TSR)                    | test buffer empty (bit 7)
    beq putc                          | Z=1 (bit = 0) ? branch
    move.b %D0, (UDR)                 | write char to buffer
    rts
    
.section .rodata 
_hexchars: .ascii "0123456789ABCDEF"
