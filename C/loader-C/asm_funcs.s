.text
.align 2

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

/* command byte for reboot into bootloader/reset address */
.set CMD_RESET, 0xCF

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| Function exports

| ASM only functions
.global _putb
.global _putw
.global _putl

.global _puts

.global _print_dec
.global _puthexlong
.global _puthexword
.global _puthexbyte

.global put_bin

| C-callable wrappers
.global putb
.global putw
.global putl

.global puts
.global print_dec
.global puthexlong
.global puthexword
.global puthexbyte

| these work with C or ASM as they do not take arguments
.global getw
.global getl
.global getb
.global check_reset_cmd

.global _hexchars

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| serial comm routines

| fetch a long from uart into %d0
getl:
    jsr getb
    lsl.w #8, %d0
    jsr getb
    lsl.l #8, %d0
    jsr getb
    lsl.l #8, %d0
    jsr getb
    rts
    
| fetch a word from uart into %d0
getw:
    jsr getb
    lsl.w #8, %d0
    jsr getb
    rts  
    
| get a character from uart, store in %d0
getb:
    | see if there is a byte pending
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    jeq getb                   | buffer empty, loop (Z=1)

    move.b (UDR), %d0          | read char from buffer
    rts
    
## C binding
putl:
    move.l (4,%sp), %d0
    
| write long in %d0 to serial
_putl:
   swap %d0
   jsr _putw
   swap %d0
   jsr _putw
   rts

## C binding
putw:
    move.w (6,%sp), %d0
    
| write word in %d0 to serial
_putw:
   move.w %d0, -(%sp)

   lsr.w #8, %d0
   jsr _putb
   
   move.w (%sp)+, %d0
   jsr _putb

   rts
   
## C binding
putb:  
    move.b (7,%sp), %d0
    
| write byte in %d0 to serial
_putb:
    btst #7, (TSR)             | test buffer empty (bit 7)
    jeq _putb                  | Z=1 (bit = 0) ? branch
    move.b %d0, (UDR)          | write char to buffer
    rts

## C binding   
puts:
    move.l (4,%sp), %a0
    
||||||||||||||||||||||||||||||||||||||
| put string in %a0    
_puts:
    move.b (%A0)+, %D0
    jeq done
    
    jsr _putb
    bra puts
    
done:
    rts
 
## C binding
pithexlong:
    move.l 4(%sp), %d0 
     
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||    
| put hex long in %d0 
_puthexlong:
    move.l %D0, -(%SP)

    swap %D0
    jsr puthexword
    
    move.l (%SP), %D0
    and.l #0xFFFF, %D0
    jsr puthexword

    move.l (%SP)+, %D0
    rts
    
## C binding
puthexword:
    move.l 4(%sp), %d0
        
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||    
| put hex word in %d0         
_puthexword:
    move.w %D0, -(%SP)

    lsr.w #8, %D0
    jsr puthexbyte
    
    move.w (%SP), %D0
    and.w #0xFF, %D0
    jsr puthexbyte

    move.w (%SP)+, %D0
    rts
    
    
## C binding   
puthexbyte:
    move.l 4(%sp), %d0
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put hex byte in %d0 
_puthexbyte:
    movem.l %A0/%D0, -(%SP)        | save regs
    
    movea.l #_hexchars, %A0
    
    lsr.b #4, %D0                  | shift top 4 bits over
    and.w #0xF, %D0
    move.b (%A0, %D0.W), %D0       | look up char
    jsr _putb
    
    move.w (2, %SP), %D0           | restore D0 from stack    
    and.w #0xF, %D0                | take bottom 4 bits
    move.b (%A0, %D0.W), %D0       | look up char
    jsr _putb
    
    movem.l (%SP)+, %A0/%D0        | restore regs

    rts
    
## C binding
print_dec:
    move.l 4(%sp), %d0
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| print number in %d0, return character count in %d0
| %d0 max value: 655359    
_print_dec:
    move.w %d1, -(%sp)         | save %d1
    clr.b %d1                  | character count
    
    tst.l %d0                  | check for zero
    jeq rz
    
    jsr pr_dec_rec             | begin recursive print
    
dec_r:
    move.b %d1, %d0            | set up return value
    move.w (%sp)+, %d1         | restore %d1
    rts
    
rz: 
    jsr ret_zero
    bra dec_r

| recursive decimal print routine
pr_dec_rec:
    tst.l %d0
    jeq pr_ret
    
    divu #10, %d0
    swap %d0
    move.w %d0, -(%sp)         | save remainder
    clr.w %d0                  | clear remainder
    swap %d0                   | back to normal
    jsr pr_dec_rec             | get next digit
    move.w (%sp)+, %d0         | restore remainder
ret_zero:
    addi.b #'0', %d0           | turn it into a character
    jsr _putb                  | print
    addi.b #1, %d1             | increment char count
pr_ret:
    rts
 
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| print %d2 bits from %d1  
put_bin: 
    move.b #'0', %D0
    btst %D2, %D1
    jeq not1
    move.b #'1', %D0
not1:
    jsr _putb
    move.b #' ', %D0
    jsr _putb
    
    dbra %D2, put_bin
    rts
       
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| check if reset command was recieved
| returns bool in %d0
| sets Z if return code is zero
check_reset_cmd:
    clr.b %d0
    
    btst #7, (RSR)             | test if buffer full (bit 7) is set
    jeq _end_chk               | buffer empty, continue

    cmp.b #CMD_RESET, (UDR)
    jne _end_chk
    
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    jne _end_chk               | if Z is not set (gpio = 1)

    move.b #1, %d0
_end_chk:
    tst.b %d0
    rts

.section .rodata

_hexchars: .string "0123456789ABCDEF"
