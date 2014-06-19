#NO_APP
	.file	"printf.c"
	.globl	__mulsi3
	.globl	__udivsi3
	.globl	__umodsi3
	.text
	.align	2
	.type	ui2a, @function
ui2a:
	link.w %fp,#0
	movm.l #0x3f38,-(%sp)
	move.l 8(%fp),%d3
	move.l 12(%fp),%d6
	move.l 16(%fp),%d7
	move.l 20(%fp),%a2
	cmp.l %d3,%d6
	jbls .L2
	moveq #1,%d4
	jbra .L4
.L2:
	moveq #1,%d4
	lea __mulsi3,%a4
	lea __udivsi3,%a3
.L5:
	move.l %d6,-(%sp)
	move.l %d4,-(%sp)
	jbsr (%a4)
	addq.l #8,%sp
	move.l %d0,%d4
	move.l %d0,-(%sp)
	move.l %d3,-(%sp)
	jbsr (%a3)
	addq.l #8,%sp
	cmp.l %d0,%d6
	jbls .L5
	tst.l %d4
	jbeq .L7
.L4:
	lea __udivsi3,%a3
	lea __umodsi3,%a4
.L19:
	moveq #0,%d5
.L8:
	move.l %d4,-(%sp)
	move.l %d3,-(%sp)
	jbsr (%a3)
	addq.l #8,%sp
	move.l %d0,%d2
	move.l %d4,-(%sp)
	move.l %d3,-(%sp)
	jbsr (%a4)
	addq.l #8,%sp
	move.l %d0,%d3
	move.l %d6,-(%sp)
	move.l %d4,-(%sp)
	jbsr (%a3)
	addq.l #8,%sp
	move.l %d0,%d4
	tst.l %d5
	jbne .L9
	tst.l %d2
	jbgt .L9
	tst.l %d0
	jbne .L19
.L9:
	moveq #9,%d0
	cmp.l %d2,%d0
	jblt .L12
	moveq #48,%d0
	jbra .L14
.L12:
	tst.l %d7
	jbeq .L15
	moveq #55,%d0
	jbra .L14
.L15:
	moveq #87,%d0
.L14:
	add.b %d2,%d0
	move.b %d0,(%a2)+
	tst.l %d4
	jbeq .L7
	addq.l #1,%d5
	jbra .L8
.L7:
	clr.b (%a2)
	movm.l -36(%fp),#0x1cfc
	unlk %fp
	rts
	.size	ui2a, .-ui2a
	.align	2
	.type	putchw, @function
putchw:
	link.w %fp,#0
	movm.l #0x383c,-(%sp)
	move.l 8(%fp),%d4
	move.l 12(%fp),%a4
	move.l 16(%fp),%a2
	move.l 24(%fp),%a5
	tst.b 23(%fp)
	jbeq .L24
	moveq #48,%d0
	jbra .L26
.L24:
	moveq #32,%d0
.L26:
	move.l %a5,%d3
	addq.l #1,%d3
	tst.b (%a5)
	jbeq .L27
	cmp.w #0,%a2
	jble .L29
	move.l %d3,%a0
.L31:
	subq.l #1,%a2
	tst.b (%a0)+
	jbne .L32
.L27:
	cmp.w #0,%a2
	jbgt .L33
	jbra .L34
.L32:
	cmp.w #0,%a2
	jbeq .L29
	jbra .L31
.L33:
	moveq #0,%d2
	ext.w %d0
	move.w %d0,%a3
.L35:
	move.l %a3,-(%sp)
	move.l %d4,-(%sp)
	jbsr (%a4)
	addq.l #1,%d2
	addq.l #8,%sp
	cmp.l %d2,%a2
	jbeq .L34
	jbra .L35
.L36:
	move.l %d3,%a2
.L40:
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	move.l %d4,-(%sp)
	jbsr (%a4)
	move.b (%a2)+,%d0
	addq.l #8,%sp
	jbeq .L39
	jbra .L40
.L34:
	move.b (%a5),%d0
	jbne .L36
	jbra .L39
