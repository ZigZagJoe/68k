    ORG $1000
    
_active_task        dc.b 0,0,0,0
_active_task_regist dc.b 0,0,0,0
_first_task         dc.b 0,0,0,0
task_count          dc.b 0,0

swap_task: ; trap #0
    ; save context into active task struct
    move.l %a0, -(%sp) 				  ; save %a0
    move.l (_active_task_regist), %a0     
    move.l (%sp)+,   32(%a0)            ; %a0       4 bytes 
	
	movem.l %d0-d7,   (%a0)             ; %d0-d7   32 bytes
	movem.l %a1-%a6, 36(%a0)             ; %a1-%a6   24 bytes
	
	move.l %usp, %a1
	move.l %a1,     60(%a0)             ; %sp       4 bytes
	move.l 2(%sp),  64(%a0)             ; PC       4 bytes
	move.w (%sp),   68(%a0)             ; flags    2 bytes
	
	jmp _run_next_task
	
_run_task:	
    move.l %a0, (_active_task)
	add.l #26, %a0
	move.l %a0, (_active_task_regist)
	
	; restore context from structure pointed by %a0
    move.w 68(%a0), (%sp)               ; flags    2 bytes
	move.l 64(%a0),2(%sp)               ; PC       4 bytes
	move.l 60(%a0), %a1
	move.l %a1, %usp                    ; %sp       4 bytes
	movem.l 36(%a0), %a1-%a6             ; %a1-%a6   24 bytes
	movem.l (%a0), %d0-d7               ; %d0-d7   32 bytes
	move.l 32(%a0), %a0                 ; %a0       4 bytes
	
    rte

; find the next runnable task and run it
_run_next_task:
    move.l (_active_task), %a2         ; first node

_next_node:
    move.l 4(%a2), %a2                  ; node = node->next
	cmp.l #0, %a2                      ; node =? null
	beq _rewind           
	
_check_runnable:
	cmp.b #0, (3,%a2)                  ; ensure runnable flag is set
	bne _got_runnable
    
	bra _next_node
	
_rewind:
    move.l (_first_task), %a2          ; node == first_node
    bra _check_runnable
	
_got_runnable:
    move.l %a2, %a0
    bra _run_task	

; trap used to manage tasks: exit current, force swap (yield), create new, etc.
task_manager:
    cmp.b #0, %d0
    beq _exit_task
    cmp.b #1, %d0
    beq swap_task
    cmp.b #$BB, %d0
    beq _run_task
    cmp.b #$CE, %d0
    beq _create_task
    rte

; end the calling task
_exit_task:
    ori.w #$700, SR ; cli
	move.l (_active_task), %a0
    sub.l #1, (task_count)
	clr.b 3(%a0)                       ; mark task as exited
	
	move.l (_first_task), %a2          ; load start of linked list
                              
_notcurr:    
    move.l 4(%a2), %a1                  ; %a1 = node->next
    cmp.l %a0, %a1                      ; node->next =? current
    beq _found
    move.l %a1, %a2                     ; node = node->next
    bra _notcurr
    
_found:
    move.l 4(%a0),4(%a2)
	
	and.w #$F8FF, SR ; sei
	jmp _run_next_task
    
; this is the user code to begin a task
; it ensures a task that returns, exits.
_task_begin:
	jsr (%a0)
	clr.b %d0
	trap #1 ; exit_task
	
; create a new task using address in %a0
; return address of struct or 0 on failure	
_create_task:
	move.l %a0, -(%sp)
	move.l %a1, -(%sp)
	move.l %a2, -(%sp)
	
	move.l #1120, %a2                  ; beginning of task state structs
	move.l #$70000, %a1                ; stack base address
	
_check_free:
    cmp.l #1000, %a2
    beq _cr_failed
    
	cmp.b #0, (3,%a2)
	beq _got_free
	sub.l #4096, %a1
	add.l #96, %a2
	jmp _check_free
	
_got_free:
	move.l %a0, 58(%a2)                 ; load starting address into %a0 of task's state struct
	move.l #_task_begin, 90(%a2)       ; set PC to task begin struct
	move.l %a1, 86(%a2)				  ; set stack pointer
	move.w #$0, 94(%a2)			      ; flags
    add.l #1, (task_count)
    move.b #1, (3,%a2)                 ; mark as runnable
    clr.l (4,%a2)                      ; clear next pointer
    
    move.l %a2, %d0                     ; return pointer
    
    move.l (_first_task), %a1          ; load start of linked list
    
    cmp.l #0, %a1                      ; first =? 0
    bne _nz                           
 
    move.l %a2, (_first_task)
    bra _cr_finished

_nz:
    cmp.l #0, 4(%a1)                   ; node->next =? null
    beq _ef
    move.l 4(%a1), %a1                  ; node = node->next
    bra _nz
    
_ef:
    move.l %a2, 4(%a1)                  ; node->next = this
    
_cr_finished:
    move.l (%sp)+,%a2
	move.l (%sp)+,%a1
	move.l (%sp)+,%a0
    rte
    
_cr_failed:
    move.l #0, %d0
    bra _cr_finished
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; C API
; task_struct * create_task(__vector function);
; void yield(void)
; void exit_task(void)

create_task:
    move.l 4(%sp), %a0
    move.b #$CE, %d0
    trap #1
    rts
    
yield:
    move.b #1, %d0
    trap #1
    rts
    
exit_task:
    clr.b %d0
    trap #1   ; will never return 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  
taskA:
    move.w #4, %d0
taL:
    move.l #$DEADBEEF, $2000
    dbra %d0, taL
   ; trap #0
    bra taskA
    rts

taskB:
    move.w #4, %d0
tbL:
    move.l #$BABEC0DE, $2000
    dbra %d0, tbL
   ; trap #0
    bra taskB
    rts
    
taskC:
    move.w #4, %d0
tcL:
    move.l #$DEADDEAD, $2000
    dbra %d0, tcL
    rts
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
START
    move.l #swap_task, (100)
    
    move.l #$460, %a0
    move.w #30, %d0
    move.w #0, %d1
    
zero_structs:
    move.w %d1, (%a0)
    clr.b (3,%a0)
    add.l #96, %a0
    add.w #1, %d1
    dbra %d0, zero_structs
    
    ; set trap vectors
    move.l #$80, %a3
    move.l #swap_task, (%a3)+
    move.l #task_manager, (%a3)+
   
    ; create tasks
    move.b #$CE, %d0
    move.l #taskA, %a0
    trap #1
    
    move.b #$CE, %d0
    move.l #taskC, %a0
    trap #1
    
    move.b #$CE, %d0
    move.l #taskB, %a0
    trap #1
    
    ; run tasks
    move.b #$BB, %d0
    move.l #$460, %a0
    trap #1    ; never return from here 
    
	SIMHALT
    END START






*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
