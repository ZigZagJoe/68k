*-----------------------------------------------------------
* Title      :
* Written by :
* Date       :
* Description:
*-----------------------------------------------------------
    ORG    $1000
START:                  ; first instruction of program

* Put program code here

* 28 + (x-1) * 18 on 68000
* 48 + (x-1) * 26 on 68008 ?

    move.l #400000, d0
lb:
    sub.l #1, d0
    bne lb

    SIMHALT             ; halt simulator

* Put variables and constants here

    END    START        ; last line of source

*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
