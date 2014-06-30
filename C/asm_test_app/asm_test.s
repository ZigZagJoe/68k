.align 2
.text
.global main


.set TIL311,        0xC8000

main:
    move.b #0xFE, (TIL311)


spin: bra spin
