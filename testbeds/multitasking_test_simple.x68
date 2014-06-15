    ORG $1000
    
_active_task        dc.b 0,0,4,0
_active_task_regist dc.b 0,0,4,26
_first_task         dc.b 0,0,0,0
_last_task          dc.b 0,0,0,0
_max_task           dc.b 0,0
task_count          dc.b 0,0

swap_task: ; trap #0
    ; save context into active task struct
    move.l A0, -(SP) 				  ; save A0
    move.l (_active_task_regist), A0     
    move.l (SP)+,   32(A0)            ; A0       4 bytes 
	
	movem.l D0-D7,   (A0)             ; D0-D7   32 bytes
	movem.l A1-A6, 36(A0)             ; A1-A6   24 bytes
	
	move.l USP, A1
	move.l A1,     60(A0)             ; SP       4 bytes
	move.l 2(SP),  64(A0)             ; PC       4 bytes
	move.w (SP),   68(A0)             ; flags    2 bytes
	
	jmp _run_next_task
	
_run_task:	
    move.l A0, (_active_task)
	add.l #26, A0
	move.l A0, (_active_task_regist)
	
	; restore context from structure pointed by A0
    move.w 68(A0), (SP)               ; flags    2 bytes
	move.l 64(A0),2(SP)               ; PC       4 bytes
	move.l 60(A0), A1
	move.l A1, USP                    ; SP       4 bytes
	movem.l 36(A0), A1-A6             ; A1-A6   24 bytes
	movem.l (A0), D0-D7               ; D0-D7   32 bytes
	move.l 32(A0), A0                 ; A0       4 bytes
	
    rte

; find the next runnable task and run it
_run_next_task:
    move.l (_active_task), A2                  ; beginning of task state structs
	add.l #96, A2
	
_check_runnable:
    cmp.l #$1000, A2
    bge _rewind
    
	cmp.b #0, (3,A2)
	bne _got_runnable

	add.l #96, A2
	jmp _check_runnable
	
_rewind:
    move.l #$460, A2
    bra _check_runnable
	
_got_runnable:
    move.l A2, A0
    bra _run_task	

; trap used to manage tasks: exit current, force swap (yield), create new, etc.
task_manager:
    cmp.b #0, D0
    beq _exit_task
    cmp.b #1, D0
    beq swap_task
    cmp.b #$BB, D0
    beq _run_task
    cmp.b #$CE, D0
    beq _create_task
    rte

; end the calling task
_exit_task:
    move.l (_active_task), A0
    sub.l #1, (task_count)
	clr.b 3(A0)                        ; mark task as exited
	jmp _run_next_task
    
; this is the user code to begin a task
; it ensures a task that returns, exits.
_task_begin:
	jsr (A0)
	clr.b D0
	trap #1 ; exit_task
	
; create a new task using address in A0
; return address of struct or 0 on failure	
_create_task:
	move.l A0, -(SP)
	move.l A1, -(SP)
	move.l A2, -(SP)
	
	move.l #1120, A2                  ; beginning of task state structs
	move.l #$70000, A1                ; stack base address
	
_check_free:
    cmp.l #1000, A2
    beq _cr_failed
    
	cmp.b #0, (3,A2)
	beq _got_free
	sub.l #4096, A1
	add.l #96, A2
	jmp _check_free
	
_got_free:
	move.l A0, 58(A2)                 ; load starting address into A0 of task's state struct
	move.l #_task_begin, 90(A2)       ; set PC to task begin struct
	move.l A1, 86(A2)				  ; set stack pointer
	move.w #$0, 94(A2)			      ; flags
    add.l #1, (task_count)
    move.b #1, (3,A2)                 ; mark as runnable
    clr.l (4,A2)                      ; clear next pointer
    
    move.l A2, D0                     ; return pointer
    
    cmp.l #0, (first_task)
    jne _nz
    move.l A2, (first_task)
    move.l A2, (last_task)
    
_nz:
    
_cr_finished:
    move.l (SP)+,A2
	move.l (SP)+,A1
	move.l (SP)+,A0
    rte
    
_cr_failed:
    move.l #0, D0
    bra _cr_finished
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; C API
; task_struct * create_task(__vector function);
; void yield(void)
; void exit_task(void)

create_task:
    move.l 4(SP), A0
    move.b #$CE, D0
    trap #1
    rts
    
yield:
    move.b #1, D0
    trap #1
    rts
    
exit_task:
    clr.b D0
    trap #1   ; will never return 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  
taskA:
    move.w #4, D0
taL:
    move.l #$DEADBEEF, $2000
    dbra D0, taL
   ; trap #0
    bra taskA
    rts

taskB:
    move.w #4, D0
tbL:
    move.l #$BABEC0DE, $2000
    dbra D0, tbL
   ; trap #0
    bra taskB
    rts
    
taskC:
    move.w #4, D0
tcL:
    move.l #$DEADDEAD, $2000
    dbra D0, tcL
    rts
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
START
    move.l #swap_task, (100)
    
    move.l #$460, A0
    move.w #30, D0
    move.w #0, D1
    
zero_structs:
    move.w D1, (A0)
    clr.b (3,A0)
    add.l #96, A0
    add.w #1, D1
    dbra D0, zero_structs
    
    ; set trap vectors
    move.l #$80, A3
    move.l #swap_task, (A3)+
    move.l #task_manager, (A3)+
   
    ; create tasks
    move.b #$CE, D0
    move.l #taskA, A0
    trap #1
    
    move.b #$CE, D0
    move.l #taskB, A0
    trap #1
    
    move.b #$CE, D0
    move.l #taskC, A0
    trap #1
    
    ; run tasks
    move.b #$BB, D0
    move.l #$460, A0
    trap #1    ; never return from here 
    
	SIMHALT
    END START






*~Font name~Courier New~
*~Font size~10~
*~Tab type~1~
*~Tab size~4~
