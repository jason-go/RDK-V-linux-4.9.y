/*
 * Copyright 2010-2011 Calxeda, Inc.
 * Copyright 2012 Pavel Machek <pavel@denx.de>
 * Based on platsmp.c, Copyright (C) 2002 ARM Ltd.
 * Copyright (C) 2012 Altera Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/cacheflush.h>
#include <asm/smp_scu.h>
#include <asm/smp_plat.h>

#include "core.h"
#include <linux/arm-smccc.h>

static int sirius_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
#ifndef CONFIG_TEE
	int trampoline_size = &secondary_trampoline_end - &secondary_trampoline;
	static u8 __iomem *zero;

	if (__pa(PAGE_OFFSET)) {
		zero = ioremap(0, trampoline_size);
		if (!zero) {
			pr_warn("BOOTUP jump vectors not accessible\n");
			return -1;
		}
	} else {
		zero = (__force u8 __iomem *)PAGE_OFFSET;
	}

	/* set the boot function for the sram code */
	siriuschip_boot_fn = virt_to_phys(secondary_startup);
	memcpy((__force void *)zero, &secondary_trampoline, trampoline_size);

	flush_cache_all();
	smp_wmb();
	outer_clean_range(0, trampoline_size);

	/* This will release CPU #1 out of reset. */
	writel(readl(rst_manager_base_addr) & (~( 1 << 16)), rst_manager_base_addr);
	writel(readl(rst_manager_base_addr+0xc) & (~( 1 << 16)), (rst_manager_base_addr+0xc));

	if (__pa(PAGE_OFFSET))
		iounmap(zero);

	return 0;
#else
	struct device_node *dn = of_find_node_by_path("/firmware/optee");
	if (dn) {
		unsigned int smccmd;
		struct arm_smccc_res res;
		of_property_read_u32(dn, "smccmd", &smccmd);
		arm_smccc_smc(smccmd,1,0,virt_to_phys(secondary_startup),0,0,0,0,&res);
	}

	return 0;
#endif
}

static void __init sirius_smp_prepare_cpus(unsigned int max_cpus)
{
}

/*
 * platform-specific code to shutdown a CPU
 *
 * Called with IRQs disabled
 */
static void sirius_cpu_die(unsigned int cpu)
{
	/* Do WFI. If we wake up early, go back into WFI */
	while (1)
		cpu_do_idle();
}

/*
 * We need a dummy function so that platform_can_cpu_hotplug() knows
 * we support CPU hotplug. However, the function does not need to do
 * anything, because CPUs going offline just do WFI. We could reset
 * the CPUs but it would increase power consumption.
 */
static int sirius_cpu_kill(unsigned int cpu)
{
	return 1;
}

static struct smp_operations sirius_smp_ops __initdata = {
	.smp_prepare_cpus	= sirius_smp_prepare_cpus,
	.smp_boot_secondary	= sirius_boot_secondary,
#ifdef CONFIG_HOTPLUG_CPU
	.cpu_die		= sirius_cpu_die,
	.cpu_kill		= sirius_cpu_kill,
#endif
};


CPU_METHOD_OF_DECLARE(sirius_smp, "nationalchip,sirius-smp", &sirius_smp_ops);
