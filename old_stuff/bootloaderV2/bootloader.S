#NO_APP
	.file	"bootloader.c"
	.text
	.align	2
	.globl	address_err
	.type	address_err, @function
address_err:
	link.w %fp,#0
	move.l %a0,-(%sp)
	move.l %d0,-(%sp)
	move.l #819200,%a0
.L3:
	move.b #-18,(%a0)
#APP
	move.l #170665,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-82,(%a0)
#APP
	move.l #170665,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	jbra .L3
	nop
	.size	address_err, .-address_err
	.align	2
	.globl	bus_error
	.type	bus_error, @function
bus_error:
	link.w %fp,#0
	move.l %a0,-(%sp)
	move.l %d0,-(%sp)
	move.l #819200,%a0
.L7:
	move.b #-18,(%a0)
#APP
	move.l #170665,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-66,(%a0)
#APP
	move.l #170665,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	jbra .L7
	nop
	.size	bus_error, .-bus_error
	.align	2
	.globl	main
	.type	main, @function
main:
	link.w %fp,#0
	movm.l #0x38,-(%sp)
	move.b #112,819200
#APP
	move.l #341330,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.l #address_err,%d1
	move.l %d1,%d0
	clr.w %d0
	swap %d0
	lsr.w #8,%d0
	move.b %d0,__vectors+12
	move.l %d1,%d0
	clr.w %d0
	swap %d0
	move.b %d0,__vectors+13
	move.l %d1,%d0
	lsr.l #8,%d0
	move.b %d0,__vectors+14
	move.b %d1,__vectors+15
	move.l #bus_error,%d1
	move.l %d1,%d0
	clr.w %d0
	swap %d0
	lsr.w #8,%d0
	move.b %d0,__vectors+8
	move.l %d1,%d0
	clr.w %d0
	swap %d0
	move.b %d0,__vectors+9
	move.l %d1,%d0
	lsr.l #8,%d0
	move.b %d0,__vectors+10
	move.b %d1,__vectors+11
	jbsr serial_start
	move.l #819200,%a2
	lea getc,%a3
	lea putc,%a4
.L10:
	move.b #-34,(%a2)
#APP
	move.l #170000,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-83,(%a2)
#APP
	move.l #170000,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-64,(%a2)
#APP
	move.l #170000,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-34,(%a2)
#APP
	move.l #170000,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-66,(%a2)
#APP
	move.l #170000,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	move.b #-17,(%a2)
#APP
	move.l #170000,%d0
1: sub.l #1, %d0
jne 1b

#NO_APP
	jbra .L11
.L12:
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	jbsr (%a4)
	addq.l #4,%sp
.L11:
	jbsr (%a3)
	cmp.w #-1,%d0
	jbne .L12
	jbra .L10
	nop
	.size	main, .-main
	.ident	"GCC: (GNU) 4.2.4"
