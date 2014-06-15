| fatal exception handling stuff

.text
.align   2

.set MFP_BASE, 0xC0000    | start of IO range for MFP
.set GPDR, MFP_BASE + 0x1 | gpio data reg
.set UDR, MFP_BASE + 0x2F | uart data register
.set TSR, MFP_BASE + 0x2D | transmitter status reg
.set RSR, MFP_BASE + 0x2B | receiver status reg
.set TIL311, 0xC8000      | til311 displays

.macro putchar ch
	move.w %D0, -(%SP)
	move.b #\ch, %D0
	jsr putc
	move.w (%SP)+, %D0
.endm

.macro newline 
	putchar '\n'
.endm

.macro put_str str
	move.l %A0, -(%SP)
	move.l #\str, %A0
	jsr puts
	move.l (%SP)+,%A0
.endm

.macro sei
	and.w #0xF8FF, %SR 	
.endm

.macro cli
	ori.w #0x700, %SR
.endm

.align   2

.global exception_address_err
.global exception_bus_error
.global exception_illegal_inst
.global exception_bad_isr
.global exception_spurious
.global exception_trap
.global exception_unknown
.global exception_generic
.global exception_privilege
.global user_dump

exception_address_err:
	move.w #0xAE, -(%SP)
	move.l #_addr_err, -(%SP)
	bra dumpregs
	
exception_privilege:
	move.w #0xBA, -(%SP)
	move.l #_privvio, -(%SP)
	bra dumpregs
	
exception_bus_error:
	move.w #0xBE, -(%SP)
	move.l #_bus_err, -(%SP)
	bra dumpregs
		
exception_illegal_inst:
	move.w #0x11, -(%SP)
	move.l #_ilginst_err, -(%SP)
	bra dumpregs
	
exception_bad_isr:
	move.w #0x1B, -(%SP)
	move.l #_bad_isr_err, -(%SP)
	bra dumpregs
	
exception_spurious:
	move.w #0x1F, -(%SP)
	move.l #_sprr_err, -(%SP)
	bra dumpregs

exception_trap:
	move.w #0xDE, -(%SP)
	move.l #_trap, -(%SP)
	bra dumpregs
	
exception_generic:
	move.w #0xEC, -(%SP)
	move.l #_random_excep, -(%SP)
	bra dumpregs
	
dumpregs:
	cli
	
	sub.l #28, %SP
	movem.l %A0-%A6, (%SP)
	
	sub.l #32, %SP
	movem.l %D0-%D7, (%SP)
		
	
	move.b 65(%SP), (TIL311)

	newline
	move.l 60(%SP), %A0
	jsr puts
	newline
	
	/*put_str _TASKNUM
	
.extern _active_task

	move.l (_active_task), %a2
	
	cmp #0x1000, %a2
	jge BAD_TASK
	
	move.w (%a2), %d0
	jsr puthexword

	bra regs
	
BAD_TASK:
	put_str _invt
	move %a2, %d0
	jsr puthexlong	
regs:
	newline*/
	
	newline
	
	put_str _D0
	move.l (%SP), %D0
	jsr puthexlong
	
	put_str _D1
	move.l 4(%SP), %D0
	jsr puthexlong
	
	put_str _D2
	move.l 8(%SP), %D0
	jsr puthexlong
	
	put_str _D3
	move.l 12(%SP), %D0
	jsr puthexlong
	
	newline
	
	put_str _D4
	move.l 16(%SP), %D0
	jsr puthexlong
	
	put_str _D5
	move.l 20(%SP), %D0
	jsr puthexlong
	
	put_str _D6
	move.l 24(%SP), %D0
	jsr puthexlong
	
	put_str _D7
	move.l 28(%SP), %D0
	jsr puthexlong
	
	newline
	newline
	
	put_str _A0
	move.l 32(%SP), %D0
	jsr puthexlong
	
	put_str _A1
	move.l 36(%SP), %D0
	jsr puthexlong
	
	put_str _A2
	move.l 40(%SP), %D0
	jsr puthexlong
	
	put_str _A3
	move.l 44(%SP), %D0
	jsr puthexlong	
	
	newline
	
	put_str _A4
	move.l 48(%SP), %D0
	jsr puthexlong	
	
	put_str _A5
	move.l 52(%SP), %D0
	jsr puthexlong
	
	put_str _A6
	move.l 56(%SP), %D0
	jsr puthexlong
			
	newline
	newline
	
	cmp.b #0xAE, 65(%SP)
	beq _group0
	cmp.b #0xBE, 65(%SP)
	beq _group0
	
	put_str _PC
	move.l 68(%SP), %D0
	jsr puthexlong

	| flags
	move.w 66(%SP), %D1
	
	bra _contp
	
_group0:

	put_str _PC
	move.l 76(%SP), %D0
	jsr puthexlong
	
	put_str _ADDR
	move.l 68(%SP), %D0
	jsr puthexlong

	put_str _INST
	move.w 72(%SP), %D0
	jsr puthexword
	
	move.w 66(%SP), %D0
	
	move.l #_WRITE, %a0
	btst #4, %d0
	beq _wr
	move.l #_READ, %a0
_wr:
	
	jsr puts
	
	move.l #_NOT, %a0
	btst #3, %d0
	beq _not
	move.l #_INSTR, %a0
