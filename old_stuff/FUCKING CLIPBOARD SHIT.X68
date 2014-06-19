*-----------------------------------------------------------
* Title      :
* Written by :
* Date       :
* Description:
*-----------------------------------------------------------
 IO_BASE     EQU     $C0000
 
    ORG    $1000
START:                  ; first instruction of program

    	move.w %D0, -(%SP)
	move.l %A0, -(%SP)
	
	move.l #rec_buffer, %A0
	
	move.b (%A0), %D0
	move.b (UDR), (2, %A0, %D0) | buffer[head] = ch
	
	addi.b #1, (%A0)	 | head+1

    move.l (%SP)+,%A0
	move.w (%SP)+,%D0

	rte  				 | return from interrupt
* Put program code here

    SIMHALT             ; halt simulator

* Put variables and constants here

    END    START        ; last line of source

*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
