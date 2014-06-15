.text
.align 2

/* functions */
.global _start
.global sleep_for
.global create_task
.global yield
.global exit_task
.global wait_for_exit
.global enter_critical
.global leave_critical

/* vars */
.global _active_task
.global millis_counter

/* external vars */
.extern main
.extern __stack_start
.extern __bss_start
.extern __bss_end
.extern default_interrupts
 
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| state variables

.set _active_task,      0x400 | long 
.set _sleep_ll_head,    0x404 | long 
.set millis_counter,    0x408 | long 
.set task_count,        0x412 | word
.set _next_thread_id,   0x414 | word
.set _swap_in_progress, 0x416 | byte
.set _skip_next_tick,     0x417 | byte

| IO device declartions
.set IO_BASE,  0xC0000
.set MFP,      IO_BASE
.set TIL311,   IO_BASE + 0x8000

| MFP registers
.set GPDR,     MFP+0x01 | GPIO data reg
.set DDR,      MFP+0x05 | data direction
.set IERB,     MFP+0x09 | interrupt enable B
.set IMRB,     MFP+0x15 | interrupt mask B
.set VR,       MFP+0x17 | vector register
.set TCDCR,    MFP+0x1D | Timers C&D control
.set TDDR,     MFP+0x25 | Timer D data
.set RSR,      MFP+0x2B | receiver status reg
.set TSR,      MFP+0x2D | transmitter status reg
.set UDR,      MFP+0x2F | uart data register

| TRAP #0 commands
.set TASK_YIELD,   0x00
.set TASK_EXIT,    0x01
.set TASK_SLEEP,   0x11
.set TASK_ENTER,   0xBB
.set TASK_CREATE,  0xCE
.set TASK_CRIT_ENTER, 0xCA
.set TASK_CRIT_LEAVE, 0xC1

.set USER_STACK_SIZE, 4096

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| macros

.macro sei
	and.w #0xF8FF, %SR 	
.endm

.macro cli
	ori.w #0x700, %SR
.endm

.macro put_char ch
	move.w %D0, -(%SP)
	move.b #\ch, %D0
	jsr putc
	move.w (%SP)+, %D0
.endm

.macro put_str str
	move.l %A0, -(%SP)
	move.l #\str, %A0
	jsr puts
	move.l (%SP)+,%A0
.endm

.macro newline 
	put_char '\n'
.endm

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| entry point
_start:
	cli
	move.b #0xE1, (TIL311) | what up my glip glops!
	
	move.l #__stack_start, %a7

	jsr default_interrupts
	
	move.b #0xCE, (TIL311) 

	/* initialize scheduler state */
	clr.l (millis_counter)
	clr.w (task_count)
	clr.b (_swap_in_progress)
	clr.b (_skip_next_tick)
	clr.l (_active_task)
	clr.w (_next_thread_id)
	
	/* initialize task structs */
 	move.l #0x460, %a0
    move.w #30, %d0

zero_structs:
    clr.b (3,%a0)
    add.l #96, %a0
    dbra %d0, zero_structs

	/* clear BSS section */
	
	move.l #__bss_start, %d1
	move.l #__bss_end, %d0
	
	cmp.l	%d0, %d1  	                | check if empty bss section
	jbeq run
	
	move.l %d1, %a0 	                | A0 = _bss start
	sub.l %d1, %d0		                | D0 = num bytes to copy
	
cbss:
	clr.b (%a0)+                        | clear [A0.b]
	subq.l #1, %d0		                | DBRA uses word size unfortunately
	jne cbss 			                | D0 != 0
	
run:
    | set up vectors
    move.l #_kern_millis_count, (0x210)
    move.l #task_manager, (0x80)

	move.b #TASK_CREATE, %d0            | create the main() task
	move.l #main, %a0
	clr.w %d1							| no arguments
	trap #0
	
	move.b #0xC2, (TIL311)
	
	| enable 100hz interrupt
	move.b #184, (TDDR)
	move.b #0x80, (VR)
	ori.b #0x7, (TCDCR)
	ori.b #0x10, (IERB)
	ori.b #0x10, (IMRB)

	bset.b #1, (DDR)                      | set DDR so scheduler can blink LED
	
	move.b #TASK_ENTER, %d0
	move.l #0x460, %a0                  | address of main() task struct
    trap #0                             | run task (current context is LOST)
    
    illegal								| sanity check exception (should never reach)
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| no viable tasks left; nothing for system to do. halt system.

_the_end_times:
	cli
	
	move.l #_system_halted, %a0
	jsr puts
	
