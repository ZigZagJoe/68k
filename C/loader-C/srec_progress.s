.text
.align 2

||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| subroutine to display progress in decimal percent on TIL311 displays
| called by parse_srec.c

.set TIL311, 0xC8000           | TIL311 address

.global srec_progress

.extern srec_pos
.extern perc_interv
.extern base_per

srec_progress:
    move.l (srec_pos), %d0
    move.w (perc_interv), %d1
    
    divu %d1, %d0             | get percentage
    
    swap %d0                  | remove quotient
    clr.w %d0
    swap %d0
    
    lsr.b #1, %d0             | per / 2 (remember, we parse s-rec twice)
    add.b (base_per), %d0     | add base factor
    
    divu #10, %d0             | d0 = 0002 0007
    
    lsl.b #4, %d0             | d1 = 0x70
    move.b %d0, %d1 
    swap %d0
    or.b %d0, %d1             | d1 = 0x72
    
    move.b %d1, (TIL311)
    
    move.b %d1, %d0           | put on serial port
    jbsr _putb

    rts

