.text

.global i2c_write_byte
.global i2c_read_byte

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data register
.set DDR, MFP_BASE + 0x5  | gpio data direction register

.set SCL, 6
.set SDA, 7

.macro SDA_IN
    bclr #SDA, (%a1)    | DDR
.endm

.macro SDA_OUT
    bset #SDA, (%a1)    | DDR
.endm

.macro TST_SDA
    btst #SDA, (%a0)    | GPDR
.endm

.macro SCL_HI  | alow SCL be pulled hi
    bclr #SCL, (%a1)    | DDR
.endm

.macro SDA_HI  | allow SDA be pulled hi
    bclr #SDA, (%a1)    | DDR
.endm

.macro SCL_LO  | force SCL low
    bclr #SCL, (%a0)    | GPDR
    bset #SCL, (%a1)    | DDR
.endm

.macro SDA_LO  | force SDA low
    bclr #SDA, (%a0)    | GPDR
    bset #SDA, (%a1)    | DDR
.endm

| set SDA according to carry bit
.macro SDA_OUTCC
    jcc 1f
    SDA_HI
    jbra 2f
1:
    SDA_LO
2:

    SCL_HI
    SCL_LO
.endm

/*
uint8_t i2c_read_byte(uint8_t nack)
argument: send ack/nack
returns byte read
*/

i2c_read_byte:
    move.l #GPDR, %a0
    move.l #DDR, %a1
    
    clr.l %d0
    SDA_IN
    
    SCL_HI
    TST_SDA
    beq 1f    
    bset.b #7, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f   
    bset.b #6, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f  
    bset.b #5, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f   
    bset.b #4, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f    
    bset.b #3, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f    
    bset.b #2, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f    
    bset.b #1, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    beq 1f   
    bset.b #0, %d0
1:
    SCL_LO

    SDA_OUT
    
    tst.b (7,%sp)
    jne _nack
    
    SDA_LO
    SCL_HI
    SCL_LO
    SDA_HI
    jbra end
    
_nack:
   
    SDA_HI
    SCL_HI
    SCL_LO
    
end:
    rts
    
/* uint8_t i2c_write_byte(uint8_t byte);
argument: byte to send
returns bool success (ack)/fail (nack)
*/

i2c_write_byte:
    move.l #GPDR, %a0
    move.l #DDR, %a1
    
    moveq #0, %d0
    
    move.b (7,%sp), %d1
    
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC
    lsl.b #1, %d1
    SDA_OUTCC

    SDA_IN
    
    SCL_HI
    TST_SDA
    jne rec_nak
    SCL_LO
    
    SDA_LO
    SDA_OUT
    moveq #1, %d0
    
rec_nak:
    rts
    