_check_cmd:                             | spin until reset command is received
	btst #7, (RSR)                      | test if buffer full (bit 7) is set.
    beq _check_cmd                 
    
    move.b (UDR), %D1
    cmp.b #0xCF, %D1
    bne _check_cmd
    
	btst #0, (GPDR)                     | gpio data register - test input 0. Z=!bit
    bne _check_cmd                      | gpio is 1, not bootoclock
     
	jmp 0x80008				            | jump to bootloader 

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| trap used to manage tasks: exit current, force swap (yield), create new, etc.
task_manager:
    cmp.b #TASK_YIELD, %d0
    beq _user_swap
    cmp.b #TASK_SLEEP, %d0
    beq _sleep_for
    cmp.b #TASK_CREATE, %d0
    beq _create_task
    cmp.b #TASK_EXIT, %d0
    beq _exit_task
    cmp.b #TASK_CRIT_ENTER, %d0
    beq enter_critical
    cmp.b #TASK_CRIT_LEAVE, %d0
    beq leave_critical
    cmp.b #TASK_ENTER, %d0
    beq _run_task
    rte

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| add a count to the millis counter and do a swap
_kern_millis_count:
	addi.l #1, (millis_counter)
	
	cmp.b #0, (_skip_next_tick)
	beq _swap_task                      | if user task did not yield, swap now
	
	clr.b (_skip_next_tick)
	rte	

| swap out the current task 
_swap_task:
	tas.b (_swap_in_progress)		    | set swap_in_progress
    bne _ret_swap						| if it was already set; exit.

	bset.b #1, (GPDR)				    | turn on the LED
	
	| save context into active task struct
    move.l %a0, -(%sp) 				    | save %a0
    move.l (_active_task), %a0     
    move.l (%sp)+,   58(%a0)            | %a0       4 bytes 
	
	movem.l %d0-%d7, 26(%a0)            | %d0-%d7  32 bytes
	movem.l %a1-%a6, 62(%a0)            | %a1-%a6  24 bytes
	
	move.l %usp, %a1
	move.l %a1,      86(%a0)            | %sp       4 bytes
	move.l 2(%sp),   90(%a0)            | PC        4 bytes
	move.w (%sp),    94(%a0)            | flags     2 bytes

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| find the next runnable task and run it 
_run_next_task:
    move.l 4(%a0), %a0                  | node = node->next

_check_runnable:
	cmp.b #2, 3(%a0)					| is it in sleep?
	bne _run_task
		
	move.l (millis_counter), %d0		
	cmp.l 8(%a0), %d0                   
	
	bls _run_next_task				    | not time for this process yet, next!
	
	move.b #1, 3(%a0)                   | it is time: mark it as runnable
										| and fall through
										
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| run the task pointed to by %a0. current context will be overwritten!	
_run_task:		
    move.l %a0, (_active_task)

	| restore context from structure pointed by %a0
    move.w 94(%a0), (%sp)               | flags     2 bytes
	move.l 90(%a0),2(%sp)               | PC        4 bytes
	move.l 86(%a0), %a1
	move.l %a1, %usp                    | %sp       4 bytes
	movem.l 62(%a0), %a1-%a6            | %a1-%a6  24 bytes
	movem.l 26(%a0), %d0-%d7            | %d0-%d7  32 bytes
	move.l 58(%a0), %a0                 | %a0       4 bytes
	
	clr.b (_swap_in_progress)	
	bclr.b #1, (GPDR)                   | turn off the LED
	
_ret_swap:
    rte

| ensure the next task will not get less than its fair share of time due 
| to the task before yielding. instead, cause the next automatic interrupt 
| to be skipped. the swapped task then gets 10ms + yielding task remaining time
_user_swap:
	move.b #1, (_skip_next_tick)
    bra _swap_task
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| end the calling task & remove it from linked list
_exit_task:
	cli
 
    move.b #1, (_skip_next_tick)		| so next task is not shafted
	sub.w #1, (task_count)
    
    move.l (_active_task), %a0
 
    clr.b 3(%a0)                        | mark task as exited		
		   
	cmp.l 4(%a0), %a0                   | if next task is self, we are the last task
    beq _the_end_times                  | so take the special case and halt the system
    
	move.l %a0, %a2                     | load start of linked list
               
_next:  
    move.l 4(%a2), %a1                  | %a1 = node->next
    cmp.l %a0, %a1                      | node->next =? current
    beq _found
    move.l %a1, %a2                     | node = node->next
    bra _next

_found:
    move.l 4(%a0), 4(%a2)

	sei
	bra _run_next_task					

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||	
| causes a task to sleep at minimum the interval specified
| warning: %d1, %a0 are thrashed in accordance with gcc abi
_sleep_for:
	cmp.l #10, %d1
	ble _sleep_short
	
	divu.w #10, %d1					    | determine number of millis ticks
    andi.l #0x0000FFFF, %d1             | remove remainder in upper word
    
    cli									| disable interrupts so an invalid state is not written
	
	add.l (millis_counter), %d1         | time when this thread needs to be run again
	move.l (_active_task), %a0
    move.l %d1, 8(%a0)
    move.b #2, 3(%a0)                   | mark as sleeping
 
 _sleep_short:   
	sei 
    bra _user_swap
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||	
| create a new task
| %a0: entry point
| %d1: argument count (number of long words)
| %a1: top address of arguments 
| returns:
| struct pointer in %d0 high word and id in low word, or 0 on failure
_create_task:
	move.l %a1, -(%sp)
	move.l %a2, -(%sp)
	move.l %a3, -(%sp)					| save %a3

	cli								    | ensure list is not modified while we are working
    
    move.l %a1, %a3                     | address of top of arguments
	move.l #0x460, %a2                  | beginning of task state structs
	move.l #__stack_end, %a1            | stack base address
	
