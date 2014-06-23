||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k bootloader

.text
.align 2
__the_start:

.global _boot
.extern parse_srec
.extern loader_start

| display a constant byte
.macro TILDBG byte
     move.b #0x\byte, (TIL311)
.endm    

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| addresses of IO devices 
.set TIL311,    0xC8000

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
| this code must run in a PIC manner
_boot:
    TILDBG BA

    ori.w #0x700, %sr          | disable external interrupts 
                               | may die here if not supervisor (user program jumped)
    
    TILDBG 10                  | display greeting
    move.l #stack_pointer, %sp | set stack pointer (maybe we were soft reset)
    
    | reset UART and any other devices attached to /RESET
    reset
    
    lea (__the_start, %pc), %a0 | load address of start of loader, relative to %PC
                                       
    movea.l #reloc_addr, %a1   | load relocation target address into %a1
    move.w #1023, %d0          | copy full 4kb of bootloader area ((1023+1) * 4)

    TILDBG 1C                  | debugging message
   
cpy_reloc:
    move.l (%a0)+, (%a1)+      | *a1++ = *a0++
    dbra %d0, cpy_reloc        | branch if (--d0) != -1

    TILDBG CD                  | debugging message
               
    jmp loader_start           | absolute jump to relocated code
    
