.text
.align 2
.global millis_count
.global millis_counter

millis_count:
	addi.l #1, (millis_counter)
	rte
	
.bss
millis_counter: .space 4
