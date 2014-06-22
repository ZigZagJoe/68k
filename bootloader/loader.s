||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader

.text
.align 2
.global _boot

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

| default stack pointer. grows down in RAM
.set stack_pointer, 0x80000
.set reloc_addr, 0x1000
.set loram_addr, 0x2000
.set hiram_addr, 0x40000

| command codes
.set CMD_BOOT, 0xCB
.set CMD_RESET, 0xCF
.set CMD_QCRC, 0xCC
.set CMD_HIRAM, 0xCE

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| boot stack pointer and program counter
| comment these out if testing in RAM...

|_isp: .long stack_pointer      | initial spvr stack pointer
|_ipc: .long _boot              | initial program counter 

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| bootloader entry point
_boot:
    TILDBG BA
    
    ori.w #0x700, %sr          | disable external interrupts 
                               | may die here if not supervisor (user program jumped)
    
    TILDBG 10                  | display greeting
    move.l #stack_pointer, %sp | set stack pointer (maybe we were soft reset)
    
    | reset UART and any other devices attached to /RESET
    reset
    
    | relocate into RAM
    lea (reloc, %pc), %a0      | load address of reloc into %a0, position indep.
    movea.l #reloc_addr, %a1   | load relocation address into %a1
    move.w #(reloc_sz/4), %d0  | load number of longs to copy (+1) into %d0 
                     
    TILDBG 1C                  | debugging message
   
cpy_reloc:
    move.l (%a0)+, (%a1)+      | *a1++ = *a0++
    dbra %d0, cpy_reloc        | branch if (--d0) != -1

    TILDBG CD                  | debugging message
   
    jmp reloc_addr             | jump to relocated code
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| bootloader code in RAM
reloc: 
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
    TILDBG B1                  | bootloader ready!
   
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
    
    TILDBG BC                  | not a recognized command
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
    jmp (%a6)                  | jump to the code loaded into RAM

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| functions

| initialize registers with load address in %d0
init_vars:
    movea.l %d0, %a0           | set address to load to
    movea.l %d0, %a6           | save boot address
    clr.l %d6                  | bytecount = 0
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
putc:
	btst #7, (TSR)           | test buffer empty (bit 7)
    beq putc                 | Z=1 (bit = 0) ? branch
	move.b %d0, (UDR)		 | write char to buffer
	rts

	
#################################################################### 
# get size of relocated section
.set reloc_sz, . - reloc
# EOF loader.s
