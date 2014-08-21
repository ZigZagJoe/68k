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
    jne wav_end
    
    move.b #2, (0xC0001)     | LED ON
     
    movem.l %d0/%a0-%a2, -(%sp)
    
    move.l (wav_ptr), %a0
    cmp.l (wav_end), %a0     | perform bounds check
    jhi skip_load            | at/beyond specified end point: do not load data
    
    move.l #TIL311, %a1      | load address; saves time in loops
    move.l #GPDR, %a2        | bounds check address
    
      
    | stage one: load bytes until half full is asserted
    moveq #127, %d0

bc_loop:
    move.l (%a0)+, (%a1)
    btst.b #2, (%a2)
    dbeq %d0, bc_loop        | exit loop if GPDR bit 2 is 0
    
    | stage two: load no more than 256 bytes - 3 bytes (worst case)
    moveq #62, %d0
    
rem_loop:
    move.l (%a0)+, (%a1)
    dbra %d0, rem_loop
    
    move.l %a0, (wav_ptr)
    
    
skip_load:
    movem.l (%sp)+, %d0/%a0-%a2
    
    clr.b (wav_semaphore)
    move.b #0, (0xC0001)     | LED off
    
wav_end:
    rte
    
    
.bss
wav_ptr: .space 4
wav_end: .space 4
wav_semaphore: .space 1
