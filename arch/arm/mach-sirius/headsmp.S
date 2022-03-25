/*
 *  Copyright (c) 2003 ARM Limited
 *  Copyright (c) u-boot contributors
 *  Copyright (c) 2012 Pavel Machek <pavel@denx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/memory.h>
#include <asm/assembler.h>

	.arch	armv7-a
	.arm

ENTRY(secondary_trampoline)
	/* CPU1 will always fetch from 0x0 when it is brought out of reset.
	 * Thus, we can just subtract the PAGE_OFFSET to get the physical
	 * address of &cpu1start_addr. This would not work for platforms
	 * where the physical memory does not start at 0x0.
	*/
	/* CNTVOFF Only accessible from Hyp mode, or from Monitor mode when SCR.NS is set to 1 */
	/* enter monitor mode */
	mrs r2, cpsr
	bic r2, r2, #0x1f
	orr r2, r2, #0x16           @ 0x16: monitor mode
	msr cpsr, r2

	/* set SCR.NS bit, copy from uboot */
	mrc p15, 0, r5, c1, c1, 0   @ read SCR
	bic r5, r5, #0x4a           @ clear IRQ, EA, nET bits
	orr r5, r5, #0x31           @ enable NS, AW, FW bits
	                            @ FIQ preserved for secure mode
	mcr p15, 0, r5, c1, c1, 0   @ write SCR (with NS bit set)
	isb

	/* clear CNTVOFF */
	movne   r4, #0
	mcrr p15, 4, r4, r4, c14    @ Reset CNTVOFF to zero
	isb

	/* clear SCR.NS for secure state */
	bic r5, r5, #0x01
	mcr p15, 0, r5, c1, c1, 0   @ write SCR (with NS bit set)
	isb

	/* back to svc mode */
	bic r2, r2, #0x1f
	orr r2, r2, #0x13           @ 0x13: svc mode
	msr cpsr, r2

	ldr	pc, 1f
ENDPROC(secondary_trampoline)
	.globl	siriuschip_boot_fn
siriuschip_boot_fn:
1:	.space	4

ENTRY(secondary_trampoline_end)
