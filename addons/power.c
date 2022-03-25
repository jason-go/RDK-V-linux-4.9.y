#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/reboot.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>

static void gx3xxx_halt(void)
{
	printk("%s.\n", __func__);
	/* close mmu */
	asm volatile (
		"mfcr   r7, cr18;"
		"bclri  r7, 0;"
		"bclri  r7, 1;"
		"mtcr   r7, cr18;"
		:
		:
		:"r7"
	);

	/* jmp */
	((void (*)(void))(0x00100000+1024))();

	while(1);
}

static void gx3xxx_restart(void)
{
	char __iomem *regs = NULL;

	printk("%s.\n", __func__);

	regs = ioremap(0x0020b000, 0x8);
	if (!regs) {
		printk ("%s, %d ioremap failed\n", __func__, __LINE__);
		return ;
	}

	__raw_writel(
	((28800000/1000000 - 1) << 16)|(0x10000 - 10000),
	(regs + 0x4)
	);

	__raw_writel( 3, regs);

	iounmap(regs);

	while(1);
}

static int gx3xxx_restart_notify(struct notifier_block *this,
				   unsigned long mode, void *cmd)
{
	gx3xxx_restart();
	return NOTIFY_DONE;
}

static struct notifier_block gx3xxx_restart_handler = {
	.notifier_call = gx3xxx_restart_notify,
	.priority = 128,
};

static int __init board_register_restart_handler(void)
{
	extern unsigned int gx_chip_id_probe(void);
	unsigned int chip_id = gx_chip_id_probe();
	switch (chip_id) {
	case 0x3201:
	case 0x3211:
		pm_power_off = gx3xxx_halt;
		register_restart_handler(&gx3xxx_restart_handler);
		break;

	default:
		break;
	}

	return 0;
}

arch_initcall(board_register_restart_handler);

