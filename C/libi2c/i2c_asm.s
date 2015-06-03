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
.global i2c_init

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
    jcc 1f         | 18 / 12
    SDA_HI         | 16
    jbra 2f        | 18
1:
    SDA_LO         | 16
2:

    SCL_HI         | 16
    SCL_LO         | 16
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
| uint8_t i2c_reg_read(uint8_t *addr, uint8_t startReg, uint16_t count) 
| arguments: buffer to write to, start reg byte, num bytes
| returns: bool success

i2c_reg_read: 
    movm.l %a2/%d2-%d3,-(%sp)

    SETUP_REGS
    
    I2C_START
    
    move.b (curr_slave), %d3 
    move.b %d3, %d0             | current slave address, LSB is 0 - write
    WRITE_BYTE

    move.b (11+12,%sp), %d0     | write start register
    WRITE_BYTE
    
    I2C_STOP
    I2C_START
    
    move.b %d3, %d0             | current slave address
    ori.b #1, %d0               | set LSB 1 = read
    WRITE_BYTE
 
    move.w (14+12,%sp),%d3      | count of bytes to move
    move.l ( 4+12,%sp),%a2      | destination

    jbra _enter_rl              | we share the read loop with i2c_bulk_read
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_bulk_read(uint8_t *addr, uint16_t count) 
| arguments: buffer to write to, num bytes
| returns: bool success

i2c_bulk_read: 
    movm.l %a2/%d2-%d3,-(%sp)

    SETUP_REGS
    
    I2C_START
    
    move.b (curr_slave), %d0    | current slave address
    ori.b #1, %d0               | set LSB 1 = read
    WRITE_BYTE
    
    move.w (10+12,%sp),%d3      | count of bytes to move
    move.l ( 4+12,%sp),%a2      | destination
    
_enter_rl:
    subq.w #2, %d3              | must send NACK on the last byte

_read_loop:                     | read count-1 bytes with acks
    READ_BYTE                   | read byte
    SEND_ACK                    | send ack
    move.b %d0, (%a2)+          | write byte to buffer
    
    dbra %d3, _read_loop
    
    READ_BYTE                   | read final byte
    SEND_NACK                   | send nack
    move.b %d0, (%a2)+          | write byte to buffer
    
    I2C_STOP
    
    moveq #1, %d0               | return success
       
_regr_exit:
    movm.l (%sp)+, %a2/%d2-%d3
    rts
    
9:                              | this is hit if the write byte macros get a NACK
    I2C_STOP
    moveq #0,%d0                | return failure 
    jbra _regr_exit
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_bulk_write(uint8_t *addr, uint16_t count) 
| arguments: buffer to read from, num bytes
| returns: bool success

i2c_bulk_write: 
    movm.l %a2/%d2-%d3,-(%sp)

    SETUP_REGS
    
    I2C_START
    
    move.b (curr_slave), %d0    | current slave address    
    WRITE_BYTE                  | LSB 0 = write
    
    move.w (10+12,%sp),%d3      | count of bytes to send
    move.l ( 4+12,%sp),%a2      | destination
    
    subq.w #1, %d3

_write_loop:                    | read count-1 bytes with acks
    move.b (%a2)+, %d0          | read byte from buffer
    WRITE_BYTE                  | write it

    dbra %d3, _write_loop
    
    I2C_STOP
    
    moveq #1, %d0               | return success
       
_w_exit:
    movm.l (%sp)+, %a2/%d2-%d3
    rts
    
9:                              | this is hit if the write byte macros get a NACK
    I2C_STOP
    moveq #0, %d0               | return failure
    jbra _w_exit
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void i2c_init() 
| sets i2c lines high. honestly, it may as well be optional

i2c_init:
    andi.b #0b00111111, (GPDR) 
    andi.b #0b00111111, (DDR)   
    move.b #0, (curr_slave)
    rts
    
