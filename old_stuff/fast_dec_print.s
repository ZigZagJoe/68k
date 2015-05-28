.text
.global print_dec

print_dec:
    move.l 4(%sp), %d0
    
    tst.l %d0
    
    jpl not_neg
    move.l #'-', -(%sp)
    jbsr putc                  | print
    addq.l #4, %sp
    
    move.l 4(%sp), %d0
    neg.l %d0
    
not_neg:
    clr.b %d1                   | character count
    
    tst.l %d0                   | check for zero
    jeq rz
    
    jbsr pr_dec_rec             | begin recursive print
    
dec_r:
    move.b %d1, %d0             | set up return value
    rts
    
rz: 
    jbsr ret_zero
    jra dec_r

| recursive decimal print routine
pr_dec_rec:
    divu #10, %d0
    swap %d0
    move.w %d0, -(%sp)         | save remainder
    clr.w %d0                  | clear remainder
    swap %d0                   | back to normal
    jeq no_more_digits         | swap sets Z if reg is zero - how cool!
    
    jbsr pr_dec_rec            | get next digit
    
no_more_digits:
    move.w (%sp)+, %d0         | restore remainder
    
ret_zero:
    addi.b #'0', %d0           | turn it into a character
    
    move.l %d0, -(%sp)
    jbsr putc                  | print
    addq.l #4, %sp
    
    addq.b #1, %d1             | increment char count
pr_ret:
    rts
