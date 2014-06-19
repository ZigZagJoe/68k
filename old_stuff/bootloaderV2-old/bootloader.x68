; 68k bootstrap loader v1
; ultra simple bootloader

	ORG $2000
	
; MFP registers
IO_BASE     EQU     $C0000
MFP_BASE    EQU     IO_BASE+$0
GPDR        EQU     MFP_BASE+$1
DDR         EQU     MFP_BASE+$05
UCR         EQU     MFP_BASE+$29
RSR         EQU     MFP_BASE+$2B
TSR         EQU     MFP_BASE+$2D
UDR         EQU     MFP_BASE+$2F
TIL311      EQU     IO_BASE+$8000
  
;SPB  dc.l    $80000          ; initial stack pointer. grows down!
;PCB  dc.l    $80008          ; initial program counter 
    
    move.b #$A0, (TIL311)    ; display greeting
    
    ; reset UART and any other devices attached to /RESET
    reset
    
    move.b #$2, DDR
	move.b #$2, GPDR
	
    ; relocate into RAM
    movea.l #reloc, A0       ; source address
    movea #$1000, A1         ; load relocation address into A1
    move.l #510, D0          ; copy 512 words (1024 bytes)
    
    move.b #$A1, (TIL311)    ; debugging message
   
cpy:
    move.w (A0)+, (A1)+      ; *A1++ = *A0++
    dbra D0, cpy             ; branch if (D0-1) != -1

	move.b #$A2, (TIL311)    ; debugging message
   
    jmp $1000                ; jump to relocated code
    
    ; ####### RAM CODE #######
reloc: 
	ORG $1000

    move.b #$B1, (TIL311)    ; debugging message
   	
    ; initialize 68901 UART
    move.b #%10001000, (UCR) ; /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)         ; receiver enable
    move.b #1, (TSR)         ; transmitter enable

	move.w #256, D0
wait:						 ; wait for uart to find its dick
	dbra D0, wait

reset:   
    move.b #$B2, (TIL311)    ; display greeting
    
    move.l #$F00DBABE, D0
    jsr putlong
        
    move.b #0, D7			 ; flash write mode = 0
	movea #$2000, A6         ; load base address into A6

	; ####### MAIN LOOP #######
loop:
    bsr getb 				 ; read a byte
   	
    move.b D0, (TIL311)      ; display on device
    
    ; check to see if the GPIO for command byte is low
    btst #0, (GPDR)          ; gpio data register - test input 0. Z=!bit
    beq cmd              	 ; gpio is 1, not a command
    
   	cmp.b #1, D7
   	beq flwr
   	
	move.b D0, (A6)			 ; write byte into RAM
	move.b (A6)+, D0		 ; read it back and increment ptr
	jsr putb			     ; echo it back
	
    bra loop                 ; begin again
    
flwr:
	; flash write arming sequence
    move.b #$AA, 0x85555
    move.b #$55, 0x82AAA
    move.b #$A0, 0x85555
    move.b D0, (A6)			 ; write byte into flash
	
	move.l #4, D0
	
flwait:
	sub.l #1, D0
    bne flwait
    
    move.b (A6)+, D0		 ; read it back and increment ptr
	jsr putb			     ; echo it back
	
	bra loop                 ; begin again	
	; ####### COMMANDS #######
cmd:
	cmp.b #$CA, D0
    beq setaddr
    
    cmp.b #$CB, D0
    beq boot

	cmp.b #$CF, D0
    beq reset
    
    cmp.b #$F1, D0
   	beq setflash
   	
   	cmp.b #$F0, D0
   	beq notflash
   	
    cmp.b #$E5, D0
    beq sectclr
    
    move.b #$BC, (TIL311)
	bra loop

sectclr:
    clr.l D0
    jsr getb
    lsl.l #7, D0
    lsl.l #5, D0
    
    movea.l D0, A0
    
    ; erase arming sequence
    move.b #$AA, 0x85555
    move.b #$55, 0x82AAA
    move.b #$80, 0x85555
    move.b #$AA, 0x85555
    move.b #$55, 0x82AAA
    move.b #$30, (A0)  
    
    ; wait for erase
	move.l #7500, D0
sectorwait:				
	sub.l #1, D0
    bne sectorwait
    
    bra loop
    
setflash:
	move.b #1, D7
	move.b #$A1, D0
	jsr putb
	
	bra loop
	
notflash:
	move.b #0, D7
	move.b #$A0, D0
	jsr putb
	
	bra loop	

setaddr: 
	; load an arbitrary 3 byte address in, MSB first
	
	clr.l D1
	
	; load 3 bytes of address
	bsr getb
	move.b D0, D1
	lsl.l #8, D1
	bsr getb
	move.b D0, D1
	lsl.l #8, D1
	bsr getb
	move.b D0, D1
	
	movea.l D1, A6
	move.l D1, D0
	
	jsr putlong				 ; write address (NOTE: 4 bytes)
	
	bra loop
	
boot:
 	move.b #$BB, (TIL311)    ; display char for debugging
    jmp $2000                ; jump to the code loaded into RAM
    
    ; ####### FUNCTIONS #######
	; read byte subroutine
getb:
 	btst #7, (RSR)           ; test if buffer full (bit 7) is set.
    beq getb                 ; Z=1 (bit = 0) ? branch
	move.b (UDR), D0         ; read char from buffer
	rts
	
	; write byte subroutine
putb:
	btst #7, (TSR)           ; test buffer empty (bit 7)
    beq putb                 ; Z=1 (bit = 0) ? branch
	move.b D0, (UDR)		 ; write char to buffer
	rts

	; bit 7 of TSR
	; 0 = register full
	; 1 = ready for more 
	; btst Z = !bit

putlong:
	move.l D0, -(SP)

	swap D0
	jsr putword
	
	move.l (SP), D0
	and.l #$FFFF, D0
	jsr putword

	move.l (SP)+, D0
	rts
	
putword:
	move.w D0, -(SP)

	lsr.w #8, D0
	jsr putb
	
	move.w (SP), D0
	and.w #$FF, D0
	jsr putb

	move.w (SP)+, D0
	rts

