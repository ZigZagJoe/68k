.align	2
.text

.global _start
.global _exit

.extern main
.extern __stack_start
.extern __bss_start
.extern __bss_length

.set IO_BASE, 0xC0000
.set TIL311, IO_BASE + 0x8000

 _start:
	move.b #0xCC, (TIL311) | what up my glip glops!
	move.l #0xFFFFFFFF, (0x400) | kernel not active
	
	move.l #__stack_start, %a7

	/* clear BSS section */

	move.l #__bss_size, %d0
	
	cmp.l #0, %d0  	| check if empty bss section
	beq run
	
	move.l #__bss_start, %a0 	| A0 = _bss start
	
	| due to use of dbra, we can clear a max of 256kb of RAM (65536 * sizeof(long))
cbss:
	clr.l (%a0)+        | clear [A0.l]
	dbra %d0, cbss 		| D0 != 0
	
run:
	jsr main            | jump to main
	
_exit:                  | loop forever
	bra _exit           | could return to monitor if present
