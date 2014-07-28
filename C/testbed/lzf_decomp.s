.text
.align 2
.global lzf_decompress

/* Compressed format

    There are two kinds of structures in LZF: literal runs and back
    references. The length of a literal run is encoded as L - 1, as it must
    contain at least one byte.  Literals are encoded as follows:

    000LLLLL <L+1 bytes>

    Back references are encoded as follows.  The smallest possible encoded
    length value is 1, as otherwise the control byte would be recognized as
    a literal run.  Since at least three bytes must match for a back reference
    to be inserted, the length is encoded as L - 2 instead of L - 1.  The
    offset (distance to the desired data in the output buffer) is encoded as
    o - 1, as all offsets are at least 1.  The binary format is:

    LLLooooo oooooooo           for backrefs of real length < 9   (1 <= L < 7)
    111ooooo LLLLLLLL oooooooo  for backrefs of real length >= 9  (L > 7)  
*/

lzf_decompress:

| init %fp & save regs
    link.w %fp, #0
    movem.l %d2/%a2-%a4, -(%sp)
    
| arguments
    move.l 16(%fp), %d2
    move.l 8(%fp), %a1          | a1 <in_ptr>
    move.l %d2, %a3             | a3 <out_ptr>
    move.l %a1, %a4       
    add.l 12(%fp), %a4          | a4 <in_end>
    
loop_head:
    clr.w %d0
    move.b (%a1)+, %d0          | read ctrl byte
    
    cmp.b #31, %d0
    jbhi backref                | if any of b7-b5 is set, then backref
    
    | copy (ctrl+1) literal bytes
literal_cpy:
    move.b (%a1)+, (%a3)+       | *<out_ptr>++ = *<in_ptr>++
    dbra %d0, literal_cpy
    
    jbra loop_chk
    
backref:
    move.w %d0, %d1
    lsr.b #5, %d1               | %d1 <len> = b7-b5 of <ctrl>
    
    cmp.b #7, %d1              
    jbne not_long_fmt 
    
    | if upper 3 bytes are all 1, then read another byte of length
    
    move.b (%a1)+, %d1          | %d1 <len> = *in_ptr++
    addq.w #7, %d1              | %d1 <len> += 7
    
not_long_fmt:
    lsl.b #3, %d0               | %d0 = -------- AAAAA000
    lsl.w #5, %d0               | %d0 = 000AAAAA 00000000
    move.b (%a1)+, %d0          | %d0 = 000AAAAA BBBBBBBB
    
    lea (-1, %a3), %a2          | %a2 <ref_ptr>  = %a3 <out_ptr> - 1
    
    sub.w %d0, %a2              | %a2 <ref_ptr> -= %d0 <backref_dist>
    
    addq.w #1, %d1              | <len> += 1
    
backref_copy:
    move.b (%a2)+, (%a3)+       | *<out_ptr>++ = *<ref_ptr>++
    dbra %d1, backref_copy
    
loop_chk:
    cmp.l %a1, %a4              | while (in_ptr != in_end)
    jbhi loop_head

    move.l %a3, %d0
    sub.l %d2, %d0              | return difference in output pointer 

| cleanup    
    movm.l (%sp)+, %d2/%a2-%a4
    unlk %fp
    rts
