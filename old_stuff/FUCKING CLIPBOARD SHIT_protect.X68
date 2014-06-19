*-----------------------------------------------------------
* Title      :
* Written by :
* Date       :
* Description:
*-----------------------------------------------------------
    ORG    $1000

GPDR     EQU     $4124
UDR     EQU     $C0000
rec_buffer EQU     $C3000
 
trap_test:

	move.w D2, -(SP)
	move.w D1, -(SP)
	move.w D0, -(SP)
	move.l A0, -(SP)
	
	move.b (UDR), D2
	cmp.b #$CF, D2
	
	bne noreb
	btst #0, (GPDR)
	bne noreb
	jmp $80008			 | return to bootloader
	
noreb:
	move.l #rec_buffer, A0
	clr.w D0
	
	move.b (A0), D0
	move.b D0, D1
	
	addi.b #1, D1
	
	cmp.b (1,A0), D1
	beq drop_byte
	
	move.b (UDR), (2, A0, D0) | buffer[head] = ch
	
	move.b D1, (A0)	 | head+1

drop_byte:

    move.l (SP)+,A0
	move.w (SP)+,D0
	move.w (SP)+,D1
	move.w (SP)+,D2
	
	rte  				 | return from interrupt
	

START:                  ; first instruction of program

    move.l #trap_test, ($84)
    move.b #1, GPDR
    
    trap #1
* Put program code here

    SIMHALT             ; halt simulator

* Put variables and constants here

    END    START        ; last line of source



*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
