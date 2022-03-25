	.global stage1_start
	.extern main
	.text
	.align  4
	.long   stage1_start
	.long   ifcp_align_isr
	.space  8
	.long   ifcp_illegal_isr
	.long   ifcp_supervisor_isr
	.space  4
	.long   ifcp_breakpoint_isr
	.long   ifcp_unrecoverable_isr
	.space  92
	.long   ifcp_mailbox_isr
	.long   ifcp_timer_isr
	.long   ifcp_misc_isr
	.long   ifcp_crypto_isr

stage1_start:
	mfcr    r10, CR<0,0>
	bseti   r10, 8
	bseti   r10, 6
	mtcr    r10, CR<0,0>

	lrw     r1, _start_text_
	mtcr    r1, vbr

stage1_stack_set:
	lrw	    r3, __user_stack
	mov	    r14, r3

stage1_bss_init:
	lrw	    r5, _end_bss_
	lrw     r6, _start_bss_
	cmphs   r6, r5
	bt      2f             // if(r6>=r5)
	xor     r4, r4
1:
	st      r4, (r6)
	addi    r6, 4
	cmphs   r6, r5
	bf      1b             // if (r6<r5)
2:

stage1_copy_data_section:
    lrw     r5, _end_data
    lrw     r6, _start_data
    lrw     r7, _end_text
    cmphs   r6, r5
    bt      2f      // if(r6>=r5)
1:
    ld      r4,(r7)
    st      r4,(r6)
    addi    r6,4
    addi    r7,4
    cmphs   r6, r5
    bf      1b      // if (r6<r5)
2:

stage2_user_main:
	lrw    r1, 0xffffffff
	lrw    r0, 0xe000e240
	st     r1, (r0)

	lrw    r1, 0xf
	lrw    r0, 0xe000e100
	st     r1, (r0)

	br	   main

.macro push_stack
	subi   sp, 28
	stm    r0-r3, (sp)
	stw    t0, (sp, 16)    //r12
	stw    t1, (sp, 20)    //r13
	stw    lr, (sp, 24)    //r15
.endm

.macro pop_stack
	ldm    r0-r3, (sp)
	ldw    t0, (sp, 16)    //r12
	ldw    t1, (sp, 20)    //r13
	ldw    lr, (sp, 24)    //r15
	addi   sp, 28
.endm

ifcp_align_isr:
	br .

ifcp_illegal_isr:
	br .

ifcp_supervisor_isr:
	br .

ifcp_breakpoint_isr:
	br .

ifcp_unrecoverable_isr:
	br .

ifcp_mailbox_isr:
#ifdef CFG_GXSE_FIRMWARE
	push_stack
	bsr	 gx_mbox_scpu_isr
	pop_stack
#endif
	rte

ifcp_timer_isr:
#ifdef CFG_GXSE_MISC_TIMER
	push_stack
	bsr	 gx_timer_isr
	pop_stack
#endif
	rte

ifcp_crypto_isr:
#ifdef CFG_GXSE_CRYPTO
	push_stack
	bsr	 gx_crypto_isr
	pop_stack
#endif
	rte

ifcp_misc_isr:
#ifdef CFG_GXSE_MISC
	push_stack
	bsr	 gx_misc_isr
	pop_stack
#endif
	rte

	.global	BIG_TO_LITTLE
BIG_TO_LITTLE:
#	revb	a0, a0
	rts

	.global	ENDIAN_SWITCH
ENDIAN_SWITCH:
	revb	a0, a0
	rts

	.global	ENDIAN_SWITCH16
ENDIAN_SWITCH16:
#	revh	a0, a0
	rts

.end
