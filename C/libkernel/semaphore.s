.text
.align 2

.global sem_acquire
.global sem_try
.global sem_release
.global sem_init

sem_acquire:
    move.l 4(%sp), %a0
semaphore_check:
    tas.b (%a0)
    jeq semaphore_acquired
    jsr yield
    bra semaphore_check
semaphore_acquired:
    rts
    
sem_try:
    move.l 4(%sp), %a0
    tas.b (%a0)
    seq.b %d0
    rts
    
sem_init:
sem_release:
    move.l 4(%sp), %a0
    move.b #0, (%a0)
    rts

