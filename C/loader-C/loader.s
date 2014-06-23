||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader

.text
.align 2

.global loader_start
.global putc
.global entry_point

.extern handle_srec

| display a constant byte
.macro TILDBG byte
     move.b #0x\byte, (TIL311)
.endm    

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| addresses of IO devices 
.set MFP,    0xC0000           | MFP address
.set TIL311, 0xC8000           | TIL311 address

| MFP registers
.set GPDR,  MFP + 0x01         | gpio data
.set UDR,   MFP + 0x2F         | uart data
.set TSR,   MFP + 0x2D         | transmitter status
.set RSR,   MFP + 0x2B         | receiver status
.set TCDCR, MFP + 0x1D         | timer c,d control
.set TCDR,  MFP + 0x23         | timer c data
.set UCR,   MFP + 0x29         | uart ctrl

.set loram_addr, 0x2000
.set hiram_addr, 0x40000

| command codes
.set CMD_BOOT, 0xCB
.set CMD_RESET, 0xCF
.set CMD_QCRC, 0xCC
.set CMD_HIRAM, 0xCE
.set CMD_SREC, 0xCD
.set CMD_SET_BOOT, 0xC1
.set CMD_SET_FLWR, 0xC2
.set CMD_SET_LDWR, 0xC3

| srec flags
.set FLAG_BOOT, 1
.set FLAG_WR_FLASH, 2
.set FLAG_WR_LOADER, 4

#####################################################################
| entry point of bootloader code in RAM
loader_start: 
    TILDBG C0                  | debugging message
    
    | set up timer C as baud rate generator at 28800 (3.6864mhz crystal)
    move.b #1, (TCDR)
    andi.b #0xF, (TCDCR)
    ori.b #0x16, (TCDCR)
      
    | initialize 68901 UART
    move.b #0b10001000, (UCR)  | /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)           | receiver enable
    move.b #1, (TSR)           | transmitter enable
    
reset_addr:
    TILDBG B2                  | bootloader ready!
   
    | initialize variables
    move.l #loram_addr, %d0
    jsr init_vars
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| main bootloader loop
loop:
    | see if there is a byte pending
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    beq loop                   | buffer empty, loop (Z=1)

    move.b (UDR), %d0          | read char from buffer
    move.b %d0, (TIL311)       | display on device
    
    | check to see if the GPIO for command byte is low
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    beq cmd_byte               | gpio is 0; command byte!
    
    move.b %d0, (UDR)          | not a command, echo it back
    move.b %d0, (%a0)+         | store to address, postincrement

    addi.l #1, %d6             | bytecount ++
    
    eor.b %d0, %d7             | qcrc update
    rol.l #1, %d7
    
    bra loop                   | and back to start we go
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| execute commmand byte in %d0
cmd_byte:
    cmp.b #CMD_BOOT, %d0       | is it the command to boot?
    beq boot

    cmp.b #CMD_RESET, %d0      | is it the command to reset address?
    beq reset_addr
    
    cmp.b #CMD_QCRC, %d0
    beq get_qcrc
    
    cmp.b #CMD_HIRAM, %d0
    beq go_hiram
    
    cmp.b #CMD_SREC, %d0
    beq do_parse
    
    cmp.b #CMD_SET_BOOT, %d0
    beq set_boot
    
    cmp.b #CMD_SET_FLWR, %d0
    beq set_flash_wr
    
    cmp.b #CMD_SET_LDWR, %d0
    beq set_loader_wr
    
    TILDBG BC                  | not a recognized command
    bra loop
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| set to boot from s-record
set_boot:
    move.l #0xD0B07CDE, %d0
    jsr putl
    ori #FLAG_BOOT, %d5
    bra loop
    
| set to allow s record to write flash
set_flash_wr:
    move.l #0xF1A5C0DE, %d0
    jsr putl
    ori #FLAG_WR_FLASH, %d5
    bra loop
    
| set to allow s record to write loader
set_loader_wr:
    move.l #0x10ADC0DE, %d0
    jsr putl
    ori #FLAG_WR_LOADER, %d5
    bra loop

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| get crc command
do_parse:
    TILDBG DC
    
    move.l #0xD0E881CC, %d0    | tell host we are initiating write
    jsr putl
    
    move.b %d5, %d0            | write mode
    jsr putc
    
    | push arguments
    move.l %d5, -(%sp)         | write mode
    move.l %d6, -(%sp)         | byte count
    move.l %a5, -(%sp)         | byte addr
    
    jsr handle_srec            | enter c
    
    | dealloc arguments
    add.l #12, %sp
    
    move.w %d0, %d1            | save return code
    move.w #0xC0DE, %d0        | write sync word #2
    jsr putw            
    
    move.b %d1, %d0            | write return code
    jsr putc
    
    move.w #0xEF00, %d0        | trailing null
    jsr putw
    
    cmp.b #0, %d1
    bne bad_srec
    
    TILDBG 0C
    
    and.b #FLAG_BOOT, %d5
    beq loop                   | not set as bootable

    | boot from the s-record address
    | ought to check this is nonzero...
    move.l (entry_point), %a5
    bra boot

bad_srec:
    TILDBG EE
    bra loop
        
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| get crc command
go_hiram:
    TILDBG 1A
    
    move.l #hiram_addr, %d0
    jsr init_vars
    
    move.l #0xCE110C00, %d0
    jsr putl
    
    bra loop

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| get crc command
get_qcrc:
    TILDBG CC                  | display code
    
    move.w #0xFCAC, %d0        | intro
    jsr putw
    
    move.l %d7, %d0            | crc data
    jsr putl                  
    
    clr.b %d0                  | tail
    jsr putc
    
    bra loop

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| boot command
boot:
    TILDBG BB                  | display boot code
    jmp (%a5)                  | jump to the code loaded into RAM

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| functions

| initialize registers with load address in %d0
init_vars:
    movea.l %d0, %a0           | set address to load to
    movea.l %d0, %a5           | save boot address
    clr.l %d6                  | bytecount = 0
    clr.l %d5                  | flash writes disallowed
    move.l #0xDEADC0DE, %d7    | init qcrc
    rts
    
| write long to serial
putl:
	swap %d0
	jsr putw
	swap %d0
	jsr putw
	rts

| write word to serial
putw:
	move.w %d0, -(%sp)

	lsr.w #8, %d0
	jsr putc
	
	move.w (%sp)+, %d0
	jsr putc

	rts
	
| write byte to serial
putb:
putc:
	btst #7, (TSR)           | test buffer empty (bit 7)
    beq putc                 | Z=1 (bit = 0) ? branch
	move.b %d0, (UDR)		 | write char to buffer
	rts
	
# EOF loader.s
