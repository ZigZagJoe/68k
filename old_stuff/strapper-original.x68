; 68k bootstrap loader v1
; ultra simple bootloader

; MFP registers
IO_BASE     EQU     $C0000
MFP_BASE    EQU     IO_BASE+$0
GPDR        EQU     MFP_BASE+$1
UCR         EQU     MFP_BASE+$29
RSR         EQU     MFP_BASE+$2B
TSR         EQU     MFP_BASE+$2D
UDR         EQU     MFP_BASE+$2F
TIL311      EQU     IO_BASE+$8000
  
SPB  dc.l    $80000          ; initial stack pointer. grows down!
PCB  dc.l    $80008          ; initial program counter 
    
    move.b #$A0, (TIL311)    ; display greeting
   
    ; reset UART and any other devices attached to /RESET
    reset
    
    ; relocate into RAM
    movea.l #reloc, A0       ; source address
    movea #$1000, A1         ; load relocation address into A1
    move.l #255, D0          ; copy 256 words (512 bytes)
    
    move.b #$A1, (TIL311)    ; debugging message
   
cpy:
    move.w (A0)+, (A1)+      ; *A1 = *A0; A1++; A0++;
    dbra D0, cpy             ; branch if (D0-1) != -1

    jmp $1000                ; jump to relocated code
    
    move.b #$A2, (TIL311); debugging message
   
reloc: 
    ; ALL CODE BEYOND THIS POINT MUST USE RELATIVE JUMPS ONLY

    move.b #$B0, (TIL311)    ; debugging message
   	
    ; initialize 68901 UART
    move.b #%10001000, (UCR) ; /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)         ; receiver enable
    move.b #1, (TSR)         ; transmitter enable

    movea #$2000, A0         ; load base address into A0
    
    move.b #$B1, (TIL311)    ; display greeting
    move.b #'R', (UDR)       ; send 'R' - we're ready
    
loop:
    ; check to see if the GPIO telling us to boot is low
    btst #0, (GPDR)          ; gpio data register - test input 0. Z=!bit
    beq getbyte              ; gpio is low, don't boot (Z=1)
    move.b #$BB, (TIL311)    ; display char for debugging
    jmp $2000                ; jump to the code loaded into RAM
    
getbyte:
    ; see if there is a byte pending
    btst #7, (RSR)           ; test if buffer full (bit 7) is set
    beq loop                 ; buffer empty, loop (Z=1)

    move.b (UDR), D0         ; read char from buffer
    move.b D0, (UDR)         ; echo it back to sender
    move.b D0, (TIL311)      ; display on device
    move.b D0, (A0)+         ; store to address, postincrement

    bra loop                 ; relative jump
