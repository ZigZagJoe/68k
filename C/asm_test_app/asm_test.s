.align 2
.text
.global main

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data reg
.set UDR, MFP_BASE + 0x2F | uart data register
.set TSR, MFP_BASE + 0x2D | transmitter status reg
.set RSR, MFP_BASE + 0x2B | receiver status reg
.set TIL311, 0xC8000      | til311 displays

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

print_hex:
    tst.l %d0              | check for zero
    beq rz
    
    lea (%pc,_hexchars), %a0
    
    bsr puthex             | begin recursive print
    
dec_r:
    rts
    
rz: 
    move.b #'0', %d0
    bsr putc
    bra dec_r

puthex:
    subi.b #1, %d1
    jmi ret_hex
    
    move.b %d0, %d2
    and.w #0xF, %d2
    
    move.b (%a0, %d2.W), -(%sp)
    
    lsr.l #4, %d0
    jeq begin_ret    | if result is 0
    
    bsr puthex
     
begin_ret: 
    move.b (%sp)+, %d0
    bsr putc
   
ret_hex: 
    rts




main:

    movem.l %d1-%d7/%a2-%a6, -(%sp)
    
cpy_l:
    movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1)
    adda.l #(12*4), %a1
    dbra %d0, cpy_l
    
    movem.l (%sp)+, %d1-%d7/%a2-%a6

    move.l #0x11111111, %d0
    move.l #0x22222222, %d1
    move.l #0x33333333, %d2
    move.l #0x44444444, %d3
    move.l #0x55555555, %d4
    move.l #0x66666666, %d5
    move.l #0x77777777, %d6
    move.l #0x88888888, %d7
    
    
    trap #14
  
    move.b #8, %d1
    move.l #0x1003123, %d0
    jsr print_hex
    
  
  
    jsr return_to_loader


||||||||||||

return_to_loader:
    move.b #'\n', %d0
    jsr putc
    jsr putc
    move.l #100000, %d0
    
wit: 
    subi.l #1, %d0
    bne wit

    move.w #0xB007, 0x400
    jmp 0x80008

putc:
    btst #7, (TSR)                 | test buffer empty (bit 7)
    beq putc                       | Z=1 (bit = 0) ? branch
    move.b %D0, (UDR)              | write char to buffer
    rts
    
 
   _hexchars: .ascii "0123456789ABCDEF"


