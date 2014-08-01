.text
.align 2
.global _boot

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

|_isp: .long stack_pointer       | initial spvr stack pointer
|_ipc: .long 0x80008             | initial program counter 

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| bootloader entry point
| This code must run in a PIC manner! No absolute references!
_boot:    
    TILDBG BA

    ori.w #0x700, %sr           | disable external interrupts 
                                | may die here if not supervisor (user program jumped)
    
    move.l #stack_pointer, %sp  | set stack pointer (maybe we were soft reset)
    
    | reset UART and any other devices attached to /RESET
    reset
     
| set up decompress vars
    lea (payload, %pc), %a1     | a1 <in_ptr>    address of payload, relative to %PC - 
    move.w #reloc_addr, %a2     | a2 <out_ptr>
    move.l (%a1)+, %a3          | <ilen>         read long from payload
    add.l %a1, %a3              | a3 <in_end>    <in_ptr> + <ilen>
   
    TILDBG DC                   | debugging message
 
| decompress payload 
loop_head:
    clr.w %d0
    move.b (%a1)+, %d0          | read ctrl byte
    
    cmpi.b #31, %d0
    jbhi backref                | if any of b7-b5 is set, then backref
    
| copy (%d0+1) literal bytes from in_ptr to out_ptr
lit_cpy:
    move.b (%a1)+, (%a2)+       | *<out_ptr>++ = *<in_ptr>++
    dbra %d0, lit_cpy
    
    jbra loop_chk
    
backref:
    move.w %d0, %d1
    lsr.b #5, %d1               | %d1 <len> = b7-b5 of <ctrl>
    
    cmpi.b #7, %d1              
    jbne not_long_fmt           | if len = 7, read another byte for len
    
    move.b (%a1)+, %d1          | %d1 <len> = *in_ptr++
    addq.w #7, %d1              | %d1 <len> += 7
    
not_long_fmt:
    lsl.b #3, %d0               | %d0 = 00000000 AAAAA000
    lsl.w #5, %d0               | %d0 = 000AAAAA 00000000
    move.b (%a1)+, %d0          | %d0 = 000AAAAA BBBBBBBB
    
    lea (-1, %a2), %a0          | %a0 <ref_ptr>  = %a2 <out_ptr> - 1
    
    sub.w %d0, %a0              | %a0 <ref_ptr> -= %d0 <backref_dist>
    
| copy (<len> + 1) bytes from ref_ptr to out_ptr   
ref_copy:
    move.b (%a0)+, (%a2)+       | *<out_ptr>++ = *<ref_ptr>++
    dbra %d1, ref_copy

    move.b (%a0)+, (%a2)+       | copy one more byte
    
loop_chk:
    cmp.l %a1, %a3              | while (in_ptr != in_end)
    jbhi loop_head

    TILDBG CB                   | debugging message
    jmp 0x1000                  | jump to payload
