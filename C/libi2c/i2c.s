|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| SUPERFAST BIT-BANG I2C FOR 68K
| I hope you like macros!

.text
.align 2

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| compile-time configuration

.set CLOCK_STRETCHING, 0    | support clock stretching? incurs 33% speed penalty 
                            | but ensures compatibility with 100khz-only devices

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| function export/input

.global i2c_init
.global i2c_stop_cond
.global i2c_start_cond
.global i2c_write_byte
.global i2c_read_byte
.global i2c_bulk_write
.global i2c_bulk_read
.global i2c_reg_read
.global i2c_reg_write

.extern curr_slave

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| constants

.set MFP_BASE, 0xC0000      | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1   | gpio data register
.set DDR, MFP_BASE + 0x5    | gpio data direction register

.set SCL, 6
.set SDA, 7

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| macros

| we keep all commonly used values in registers for massive speed ups
.macro SETUP_REGS
    moveq #SCL, %d2          
    moveq #SDA, %d1   
    
    move.l #GPDR, %a0
    move.l #DDR, %a1
    
    andi.b #0b00111111, (%a0) | clear bits in GPDR so subroutines do not have to
.endm

.macro DELAY_IF_SLOW
.if CLOCK_STRETCHING
    nop
    nop
    nop
.endif
.endm

| alow SCL be pulled hi
.macro SCL_HI
    bclr %d2, (%a1)         | DDR
  
.if CLOCK_STRETCHING  
1:                          | detect clock stretching: wait for SCL to go high 
    btst %d2, (%a0)         | GPDR
    jeq 1b
.endif

.endm

| force SCL low
.macro SCL_LO   
    bset %d2, (%a1)         | DDR
.endm

.macro TST_SDA
    btst %d1, (%a0)         | GPDR
.endm

| allow SDA be pulled hi
.macro SDA_HI  
    bclr %d1, (%a1)         | DDR
.endm

| force SDA low
.macro SDA_LO  
    bset %d1, (%a1)         | DDR
.endm

| line start state
.macro I2C_START 
    SDA_LO
    DELAY_IF_SLOW
    SCL_LO
    DELAY_IF_SLOW
.endm

| line stop state
.macro I2C_STOP
    DELAY_IF_SLOW
    SCL_HI
    DELAY_IF_SLOW
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
| output a byte in %d0 and jump forward to ON_XMIT_FAILED if nack
| in  SCL 0 SDA X
| out SCL 0 SDA 0

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

    SDA_HI                   | allow the slave to drive the bus low
    
    SCL_HI
    TST_SDA
    jne 9f                   | nack received!
    
    ori.b #0b11000000, (%a1) | set SDA and SCL both low
.endm

.macro ON_XMIT_FAILED
9:                           | this is hit if the write byte macros get a NACK
    SCL_LO
    SDA_LO
    I2C_STOP
.endm

| unrolled bit of code for READ_BYTE
.macro READBIT 
1:
    SCL_LO
    DELAY_IF_SLOW
    SCL_HI
    TST_SDA
    jeq 1f   
.endm

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| read a byte into %d0
| must be followed by SEND_ACK or SEND_NACK
| in  SCL 0 SDA X
| out SCL 0 SDA 0

.macro READ_BYTE
    clr.b %d0
    SDA_HI
    
    SCL_HI
    TST_SDA
    jeq 1f 
    bset %d1, %d0            | contains SDA (7)
    READBIT  
    bset %d2, %d0            | contains SCL (6)
    READBIT
    ori.b #0b00100000, %d0   
    READBIT
    ori.b #0b00010000, %d0   
    READBIT    
    ori.b #0b00001000, %d0   
    READBIT
    ori.b #0b00000100, %d0   
    READBIT
    ori.b #0b00000010, %d0   
    READBIT 
    ori.b #0b00000001, %d0   
1:
    SCL_LO
.endm

.macro SEND_ACK
    SDA_LO
    SCL_HI
    DELAY_IF_SLOW
    SCL_LO
.endm

.macro SEND_NACK    
    SDA_HI
    SCL_HI
    DELAY_IF_SLOW
    SCL_LO
    SDA_LO
.endm
 
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void i2c_init() 
| initializes i2c bus. Effectively optional.

i2c_init:
    jsr i2c_stop_cond
    move.b #0, (curr_slave)
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void i2c_start_cond()
| emits a stop then a start condition on the i2c bus

i2c_start_cond:
    move.l #DDR, %a1
    
    andi.b #0b00111111, (GPDR) 
    
    bclr #SCL, (%a1)            | stop condition
    bclr #SDA, (%a1)
    
    bset #SDA, (%a1)            | start condition
    bset #SCL, (%a1)
    
    rts  
 
 ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| void i2c_stop_cond()
| emits a stop condition on the i2c bus   

i2c_stop_cond:
    move.l #DDR, %a1
    
    andi.b #0b00111111, (GPDR) 
    
    bclr #SCL, (%a1)            | stop condition
    bclr #SDA, (%a1)
    
    rts  
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_read_byte(uint8_t nack)
| reads a single byte from the i2c bus with nack or ack 
| argument: send ack/nack
| returns: byte read

i2c_read_byte:
    move.l %d2, -(%sp)
    
    SETUP_REGS
             
    READ_BYTE
  
    tst.b (11, %sp)              | send ACK or NACK?
    jne 2f
    
    SEND_ACK
    jbra 1f
    
2:
    SEND_NACK
1:

    move.l (%sp)+, %d2
    rts
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| writes a single byte to the i2c bus
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
    
ON_XMIT_FAILED
    moveq #0, %d0
    move.l (%sp)+, %d2
    rts
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_reg_read(uint8_t *addr, uint8_t startReg, uint16_t count) 
| writes a register byte to bus then reads count bytes from i2c bus into addr
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
    DELAY_IF_SLOW
    DELAY_IF_SLOW
    I2C_START
    
    move.b %d3, %d0             | current slave address
    ori.b #1, %d0               | set LSB 1 = read
    WRITE_BYTE
 
    move.w (14+12,%sp),%d3      | count of bytes to move
    move.l ( 4+12,%sp),%a2      | destination

    jbra _enter_rl              | we share the read loop with i2c_bulk_read
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_bulk_read(uint8_t *addr, uint16_t count) 
| reads count bytes from i2c bus into addr
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
    
ON_XMIT_FAILED
    moveq #0,%d0                | return failure 
    jbra _regr_exit
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_reg_write(uint8_t *addr, uint8_t startReg, uint16_t count) 
| writes startReg then count bytes from addr to i2c bus
| arguments: buffer to read from, start reg byte, num bytes
| returns: bool success

i2c_reg_write: 
    movm.l %a2/%d2-%d3,-(%sp)

    SETUP_REGS
    
    I2C_START
    
    move.b (curr_slave), %d3 
    move.b %d3, %d0             | current slave address, LSB is 0 - write
    WRITE_BYTE

    move.b (11+12,%sp), %d0     | write start register
    WRITE_BYTE
 
    move.w (14+12,%sp),%d3      | count of bytes to move
    move.l ( 4+12,%sp),%a2      | destination

    jbra _enter_wl              | we share the read loop with i2c_bulk_read
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| uint8_t i2c_bulk_write(uint8_t *addr, uint16_t count) 
| writes count bytes from addr to i2c bus
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

_enter_wl:   
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
    
ON_XMIT_FAILED
    moveq #0, %d0               | return failure
    jbra _w_exit
