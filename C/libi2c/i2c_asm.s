.text

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data register
.set DDR, MFP_BASE + 0x5  | gpio data direction register

.set SCL, 6
.set SDA, 7

.global i2c_write_byte
.global i2c_read_byte
.global i2c_reg_read

.extern curr_slave

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
    bset #SCL, (%a1)    | DDR
.endm

.macro SDA_LO  | force SDA low
    bset #SDA, (%a1)    | DDR
.endm

.macro I2C_START 
    SDA_LO
    SCL_LO
.endm

.macro I2C_STOP
    SCL_HI
    SDA_HI
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
    
    | clear bits in GPDR, so dont need to unset them during run
    andi.b #0b00111111, (%a0)

    move.b (7,%sp), %d1
    
_i2c_read_byte:       | for internal use only 
    clr.l %d0
    SDA_IN
    
    SCL_HI
    TST_SDA
    jeq 1f    
    bset.b #7, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f   
    bset.b #6, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f  
    bset.b #5, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f   
    bset.b #4, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f    
    bset.b #3, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f    
    bset.b #2, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f    
    bset.b #1, %d0
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f   
    bset.b #0, %d0
1:
    SCL_LO

    SDA_OUT
    
    tst.b %d1
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
    
    | clear bits in GPDR, so dont need to unset them during run
    andi.b #0b00111111, (%a0)
  
    move.b (7,%sp), %d1
    
_i2c_write_byte:       | for internal use only 
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
    rts
    
rec_nak:
    moveq #0, %d0
    rts
    
/* uint8_t i2c_reg_read(uint8_t *addr, uint8_t startReg, uint8_t count) 
arguments: buffer to write to, start register, num bytes
returns: bool success
*/
i2c_reg_read: 
	movm.l %a2-%a3/%d2,-(%sp)

    move.l #GPDR, %a0
    move.l #DDR, %a1
    
	andi.b #0b00111111, (%a0)   | clear bits in GPDR so routines dont need to
    
	lea _i2c_write_byte, %a2
	
	I2C_START
	
	/* LSB 0 = write */
    move.b (curr_slave), %d2
    move.b %d2, %d1             | current slave address
    jbsr (%a2)
	jbeq _return_failed

	move.b (11+12,%sp), %d1       start reg
	jbsr (%a2)
	jbeq _return_failed
	
	I2C_STOP
	I2C_START
	
    move.b %d2, %d1             | current slave address
    ori.b #1, %d1               | set LSB = read
    jbsr (%a2)
	jbeq _return_failed
	
	move.w (14+12,%sp),%d2      | count of bytes to move
	move.l ( 4+12,%sp),%a3      | destination
	
	/* a3 = dest
	   d2 = count */
	   
	subq.w #2, %d2              | gotta send NACK on the last byte
	
    lea _i2c_read_byte, %a2
	
	moveq #0, %d1               | send ack 
	
_read_loop:
	jbsr (%a2)
    move.b %d0, (%a3)+
    
	dbra %d2, _read_loop
	
	moveq #1, %d1               | send nack
	jbsr (%a2)
    move.b %d0, (%a3)+
    
	I2C_STOP
	
	moveq #1, %d0
_r_exit:
	movm.l (%sp)+, %a2-%a3/%d2
	rts

_return_failed:
	moveq #0,%d0
	jbra _r_exit

