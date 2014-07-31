.text
.align 2
.global lzf_inflate

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

    LLLooooo oooooooo           for backrefs of real length < 9   (1 <= L < 7)   len = L + 2
    111ooooo LLLLLLLL oooooooo  for backrefs of real length >= 9  (L > 7)        len = L + 2 + 7
*/

# int lzf_inflate(const void* ibuf, unsigned int ilen, void* obuf)
lzf_inflate:

| save regs
    movem.l %a2-%a3, -(%sp)
    
| arguments    8 = size of saved registers
    move.l ( 4+8,%sp), %a1      | a1 <in_ptr>       
    move.l (12+8,%sp), %a2      | a2 <out_ptr>
    move.l ( 8+8,%sp), %a3      | <ilen>
    add.l %a1, %a3              | a3 <in_end> = in_ptr + ilen
   
loop_head:
    clr.w %d0
    move.b (%a1)+, %d0          | read ctrl byte
    
    cmpi.b #31, %d0
    jbhi backref                | if any of b7-b5 is set, then backref
    
| copy (%d0+1) literal bytes from in_ptr to out_ptr
lit_cpy:
    move.b (%a1)+, (%a2)+       | *<out_ptr>++ = *<in_ptr>++
    dbra %d0, lit_cpy
    
    jbra loop_chk
    
backref:
    move.w %d0, %d1
    lsr.b #5, %d1               | %d1 <len> = b7-b5 of <ctrl>
    
    cmpi.b #7, %d1              
    jbne not_long_fmt           | if len = 7, read another byte for len
    
    move.b (%a1)+, %d1          | %d1 <len> = *in_ptr++
    addq.w #7, %d1              | %d1 <len> += 7
    
not_long_fmt:
    lsl.b #3, %d0               | %d0 = 00000000 AAAAA000
    lsl.w #5, %d0               | %d0 = 000AAAAA 00000000
    move.b (%a1)+, %d0          | %d0 = 000AAAAA BBBBBBBB
    
    lea (-1, %a2), %a0          | %a0 <ref_ptr>  = %a2 <out_ptr> - 1
    
    sub.w %d0, %a0              | %a0 <ref_ptr> -= %d0 <backref_dist>
    
| copy (<len> + 1) bytes from ref_ptr to out_ptr   
ref_copy:
    move.b (%a0)+, (%a2)+       | *<out_ptr>++ = *<ref_ptr>++
    dbra %d1, ref_copy

    move.b (%a0)+, (%a2)+       | copy one more byte
    
loop_chk:
    cmp.l %a1, %a3              | while (in_ptr != in_end)
    jbhi loop_head

    move.l %a2, %d0
    sub.l (12+8,%sp), %d0       | return difference in output pointer 

| cleanup    
    movm.l (%sp)+, %a2-%a3
    rts
