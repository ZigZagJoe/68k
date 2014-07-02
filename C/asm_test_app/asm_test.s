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


    trap #15
    
    ori #0x8000, %sr
    
    
    move.l #0xDEADBEEF, %d0
    move.l #0xDBEEFEAD, %d1
    move.l #0xCAFEBABE, %d2
    move.l #0x8BADF00D, %d3
    move.l #0xDEADC0DE, %d4
    move.l #0xBEEFCAFE, %d5
    move.l #0xC0DEBAC0, %d6
    move.l #0xB0771011, %d7
    
    andi #0x7FFF, %sr
    
sp: bra sp

    move.w #0, %sr
    move.w #0, %sr

    |trap #1
    |jmp 0xFFFFFF
    move.b #0, 0xFFFFFF

spin: bra spin


