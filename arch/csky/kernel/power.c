// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/reboot.h>
#include <asm/io.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

#ifdef CONFIG_MTD_M25P80
extern int m25p80_halt(void);
#else
int m25p80_halt(void)
{
	return 0;
}
EXPORT_SYMBOL(m25p80_halt);
#endif

void machine_power_off(void)
{
	local_irq_disable();
	if (pm_power_off)
		pm_power_off();
	asm volatile ("bkpt");
}

void machine_halt(void)
{
	printk("csky %s.\n", __func__);
	m25p80_halt();
	local_irq_disable();
	if (pm_power_off)
		pm_power_off();
	asm volatile ("bkpt");
}

void machine_restart(char *cmd)
{
	printk("csky %s.\n", __func__);
	
	m25p80_halt();
	local_irq_disable();
//	do_kernel_restart(cmd);
//	asm volatile ("bkpt");
#ifdef CONFIG_TAURUS
	{
		void __iomem *base = ioremap(0x020b000, 0x14);
		__raw_writel(
		((28800000/1000000 - 1) << 16)|(0x10000 - 10000),
		base
		);

		__raw_writel( 3, base);
		iounmap(base);
	}
#endif
	while(1);
}


