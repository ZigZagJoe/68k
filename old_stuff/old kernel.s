.text
.align 2

.set IO_BASE, 0xC0000
.set MFP,    IO_BASE
.set TIL311, IO_BASE + 0x8000

| MFP registers
.set GPDR,   MFP+0x01
.set DDR,    MFP+0x05
.set IERB,   MFP+0x09
.set IMRB,   MFP+0x15
.set VR,     MFP+0x17
.set TCDCR,  MFP+0x1D
.set TDDR,   MFP+0x25
.set TSR,    MFP+0x2D | transmitter status reg
.set UDR,    MFP+0x2F | uart data register

.macro sei
	and.w #0xF8FF, %SR 	
.endm

.macro cli
	ori.w #0x700, %SR
.endm

/* C API
typedef void(*__vector)(void);

task_struct * create_task(__vector function);
void yield(void);
void exit_task(void);
void sleep_for(uint32_t time);

typedef struct __attribute__((packed)) {
    uint16_t ID;
    uint16_t runnable;
    uint32_t next_task;
    uint32_t sleep_time;
    uint8_t  __reserved[14];
    uint32_t D[8];
    uint32_t A[7];
    uint32_t SP;
    uint32_t PC;
    uint16_t FLAGS;
} task_struct;
*/

.global sleep_for
.global create_task
.global yield
.global exit_task
.global millis_counter
.global _start
.global _active_task

.extern main
.extern __stack_start
.extern __bss_start
.extern __bss_end
.extern default_interrupts

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| startup code: first code run
_start:
	cli
	move.b #0xE1, (TIL311) | what up my glip glops!
	
	move.l #__stack_start, %a7

	jsr default_interrupts
	
	move.b #0xCE, (TIL311) 

	| init global vars
	clr.l (millis_counter)
	clr.w (task_count)
	clr.b (swap_in_progress)
	clr.l (_first_task)
	
	| set the struct ids & set statuses to inactive
 	move.l #0x460, %a0
    move.w #30, %d0
    move.w #0, %d1

zero_structs:
    move.w %d1, (%a0)
    clr.b (3,%a0)
    add.l #96, %a0
    add.w #1, %d1
    dbra %d0, zero_structs

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
  
    | set up vectors
    move.l #_kern_millis_count, (0x210)
    move.l #task_manager, (0x80)

	move.b #0xCE, %d0   | create the main() task
	move.l #main, %a0
	trap #0
	
	move.b #0xC2, (TIL311)
	
	| enable 100hz interrupt
	move.b #183, (TDDR)
	move.b #0x80, (VR)
	ori.b #0x7, (TCDCR)
	ori.b #0x10, (IERB)
	ori.b #0x10, (IMRB)

	move.b #2, (DDR)    | set DDR so scheduler can blink LED
	
	move.b #0xBB, %d0
	move.l #0x460, %a0  | address of main() task struct
    trap #0             | run task (current context is LOST)
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| trap used to manage tasks: exit current, force swap (yield), create new, etc.
task_manager:
    cmp.b #0, %d0
    beq _user_swap
    cmp.b #0x11, %d0
    beq _sleep_for
    cmp.b #1, %d0
    beq _exit_task
    cmp.b #0xBB, %d0
    beq _run_task
    cmp.b #0xCE, %d0
    beq _create_task
    rte
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| add a count to the millis counter and do a swap
_kern_millis_count:
	addi.l #1, (millis_counter)
	
	tas.b (user_yielded)
	beq _swap_task                      | if user task did not yield, swap now
	
	clr.b (user_yielded)
	rte	

| swap out the current task 
_swap_task:
	cli
	tas.b (swap_in_progress)		    | ensure we are not swapping already
    bne _ret_swap
    move.b #1, (swap_in_progress)
    sei

	move.b #2, (GPDR)				    | turn on the LED
	
	| save context into active task struct
    move.l %a0, -(%sp) 				    | save %a0
    move.l (_active_task_regist), %a0     
    move.l (%sp)+,   32(%a0)            | %a0       4 bytes 
	
	movem.l %d0-%d7,   (%a0)            | %d0-d7   32 bytes
	movem.l %a1-%a6, 36(%a0)            | %a1-%a6   24 bytes
	
	move.l %usp, %a1
	move.l %a1,     60(%a0)             | %sp       4 bytes
	move.l 2(%sp),  64(%a0)             | PC       4 bytes
	move.w (%sp),   68(%a0)             | flags    2 bytes
	
	jmp _run_next_task

| ensure the next task will not get less than its fair share of time due 
| to the task before yielding. instead, cause the next automatic interrupt 
| to be skipped. the swapped task then gets 10ms + yielding task remaining time
_user_swap:
	move.b #1,(user_yielded)
    bra _swap_task

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| run the task pointed to by %a0. current context will be overwritten!	
_run_task:		
    move.l %a0, (_active_task)
	add.l #26, %a0
	move.l %a0, (_active_task_regist)
	
	| restore context from structure pointed by %a0
    move.w 68(%a0), (%sp)               | flags    2 bytes
	move.l 64(%a0),2(%sp)               | PC       4 bytes
	move.l 60(%a0), %a1
	move.l %a1, %usp                    | %sp       4 bytes
	movem.l 36(%a0), %a1-%a6            | %a1-%a6   24 bytes
	movem.l (%a0), %d0-%d7              | %d0-d7   32 bytes
	move.l 32(%a0), %a0                 | %a0       4 bytes
	
	clr.b (swap_in_progress)	
	
