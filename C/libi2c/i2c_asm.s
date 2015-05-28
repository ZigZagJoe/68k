|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| SUPERFAST BIT-BANG I2C FOR 68K
| I hope you like macros!

.text
.align 2

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| export/input

.global i2c_write_byte
.global i2c_read_byte
.global i2c_reg_read

.extern curr_slave

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| constants

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data register
.set DDR, MFP_BASE + 0x5  | gpio data direction register

.set SCL, 6
.set SDA, 7

| we keep all commonly used values in registers for massive speed ups
.macro SETUP_REGS
    moveq #SCL, %d2          
    moveq #SDA, %d1   
    
    move.l #GPDR, %a0
    move.l #DDR, %a1
    
    andi.b #0b00111111, (%a0)   | clear bits in GPDR so subroutines do not have to
.endm

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| line control macros
.macro SDA_IN
    bclr %d1, (%a1)         | DDR
.endm

.macro SDA_OUT
    bset %d1, (%a1)         | DDR
.endm

.macro TST_SDA
    btst %d1, (%a0)         | GPDR
.endm

| alow SCL be pulled hi
.macro SCL_HI
    bclr %d2, (%a1)         | DDR
.endm

| allow SDA be pulled hi
.macro SDA_HI  
    bclr %d1, (%a1)         | DDR
.endm

| force SCL low
.macro SCL_LO   
    bset %d2, (%a1)         | DDR
.endm

| force SDA low
.macro SDA_LO  
    bset %d1, (%a1)         | DDR
.endm

| line start state
.macro I2C_START 
    SDA_LO
    SCL_LO
.endm

| line stop state
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

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| output a byte in %d0 and jump to local label 9f on nack

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

    SDA_IN
    
    SCL_HI
    TST_SDA
    jne 9f
    SCL_LO
    
    SDA_LO
    SDA_OUT
.endm

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| read a byte into %d0
| must be followed by SEND_ACK or SEND_NACK

.macro READ_BYTE
    clr.b %d0
        
    SDA_IN
    
    SCL_HI
    TST_SDA
    jeq 1f 
    bset %d1, %d0           | contains SDA (7)
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f   
    bset %d2, %d0           | contains SCL (6)
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f  
    ori.b #0b00100000, %d0   
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f   
    ori.b #0b00010000, %d0   
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f    
    ori.b #0b00001000, %d0   
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f    
    ori.b #0b00000100, %d0   
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f    
    ori.b #0b00000010, %d0   
1:
    SCL_LO
    SCL_HI
    TST_SDA
    jeq 1f   
    ori.b #0b00000001, %d0   
1:
    SCL_LO

    SDA_OUT
.endm

.macro SEND_ACK
    SDA_LO
    SCL_HI
    SCL_LO
    SDA_HI
.endm

.macro SEND_NACK    
    SDA_HI
    SCL_HI
    SCL_LO
.endm

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_read_byte(uint8_t nack)
| argument: send ack/nack
| returns: byte read

i2c_read_byte:
    move.l %d2, -(%sp)
    
    SETUP_REGS
    
    READ_BYTE
  
    tst.b (11,%sp)              | send ACK or NACK?
    jne 2f
    
    SEND_ACK
    jbra 1f
    
2:
    SEND_NACK
1:

    move.l (%sp)+, %d2
    rts
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_write_byte(uint8_t byte)
| argument: byte to send
| returns bool success (ack)/fail (nack)

i2c_write_byte:
    move.l %d2, -(%sp)
    
    SETUP_REGS
  
    move.b (11,%sp), %d0
  
    WRITE_BYTE
    
    moveq #1, %d0
    move.l (%sp)+, %d2
    rts
    
9:                              | nack, xmit failed
    moveq #0, %d0
    move.l (%sp)+, %d2
    rts
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_reg_read(uint8_t *addr, uint8_t startReg, uint8_t count) 
| arguments: buffer to write to, start reg byte, num bytes
| returns: bool success

i2c_reg_read: 
	movm.l %a2/%d2-%d3,-(%sp)

	SETUP_REGS
	
	I2C_START
	
	/* LSB 0 = write */
    move.b (curr_slave), %d3
    move.b %d3, %d0             | current slave address
    WRITE_BYTE

	move.b (11+12,%sp), %d0     | start reg
	WRITE_BYTE
	
	I2C_STOP
	I2C_START
	
    move.b %d3, %d0             | current slave address
    ori.b #1, %d0               | set LSB = read
    WRITE_BYTE
	
	move.w (14+12,%sp),%d3      | count of bytes to move
	move.l ( 4+12,%sp),%a2      | destination

	subq.w #2, %d3              | must send NACK on the last byte

_read_loop:                     | send count-1 bytes with acks
	READ_BYTE                   | read byte with ack
	SEND_ACK
    move.b %d0, (%a2)+          | write byte to buffer
    
	dbra %d3, _read_loop
	
	READ_BYTE                   | read final byte with nack
	SEND_NACK
    move.b %d0, (%a2)+          | write byte to buffer
    
	I2C_STOP
	
	moveq #1, %d0
	
_r_exit:
	movm.l (%sp)+, %a2/%d2-%d3
	rts

9:                              | this is hit if the write byte instructions rec a NACK
	moveq #0,%d0                | return failure
	jbra _r_exit

