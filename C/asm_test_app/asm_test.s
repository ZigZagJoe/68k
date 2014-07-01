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

main:

    divu #0, %d0

    move.w #0, %sr
    move.w #0, %sr

    |trap #1
    |jmp 0xFFFFFF
    move.b #0, 0xFFFFFF

spin: bra spin


