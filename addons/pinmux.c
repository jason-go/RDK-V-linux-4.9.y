#include <linux/init.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/io.h>

static int __init board_devices_init(void)
{
	extern unsigned int gx_chip_id_probe(void);
	unsigned int chip_id = gx_chip_id_probe();
	char __iomem *regs = NULL;
	unsigned int tmp;

	switch (chip_id) {
	case 0x3201:
		regs = ioremap(0x0030a140, 0xc);
		if (!regs)
			return -1;

		// 0x0030a140
		tmp  = readl(regs + 0x0);
		tmp |= (1 << 17) | (1 << 18);
		writel(tmp, regs + 0x0);

		// 0x0030a148
		tmp  = readl(regs + 0x8);
		tmp |= (1 << 6);
		writel(tmp, regs + 0x8);

		iounmap(regs);
		break;

	case 0x3211:
		regs = ioremap(0x0030a14c, 0x4);
		if (!regs)
			return -1;

		tmp  = readl(regs);
		tmp |= (1 << 22) | (1 << 23);
		writel(tmp, regs);

		iounmap(regs);
		break;

	case 0x6701: // gemini
	case 0x6616: // taurus
		regs = ioremap(0x0030a150, 0x4);
		if (!regs)
			return -1;

		tmp  = readl(regs);
		tmp |= (1 << 22);
		writel(tmp, regs);

		iounmap(regs);
		break;

	default:
		break;
	}

	return 0;
}

arch_initcall(board_devices_init);