_ret_swap:
	sei
	move.b #0, (GPDR)                   | turn off the LED
	
    rte
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| find the next runnable task and run it
_run_next_task:
    move.l (_active_task), %a2          | first node
	
_next_node:
    move.l 4(%a2), %a2                  | node = node->next
	cmp.l #0, %a2                       | node =? null
	beq _rewind           
	
_check_runnable:
    cmp.b #1, 3(%a2)                    | check sleep
	beq _got_runnable				    | no? runnable task!
	
	cmp.b #2, 3(%a2)
	bne _next_node
		
_check_sleep:
	move.l (millis_counter), %d0
	
	cmp.l 8(%a2), %d0
	blt _next_node					    | not time for this process yet
	
	move.b #1, 3(%a2)                   | mark as runnable
	bra _got_runnable
	
_rewind:
    move.l (_first_task), %a2           | node == first_node
    bra _check_runnable
	
_got_runnable:
    move.l %a2, %a0
    bra _run_task	

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| end the calling task & remove it from linked list
_exit_task:
    cli
    
	move.l (_active_task), %a0
    sub.l #1, (task_count)
	clr.b 3(%a0)                        | mark task as exited
		
	move.l (_first_task), %a2           | load start of linked list
    cmp  %a2, %a0
	bne _notcurr
	
	move.l 4(%a0), (_first_task)
	bra _end_exit
	                    
_notcurr:    
    move.l 4(%a2), %a1                  | %a1 = node->next
    cmp.l %a0, %a1                      | node->next =? current
    beq _found
    move.l %a1, %a2                     | node = node->next
    bra _notcurr

_found:
    move.l 4(%a0),4(%a2)

_end_exit:	
	sei
	jmp _run_next_task

|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||	
| causes a task to sleep at minimum the interval specified
_sleep_for:
	divu.w #10, %d1
    andi.l #0x0000FFFF, %d1  | remove remainder in upper word
    add.l (millis_counter), %d1

    move.l (_active_task), %a0
    
    move.l %d1, 8(%a0)       | set time to run at
    move.b #2, 3(%a0)        | mark as sleeping
 
    bra _user_swap
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||	
| create a new task using address in %a0
| return address of struct or 0 on failure IN %D0	
_create_task:
	move.l %a0, -(%sp)
	move.l %a1, -(%sp)
	move.l %a2, -(%sp)
	
	move.l #1120, %a2                  | beginning of task state structs
	move.l #0x70000, %a1                | stack base address
	
_check_free:
    cmp.l #1000, %a2
    beq _cr_failed
    
	cmp.b #0, (3,%a2)
	beq _got_free
	sub.l #4096, %a1
	add.l #96, %a2
	jmp _check_free
	
_got_free:
	move.l %a0, 58(%a2)                 | load starting address into %a0 in struct
	move.l #_task_begin, 90(%a2)        | set PC to task begin struct
	move.l %a1, 86(%a2)				    | set stack pointer
	move.w #0x0, 94(%a2)			    | set flags (none)
    add.l #1, (task_count)
    move.b #1, (3,%a2)                  | mark as runnable
    clr.l (4,%a2)                       | clear next pointer
    
    move.l %a2, %d0                     | return pointer
    
    move.l (_first_task), %a1           | load start of linked list
    
    cmp.l #0, %a1                       | first =? 0
    bne _nz                           
 
    move.l %a2, (_first_task)
    bra _cr_finished

_nz:
    cmp.l #0, 4(%a1)                    | node->next =? null
    beq _ef
    move.l 4(%a1), %a1                  | node = node->next
    bra _nz
    
_ef:
    move.l %a2, 4(%a1)                  | node->next = this
    
_cr_finished:
    move.l (%sp)+,%a2
	move.l (%sp)+,%a1
	move.l (%sp)+,%a0
    rte
    
_cr_failed:
    move.l #0, %d0
    bra _cr_finished
    
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| this is the user mode where a task is started
| it ensures a task that returns always exits.
_task_begin:
	jsr (%a0)
	move.b #1, %d0
	trap #0 | exit_task
	
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
| C Functions

create_task:
    move.l 4(%sp), %a0
    move.b #0xCE, %d0
    trap #0
    rts
    
sleep_for:
	move.l 4(%sp), %d1       | argument
    move.b #0x11, %d0
    trap #0
    rts
    
yield:
    clr.b %d0
    trap #0                  | force a swap
    rts
    
exit_task:
    move.b #1, %d0
    trap #0                  | will never return 

|||||||||||||||||||||||||||||||||||||||||||||||||
| state variables

.org 0x400

_active_task:         .space 4
_active_task_regist:  .space 4
_first_task:          .space 4
millis_counter:       .space 4
task_count:           .space 2
swap_in_progress:     .space 1
user_yielded:         .space 1
