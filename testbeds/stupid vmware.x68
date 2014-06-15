    ORG $1000
    
active_task_regist dc.b 0,0,4,0

trap:
    move.l A0, -(SP) 				 ; save A0
	move.l (active_task_regist), A0     
	
	movem.l D0-D7,   (A0)             ; D0-D7   32 bytes
	move.l (SP)+,   32(A0)             ; A0       4 bytes
	movem.l A1-A6, 36(A0)             ; A1-A6   24 bytes
	
	move.l USP, A1
	move.l A1,     60(A0)             ; SP       4 bytes
	move.l 2(SP),  64(A0)             ; PC       4 bytes
	move.l (SP),   68(A0)             ; flags    2 bytes
	
    rte

    
START
    move.l #trap, ($80)
    trap #0
	
	SIMHALT
    END START
    


*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
