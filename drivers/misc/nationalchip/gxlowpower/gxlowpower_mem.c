#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/firmware.h>
#include <linux/gx_gpio.h>
#include <linux/slab.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/unistd.h>
#include "gxlowpower.h"

static int write_cmdline_mem_mode(unsigned long arg, struct gx_lowpower_info *info)
{
	int err = 0;
	struct gx_tag *params;
	int len;
	u32 WakeTime;
	u32 GpioMask;
	u32 GpioData;
	u32 key;
	u8 *cmdline;
	u8 *pcmd;
	char *param, *val;
	char *args;
	struct lowpower_info_s *lowpower = (struct lowpower_info_s __user *)arg;

	__get_user(WakeTime, (u32 __user *)&lowpower->WakeTime);
	__get_user(GpioMask, (u32 __user *)&lowpower->GpioMask);
	__get_user(GpioData, (u32 __user *)&lowpower->GpioData);
	__get_user(key, (u32 __user *)&lowpower->key);

	params = (struct gx_tag*)(info->regs);
	params->tag_header = GXLOWPOWER_MAGIC_NUMBER; /* MAGIC */
	len=sprintf(params->cmdline, "waketime=0x%x gpiomask=0x%x gpiodata=0x%x ", WakeTime, GpioMask, GpioData);
	if (key == 0) {
		__get_user(cmdline, (u32 __user *)&lowpower->cmdline);
		pcmd = kmalloc(strlen_user(cmdline), GFP_KERNEL);
		if (!pcmd)
			return err;
		err = copy_from_user(pcmd, cmdline, strlen_user(cmdline));
		args = pcmd;
		err = 0;
		/* Chew leading spaces */
		while (*args== ' ')
			args++;
		while (*args) {
			args = next_arg(args, &param, &val);
#ifdef CONFIG_GX_GPIO
			if(0 == strcmp(param, "powercut")) {
				int ret=0;
				unsigned int ints[3];
				unsigned int phy=0;
				get_options(val, 3, ints);
				ret = gx_gpio_getphy(ints[1], (unsigned long *)&phy);
				if (!ret) {
					ints[2] = ints[2] > 0 ? 1 : 0;
					len += sprintf(params->cmdline+len, " %s=%d,%d ", param, phy, ints[2]);
					err = 0;
				} else {
					err = 0;
					printk("powercut io %d can not find in gpio table, please check gpio_table !\n", ints[1]);
				}
			}
			else
#endif
				len += sprintf(params->cmdline+len," %s=%s ",param,val);
		}
		kfree(pcmd);
	} else
		sprintf(params->cmdline+len,"keys=0x%x ",key);

	return err;
}

static int load_firmware_mem_mode(const char *firmware_mem, unsigned int firmware_len, char *load_mem, struct gx_lowpower_info *info)
{
	int err = -1;
	unsigned int mem_max_len = 0;

	mem_max_len = info->reg_size - FIRMWARE_OFFSET;

	if (firmware_len > mem_max_len) {
		printk("error: [%s %d] firmware size(%d) is lager than ram size(%d) \n",
				__func__, __LINE__, firmware_len, mem_max_len);
		return err;
	}

	memcpy(load_mem + FIRMWARE_OFFSET, firmware_mem, firmware_len);

	err = 0;
	return err;
}

static int set_utctime_mem_mode(struct gx_lowpower_info *info)
{
	int ret = 0;

	/* do nothing */

	return ret;
}

static int enter_lowpower_mem_mode(struct gx_lowpower_info *info)
{
	/* do nothing */
	return 0;
}

struct gx_lowpower_ops gx_lowpower_mem_ops = {
	.write_cmdline = write_cmdline_mem_mode,
	.load_firmware = load_firmware_mem_mode,
	.set_utctime = set_utctime_mem_mode,
	.enter_lowpower = enter_lowpower_mem_mode
};
