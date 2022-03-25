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

static int get_keys_pmu_mode(char *val, struct gx_tag_pmu *params)
{
	int i=0;

	params->keybuf[i++] = simple_strtoul(val, NULL, 0);

	for(;*val != 0;val++) {
		if(*val == ',' && *(++val) != 0) {
			params->keybuf[i++] = simple_strtoul(val, NULL, 0);
		}

		if (i >= sizeof(params->keybuf)/sizeof(unsigned int)) {
			break;
		}
	}
	params->keynum = i;
	return 0;
}

static int write_cmdline_pmu_mode(unsigned long arg, struct gx_lowpower_info *info)
{
	int err = 0;
	struct gx_tag_pmu *params_pmu;
	u32 WakeTime;
	u32 GpioMask;
	u32 GpioData;
	u32 key;
	u8 *cmdline;
	u8 *pcmd;
	char *param, *val;
	char *args;
	struct lowpower_info_s *lowpower = (struct lowpower_info_s __user *)arg;
	struct timex utctime;

	__get_user(WakeTime, (u32 __user *)&lowpower->WakeTime);
	__get_user(GpioMask, (u32 __user *)&lowpower->GpioMask);
	__get_user(GpioData, (u32 __user *)&lowpower->GpioData);
	__get_user(key, (u32 __user *)&lowpower->key);

	params_pmu = (struct gx_tag_pmu*)(info->regs);
	memset(params_pmu, 0x00, sizeof(struct gx_tag_pmu));
	params_pmu->tag_header = GXLOWPOWER_MAGIC_NUMBER; /* MAGIC */

	params_pmu->waketime = WakeTime;
	params_pmu->gpiomask = GpioMask;
	params_pmu->gpiodata = GpioData;
	do_gettimeofday(&(utctime.time));
	params_pmu->utctime = utctime.time.tv_sec;
	params_pmu->clock_speed = 27000000; /* lowpower default clock spped is 27MHZ */
	params_pmu->timeshowflag = 0;
	params_pmu->timezone = 0;
	params_pmu->timesummer = 0;
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
			if(0 == strcmp(param, "powercut")) {
				unsigned int ints[3];
				get_options(val, 3, ints);
				ints[2] = ints[2] > 0 ? 1 : 0;
				params_pmu->powerio[0] = ints[1];
				params_pmu->powerio[1] = ints[2];
			} else if (0 == strcmp(param, "lowpower_clock_speed")) {
				params_pmu->clock_speed = simple_strtoul(val, NULL, 0);
			} else if (0 == strcmp(param, "panelio")) {
				unsigned int ints[3];
				get_options(val, 3, ints);
				params_pmu->panelio[0] = ints[1];
				params_pmu->panelio[1] = ints[2];

			} else if (0 == strcmp(param, "timeshowflag")) {
				params_pmu->timeshowflag = simple_strtol(val, NULL, 0);
			} else if (0 == strcmp(param, "timezone")) {
				params_pmu->timezone = simple_strtol(val, NULL, 0);
			} else if (0 == strcmp(param, "timesummer")) {
				params_pmu->timesummer = simple_strtol(val, NULL, 0);
			} else if (0 == strcmp(param, "time_sec")) {
				params_pmu->utctime = simple_strtol(val, NULL, 0);
			} else
				get_keys_pmu_mode(val, params_pmu);
		}
		kfree(pcmd);
	} else {
		params_pmu->keybuf[0] = key;
		params_pmu->keynum = 1;
	}

	return err;
}

static int load_firmware_pmu_mode(const char *firmware_mem, unsigned int firmware_len, char *load_mem, struct gx_lowpower_info *info)
{
	int i = 0;
	int err = -1;
	unsigned int value = 0;
	unsigned int mem_max_len = DW8051_SRAM_SIZE;

	if (firmware_len > mem_max_len) {
		printk("error: [%s %d] firmware size(%d) is lager than ram size(%d) \n",
				__func__, __LINE__, firmware_len, mem_max_len);
		return err;
	}

	for (i = 0; i < firmware_len; i++) {
		value = (firmware_mem[i]) & 0xff;
		value |= ((DW8051_SRAM_BASE + i) & 0xffff) << 8;
		value |= (0 << 24);
		lowpower_writel(load_mem, CPU_WR_SRAM, value);
	}

	err = 0;
	return err;
}

static int set_utctime_pmu_mode(struct gx_lowpower_info *info)
{
	int ret = 0;
	struct gx_tag_pmu *dpram_pmu;

	/* if CPU was wakeup by PMU, CPU could read DPRAM VALUE to set
		* system utc time */
	dpram_pmu = (struct gx_tag_pmu*)(info->regs);
	if (dpram_pmu->tag_header == GXLOWPOWER_MAGIC_NUMBER) { /* Magic */
		struct timespec64 tv64 = {
			.tv_nsec = 0,
		};

		tv64.tv_sec = dpram_pmu->utctime;

		ret = do_settimeofday64(&tv64);
	}

	return ret;
}

static int get_wake_flag_pmu_mode(struct gx_lowpower_info *info)
{
	struct gx_tag_pmu *dpram_pmu;

	dpram_pmu = (struct gx_tag_pmu*)(info->regs);
	if (dpram_pmu->tag_header == GXLOWPOWER_MAGIC_NUMBER) { /* Magic */
		if (dpram_pmu->wakeflag != MANUAL_WAKE_FLAG &&
				dpram_pmu->wakeflag != AUTO_WAKE_FLAG)
		{
			dpram_pmu->wakeflag = NOT_WAKE_FLAG;
		}
		return dpram_pmu->wakeflag;
	}

	return NOT_WAKE_FLAG;
}

static int enter_lowpower_pmu_mode(struct gx_lowpower_info *info)
{
	char *load_mem = info->regs;
	unsigned int config_reg_value = 0;

	/* clear bit4 in CPU_CONFIG_REGS */
	config_reg_value = lowpower_readl(load_mem, CPU_CONFIG_REGS);
	config_reg_value &= ~(1 << 4);
	lowpower_writel(load_mem, CPU_CONFIG_REGS, config_reg_value);

	/* start mcu */
	config_reg_value = lowpower_readl(load_mem, CPU_CONFIG_REGS);
	config_reg_value |= 1;
	lowpower_writel(load_mem, CPU_CONFIG_REGS, config_reg_value);

	return 0;
}

struct gx_lowpower_ops gx_lowpower_pmu_ops = {
	.write_cmdline = write_cmdline_pmu_mode,
	.load_firmware = load_firmware_pmu_mode,
	.set_utctime = set_utctime_pmu_mode,
	.get_wakeflag = get_wake_flag_pmu_mode,
	.enter_lowpower = enter_lowpower_pmu_mode
};
