.align 2
.text

.global wav_isr
.global wav_ptr
.global wav_end

.set TIL311, 0xC8000           | TIL311 address

wav_isr:
    move.l %a0, -(%sp)
    move.l (wav_ptr), %a0
    cmp.l (wav_end), %a0
    jeq skip_wav
    move.b (%a0)+, (TIL311)
    move.l %a0, (wav_ptr)
skip_wav:
    move.l (%sp)+, %a0
    rte
    
    
.bss
wav_ptr: .space 4
wav_end: .space 4
