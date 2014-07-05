.align    2
.text

.global _start
.global _exit

.extern __stack_start
.extern __bss_start
.extern __bss_length
.extern __start_func

.set IO_BASE, 0xC0000
.set TIL311, IO_BASE + 0x8000

 _start:
    move.b #0xCC, (TIL311)        | what up my glip glops!
    move.l #0xFFFFFFFF, (0x400)   | mark kernel not active
    
    move.l #__stack_start, %a7

    /* clear BSS section */

    move.l #__bss_size, %d0
    
    tst.l %d0                 | check if empty bss section
    beq run
        
    move.l #__bss_start, %a0      | A0 = _bss start
    
    | due to use of dbra, we can clear a max of 256kb of RAM (65536 * sizeof(long))
cbss:
    moveq.l #0, (%a0)+            | clear [A0.l], save two cycles
    dbra %d0, cbss                | D0 != 0
     
run:
    jsr __start_func              | jump to main
    
_exit:                            | loop forever
    bra _exit                     | could return to monitor if present
