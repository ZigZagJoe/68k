.global init_vectors
.text
.align 2

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data reg
.set UDR, MFP_BASE + 0x2F | uart data register
.set TSR, MFP_BASE + 0x2D | transmitter status reg
.set RSR, MFP_BASE + 0x2B | receiver status reg
.set TIL311, 0xC8000      | til311 displays

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| macros

.macro sei
    and.w #0xF8FF, %SR     
.endm

.macro put_char ch
    move.b #\ch, %d0
    jsr putc
.endm

.macro put_str str
    movea.l #\str, %a0
    jsr puts
.endm

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| load all vectors with exception_handler
init_vectors:
    move.l #0, %a0
    clr.b %d0
    move.w #255, %d1
    
load_vec:
    move.l #exception_handler, (%a0)
    move.b %d0, (%a0)
    addq.l #4, %a0
    addq.b #1, %d0
    dbra %d1, load_vec
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| the reason you came here yo
exception_handler:
    movem.l %d0-%d7/%a0-%a7, -(%sp)
    
    lea (%pc), %a0
    move.l %a0, %d7
    swap %d7
    lsr.w #8, %d7
    | %d7 should contain calling vector
    
    put_str _HEAD
    move.b %d7, %d0
    jsr puthexbyte
    move.b #':', %d0
    jsr putc
    jsr put_sp
    
    move.b %d7, %d0
    jsr vec_num_to_str
    move.b (%a0)+, %d6           | number to blink
    
    movea.l %a0, %a1
   
    jsr puts
    
    movea.l %a1, %a0
    move.w #27, %d2

_lf:
    tst.b (%A1)+
    jeq _fl
    
    addq.b #1, %d2
    bra _lf
    
_fl:

    move.w #62, %d1
    sub.b %d2, %d1
    
    jsr putbash
    
    | 27 in header
    
    jsr dblnewl
    
    | dump registers
    move.l %sp, %a0
    
    | D0-D7
    move.b #0xD0, %d2
    moveq #8, %d3
    jsr dump_regs
    
    | A0-A6
    move.b #0xA0, %d2
    moveq #7, %d3
    jsr dump_regs
   
    cmp.b #0x3, %d7
    ble group0
    
    jsr trisp
    put_str _PC 
    
    | get PC off stack
    move.l 66(%SP), %D0
    jsr puthexlong
    
    cmp.b #9, %d7
    bne not_tr
      
    jsr trisp
    put_str _INST
    
    move.l 66(%SP), %a0
    move.w (%a0), %d0
 
    jsr puthexword
    jsr put_sp
    
not_tr:
    
    | flags
    move.w 64(%SP), %D1
    
    bra contp
    
    | larger exception frame for group 0 exceptions
group0:
    jsr trisp
    
    put_str _PC
    move.l 74(%SP), %D0
    jsr puthexlong
    
    put_str _ADDR
    move.l 66(%SP), %D0
    jsr puthexlong

    put_str _INST
    move.w 70(%SP), %D0
    jsr puthexword
    
    | get access type flags
    move.w 64(%SP), %D2
    
    move.l #_WRITE, %a0
    btst #4, %d2
    beq _wr
    move.l #_READ, %a0
_wr:
    jsr puts
    
    btst #3, %d2
    beq _not
    
    move.l #_INSTR, %a0
    jsr puts
_not:
    
    move.l #_USR, %a0
    btst #2, %d2
    beq _usr
    move.l #_SUPR, %a0
_usr:
    jsr puts
    
    move.l #_PGM, %a0
    btst #0, %d2
    beq _pgm
    move.l #_DATA, %a0
_pgm:
    jsr puts

    | flags
    move.w 72(%SP), %D1
    
    jsr dblnewl
    
contp: 
    | continue: print out stack pointers, flags (in D1).
    
    jsr dblsp
    put_str _usersp
    move.l %USP, %A0
    move.l %A0, %D0
    jsr puthexlong
    
    jsr dblsp
    put_str _supsp
    move.l 60(%sp), %D0
    jsr puthexlong
    
    jsr dblnewl
    
    put_str _flags    
    
    | %d1 is assumed to contain flags!
    
    move.w #15, %D2
    jsr put_bin
    
    jsr dblnewl
    move.w #62, %d1
    jsr putbash
    jsr dblnewl
     
    cmp.b #9, %d7
    beq return_eh
    
    cmp.b #47, %d7
    beq return_eh
    
| end of handler:
| loop forever, toggling between 0xEE and relevant number
wait_for_reset:
    move.b #0xEE, (TIL311)
    jsr half_sec_delay
    
    move.b %d6, (TIL311)
    jsr half_sec_delay

    bra wait_for_reset
    
return_eh:
    movem.l (%sp)+, %d0-%d7/%a0-%a7
    rte
    

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| wait for half a second, while waiting for boot
half_sec_delay:
    move.l #40000,%d0                                 
delay: 
    jsr check_boot
    sub.l #1, %d0
    jne delay
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| check if boot char recvd    
check_boot:
    btst #7, (RSR)             | test if buffer full (bit 7) is set.
    beq retu                 
    
    move.b (UDR), %D1
    cmp.b #0xCF, %D1
    bne retu
    
    btst #0, (GPDR)            | gpio data register - test input 0. Z=!bit
    bne retu                   | gpio is 1, not bootoclock
     
    jmp 0x80008
    
