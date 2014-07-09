| 68k global bootloader code
.text
.align 2

.set TIL311, 0xC8000      | til311 displays

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| function imports/exports
.global init_vectors

.extern _print_dec
.extern _puthexlong
.extern _puthexword
.extern _puthexbyte

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| macros

.macro sei
    and.w #0xF8FF, %SR     
.endm

.macro put_char ch
    moveq #\ch, %d0
    bsr _putb
.endm

.macro put_str str
    lea (%pc,\str), %a0
    bsr _puts
.endm

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| load all vectors with exception_handler
init_vectors:
    moveq.l #0, %d0
    move.l %d0, %a0
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
    
    move.w #24, %d2              | char count

    put_str _HEAD
    
    move.b %d7, %d0
    andi.l #0xFF, %d0
    
    bsr _print_dec
    
    add.b %d0, %d2
    
    move.b #':', %d0
    bsr _putb
    bsr put_sp
    
    move.b %d7, %d0
    bsr vec_num_to_str
    move.b (%a0)+, %d6           | number to blink
    
    movea.l %a0, %a1
   
    bsr _puts
    
    movea.l %a1, %a0
    
_lf:
    tst.b (%A1)+
    jeq _fl
    
    addq.b #1, %d2
    bra _lf
    
_fl:

    move.w #62, %d1
    sub.b %d2, %d1
    
    bsr putbash
    bsr dblnewl
    
    | dump registers
    move.l %sp, %a0
    
    | D0-D7
    move.b #0xD0, %d2
    moveq #8, %d3
    bsr dump_regs
    
    | A0-A6
    move.b #0xA0, %d2
    moveq #7, %d3
    bsr dump_regs
   
    cmp.b #0x3, %d7
    ble group0
    
    bsr trisp
    put_str _PC 
    
    | get PC off stack
    move.l 66(%SP), %D0
    bsr _puthexlong
    
    cmp.b #9, %d7
    bne not_tr
      
    bsr trisp
    put_str _INST
    
    move.l 66(%SP), %a0
    move.w (%a0), %d0
 
    bsr _puthexword
    bsr put_sp
    
not_tr:
    
    | flags
    move.w 64(%SP), %D1
    
    bra contp
    
    | larger exception frame for group 0 exceptions
group0:
    bsr trisp
    
    put_str _PC
    move.l 74(%SP), %D0
    bsr _puthexlong
    
    put_str _ADDR
    move.l 66(%SP), %D0
    bsr _puthexlong

    put_str _INST
    move.w 70(%SP), %D0
    bsr _puthexword
    
    | get access type flags
    move.w 64(%SP), %D2
    
    lea (%pc, _WRITE), %a0
    btst #4, %d2
    beq _wr
    lea (%pc, _READ), %a0
_wr:
    bsr _puts
    
    btst #3, %d2
    beq _not
    
    lea (%pc, _INSTR), %a0
    bsr _puts
_not:
    
    lea (%pc, _USR), %a0
    btst #2, %d2
    beq _usr
    lea (%pc, _SUPR), %a0
_usr:
    bsr _puts
    
    lea (%pc, _PGM), %a0
    btst #0, %d2
    beq _pgm
    lea (%pc, _DATA), %a0
_pgm:
    bsr _puts

    | flags
    move.w 72(%SP), %D1
    
    bsr dblnewl
    
contp: 
    | continue: print out stack pointers, flags (in D1).
    
    bsr dblsp
    put_str _usersp
    move.l %USP, %A0
    move.l %A0, %D0
    bsr _puthexlong
    
    bsr.s dblsp
    put_str _supsp
    move.l 60(%sp), %D0
    bsr _puthexlong
    
    bsr.s dblnewl
    
    put_str _flags    
    
    | %d1 is assumed to contain flags!
    
    move.w #15, %D2
    bsr put_bin
    
    bsr.s dblnewl
    move.w #62, %d1
    bsr putbash
    bsr.s dblnewl
     
    cmp.b #9, %d7
    beq return_eh
    
    cmp.b #47, %d7
    beq return_eh
    
