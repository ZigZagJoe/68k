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


.macro SDA_INf
    bclr %d1, (%a1)    | DDR
.endm

.macro SDA_OUTf
    bset %d1, (%a1)    | DDR
.endm

.macro TST_SDAf
    btst %d1, (%a0)    | GPDR
.endm


.macro SCL_HI  | alow SCL be pulled hi
    bclr #SCL, (%a1)    | DDR
.endm

.macro SCL_HIf  | alow SCL be pulled hi
    bclr %d1, (%a1)    | DDR
.endm


.macro SDA_HI  | allow SDA be pulled hi
    bclr #SDA, (%a1)    | DDR
.endm

.macro SDA_HIf  | allow SDA be pulled hi
    bclr %d1, (%a1)    | DDR
.endm


.macro SCL_LO  | force SCL low
    bset #SCL, (%a1)    | DDR
.endm

.macro SCL_LOf   | force SCL low
    bset %d1, (%a1)    | DDR
.endm


.macro SDA_LO   | force SDA low
    bset #SDA, (%a1)    | DDR
.endm

.macro SDA_LOf  | force SDA low
    bset %d1, (%a1)    | DDR
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
    SDA_HIf
    jbra 2f
1:
    SDA_LOf
2:

    SCL_HI
    SCL_LO
.endm

.macro WRITE_BYTE
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC
    lsl.b #1, %d0
    SDA_OUTCC

    SDA_INf
    
    SCL_HI
    TST_SDAf
    jne 9f
    SCL_LO
    
    SDA_LOf
    SDA_OUTf
.endm

.macro READ_BYTE
    clr.b %d0
        
    SDA_IN
    
    SCL_HIf
    TST_SDA
    jeq 1f 
    ori.b #0b10000000, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f   
    ori.b #0b01000000, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f  
    ori.b #0b00100000, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f   
    ori.b #0b00010000, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f    
    ori.b #0b00001000, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f    
    ori.b #0b00000100, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f    
    ori.b #0b00000010, %d0   
1:
    SCL_LOf
    SCL_HIf
    TST_SDA
    jeq 1f   
    ori.b #0b00000001, %d0   
1:
    SCL_LOf

    SDA_OUT
.endm

.macro SEND_ACK
    SDA_LO
    SCL_HIf
    SCL_LOf
    SDA_HI
.endm

.macro SEND_NACK    
    SDA_HI
    SCL_HIf
    SCL_LOf
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

    moveq #SCL, %d1   | keep SCL in %d1 to speed up clock cycles
    
    READ_BYTE
  
    tst.b (7,%sp)     | send ACK or NACK?
    jne 2f
    
    SEND_ACK
    jbra 1f
    
2:
    SEND_NACK
1:
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
  
    move.b (7,%sp), %d0
    
    moveq #SDA, %d1    | keep SDA in %d1 to speed up data bits
    WRITE_BYTE
    
    moveq #1, %d0
    rts
    
9:                     | nack, xmit failed
    moveq #0, %d0
    rts
    
/* uint8_t i2c_reg_read(uint8_t *addr, uint8_t startReg, uint8_t count) 
arguments: buffer to write to, start register, num bytes
returns: bool success
*/
i2c_reg_read: 
	movm.l %a2/%d2,-(%sp)

    move.l #GPDR, %a0
    move.l #DDR, %a1
    
	andi.b #0b00111111, (%a0)   | clear bits in GPDR so subroutines do not have to
    moveq #SDA, %d1             | keep SDA in %d1 to speed up bits out for writes
    
	I2C_START
	
	/* LSB 0 = write */
    move.b (curr_slave), %d2
    move.b %d2, %d0             | current slave address
    WRITE_BYTE

	move.b (11+8,%sp), %d0      | start reg
	WRITE_BYTE
	
	I2C_STOP
	I2C_START
	
    move.b %d2, %d0             | current slave address
    ori.b #1, %d0               | set LSB = read
    WRITE_BYTE
	
	move.w (14+8,%sp),%d2       | count of bytes to move
	move.l ( 4+8,%sp),%a2       | destination
	
	/* a3 = dest
	   d2 = count */
	   
	subq.w #2, %d2              | must send NACK on the last byte
	moveq #SCL, %d1             | keep SCL in %d1 to speed up clock cycles
    
_read_loop:                     | send count-1 bytes with acks
	READ_BYTE                   | send byte with ack
	SEND_ACK
    move.b %d0, (%a2)+
    
	dbra %d2, _read_loop
	
	READ_BYTE                   | send final byte with nack
	SEND_NACK
    move.b %d0, (%a2)+
    
	I2C_STOP
	
	moveq #1, %d0
_r_exit:
	movm.l (%sp)+, %a2/%d2
	rts

9:
_return_failed:
	moveq #0,%d0
	jbra _r_exit

