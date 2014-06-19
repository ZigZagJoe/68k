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
.set boot_addr, 0x2000

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| boot stack pointer and program counter
| comment these out if testing in RAM...

_isp: .long stack_pointer      | initial spvr stack pointer
_ipc: .long _boot              | initial program counter 

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| bootloader entry point
_boot:
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
    
    | enable baud rate generator at 28800
    move.b #1, (TCDR)
    andi.b #0xF, (TCDCR)
    ori.b #0x16, (TCDCR)
      
    | initialize 68901 UART
    move.b #0b10001000, (UCR)  | /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)           | receiver enable
    move.b #1, (TSR)           | transmitter enable

reset_addr:
    TILDBG B1                  | display greeting
    movea #boot_addr, %a0      | load boot address into %a0

    movea.l %a0, %a6           | save boot address
    clr.l %d7                  | bytecount = 0
    
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

    addi.l #1, %d7             | bytecount ++
    bra loop                   | and back to start we go
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| execute commmand byte in %d0
cmd_byte:
    cmp.b #0xCB, %d0           | is it the command to boot?
    beq boot

    cmp.b #0xCF, %d0           | is it the command to reset address?
    beq reset_addr
    
    TILDBG BC                  | not a recognized command
    bra loop
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||   
| boot command
boot:
    TILDBG BB                  | display boot code
    jmp (%a6)                  | jump to the code loaded into RAM

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||    
| get size of relocated section
.set reloc_sz, . - reloc

# EOF loader.s
