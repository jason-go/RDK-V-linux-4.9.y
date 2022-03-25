/*
 *  Copyright (C) 2012-2015 Altera Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/irqchip.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/reboot.h>

#include <asm/hardware/cache-l2x0.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>

#include "core.h"

void __iomem *sys_manager_base_addr;
void __iomem *rst_manager_base_addr;
void __iomem *sdr_ctl_base_addr;

void __init sirius_sysmgr_init(void)
{

	struct device_node *np;

	//np = of_find_compatible_node(NULL, NULL, "nationalchip,sys-mgr");
	//sys_manager_base_addr = of_iomap(np, 0);

	np = of_find_compatible_node(NULL, NULL, "nationalchip,rst-mgr");
	rst_manager_base_addr = of_iomap(np, 0);

	//np = of_find_compatible_node(NULL, NULL, "nationalchip,sdr-ctl");
	//sdr_ctl_base_addr = of_iomap(np, 0);
}

static void __init sirius_init_irq(void)
{
	irqchip_init();
	sirius_sysmgr_init();
}

static void sirius_restart(enum reboot_mode mode, const char *cmd)
{
#define WATCHDOG_BASE (0x8200E000)
#define SYS_CLOCK     (27000000)
#define WDT_CLOCK     (1000)
#define WDT_TIME      (0x10000 - 1000)

	printk("watchdog to restart!\n");

	int __iomem *regs = ioremap(WATCHDOG_BASE, 0x10);
	if (!regs) {
		printk("%s %d : ioremap falied!\n", __func__, __LINE__);
		return;
	}

	writel(((SYS_CLOCK/WDT_CLOCK - 1) << 16) | WDT_TIME, regs + 4);
	writel(3, regs);
	iounmap(regs);
}

static const char *nationalchip_dt_match[] = {
	"nationalchip,sirius",
	NULL
};

DT_MACHINE_START(sirius, "nationalchip sirius")
	.l2c_aux_val	= 0,
	.l2c_aux_mask	= ~0,
	.init_irq	= sirius_init_irq,
	.restart	= sirius_restart,
	.dt_compat	= nationalchip_dt_match,
MACHINE_END

