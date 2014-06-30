||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| 68k run from flash test program

.text
.align 2
.global _start

| display a constant byte
.macro TILDBG byte
     move.b #0x\byte, (TIL311)
.endm    

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

| boot magic, required for bootloader to run this from flash
| it translates into exg %d0, %d1; exg %d1, %d0;
| does absolutely nothing and thus would never be used normally

.long 0xc141c340

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| entry point
_start:
    clr.b %d1
  
_inc:
    | increment counter
    addq.b #1, %d1  
    move.b %d1, (TIL311)
    
    | delay for a little bit
    move.w #0xFFFF, %d0
dl: dbra %d0, dl
    
    bra _inc    

# EOF
