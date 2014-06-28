||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader - main code

.align 2
.data

| entry point
.global loader_start

| C-callable serial functions
.global putb
.global putw
.global putl
.global getb
.global getw
.global getl

.extern entry_point
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

.set default_addr,   0x2000
.set sector1_entry, 0x81000

.set bootable_magic, 0xc141c340

/* this is code for:

   exg %d0, %d1; exg %d1, %d0;
   
   which is absolutely harmless with no impact on registers or flags
   thus, it will never be used in any sane code, but it does not need
   to be removed when testing a program in RAM, like boot vectors do.
   misc: a flash address can be nulled later without erasing the sector
*/  

| command codes
.set CMD_SET_BOOT,  0xC1
.set CMD_SET_FLWR,  0xC2
.set CMD_SET_LDWR,  0xC3
.set CMD_SET_BINSR, 0xC4
.set CMD_DUMP,      0xCA
.set CMD_BOOT,      0xCB
.set CMD_QCRC,      0xCC
.set CMD_SREC,      0xCD
.set CMD_SET_ADDR,  0xCE
.set CMD_RESET,     0xCF

cmd_jmp_table:
    .long __bad_cmd         | C0
    .long set_boot          | C1
    .long set_flash_wr      | C2
    .long set_loader_wr     | C3
    .long set_binary_srec   | C4
    .long __bad_cmd         | C5
    .long __bad_cmd         | C6
    .long __bad_cmd         | C7
    .long __bad_cmd         | C8
    .long __bad_cmd         | C9
    .long memory_dump       | CA
    .long boot_ram          | CB
    .long get_qcrc          | CC
    .long do_parse_srec     | CD
    .long set_addr          | CE
    .long reset_addr        | CF (never used as reset_addr can not use rts)

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
    
    | check if boot magic is present on sector 1
    cmp.l #bootable_magic, (sector1_entry)
    beq wait_for_command 
    
reset_addr:
    TILDBG B7                  | bootloader ready!
   
    | initialize variables
    move.l #default_addr, %d0
    jsr init_vars
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| main bootloader loop
loop:
    jsr getb
    
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
| wait for a command to stay in bootloader, otherwise boot
wait_for_command:
    TILDBG CD
    move.l #60000, %d2         | number of loops - should result about a second delay
    
_cmd_w_loop:
    jsr check_reset_cmd
    bne reset_addr             | enter bootloader mode

    subi.l #1, %d2
    jne _cmd_w_loop            | timeout, boot from flash
    
    TILDBG BF                
    jmp (sector1_entry)
    
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
    jsr _putl
    ori.b #FLAG_BIN_SREC, %d5
    rts
 
| set to boot from s-record
set_boot:
    move.l #0xD0B07CDE, %d0
    jsr _putl
    ori.b #FLAG_BOOT, %d5
    rts
    
| set to allow s record to write flash
set_flash_wr:
    move.l #0xF1A5C0DE, %d0
    jsr _putl
    ori.b #FLAG_WR_FLASH, %d5
    rts
    
| set to allow s record to write loader
set_loader_wr:
    move.l #0x10ADC0DE, %d0
    jsr _putl
    ori.b #FLAG_WR_LOADER, %d5
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||       
| set address to load data to
set_addr:
    TILDBG 1A
    
    move.l #0xCE110C00, %d0
    jsr _putl
    
    jsr getl                   | get address
    jsr _putl                  | echo it back
    jsr init_vars              | init vars with addr in %d0
    move.w #0xACC0, %d0        | send tail
    jsr _putw
    
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command to dump a region of memory
memory_dump:
    TILDBG D0
    
    move.w #0x10AD, %d0
    jsr _putw
    
    jsr getl                   | read source address
    move.l %d0, %a0            | source
    jsr getl                   | read length
    move.l %d0, %d1            | length   
    
    jsr _putl                  | echo length
    move.l %a0, %d0
    jsr _putl                  | echo source
    
    jsr getw                   | get final confirmation to go (addr/len correct)
    cmp.w #0x1F07, %d0
    bne dump_end               | host aborted / out of sync
    
    move.l #0xDEADC0DE, %d2    | qcrc initial value
    
dump_loop:
    jsr check_reset_cmd
    bne dump_end
        
    move.b (%a0)+, %d0         | read a byte
    move.b %d0, TIL311
    
    eor.b %d0, %d2             | qcrc update
    rol.l #1, %d2
    
    jsr _putb                  | send it out
    
    subi.l #1, %d1
    bne dump_loop   
   
    move.l %d2, %d0            | send out the crc
    jsr _putl
   
dump_end:
    move.w #0xEEAC, %d0        | finale 
    jsr _putw
    
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command to handle s-record loaded into RAM
do_parse_srec:
    TILDBG DC
    
    move.l #0xD0E881CC, %d0    | tell host we are initiating write
    jsr _putl
    
    move.b %d5, %d0            | write mode
    jsr _putb
    
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
    jsr _putw            
    
    move.b %d1, %d0            | write return code
    jsr _putb
    
    move.w #0xEF00, %d0        | trailing null
    jsr _putw
    
    cmp.b #0, %d1
    bne bad_srec
    
    TILDBG 0C
    
    and.b #FLAG_BOOT, %d5
    bne do_boot                | not set as bootable
        
    clr.b %d5                  | clear write flags
    
    rts
    
do_boot:
    | boot from the s-record address
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
    jsr _putw
    
    move.l %d7, %d0            | crc data
    jsr _putl                  
    
    clr.b %d0                  | tail
    jsr _putb
    
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
    
| check if reset command was recieved
| returns bool in %d0
| sets Z if return code is zero
check_reset_cmd:
    clr.b %d0
    
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    beq _end_chk               | buffer empty, continue

    cmp.b #CMD_RESET, (UDR)
    bne _end_chk
    
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    bne _end_chk               | if Z is not set (gpio = 1)

    move.b #1, %d0
    
_end_chk:
    tst.b %d0
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| serial comm routines

| fetch a long from uart into %d0
getl:
    jsr getb
    lsl.w #8, %d0
    jsr getb
    lsl.l #8, %d0
    jsr getb
    lsl.l #8, %d0
    jsr getb
    rts
    
| fetch a word from uart into %d0
getw:
    jsr getb
    lsl.w #8, %d0
    jsr getb
    rts  
    
| get a character from uart, store in %d0
getb:
    | see if there is a byte pending
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    beq getb                   | buffer empty, loop (Z=1)

    move.b (UDR), %d0          | read char from buffer
    rts
    
| c binding for putl
putl:
    move.l (4,%sp), %d0
    
| write long in %d0 to serial
_putl:
   swap %d0
   jsr _putw
   swap %d0
   jsr _putw
   rts

| c binding for putw
putw:
    move.w (6,%sp), %d0
    
| write word in %d0 to serial
_putw:
   move.w %d0, -(%sp)

   lsr.w #8, %d0
   jsr _putb
   
   move.w (%sp)+, %d0
   jsr _putb

   rts
   
| c binding for putb
putb:  
    move.b (7,%sp), %d0
    
| write byte in %d0 to serial
_putb:
   btst #7, (TSR)             | test buffer empty (bit 7)
   beq _putb                  | Z=1 (bit = 0) ? branch
   move.b %d0, (UDR)          | write char to buffer
   rts
   
# EOF loader.s