.L29:
	move.b (%a5),%d0
	move.l %d3,%a2
	jbra .L40
.L39:
	movm.l -28(%fp),#0x3c1c
	unlk %fp
	rts
	.size	putchw, .-putchw
	.align	2
	.globl	tfp_format
	.type	tfp_format, @function
tfp_format:
	link.w %fp,#-12
	movm.l #0x3f3c,-(%sp)
	move.l 8(%fp),%d4
	move.l 12(%fp),%a4
	move.l 16(%fp),%a2
	move.l 20(%fp),%a3
	move.l #ui2a,%d7
	lea (-12,%fp),%a5
	move.l #putchw,%d6
	jbra .L83
.L46:
	addq.l #1,%a2
	cmp.b #37,%d0
	jbeq .L47
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	move.l %d4,-(%sp)
	jbsr (%a4)
	addq.l #8,%sp
	jbra .L83
.L47:
	move.b (%a2)+,%d1
	cmp.b #48,%d1
	jbeq .L49
	clr.b %d5
	jbra .L51
.L49:
	move.b (%a2)+,%d1
	moveq #1,%d5
.L51:
	move.b %d1,%d0
	add.b #-48,%d0
	cmp.b #9,%d0
	jbls .L52
	moveq #0,%d3
	jbra .L54
.L52:
	moveq #0,%d3
	jbra .L55
.L56:
	moveq #10,%d0
	cmp.l %a0,%d0
	jblt .L54
	move.l %d3,%d0
	add.l %d3,%d0
	add.l %d0,%d0
	add.l %d3,%d0
	add.l %d0,%d0
	move.l %a0,%d3
	add.l %d0,%d3
	move.b (%a2)+,%d1
.L55:
	move.b %d1,%d0
	ext.w %d0
	move.w %d0,%a0
	move.b %d1,%d0
	add.b #-48,%d0
	cmp.b #9,%d0
	jbhi .L58
	lea (-48,%a0),%a0
	jbra .L60
.L58:
	move.b %d1,%d0
	add.b #-97,%d0
	cmp.b #5,%d0
	jbhi .L61
	lea (-87,%a0),%a0
	jbra .L60
.L61:
	move.b %d1,%d0
	add.b #-65,%d0
	cmp.b #5,%d0
	jbhi .L54
	lea (-55,%a0),%a0
.L60:
	cmp.w #0,%a0
	jbge .L56
.L54:
	cmp.b #100,%d1
	jbeq .L68
	jbgt .L71
	cmp.b #88,%d1
	jbeq .L66
	jbgt .L72
	tst.b %d1
	jbeq .L76
	cmp.b #37,%d1
	jbne .L83
	jbra .L65
.L72:
	cmp.b #99,%d1
	jbne .L83
	jbra .L67
.L71:
	cmp.b #117,%d1
	jbeq .L70
	cmp.b #120,%d1
	jbeq .L66
	cmp.b #115,%d1
	jbne .L83
	jbra .L69
.L70:
	move.l %a3,%d2
	addq.l #4,%d2
	move.l %a5,-(%sp)
	clr.l -(%sp)
	pea 10.w
	move.l (%a3),-(%sp)
	move.l %d7,%a0
	jbsr (%a0)
	move.l %a5,-(%sp)
	move.b %d5,%d0
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	move.l %d3,-(%sp)
	move.l %a4,-(%sp)
	move.l %d4,-(%sp)
	move.l %d6,%a0
	jbsr (%a0)
	move.l %d2,%a3
	lea (36,%sp),%sp
	jbra .L83
.L68:
	move.l (%a3),%d0
	jblt .L73
	move.l %a5,%d1
	jbra .L75
.L73:
	neg.l %d0
	move.b #45,-12(%fp)
	moveq #-11,%d1
	add.l %fp,%d1
.L75:
	addq.l #4,%a3
	move.l %d1,-(%sp)
	clr.l -(%sp)
	pea 10.w
	move.l %d0,-(%sp)
	move.l %d7,%a0
	jbsr (%a0)
	move.l %a5,-(%sp)
	move.b %d5,%d0
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	move.l %d3,-(%sp)
	move.l %a4,-(%sp)
	move.l %d4,-(%sp)
	move.l %d6,%a0
	jbsr (%a0)
	lea (36,%sp),%sp
	jbra .L83
