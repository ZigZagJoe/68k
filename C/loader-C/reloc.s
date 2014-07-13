||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader - relocation code

.text
.align 2

.global _boot

.extern loader_start
.extern __begin
.extern __size

| display a constant byte
.macro TILDBG byte
     move.b #0x\byte, (TIL311)
.endm    

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| addresses of IO devices 
.set TIL311,        0xC8000

| default stack pointer. grows down in RAM
.set stack_pointer, 0x80000
.set reloc_addr,    0x1000

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| boot stack pointer and program counter
| comment these out if testing in RAM...

_isp: .long stack_pointer      | initial spvr stack pointer
_ipc: .long 0x80008            | initial program counter 

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| bootloader entry point
| This code must run in a PIC manner! No absolute references!
_boot:
    TILDBG BA

    ori.w #0x700, %sr          | disable external interrupts 
                               | may die here if not supervisor (user program jumped)
    
    TILDBG 10                  | display greeting
    move.l #stack_pointer, %sp | set stack pointer (maybe we were soft reset)
    
    | reset UART and any other devices attached to /RESET
    reset
    
    lea (__begin, %pc), %a0    | load address of start of loader, relative to %PC
                                       
    movea.w #reloc_addr, %a1   | load word relocation target address into %a1
    move.w #__size, %d0        | copy bootloader size (in longs) into %d0

    TILDBG 1C                  | debugging message
   
cpy_reloc:
    move.l (%a0)+, (%a1)+      | *a1++ = *a0++
    dbra %d0, cpy_reloc        | branch if (--d0) != -1

    TILDBG CB                  | debugging message
               
    jmp loader_start           | absolute jump to relocated code
    
