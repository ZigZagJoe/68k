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
    
    move.l #_hexchars, %a0
    
    jsr puthex             | begin recursive print
    
dec_r:
    rts
    
rz: 
    move.b #'0', %d0
    jsr putc
    bra dec_r

puthex:
    tst.l %d0
    jeq ret_hex
    
    tst.b %d1
    jeq ret_hex
    
    subi #1, %d1
    
    move.b %d0, %d2
    and.w #0xF, %d2
    
    move.b (%a0, %d2.W), -(%sp)
    
    lsr.l #4, %d0
    jsr puthex
    
    move.b (%sp)+, %d0
    jsr putc
   
ret_hex: 
    rts




main:
    move.b #0x11, TIL311
  
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


