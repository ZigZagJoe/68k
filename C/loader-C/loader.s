||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader - main code

.align 2
.text

| entry point
.global loader_start

| see c_funcs.c
.extern handle_srec
.extern init_vectors

| see asm_funcs.s
.extern getw
.extern getl
.extern getb
.extern _putb
.extern _putw
.extern _putl

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

/* default write address */
.set default_addr,   0x2000

/* sector 1 start address */
.set sector1_entry, 0x81000

.set bootable_magic, 0xc141c340

/* this is code for:

   exg %d0, %d1; exg %d1, %d0;
   
   which is absolutely harmless with no impact on registers or flags
   thus, it will never be used in any sane code, but it does not need
   to be removed when testing a program in RAM, like boot vectors do.
   misc: a flash address can be nulled later without erasing the sector
*/  

| srec flags
.set FLAG_WR_FLASH,    1
.set FLAG_WR_LOADER,   2
.set FLAG_BIN_SREC,    4
  
#####################################################################
| entry point of bootloader code in RAM
loader_start: 
    TILDBG C0                  | debugging message
    
    | set up timer C as baud rate generator at 28800 (3.6864mhz crystal)
    move.b #1, (TCDR)  
    andi.b #0xF, (TCDCR)       | disable timer C
    ori.b #0x16, (TCDCR)       | set timer C active with prescaler of /4
      
    | initialize 68901 UART
    move.b #0b10001000, (UCR)  | /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)           | receiver enable
    move.b #1, (TSR)           | transmitter enable
    
    bsr init_vectors
    
    cmpi.w #0xB007, 0x400      | check addr
    jeq reset_addr             | if it is set to magic val, do not (possibly) boot from rom
                               | as we were reset during the last stay-in-loader timeout period

    | check if boot magic is present on sector 1
    cmp.l #bootable_magic, (sector1_entry)
    jeq wait_for_command       | if it is, wait for a command to stay in bootloader
     
reset_addr:
    TILDBG B1                  | bootloader ready!
    clr.w 0x400
    
    | initialize variables
    move.l #default_addr, %d0
    bsr init_vars
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| main bootloader loop
loop:
    bsr getb
    
    move.b %d0, (TIL311)       | display on device
    
    | check to see if the GPIO for command byte is low
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    jeq handle_cmd             | gpio is 0; command byte!
    
    move.b %d0, (UDR)          | not a command, echo it back
    move.b %d0, (%a4)+         | store to address, postincrement

    addq.l #1, %d6             | bytecount ++
    
    eor.b %d0, %d7             | qcrc update
    rol.l #1, %d7
    
    jra loop                   | and back to start we go

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| wait for a command to stay in bootloader, otherwise boot
wait_for_command:
    TILDBG CD
    
    move.w #0xB007, 0x400      | set trying-to-boot magic
    
    move.l #100000, %d2        | number of loops - should result about a second delay
    
_cmd_w_loop:
    bsr check_reset_cmd
    jne reset_addr             | enter bootloader mode

    subq.l #1, %d2
    jne _cmd_w_loop            | timeout, boot from flash
    
    clr.w 0x400                | booting from rom, clear magic
       
    TILDBG BF   
    jmp (sector1_entry)
    
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| execute commmand byte in %d0
handle_cmd:
    
    subi.b #0xC0, %d0          | get table offset
    
    cmp.b #0xF, %d0
    jeq reset_addr             | 'reset' is not a subroutine, so jump directly.
    jhi __cmd_oor              | out of range command (ie. not valid)
    
    | put return address on stack as subroutines use RTS
    | so, we return straight to the loop function!
    pea loop
    
    and.w #0xF, %d0
    
    | quick %d0 * 2 (size of word)
    add.b %d0, %d0
    
    | read offset from cmd_jmp_table[%d0] (w/ pc-relative addressing)
    move.w (cmd_jmp_table, %pc, %d0.W), %d0

    | jump to address relative to %pc
    | 2: %pc has moved past instruction word when calculating address
    | 'The value in the PC is the address of the extension word'
    jmp (2, %pc, %d0.W)        | jump to address
    
jmp_pt:
    | point for calculating jump offsets against
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| commands

| jump table for commands
cmd_jmp_table:
    .word __bad_cmd        - jmp_pt | C0
    .word set_flash_wr     - jmp_pt | C1
    .word set_loader_wr    - jmp_pt | C2
    .word set_binary_srec  - jmp_pt | C3
    .word __bad_cmd        - jmp_pt | C4
    .word __bad_cmd        - jmp_pt | C5
    .word __bad_cmd        - jmp_pt | C6
    .word __bad_cmd        - jmp_pt | C7
    .word __bad_cmd        - jmp_pt | C8
    .word decompress       - jmp_pt | C9
    .word memory_dump      - jmp_pt | CA
    .word boot_ram         - jmp_pt | CB
    .word get_qcrc         - jmp_pt | CC
    .word do_parse_srec    - jmp_pt | CD
    .word set_addr         - jmp_pt | CE

| command out of range
__cmd_oor:
    TILDBG B0                  
    jra loop
   