retu:
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| resolve a vector number to a string in %a0
vec_num_to_str:
    move.l #_VEC_USER, %a0
    cmp.b #64, %d0
    jge end
    
    move.l #_VEC_RESERVED, %a0
    cmp.b #48, %d0
    jge end
    
    move.l #_VEC_TRAP, %a0
    cmp.b #32, %d0
    jge end
     
    move.l #_VEC_AUTOVEC, %a0
    cmp.b #25, %d0
    jge end   
             
    move.l #_VEC_RESERVED, %a0
    cmp.b #16, %d0
    jge end   
    
    move.l #_VEC_UNINIT, %a0
    cmp.b #15, %d0
    jge end   
    
    move.l #_VEC_RESERVED, %a0
    cmp.b #12, %d0
    jge end 
        
    andi.w #0xF, %d0
    lsl.b #2, %d0
    move.l #vec_lookup, %a0
    
    move.l (%a0, %d0.w), %a0
end:
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| dump %d3 registers starting at %a0, newline after 4th
| reg base name in %d2

dump_regs:
    clr.b %d1
dr_l:  
    jsr trisp
    
    move.b %d1, %d0
    add.b %d2, %d0
    jsr puthexbyte
    
    put_char ':'
    
    jsr put_sp
    
    move.l (%a0)+, %d0
    jsr puthexlong
    
    addi.b #1, %d1
    
    cmp.b #4, %d1
    bne sk
    jsr put_newl
    
sk:
    cmp.b %d3, %d1
    bne dr_l
    
    jsr dblnewl
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| various quick character put routines

| put space, then %d1 bash characters
putbash: 
    jsr put_sp
    move.b #'#', %d0
    subq.b #1, %d1
_putcl:
    jsr putc
    dbra %d1, _putcl
    rts
   
| two newlines   
dblnewl:
    jsr put_newl
 
| one newline  
put_newl:
    put_char '\n'
    rts

| three spaces
trisp:
    jsr put_sp
    
| two spaces
dblsp:
    jsr put_sp
    
| one space   
put_sp:
    put_char ' '
    rts
    
||||||||||||||||||||||||||||||||||||||
| put string in %a0 
puts:
    move.b (%A0)+, %D0
    jeq done
    
    jsr putc
    bra puts
    
done:
    rts
  
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| print %d2 bits from %d1  
put_bin: 
    move.b #'0', %D0
    btst %D2, %D1
    jeq not1
    move.b #'1', %D0
not1:
    jsr putc
    move.b #' ', %D0
    jsr putc
    
    dbra %D2, put_bin
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||    
| put hex long in %d0 
puthexlong:
    move.l %D0, -(%SP)

    swap %D0
    jsr puthexword
    
    move.l (%SP), %D0
    and.l #0xFFFF, %D0
    jsr puthexword

    move.l (%SP)+, %D0
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||    
| put hex word in %d0         
puthexword:
    move.w %D0, -(%SP)

    lsr.w #8, %D0
    jsr puthexbyte
    
    move.w (%SP), %D0
    and.w #0xFF, %D0
    jsr puthexbyte

    move.w (%SP)+, %D0
    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put hex byte in %d0 
puthexbyte:
    movem.l %A0/%D0, -(%SP)        | save regs
    
    movea.l #_hexchars, %A0
    
    lsr #4, %D0                    | shift top 4 bits over
    and.w #0xF, %D0
    move.b (%A0, %D0.W), %D0       | look up char
    jsr putc
    
    move.w (2, %SP), %D0           | restore D0 from stack    
    and.w #0xF, %D0                | take bottom 4 bits
    move.b (%A0, %D0.W), %D0       | look up char
    jsr putc
    
    movem.l (%SP)+, %A0/%D0        | restore regs

    rts
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| put char in %d0    
putc:
    btst #7, (TSR)                 | test buffer empty (bit 7)
    beq putc                       | Z=1 (bit = 0) ? branch
    move.b %D0, (UDR)              | write char to buffer
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| Strings

.section .rodata 

_PC: .string "PC: "
_usersp: .string "USP: "
_supsp: .string "SSP: "
_flags: .string "    T   S     IMASK       X N Z V C\n    "
_ADDR: .string " ADDR: "
_INST: .string " INST: "
_READ: .string "  Read "
_WRITE: .string "  Write "
_INSTR: .string "instruction "
_USR: .string "user "
_SUPR: .string "supervisor "
_PGM: .string "program "
_DATA: .string "data "
_HEAD: .string "\n ########## Exception $"
_EX: .string "Unknown"

| vectors
_VEC_00:        .byte 0xE0;.string "{unknown}"
_VEC_01:        .byte 0xE0;.string "{unknown}"
_VEC_02:        .byte 0xBE;.string "Bus Error"
_VEC_03:        .byte 0xAE;.string "Address Error"
_VEC_04:        .byte 0x11;.string "Illegal Instruction"
_VEC_05:        .byte 0xD0;.string "Divide by 0"
_VEC_06:        .byte 0xCC;.string "CHK"
_VEC_07:        .byte 0x7B;.string "TRAPV"
_VEC_08:        .byte 0xA1;.string "Privilege Violation"
_VEC_09:        .byte 0x7E;.string "Trace"
_VEC_10:        .byte 0xE0;.string "Line 1010"
_VEC_11:        .byte 0xE1;.string "Line 1111"
_VEC_USER:      .byte 0xAA;.string "User Vector"
_VEC_TRAP:      .byte 0x7A;.string "Trap"
_VEC_AUTOVEC:   .byte 0xA7;.string "Autovectored"
_VEC_RESERVED:  .byte 0x5E;.string "{reserved}"
_VEC_UNINIT:    .byte 0x1A;.string "Uninitialized ISR"

_hexchars: .ascii "0123456789ABCDEF"

.align 2
vec_lookup:
    .long _VEC_00
    .long _VEC_01
    .long _VEC_02
    .long _VEC_03
    .long _VEC_04
    .long _VEC_05
    .long _VEC_06
    .long _VEC_07
    .long _VEC_08
    .long _VEC_09
    .long _VEC_10
    .long _VEC_11
