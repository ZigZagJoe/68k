.align 2
.text

.global wav_isr
.global wav_ptr
.global wav_end
.global wav_semaphore

.set IO_BASE,  0xC0000
.set MFP,      IO_BASE
.set TIL311,   IO_BASE + 0x8000

| MFP registers
.set GPDR,     MFP+0x01 | GPIO data reg
.set DDR,      MFP+0x05 | data direction
.set IERB,     MFP+0x09 | interrupt enable B
.set IMRB,     MFP+0x15 | interrupt mask B

.macro sei
    and.w #0xF8FF, %SR     
.endm

wav_isr:
    | sei                    | allow interrupts
    
    tas.b (wav_semaphore)
    jne isr_end
    
    move.b #2, (0xC0001)     | LED ON
     
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    
    move.l (wav_ptr), %a0
    cmp.l (wav_end), %a0     | perform bounds check
    jhi skip_load            | at/beyond specified end point: do not load data
                             | for a circular buffer, reset to a start address
    
    move.l #TIL311, %a1      | load address; save time
  
    | load 256 bytes into FIFO

    movem.l (%a0)+, %d0-%d7/%a2-%a6
    movem.l %d0-%d7/%a2-%a6, (%a1)    | 52 
    
	movem.l (%a0)+, %d0-%d7/%a2-%a6
    movem.l %d0-%d7/%a2-%a6, (%a1)    | 52 
    
	movem.l (%a0)+, %d0-%d7/%a2-%a6
    movem.l %d0-%d7/%a2-%a6, (%a1)    | 52 
    
	movem.l (%a0)+, %d0-%d7/%a2-%a6
    movem.l %d0-%d7/%a2-%a6, (%a1)    | 52
    
	movem.l (%a0)+, %d1-%d7/%a2-%a6
    movem.l %d1-%d7/%a2-%a6, (%a1)    | 48 
  
    move.l %a0, (wav_ptr)
    
skip_load:
    movem.l (%sp)+, %d0-%d7/%a0-%a6
    
    clr.b (wav_semaphore)
    move.b #0, (0xC0001)     | LED off
    
isr_end:
    rte
    
    
.bss
wav_ptr: .space 4
wav_end: .space 4
wav_semaphore: .space 1
