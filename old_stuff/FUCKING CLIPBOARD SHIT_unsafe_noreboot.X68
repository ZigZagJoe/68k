*-----------------------------------------------------------
* Title      :
* Written by :
* Date       :
* Description:
*-----------------------------------------------------------
    ORG    $1000

UDR     EQU     $C0000
rec_buffer EQU     $C3000
 
trap_test:

    move.w D1, -(SP)
	move.w D0, -(SP)
	move.l A0, -(SP)
	
	move.l #rec_buffer, A0
	
	clr.w D0            | clear upper bits of D0 (indexing is 16 bit only)
	move.b (A0), D0
	move.b D0, D1
	addi.b #1, D0
	cmp D0, D1
	beq welp
	
	move.b (UDR), D1
    move.b D1, (2, A0, D0) | buffer[head] = ch
    
    cmp.b #$CB, D1
    beq maybe
	
	addi.b #1, (A0)	 | head+1

welp:
    move.l (SP)+,A0
	move.w (SP)+,D0
    move.w (SP)+,D1
    
	rte  				 | return from interrupt
	
maybe:
    btst #7, $0
    bne noreb
    jmp $80008
noreb:
    rte
	

START:                  ; first instruction of program

    move.l #trap_test, ($84)
    trap #1
* Put program code here

    SIMHALT             ; halt simulator

* Put variables and constants here

    END    START        ; last line of source


*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
