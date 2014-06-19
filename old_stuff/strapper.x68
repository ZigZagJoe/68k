; 68k bootstrap loader v1
; ultra simple bootloader

    ORG     $80000           ; code orgin - beginning of flash
    
; MFP registers
MFP_BASE    EQU     $C0000
GPDR        EQU     MFP_BASE+$1
UCR         EQU     MFP_BASE+$29
RSR         EQU     MFP_BASE+$2B
TSR         EQU     MFP_BASE+$2D
UDR         EQU     MFP_BASE+$2F
  
SP  dc.l    $60000           ; initial stack pointer
PC  dc.l    $80008           ; initial program counter 

    ; initialize 68901 UART
    move.b #%10001000, (UCR) ; /16, 8 bits, 1 stop bit, no parity
    move.b #$1, (RSR)        ; receiver enable
    move.b #$1, (TSR)        ; transmitter enable
    
    movea #$1000, A0         ; load base address into A0

loop:
    ; check to see if the GPIO telling us to boot is low
    btst #0, (GPDR)          ; gpio data register - test input 0. Z=!bit
    beq getbyte              ; gpio is low, don't boot (Z=1)
    jmp $1000                ; jump to the code loaded into RAM
    
getbyte:
    ; see if there is a byte pending
    btst #7, (RSR)           ; test if buffer full (bit 7) is set
    beq loop                 ; buffer empty, loop (Z=1)

    move.b (UDR), D0         ; read char from buffer
    move.b D0, (UDR)         ; echo it back to sender
    move.b D0, (A0)+         ; store to address, postincrement

    jmp loop
