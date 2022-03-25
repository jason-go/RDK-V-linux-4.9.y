/*
 *  arch/arm/mach-sirius/pm.c
 *
 * Copyright (C) 2014-2015 nationalchip Corporation. All rights reserved.
 *
 * with code from pm-imx6.c
 * Copyright 2011-2014 Freescale Semiconductor, Inc.
 * Copyright 2011 Linaro Ltd.
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

#include <linux/bitops.h>
#include <linux/genalloc.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/suspend.h>
#include <asm/suspend.h>
#include <asm/fncpy.h>
#include "core.h"

/* Pointer to function copied to ocram */
static u32 (*sirius_sdram_self_refresh_in_ocram)(u32 sdr_base, u32 resume_base);
static void __iomem *resume_base_addr;
static phys_addr_t resume_addr; /* The physical resume address for asm code */

static int sirius_setup_ocram_self_refresh(void)
{
	struct platform_device *pdev;
	phys_addr_t ocram_pbase;
	struct device_node *np;
	struct gen_pool *ocram_pool;
	unsigned long ocram_base;
	void __iomem *suspend_ocram_base;
	int ret = 0;

	np = of_find_compatible_node(NULL, NULL, "mmio-sram");
	if (!np) {
		pr_err("%s: Unable to find mmio-sram in dtb\n", __func__);
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		pr_warn("%s: failed to find ocram device!\n", __func__);
		ret = -ENODEV;
		goto put_node;
	}

	ocram_pool = gen_pool_get(&pdev->dev, NULL);
	if (!ocram_pool) {
		pr_warn("%s: ocram pool unavailable!\n", __func__);
		ret = -ENODEV;
		goto put_node;
	}

	ocram_base = gen_pool_alloc(ocram_pool, sirius_sdram_self_refresh_sz);
	if (!ocram_base) {
		pr_warn("%s: unable to alloc ocram!\n", __func__);
		ret = -ENOMEM;
		goto put_node;
	}

	ocram_pbase = gen_pool_virt_to_phys(ocram_pool, ocram_base);

	suspend_ocram_base = __arm_ioremap_exec(ocram_pbase,
			sirius_sdram_self_refresh_sz,
			false);
	if (!suspend_ocram_base) {
		pr_warn("%s: __arm_ioremap_exec failed!\n", __func__);
		ret = -ENOMEM;
		goto put_node;
	}

	np = of_find_compatible_node(NULL, NULL, "nationalchip,resume-reg");
	resume_base_addr = of_iomap(np, 0);

	/* Copy the code that puts DDR in self refresh to ocram */
	sirius_sdram_self_refresh_in_ocram =
		(void *)fncpy(suspend_ocram_base,
				&sirius_sdram_self_refresh,
				sirius_sdram_self_refresh_sz);

	resume_addr = virt_to_phys(v7_cpu_resume);

	WARN(!sirius_sdram_self_refresh_in_ocram,
			"could not copy function to ocram");
	if (!sirius_sdram_self_refresh_in_ocram)
		ret = -EFAULT;

put_node:
	of_node_put(np);

	return ret;
}

static int sirius_pm_suspend(unsigned long arg)
{
	u32 ret;

	if (!sdr_ctl_base_addr )
		return -EFAULT;

	writel(resume_addr, resume_base_addr);
	ret = sirius_sdram_self_refresh_in_ocram((u32)sdr_ctl_base_addr, (u32)resume_base_addr);

	pr_debug("%s self-refresh loops request=%d exit=%d\n", __func__,
			ret & 0xffff, (ret >> 16) & 0xffff);

	return 0;
}

static int sirius_pm_enter(suspend_state_t state)
{
	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
	case PM_SUSPEND_FREEZE:
		outer_disable();
		/* flush cache back to ram */
		flush_cache_all();
		cpu_suspend(0, sirius_pm_suspend);
		writel(0, resume_base_addr);
		outer_resume();
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct platform_suspend_ops sirius_pm_ops = {
	.valid	= suspend_valid_only_mem,
	.enter	= sirius_pm_enter,
};

static int __init sirius_pm_init(void)
{
	int ret;

	ret = sirius_setup_ocram_self_refresh();
	if (ret)
		return ret;

	suspend_set_ops(&sirius_pm_ops);
	pr_info("sirius initialized for DDR self-refresh during suspend.\n");

	return 0;
}
arch_initcall(sirius_pm_init);