_check_free:
    cmp.l #0x1000, %a2                  | no room available; fail.
    bge _cr_failed
    
	cmp.b #0, (3, %a2)
	beq _got_free
	
	sub.l #USER_STACK_SIZE, %a1
	add.l #96, %a2
	jmp _check_free
	
_got_free:                              | copy the arguments to the new stack
	cmp.b #0, %d1
	beq _no_args
	
	subi.w #1, %d1
	
_copy_args:
	move.l -(%a3), -(%a1)
	dbra %d1, _copy_args
	
_no_args:
    move.w (_next_thread_id), (%a2)     | set the thread ID
	move.l %a0, 58(%a2)                 | load starting address into %a0 in struct
	move.l #_task_entry, 90(%a2)        | set PC to task begin struct
	
	move.l %a1, 86(%a2)				    | set stack pointer
	move.w #0, 94(%a2)			        | set flags (none)
    move.b #1, (3, %a2)                 | mark as runnable
	         
    move.l %a2, %d0                     | pointer in %d0 high word...
    swap %d0
    move.w (%a2), %d0                   | ... and id in %d0 low word
    
    add.w #1, (task_count)
    add.w #1, (_next_thread_id)
	
    move.l (_active_task), %a1          | load linked list node
    
    cmp.l #0, %a1                       | _active_task =? 0
    bne _nz                           
 
    move.l %a2, (_active_task)
    move.l %a2, 4(%a2)                  | me->next = me
   
    bra _cr_finished

_nz:
	move.l (4, %a1), (4, %a2)			| me->next = node->next
	move.l %a2, (4, %a1)                | node->next = me
	 
_cr_finished:
	sei
    move.l (%sp)+,%a3
	move.l (%sp)+,%a2
	move.l (%sp)+,%a1
    rte
    
_cr_failed:
    clr.l %d0
    bra _cr_finished
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| this is the user mode where a task is started
| it ensures a task that returns always exits.
_task_entry:
	jsr (%a0)
	move.b #TASK_EXIT, %d0
	trap #0                  | exit_task
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| C function interface
| these all adhere to the GNU m68k abi: 
| arguments on stack, first arg lowest in memory
| %d0, %d1, %a0, %a1 may be thrashed
| return value in %d0

| task_t create_task(void *task, uint16_t argc, ...)
create_task:
    move.l 8(%sp), %d1       | num arguments on stack
    move.l 4(%sp), %a0       | function pointer
    
    move.w %d1, %d0			 | number of longwords to copy
	rol.w #2, %d0
	
	move.l %sp, %a1			 | copy address of sp into a1
	lea 12(%a1, %d0.W), %a1  | offset it properly
	
    move.b #TASK_CREATE, %d0 | function
    trap #0
    rts

| sleep for at least a certain time   
sleep_for:
	move.l 4(%sp), %d1       | argument in ms
    move.b #TASK_SLEEP, %d0
    trap #0
    rts
    
| yield control to next task    
yield:
    move.b #TASK_YIELD, %d0
    trap #0                  | force a swap
    rts

| terminate calling task   
exit_task:
    move.b #TASK_EXIT, %d0
    trap #0                  | will never return
  
| enter a critical section (prevent swaps)
enter_critical:
	move.b #1, (_swap_in_progress)
	rts

| leave a critical section
| task termination will also exit critical section
leave_critical:
	clr.b (_swap_in_progress)
    rts
    
| wait for the task to exit; takes a task_t returned from create_task
wait_for_exit:
	clr.l %d0
	move.w 4(%sp), %d0  | pointer to task_struct
	move.l %d0, %a0
	move.w 6(%sp), %d1  | thread id
	
_waitexit:	
	cmp.w (%a0), %d1    | check if task id has changed
	bne _waitdone
	cmp.b #0, 3(%a0)    | check if task is still runnable
	beq _waitdone
	jsr yield
	bra _waitexit
	
_waitdone:
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| utility functions

puts:
	move.w %D0, -(%SP)
	move.l %A0, -(%SP)

_read_ch:
	move.b (%A0)+, %D0
	
	cmp.b #0, %D0
	jeq _str_done
	
	jsr putc
	bra _read_ch
	
_str_done:
	move.l (%SP)+, %A0
	move.w (%SP)+, %D0
	rts
	
putc:
	btst #7, (TSR)           | test buffer empty (bit 7)
    beq putc                 | Z=1 (bit = 0) ? branch
	move.b %D0, (UDR)		 | write char to buffer
	rts
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| strings	
_system_halted: .string "\n\n*** All tasks exited. System halt. ***\n";