| end of handler:
| loop forever, toggling between 0xEE and relevant number
wait_for_reset:
    move.b #0xEE, (TIL311)
    bsr.s half_sec_delay
    
    move.b %d6, (TIL311)
    bsr.s half_sec_delay

    bra wait_for_reset
    
return_eh:
    movem.l (%sp)+, %d0-%d7/%a0-%a7
    rte
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| various quick character put routines
  
| two newlines   
dblnewl:
    bsr put_newl
 
| one newline  
put_newl:
    put_char '\n'
    rts

| three spaces
trisp:
    bsr.s put_sp
    
| two spaces
dblsp:
    bsr put_sp
    
| one space   
put_sp:
    put_char ' '
    rts

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| dump %d3 registers starting at %a0, newline after 4th
| reg base name in %d2

dump_regs:
    clr.b %d1
dr_l:  
    bsr.s trisp
    
    move.b %d1, %d0
    add.b %d2, %d0
    bsr _puthexbyte
    
    put_char ':'
    
    bsr.s put_sp
    
    move.l (%a0)+, %d0
    bsr _puthexlong
    
    addi.b #1, %d1
    
    cmp.b #4, %d1
    bne sk
    bsr put_newl
    
sk:
    cmp.b %d3, %d1
    bne dr_l
    
    bsr.s dblnewl
    rts

| put space, then %d1 bash characters
putbash: 
    bsr.s put_sp
    move.b #'#', %d0
    subq.b #1, %d1
__putbl:
    bsr _putb
    dbra %d1, __putbl
    rts
 
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| wait for half a second, while waiting for boot
half_sec_delay:
    move.l #40000,%d2                             
delay: 
    bsr check_reset_cmd
    bne do_reset
    
    subq.l #1, %d2
    jne delay
    rts

do_reset:
    jmp 0x80008
    
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| resolve a vector number to a string in %a0
vec_num_to_str:
    lea (%pc,_VEC_USER), %a0
    cmp.b #64, %d0
    jge vec_end
    
    lea (%pc,_VEC_RESERVED), %a0
    cmp.b #48, %d0
    jge vec_end
    
    lea (%pc,_VEC_TRAP), %a0
    cmp.b #32, %d0
    jge vec_end
     
    lea (%pc,_VEC_AUTOVEC), %a0
    cmp.b #25, %d0
    jge vec_end   
             
    lea (%pc,_VEC_RESERVED), %a0
    cmp.b #16, %d0
    jge vec_end   
    
    lea (%pc,_VEC_UNINIT), %a0
    cmp.b #15, %d0
    jge vec_end   
    
    lea (%pc,_VEC_RESERVED), %a0
    cmp.b #12, %d0
    jge vec_end 
      
    | clear %d0 upper bits  
    andi.w #0xF, %d0
    
    | quick %d0 * 2 (size of word)
    add.b %d0, %d0

    | read offset from vec_lookup[%d0] (pc-relative addressing)
    move.w (vec_lookup, %pc, %d0.W), %d0
    
    | load address relative to %pc into %a0
    | i do not know why the two is required.
    lea (2, %pc, %d0.W), %a0
    
vec_end:
    rts
    
| vector lookup table
vec_lookup:
    .word _VEC_00 - vec_end
    .word _VEC_00 - vec_end
    .word _VEC_02 - vec_end
    .word _VEC_03 - vec_end
    .word _VEC_04 - vec_end
    .word _VEC_05 - vec_end
    .word _VEC_06 - vec_end
    .word _VEC_07 - vec_end
    .word _VEC_08 - vec_end
    .word _VEC_09 - vec_end
    .word _VEC_10 - vec_end
    .word _VEC_11 - vec_end

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
_HEAD: .string "\n ########## Exception "
_EX: .string "Unknown"

| Vector strings. First byte is what to flash on TIL311.
_VEC_00:        .byte 0xE0;.string "{unknown}"
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
_VEC_USER:      .byte 0xAA;.string "User"
_VEC_TRAP:      .byte 0x7A;.string "Trap"
_VEC_AUTOVEC:   .byte 0xA7;.string "Autovector"
_VEC_RESERVED:  .byte 0x5E;.string "{reserved}"
_VEC_UNINIT:    .byte 0x1A;.string "Uninitialized ISR"