.L66:
	move.l %a3,%d2
	addq.l #4,%d2
	move.l %a5,-(%sp)
	cmp.b #88,%d1
	seq %d0
	ext.w %d0
	ext.l %d0
	neg.l %d0
	move.l %d0,-(%sp)
	pea 16.w
	move.l (%a3),-(%sp)
	move.l %d7,%a0
	jbsr (%a0)
	move.l %a5,-(%sp)
	move.b %d5,%d0
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	move.l %d3,-(%sp)
	move.l %a4,-(%sp)
	move.l %d4,-(%sp)
	move.l %d6,%a0
	jbsr (%a0)
	move.l %d2,%a3
	lea (36,%sp),%sp
	jbra .L83
.L67:
	move.l %a3,%d2
	addq.l #4,%d2
	move.l (%a3),%d0
	ext.w %d0
	move.w %d0,%a0
	move.l %a0,-(%sp)
	move.l %d4,-(%sp)
	jbsr (%a4)
	move.l %d2,%a3
	addq.l #8,%sp
	jbra .L83
.L69:
	move.l %a3,%d2
	addq.l #4,%d2
	move.l (%a3),-(%sp)
	clr.l -(%sp)
	move.l %d3,-(%sp)
	move.l %a4,-(%sp)
	move.l %d4,-(%sp)
	move.l %d6,%a0
	jbsr (%a0)
	move.l %d2,%a3
	lea (20,%sp),%sp
	jbra .L83
.L65:
	pea 37.w
	move.l %d4,-(%sp)
	jbsr (%a4)
	addq.l #8,%sp
.L83:
	move.b (%a2),%d0
	jbne .L46
.L64:
.L76:
	movm.l -52(%fp),#0x3cfc
	unlk %fp
	rts
	.size	tfp_format, .-tfp_format
	.align	2
	.globl	init_printf
	.type	init_printf, @function
init_printf:
	link.w %fp,#0
	move.l 12(%fp),stdout_putf
	move.l 8(%fp),stdout_putp
	unlk %fp
	rts
	.size	init_printf, .-init_printf
	.align	2
	.type	putcp, @function
putcp:
	link.w %fp,#0
	move.l 8(%fp),%a1
	move.l (%a1),%a0
	move.b 15(%fp),(%a0)+
	move.l %a0,(%a1)
	unlk %fp
	rts
	.size	putcp, .-putcp
	.align	2
	.globl	tfp_sprintf
	.type	tfp_sprintf, @function
tfp_sprintf:
	link.w %fp,#-4
	move.l %d2,-(%sp)
	lea (16,%fp),%a0
	move.l %a0,-4(%fp)
	move.l %a0,-(%sp)
	move.l 12(%fp),-(%sp)
	pea putcp
	move.l %fp,%d2
	addq.l #8,%d2
	move.l %d2,-(%sp)
	jbsr tfp_format
	clr.l -(%sp)
	move.l %d2,-(%sp)
	jbsr putcp
	lea (24,%sp),%sp
	move.l -8(%fp),%d2
	unlk %fp
	rts
	.size	tfp_sprintf, .-tfp_sprintf
	.align	2
	.globl	tfp_printf
	.type	tfp_printf, @function
tfp_printf:
	link.w %fp,#-4
	lea (12,%fp),%a0
	move.l %a0,-4(%fp)
	move.l %a0,-(%sp)
	move.l 8(%fp),-(%sp)
	move.l stdout_putf,-(%sp)
	move.l stdout_putp,-(%sp)
	jbsr tfp_format
	lea (16,%sp),%sp
	unlk %fp
	rts
	.size	tfp_printf, .-tfp_printf
	.local	stdout_putf
	.comm	stdout_putf,4,2
	.local	stdout_putp
	.comm	stdout_putp,4,2
	.ident	"GCC: (GNU) 4.2.4"
