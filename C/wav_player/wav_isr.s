.align 2
.text

.global wav_isr
.global wav_start
.global wav_ptr
.global wav_end
.global wav_semaphore

.set IO_BASE,  0xC0000
.set IO_DEV0,   IO_BASE
.set IO_DEV1,   IO_BASE + (0x8000 * 1)
.set IO_DEV2,   IO_BASE + (0x8000 * 2)
.set IO_DEV3,   IO_BASE + (0x8000 * 3)

.set MFP,      IO_DEV0
.set TIL311,   IO_DEV1
.set LCD,      IO_DEV2
.set DACFIFO,  IO_DEV3

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
        
    movem.l %d0-%d7/%a0-%a6, -(%sp)
    
    move.l (wav_ptr), %a0
    cmp.l (wav_end), %a0     | perform bounds check
    jle no_reset_pos         | if higher, reset to start 
    
    move.l (wav_start), %a0
    
no_reset_pos:
 
    move.l #DACFIFO, %a1     | load address into reg; save time
  
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

    movem.l (%sp)+, %d0-%d7/%a0-%a6
    
    clr.b (wav_semaphore)

isr_end:
    rte
    
    
.bss
wav_start: .space 4
wav_ptr: .space 4
wav_end: .space 4
wav_semaphore: .space 1
