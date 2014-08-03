| ASM serial routines for asm app
.text
.align 2

.global return_to_loader

.global put_bin
.global print_dec
.global puts

.global putc
.global putb
.global putw
.global putl

.global puthexlong
.global puthexword
.global puthexbyte

.global getw
.global getl
.global getb

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

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| write long in %d0 to serial
putl:
    swap %d0
    bsr.s putw
    swap %d0
    bsr.s putw
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| write word in %d0 to serial
putw:
    move.w %d0, -(%sp)

    lsr.w #8, %d0
    bsr.s putb
   
    move.w (%sp)+, %d0
    bsr.s putb

    rts
   
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| write byte in %d0 to serial
putc:
putb:
    | see if transmitter is idle (uppermost bit = 1)
    tst.b (TSR)                | set N according to uppermost bit
    jpl putb                  | if N is not set, branch.

    move.b %d0, (UDR)          | write char to buffer
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put string in %a0    
puts:
    move.b (%A0)+, %D0
    jeq done
    
    jbsr putb
    jra puts
    
done:
    rts
 
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put hex long in %d0 
puthexlong:
    move.l %D0, -(%SP)

    swap %D0
    jbsr puthexword
    
    move.l (%SP), %D0
    jbsr puthexword

    move.l (%SP)+, %D0
    rts
            
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put hex word in %d0         
puthexword:
    move.w %D0, -(%SP)

    lsr.w #8, %D0
    jbsr puthexbyte
    
    move.w (%SP), %D0
    jbsr puthexbyte

    move.w (%SP)+, %D0
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put hex byte in %d0 
puthexbyte:
    movem.l %A0/%D0, -(%SP)        | save regs
    
    lsr.b #4, %D0                  | shift top 4 bits over
    bsr.s _puthdigit
    
    move.w (2, %SP), %D0           | restore D0 from stack    
    bsr.s _puthdigit
    
    movem.l (%SP)+, %A0/%D0        | restore regs
    rts
    
| put single hex digit from %D0 (trashes D0, A0)
_puthdigit:
    lea (%pc,_hexchars), %A0
    and.w #0xF, %D0
    move.b (%A0, %D0.W), %D0       | look up char
    jbsr putb
    rts
              
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| print number in %d0, return character count in %d0
| %d0 max value: 655359    
print_dec:
    move.w %d1, -(%sp)          | save %d1
    clr.b %d1                   | character count
    
    tst.l %d0                   | check for zero
    jeq rz
    
    jbsr pr_dec_rec             | begin recursive print
    
dec_r:
    move.b %d1, %d0             | set up return value
    move.w (%sp)+, %d1          | restore %d1
    rts
    
rz: 
    jbsr ret_zero
    jra dec_r

| recursive decimal print routine
pr_dec_rec:
    divu #10, %d0
    swap %d0
    move.w %d0, -(%sp)         | save remainder
    clr.w %d0                  | clear remainder
    swap %d0                   | back to normal
    jeq no_more_digits         | swap sets Z if reg is zero - how cool!
    
    jbsr pr_dec_rec            | get next digit
    
no_more_digits:
    move.w (%sp)+, %d0         | restore remainder
    
ret_zero:
    addi.b #'0', %d0           | turn it into a character
    jbsr putb                 | print
    addq.b #1, %d1             | increment char count
pr_ret:
    rts
 
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| print %d2 bits from %d1  
put_bin: 
    moveq #'0', %D0
    btst %D2, %D1
    jeq not1
    moveq #'1', %D0
not1:
    jbsr putb
    moveq #' ', %D0
    jbsr putb
    
    dbra %D2, put_bin
    rts
    
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

.section .rodata

_hexchars: .string "0123456789ABCDEF"
 