_not:
	
	jsr puts
	
	move.l #_USR, %a0
	btst #2, %d0
	beq _usr
	move.l #_SUPR, %a0
_usr:
	
	jsr puts
	
	move.l #_PGM, %a0
	btst #0, %d0
	beq _pgm
	move.l #_DATA, %a0
_pgm:
	
	jsr puts

	| flags
	move.w 74(%SP), %D1
	
	newline
	newline
	
_contp:
	put_str _usersp
	move.l %USP, %A0
	move.l %A0, %D0
	jsr puthexlong
	
	put_str _supsp
	move.l %SP, %D0
	jsr puthexlong

	newline 
	newline 
	
	put_str _flags	
	
	| %d1 must contain flags
	
	move.w #15, %D2
	
	|   T 0 S 0 | 0 I2 I1 I0 | 0 0 0 X | N Z V C
	
	jsr put_bin
	
	newline
	newline
	move.l #_end, %A0
	jsr puts
	newline
	
loop_forever:
	move.b #0xEE, (TIL311)
	jsr half_sec_delay
	
	move.b 65(%SP), (TIL311)
	jsr half_sec_delay

	bra loop_forever

half_sec_delay:
	move.l #40000,%d0    	                         
delay: 
	jsr check_boot
	sub.l #1, %d0
	jne delay
	rts
	
check_boot:
	btst #7, (RSR)           | test if buffer full (bit 7) is set.
    beq retu                 
    
    move.b (UDR), %D1
    cmp.b #0xCF, %D1
    bne retu
    
	btst #0, (GPDR)          | gpio data register - test input 0. Z=!bit
    bne retu              	 | gpio is 1, not bootoclock
     
	jmp 0x80008
retu:
	rts
	
| ############## BEGIN FUNCTIONS ###############
puts:
	move.w %D0, -(%SP)
	move.l %A0, -(%SP)

rdc:
	move.b (%A0)+, %D0
	
	cmp.b #0, %D0
	jeq done
	
	jsr putc
	bra rdc
	
done:
	move.l (%SP)+, %A0
	move.w (%SP)+, %D0
	rts
	
putc:
	btst #7, (TSR)           | test buffer empty (bit 7)
    beq putc                 | Z=1 (bit = 0) ? branch
	move.b %D0, (UDR)		 | write char to buffer
	rts

put_bin: 
	move.b #'0', %D0
	btst %D2, %D1
	jeq not1
	move.b #'1', %D0
not1:
	jsr putc
	move.b #' ', %D0
	jsr putc
	
	dbra %D2, put_bin
	rts
	
puthexlong:
	move.l %D0, -(%SP)

	swap %D0
	jsr puthexword
	
	move.l (%SP), %D0
	and.l #0xFFFF, %D0
	jsr puthexword

	move.l (%SP)+, %D0
	rts
		
puthexword:
	move.w %D0, -(%SP)

	lsr.w #8, %D0
	jsr puthexbyte
	
	move.w (%SP), %D0
	and.w #0xFF, %D0
	jsr puthexbyte

	move.w (%SP)+, %D0
	rts

puthexbyte:
	move.l %A0, -(%SP)
	move.w %D0, -(%SP)
	
	movea #_hexchars, %A0
	
	lsr #4, %D0				    | shift top 4 bits over
	and.w #0xF, %D0
	move.b (%A0, %D0.W), %D0    | look up char
	jsr putc
	
	move.w (%SP), %D0			
	and.w #0xF, %D0			    | take bottom 4 bits
	move.b (%A0, %D0.W), %D0	| look up char
	jsr putc
	
	move.w (%SP)+, %D0		    | restore byte
	move.l (%SP)+, %A0		    | restore address
	
	rts

.data 

_hexchars: .ascii "0123456789ABCDEF"

_flags: .string "    T   S     IMASK       X N Z V C\n    "

_random_excep:.string " ######################## Exception ##########################"
_addr_err:    .string " ###################### Address Error ########################"
_bus_err:     .string " ######################## Bus Error ##########################"
_ilginst_err: .string " ################### Illegal Instruction #####################"
_bad_isr_err: .string " ######################### Bad ISR ###########################"
_sprr_err:    .string " #################### Spurious Interrupt #####################"
_trap:        .string " ########################### Trap ############################"
_privvio:        .string " ################### Privilege Violation #####################"
_end:         .string " #############################################################"

_invt: .string "<invalid task>  0x"
_TASKNUM: .string "  TASK "
_PC: .string "   PC: "
_D0: .string "   D0: "
_D1: .string "   D1: "
_D2: .string "   D2: "
_D3: .string "   D3: "
_D4: .string "   D4: "
_D5: .string "   D5: "
_D6: .string "   D6: "
_D7: .string "   D7: "
_A0: .string "   A0: "
_A1: .string "   A1: "
_A2: .string "   A2: "
_A3: .string "   A3: "
_A4: .string "   A4: "
_A5: .string "   A5: "
_A6: .string "   A6: "
_usersp: .string "  USP: "
_supsp: .string "  SSP: "
_ADDR: .string " ADDR: "
_INST: .string " INST: "

_READ: .string "  Read "
_WRITE: .string "  Write"
_INSTR: .string "instruction "
_NOT: .string ""
_USR: .string "user "
_SUPR: .string "supervisor "
_PGM: .string "program "
_DATA: .string "data "
