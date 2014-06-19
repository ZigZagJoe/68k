||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader

.text
.align 2
.global _boot

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| addresses of IO devices 
.set MFP,    0xC0000           | MFP address
.set TIL311, 0xC8000           | til311 address

| MFP registers
.set GPDR,  MFP + 0x01         | gpio data
.set UDR,   MFP + 0x2F         | uart data
.set TSR,   MFP + 0x2D         | transmitter status
.set RSR,   MFP + 0x2B         | receiver status
.set TCDCR, MFP + 0x1D         | timer c,d control
.set TCDR,  MFP + 0x23         | timer c data
.set UCR,   MFP + 0x29         | uart ctrl

| stack pointer. grows down in RAM
.set stack_pointer, 0x80000

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| boot stack pointer and program counter
| comment these out if testing in RAM

_isp: .long stack_pointer      | initial spvr stack pointer
_ipc: .long 0x80008            | initial program counter 

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| actual bootloader code

_boot:
    move.b #0x10, (TIL311)     | display greeting
    move.l #stack_pointer, %sp | set stack pointer (maybe we were soft reset)
    
    | reset UART and any other devices attached to /RESET
    reset
    
    | relocate into RAM
    lea (reloc, %pc), %a0	   | load address of reloc into %a0, position indep.
    movea.l #0x1000, %a1       | load relocation address into %a1
 	move.w #(reloc_sz/4), %d0  | load number of longs to copy (+1) into %d0 
            		   
    move.b #0x1C, (TIL311)     | debugging message
   
cpy_reloc:
    move.l (%a0)+, (%a1)+      | *a1++ = *a0++
    dbra %d0, cpy_reloc        | branch if (--d0) != -1

    move.b #0xCD, (TIL311)     | debugging message
   
    jmp 0x1000                 | jump to relocated code
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| bootloader code in RAM

reloc: 
    move.b #0xC0, (TIL311)     | debugging message
    
    | enable baud rate generator at 28800
    move.b #1, (TCDR)
    andi.b #0xF, (TCDCR)
    ori.b #0x16, (TCDCR)
      
    | initialize 68901 UART
    move.b #0b10001000, (UCR)  | /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)           | receiver enable
    move.b #1, (TSR)           | transmitter enable

reset:
    movea #0x2000, %a0         | load base address into %a0
    move.b #0xB1, (TIL311)     | display greeting

loop:
    | see if there is a byte pending
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    beq loop                   | buffer empty, loop (Z=1)

    move.b (UDR), %d0          | read char from buffer
    move.b %d0, (TIL311)       | display on device
    
    | check to see if the GPIO for command byte is low
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    bne wrb                    | gpio is 1| not a command byte
    
    cmp.b #0xCB, %d0           | is it the command to boot?
    beq boot

    cmp.b #0xCF, %d0           | is it the command to reset address?
    beq reset
    
    move.b #0xBC, (TIL311)     | not a recognized command
    bra loop
    
wrb:  
    move.b %d0, (UDR)          | echo it back to sender
    move.b %d0, (%a0)+         | store to address, postincrement

    bra loop                   | relative jump
    
boot:
    move.b #0xBB, (TIL311)     | display char for debugging
    jmp 0x2000                 | jump to the code loaded into RAM
    
| get size of relocated section
.set reloc_sz, . - reloc

| EOF loader.s
