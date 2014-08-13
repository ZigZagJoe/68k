.text
.align 2
.global compute_crc

## C binding
compute_crc:
    move.l 4(%sp), %a0
    move.l 8(%sp), %d1
   
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| compute the crc of %d1 bytes from %a0 
_compute_crc:
    move.w %d2, -(%sp)
    
    move.l #0xDEADC0DE, %d0
       
crc_l:
    move.b (%a0)+, %d2
    eor.b %d2, %d0
    rol.l #1, %d0
    subq.l #1, %d1
    bne.s crc_l
    
    move.w (%sp)+, %d2
    
    rts

