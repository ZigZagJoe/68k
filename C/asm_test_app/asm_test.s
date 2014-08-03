.align 2
.text
.global main

.extern putb
.extern putw
.extern putl

.extern puts

.extern print_dec
.extern puthexlong
.extern puthexword
.extern puthexbyte

.extern getw
.extern getl
.extern getb

.set TIL311, 0xC8000      | til311 displays


||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| main function
main:
  
    move.b #0xDE, %d0
    trap #15
    move.b %d0, -(%sp)
    trap #15
    move.w (%sp)+, %d1
    trap #15
   
    jsr return_to_loader