| not an implemented command 
__bad_cmd:
    TILDBG BC                  
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||  
| flag setting commands
 
| parse binary srec
set_binary_srec:
    move.l #0xB17AC5EC, %d0
    bsr _putl
    ori.b #FLAG_BIN_SREC, %d5
    rts
    
| set to allow s record to write flash
set_flash_wr:
    move.l #0xF1A5C0DE, %d0
    bsr _putl
    ori.b #FLAG_WR_FLASH, %d5
    rts
    
| set to allow s record to write loader
set_loader_wr:
    move.l #0x10ADC0DE, %d0
    bsr _putl
    ori.b #FLAG_WR_LOADER, %d5
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||       
| command: set address to load data to
set_addr:
    TILDBG 1A
    
    move.l #0xCE110C00, %d0
    bsr _putl
    
    bsr getl                   | get address
    bsr _putl                  | echo it back
    bsr init_vars              | init vars with addr in %d0
    move.w #0xACC0, %d0        | send tail
    bsr _putw
    
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command: dump a region of memory
memory_dump:
    TILDBG DC
    
    move.w #0x10AD, %d0
    bsr _putw
    
    move.l %a4, %d0
    bsr _putl                  | put address
    
    bsr getl                   | read length
    bsr _putl                  | echo length
    move.l %d0, %d1            | %d1 = num bytes    

    bsr getw                   | get final confirmation to go (len correct)
    cmp.w #0x1F07, %d0
    jne dump_end               | host aborted / out of sync
    
    move.l #0xDEADC0DE, %d2    | qcrc initial value
    
dump_loop:
    bsr check_reset_cmd
    jne dump_end
        
    move.b (%a4)+, %d0         | read a byte
    move.b %d0, TIL311
    
    eor.b %d0, %d2             | qcrc update
    rol.l #1, %d2
    
    bsr _putb                  | send it out
    
    subq.l #1, %d1
    jne dump_loop   
   
    move.l %d2, %d0            | send out the crc
    bsr _putl
   
dump_end:
    move.w #0xEEAC, %d0        | finale 
    bsr _putw
    
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command: decompress %d6 bytes from %a5, store to address specified
| set boot address/read address to address
| set byte count to number of bytes decompressed
decompress:
    TILDBG DF
    
    move.l #0xD14F1A7E, %d0    | tell host we are doing inflate
    bsr _putl
    
    bsr getl                   | read target
    bsr _putl                  | echo target
    
    move.l %d0, -(%sp)         | target addr
    move.l %d6, -(%sp)         | byte count
    pea (%a5)                  | src addr
    
    bsr init_vars              | set address to target address          
    
    bsr lzf_inflate
    add.l #12, %sp             | dealloc args
    
    move.l %d0, %d6            | set byte count to decompressed byte count
    
    move.w #0xC0DE, %d0        | echo 0xC0DE
    bsr _putw
    
    move.l %d6, %d0  
    bsr _putl                  | return code (should be num bytes)

    move.l %a5, %a0            | read addr
    move.l %d6, %d1            | num of bytes
    
    bsr _compute_crc           | generate a crc of the decompressed data
    
    bsr _putl                  | send the crc out
    
    move.w #0x1F00, %d0        | trailing code
    bsr _putw
    
    TILDBG FE
   
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command: handle s-record loaded into RAM
do_parse_srec:
    TILDBG D0
    
    move.l #0xD0E881CC, %d0    | tell host we are initiating write
    bsr _putl
    
    move.b %d5, %d0            | write mode
    bsr _putb
    
    | push arguments
    move.l %d5, -(%sp)         | write mode
    move.l %d6, -(%sp)         | byte count
    move.l %a5, -(%sp)         | byte addr
    
    bsr handle_srec            | enter c code
    | d0-d1, a0-a1 will be clobbered, return code in d0
    
    | dealloc arguments
    add.l #12, %sp

    swap %d0                   | %d0 = [ RET]xxxx
    move.w #0xC0DE, %d0        | %d0 = [ RET]C0DE 
    swap %d0                   | %d0 = C0DE[ RET]
    bsr _putl          
    
    move.w #0xEF00, %d0        | trailing null
    bsr _putw
    
    clr.b %d5                  | clear write flags
   
    tst.b %d0
    jne bad_srec
    
    TILDBG 0C    
    rts

| an error occured
bad_srec:
    TILDBG FA
    rts
        
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command: get current qcrc
get_qcrc:
    TILDBG CC                  | display code
    
    move.w #0xFCAC, %d0        | intro
    bsr _putw
    
    move.l %d7, %d0            | crc data
    bsr _putl                  
    
    clr.b %d0                  | tail
    bsr _putb
    
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| command: boot from last address loaded
boot_ram:
    TILDBG BB                  | display boot code
    jmp (%a5)                  | jump to the code loaded into RAM

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| functions

| initialize var-registers with address in %d0
init_vars:
    movea.l %d0, %a4           | set address to load to ...
    movea.l %d0, %a5           | ... and save boot address
    clr.l %d6                  | bytecount = 0
    clr.l %d5                  | clear flags
    move.l #0xDEADC0DE, %d7    | init qcrc value
    rts
   
# EOF loader.s
