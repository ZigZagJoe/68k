; 68k bootstrap loader v2
; ultra simple bootloader

	ORG $80000
	
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
    move.w #255, D0          ; copy 256 words (512 bytes)
    
    move.b #$A1, (TIL311)    ; debugging message
   
cpy:
    move.w (A0)+, (A1)+      ; *A1 = *A0; A1++; A0++;
    dbra D0, cpy             ; branch if (D0-1) != -1

	move.b #$A2, (TIL311)    ; debugging message
   
    jmp $1000                ; jump to relocated code
    
reloc: 
    ; ALL CODE BEYOND THIS POINT MUST USE RELATIVE JUMPS ONLY

    move.b #$B0, (TIL311)    ; debugging message
   	
    ; initialize 68901 UART
    move.b #%10001000, (UCR) ; /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)         ; receiver enable
    move.b #1, (TSR)         ; transmitter enable

reset:
    movea #$2000, A0         ; load base address into A0
    move.b #$B1, (TIL311)    ; display greeting

loop:
    ; see if there is a byte pending
    btst #7, (RSR)           ; test if buffer full (bit 7) is set
    beq loop                 ; buffer empty, loop (Z=1)

    move.b (UDR), D0         ; read char from buffer
    move.b D0, (TIL311)      ; display on device
    
    ; check to see if the GPIO for command byte is low
    btst #0, (GPDR)          ; gpio data register - test input 0. Z=!bit
    bne wrb              	 ; gpio is 1; not a command byte
    
    cmp.b #$CB, D0			 ; is it the command to boot?
    beq boot

	cmp.b #$CF, D0			 ; is it the command to reset address?
    beq reset
    
    move.b #$BC, (TIL311)    ; not a recognized command
    bra loop
    
wrb:  
    move.b D0, (UDR)         ; echo it back to sender
    move.b D0, (A0)+         ; store to address, postincrement

    bra loop                 ; relative jump
    
boot:
    move.b #$BB, (TIL311)    ; display char for debugging
    jmp $2000                ; jump to the code loaded into RAM
    
