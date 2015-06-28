.text
.align 2

.global syscall_trap

syscall_trap:
    
    move.l %usp, %a0
    
    move.l (%a0)+, -(%sp)        | save ret addr from usp onto ssp

    move.l %a6, -(%sp)           | save %a6 to ssp
    move.l %sp, %a6              | save supervisor stack ptr into a6
    move.l %a0, %ssp             | set SSP = USP (specifically, the function args)
    
    jsr (%a1)                    | call func with args on stack
    
    move.l %sp, %a1              | save usp
    
    move.l %a6, %sp              | restore usp
    move.l (%sp)+, %a6
    move.l (%sp)+, -(%a1)        | restore clobbered return address

    rte
