#include <gxse_baseaddr.h>
#include <sirius_mpu.h>

	.global stage1_start
	.extern main
	.text
	.align  4
	.long   stage1_start
	.long   ifcp_align_isr
	.long   ifcp_mpu_isr
	.space  4
	.long   ifcp_illegal_isr
	.long   ifcp_supervisor_isr
	.space  4
	.long   ifcp_breakpoint_isr
	.long   ifcp_unrecoverable_isr
	.space  92
	.long   ifcp_timer_isr
	.space  12
	.long   ifcp_mailbox_isr
	.long   ifcp_mailbox_tee_isr
	.long   ifcp_klm_isr
	.long   ifcp_hash_isr
	.long   ifcp_crypto_isr
	.long   ifcp_sensor_isr
	.long   ifcp_misc_isr
	.long   ifcp_bus_parity_isr

stage1_start:
	mfcr    r10, CR<0,0>
	bseti   r10, 8
	bseti   r10, 6
	mtcr    r10, CR<0,0>

	lrw     r1, _start_text_
	mtcr    r1, vbr

stage1_stack_set:
	lrw	    r3, __super_stack
	mov	    r14, r3

	lrw	    r3, __user_stack
	mtcr    r3, cr<7,3>

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

stage2_mpu_config:
	lrw     r1, GXSCPU_BASE_ADDR_MPU
	lrw     r0, _start_text_
	st.w    r0, (r1, MPU_REG_REGION1_START)

	lrw     r0, __user_stack + 4 -1
	st.w    r0, (r1, MPU_REG_REGION1_END)

	lrw     r0, GXSCPU_BASE_ADDR_MAILBOX
	st.w    r0, (r1, MPU_REG_REGION2_START)

	lrw     r0, GXSCPU_BASE_ADDR_SENSOR+0x10000-1
	st.w    r0, (r1, MPU_REG_REGION2_END)

	lrw     r0, 0x70667710
	st.w    r0, (r1, MPU_REG_AUTH0)

	lrw     r0, 0x7
	st.w    r0, (r1, 0)

	lrw     r0, __user_stack + 4
	st.w    r0, (r1, MPU_REG_REGION3_START)

	lrw     r0, __super_stack
	st.w    r0, (r1, MPU_REG_REGION3_END)

	lrw     r0, 0x60667710
	st.w    r0, (r1, MPU_REG_AUTH0)

	lrw     r0, 0xf
	st.w    r0, (r1, 0)

stage2_user_main:
	lrw    r1, 0xffffffff
	lrw    r0, 0xe000e240
	st     r1, (r0)

	lrw    r1, 0xfff
	lrw    r0, 0xe000e100
	st     r1, (r0)

	/*enter user mode*/
	mfcr   r10, CR<0,0>
	bclri  r10, 31
	mtcr   r10, CR<0,0>

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

ifcp_mpu_isr:
#ifdef CFG_GXSE_FIREWALL
	push_stack
	bsr	 gx_mpu_isr
	pop_stack
#endif
	rte

ifcp_illegal_isr:
	br .

ifcp_supervisor_isr:
	br .

ifcp_breakpoint_isr:
	br .

ifcp_unrecoverable_isr:
#ifdef CFG_GXSE_FIREWALL
	push_stack
	bsr	 gx_mpu_isr
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

ifcp_mailbox_isr:
#ifdef CFG_GXSE_FIRMWARE
	push_stack
	bsr	 gx_mbox_scpu_isr
	pop_stack
#endif
	rte

ifcp_mailbox_tee_isr:
#ifdef CFG_GXSE_FIRMWARE_MBOX_TEE
	push_stack
	bsr	 gx_mbox_tee_scpu_isr
	pop_stack
#endif
	rte

ifcp_klm_isr:
#TODO
#ifdef CFG_GXSE_KLM
#	push_stack
#	bsr	 gx_klm_isr
#	pop_stack
#endif
	rte

ifcp_hash_isr:
#ifdef CFG_GXSE_MISC_HASH
	push_stack
	bsr	 gx_hash_isr
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

ifcp_sensor_isr:
#ifdef CFG_GXSE_MISC_SENSOR
	push_stack
	bsr	 gx_sensor_isr
	pop_stack
#endif
	rte

ifcp_bus_parity_isr:
	br .

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
