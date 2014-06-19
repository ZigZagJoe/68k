*-----------------------------------------------------------
* Title      :
* Written by :
* Date       :
* Description:
*-----------------------------------------------------------

MFP_BASE    EQU     $C0000
GPDR        EQU     MFP_BASE + $1
UCR         EQU     MFP_BASE + $29
RSR         EQU     MFP_BASE + $2B
TSR         EQU     MFP_BASE + $2D
UDR         EQU     MFP_BASE + $2F
  

    ORG     $80000  ; code orgin - beginning of flash
    
SP  dc.l    $60000  ; initial stack pointer
PC  dc.l    $80008  ; initial program counter 

	;Initialize 68901 UART (set /16, enable receiver, transmitter, etc.)

	movea #$1000, A0        ; load base address into A0

loop:

	btst #0, GPDR         ; gpio data register - test input 0
	bne getbyte            ; bit 0 not set
	jmp $1000            ; jump to the code loaded into RAM

getbyte:

	btst #7, RSR            ; test if BF (buffer full) is set - bit 7
	bne loop            ; bit 7 not set

	move.b UDR, D0        ; read char from buffer
	move.b D0, UDR        ; echo it back to sender
	move.b D0, (A0)+        ; store to address, postincrement

	jmp loop



*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
