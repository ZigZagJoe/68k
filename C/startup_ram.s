.align	2
.text

.global _start
.global _exit

.extern main
.extern __stack_start
.extern __bss_start
.extern __bss_end

.set IO_BASE, 0xC0000
.set TIL311, IO_BASE + 0x8000

 _start:
	move.b #0xCC, (TIL311) | what up my glip glops!
	move.l #0xFFFFFFFF, (0x400) | kernel not active
	
	move.l #__stack_start, %a7

	/* clear BSS section */
	
	move.l #__bss_start, %d1
	move.l #__bss_end, %d0
	
	cmp.l	%d0, %d1  	| check if empty bss section
	jbeq run
	
	move.l %d1, %a0 	| A0 = _bss start
	sub.l %d1, %d0		| D0 = num bytes to copy
	
cbss:
	clr.b (%a0)+        | clear [A0.b]
	subq.l #1, %d0		| DBRA uses word size unfortunately
	jne cbss 			| D0 != 0
	
run:
	jsr main            | jump to main
	
_exit:                  | loop forever
	bra _exit           | could return to monitor if present
