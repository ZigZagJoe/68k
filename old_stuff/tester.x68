; 68k serial tester

	ORG $2000

; MFP registers
IO_BASE     EQU     $C0000
MFP_BASE    EQU     IO_BASE+$0
GPDR        EQU     MFP_BASE+$1
UCR         EQU     MFP_BASE+$29
RSR         EQU     MFP_BASE+$2B
TSR         EQU     MFP_BASE+$2D
UDR         EQU     MFP_BASE+$2F
TIL311      EQU     IO_BASE+$8000
  
;SPB  dc.l    $80000          ; initial stack pointer. grows down!
;PCB  dc.l    $80008          ; initial program counter 
    
    move.b #$AA, (TIL311)    ; display greeting
    
    ; reset UART and any other devices attached to /RESET
    reset
    
    move.b #%10001000, (UCR) ; /16, 8 bits, 1 stop bit, no parity
    move.b #1, (RSR)         ; receiver enable
    move.b #1, (TSR)         ; transmitter enable

    move.b #$BB, (TIL311)    ; display greeting
    
reprint:  
	movea.l #string, A0
	 
printstr: 
	move.b (A0)+, D0
	cmp.b #0, D0
	beq reprint
	
	move.b #'H', (UDR)
	
sendwait: btst #7, (TSR)       ; test if buffer full (bit 7) is set
    beq sendwait               ; buffer empty, loop (Z=1)
	
		move.b #'I', (UDR)
	
sendwait2: btst #7, (TSR)       ; test if buffer full (bit 7) is set
    beq sendwait2               ; buffer empty, loop (Z=1)
	
	bra printstr
	

string dc.b "Begin. This is a string. Hi. The quick brown fox jumps over the lazy dog. But what they don't tell you is the fox is a mean sumbitch and a card carrying communist!#$%&()*+,-./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.", 255, 178, 0
