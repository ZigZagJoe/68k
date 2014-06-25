||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader - main code

.align 2
.data

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

.set default_addr,  0x2000

| command codes
.set CMD_SET_BOOT,  0xC1
.set CMD_SET_FLWR,  0xC2
.set CMD_SET_LDWR,  0xC3
.set CMD_SET_ADDR,  0xC4
.set CMD_SET_BINSR, 0xC5
.set CMD_DUMP,      0xCA
.set CMD_BOOT,      0xCB
.set CMD_QCRC,      0xCC
.set CMD_SREC,      0xCD
.set CMD_RESET,     0xCF

cmd_jmp_table:
    .long __bad_cmd         | C0
    .long set_boot          | C1
    .long set_flash_wr      | C2
    .long set_loader_wr     | C3
    .long set_addr          | C4
    .long set_binary_srec   | C5
    .long __bad_cmd         | C6
    .long __bad_cmd         | C7
    .long __bad_cmd         | C8
    .long __bad_cmd         | C9
    .long memory_dump       | CA
    .long boot_ram          | CB
    .long get_qcrc          | CC
    .long do_parse_srec     | CD
    .long __bad_cmd         | CE
    .long reset_addr        | CF - not used as reset_addr can not use RTS

| srec flags
.set FLAG_BOOT,        1
.set FLAG_WR_FLASH,    2
.set FLAG_WR_LOADER,   4
.set FLAG_BIN_SREC,    8
  
.text
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
    TILDBG B7                  | bootloader ready!
   
    | initialize variables
    move.l #default_addr, %d0
    jsr init_vars
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| main bootloader loop
loop:
    jsr getc
    
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
    
    subi.b #0xC0, %d0          | get table offset
    
    cmp.b #0xF, %d0
    beq reset_addr             | reset is not a subroutine... jump directly.
    bhi __cmd_oor              | out of range command (ie. not valid)
    
    and.w #0xF, %d0
    lsl.w #2, %d0              | ind * 4 (size of long)
    
    movea.l #cmd_jmp_table, %a0 | load table address
    move.l (%a0, %d0.W), %a0   | read address at offset
    jsr (%a0)                  | jump to address
    
    bra loop
    
__cmd_oor:
    TILDBG B0                  | command out of range
    bra loop
    
__bad_cmd:
    TILDBG BC                  | not an implemented command
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||  
| flag setting handlers
 
| parse binary srec
set_binary_srec:
    move.l #0xB17AC5EC, %d0
    jsr putl
    ori.b #FLAG_BIN_SREC, %d5
    rts
 
| set to boot from s-record
set_boot:
    move.l #0xD0B07CDE, %d0
    jsr putl
    ori.b #FLAG_BOOT, %d5
    rts
    
| set to allow s record to write flash
set_flash_wr:
    move.l #0xF1A5C0DE, %d0
    jsr putl
    ori.b #FLAG_WR_FLASH, %d5
    rts
    
| set to allow s record to write loader
set_loader_wr:
    move.l #0x10ADC0DE, %d0
    jsr putl
    ori.b #FLAG_WR_LOADER, %d5
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||       
| set address to load data to
set_addr:
    TILDBG 1A
    
    move.l #0xCE110C00, %d0
    jsr putl
    
    jsr getl               | get address
    jsr putl               | echo it back
    jsr init_vars          | init vars
    move.w #0xACC0, %d0    | send tail
    jsr putw
    
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command to dump a region of memory
memory_dump:
    TILDBG D0
    
    move.w #0x10AD, %d0
    jsr putw
    
    jsr getl                   | read source address
    move.l %d0, %a0            | source
    jsr getl                   | read length
    move.l %d0, %d1            | length   
    
    jsr putl                   | echo length
    move.l %a0, %d0
    jsr putl                   | echo source
    
    jsr getw                   | get final confirmation to go (addr/len correct)
    cmp.w #0x1F07, %d0
    bne dump_end               | host aborted / out of sync
    
    move.l #0xDEADC0DE, %d2    | crc
    
dump_loop:
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    beq _cont                  | buffer empty, continue

    move.b (UDR), %d0
    
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    bne _cont                  | if Z is not set
    
    cmp.b #CMD_RESET, %d0      | check if byte is reset
    beq reset_addr
    
_cont:
    move.b (%a0)+, %d0         | read a byte
    move.b %d0, TIL311
    
    eor.b %d0, %d2             | qcrc update
    rol.l #1, %d2
    
    jsr putb                   | send it out
    
    subi.l #1, %d1
	bne dump_loop	
	
	move.l %d2, %d0            | send out the crc
	jsr putl
	
dump_end:
    move.w #0xEEAC, %d0        | finale 
    jsr putw
    
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command to handle s-record loaded into RAM
do_parse_srec:
    TILDBG DC
    
    move.l #0xD0E881CC, %d0    | tell host we are initiating write
    jsr putl
    
    move.b %d5, %d0            | write mode
    jsr putc
    
    | push arguments
    move.l %d5, -(%sp)         | write mode
    move.l %d6, -(%sp)         | byte count
    move.l %a5, -(%sp)         | byte addr
    
    jsr handle_srec            | enter c code
    | d0-d1, a0-a1 will be clobbered, return code in d0
    
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
    bne do_boot                   | not set as bootable
    rts
    
do_boot:
    | boot from the s-record address
    | ought to check this is nonzero...!
    TILDBG BC
    move.l (entry_point), %a0
    jmp.l (%a0)

| an error occured, do not boot.
bad_srec:
    TILDBG EC
    rts
        
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
    
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| boot command
boot_ram:
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
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| serial comm routines

| get a long from uart
getl:
    jsr getc
    lsl.w #8, %d0
    jsr getc
    lsl.l #8, %d0
    jsr getc
    lsl.l #8, %d0
    jsr getc
    rts
    
| get a word from uart
getw:
    jsr getc
    lsl.w #8, %d0
    jsr getc
    rts  
    
| get a character from uart 
getb:
getc:
    | see if there is a byte pending
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    beq getc                   | buffer empty, loop (Z=1)

    move.b (UDR), %d0          | read char from buffer
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
