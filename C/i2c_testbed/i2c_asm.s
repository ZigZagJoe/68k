
.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data register
.set DDR, MFP_BASE + 0x5 | gpio data direction

.set SCL, 6
.set SDL, 7

.global toggle_scl

/*
if(GPDR & (1<<SCL)) {
        bset_a(DDR,SCL); //output 
        bclr_a(GPDR,SCL);
    } else {
        bclr_a(DDR, SCL); //tristate it
        //check clock stretching
        while(!READ_SCL());
    }	


toggle_scl:
	
	move.l #GPDR, %a1
    move.l #DDR, %a0
	
	btst #SCL, (%a1) 
	jbeq _scl_zero
	
	bset.b #SCL, (%a0)
	bclr.b #SCL, (%a1)

	jbra _scl_toggle_end
	
_scl_zero:
	bclr.b #SCL, (%a0)

.L25:
	btst #SCL,(%a1)
	jbeq .L25


_scl_toggle_end:

	rts
*/
