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

extern unsigned int gx_chip_id_probe(void);
static struct gx_lowpower_info lowpower_info;
extern struct gx_lowpower_ops gx_lowpower_mem_ops;
extern struct gx_lowpower_ops gx_lowpower_pmu_ops;
static struct gx_lowpower_ops *lowpower_ops = NULL;

static int load_firmware(char *firmware_name, struct device *dev, char *load_mem)
{
	int err = 0;
	const struct firmware *fw_entry = NULL;

	err = request_firmware(&fw_entry, firmware_name, dev);
	if (err) {
		printk("error: [%s %d] requet firmware failed. err = %d.\n", __func__, __LINE__, err);
		return err;
	}

	if (lowpower_ops == NULL) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -1;
	}

	err = lowpower_ops->load_firmware(fw_entry->data, fw_entry->size, load_mem, &lowpower_info);
	if (err)
		return err;

	err = 0;
	return err;
}

/* You can use " around spaces, but can't escape ". */
/* Hyphens and underscores equivalent in parameter names. */
char *next_arg(char *args, char **param, char **val)
{
	unsigned int i, equals = 0;
	int in_quote = 0, quoted = 0;
	char *next;

	if (*args == '"') {
		args++;
		in_quote = 1;
		quoted = 1;
	}

	for (i = 0; args[i]; i++) {
		if (args[i] == ' ' && !in_quote)
			break;
		if (equals == 0) {
			if (args[i] == '=')
				equals = i;
		}
		if (args[i] == '"')
			in_quote = !in_quote;
	}

	*param = args;
	if (!equals)
		*val = NULL;
	else {
		args[equals] = '\0';
		*val = args + equals + 1;

		/* Don't include quotes in value. */
		if (**val == '"') {
			(*val)++;
			if (args[i-1] == '"')
				args[i-1] = '\0';
		}
		if (quoted && args[i-1] == '"')
			args[i-1] = '\0';
	}

	if (args[i]) {
		args[i] = '\0';
		next = args + i + 1;
	} else
		next = args + i;

	/* Chew up trailing spaces. */
	while (*next == ' ')
		next++;
	return next;
}


static int gx_lowpower_entry(unsigned long arg)
{
	int err = -1;
	struct gx_lowpower_info *info = &lowpower_info;

	err = load_firmware(GXLOWPOWER_FW_NAME, info->dev, info->regs);
	if (err)
		return err;

	if (lowpower_ops == NULL) {
		return -1;
	}
	err = lowpower_ops->write_cmdline(arg, info);
	if (err)
		return err;

	err = lowpower_ops->enter_lowpower(info);

	return err;
}

static int gxlowpower_open(struct inode *inode, struct file *filp)
{
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
static long gxlowpower_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int gxlowpower_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int ret = 0;
	struct gx_lowpower_info *info = &lowpower_info;
	unsigned int wakeflag = NOT_WAKE_FLAG;

	switch(cmd) {
		case LOWPOWER_ENTRY:
			ret = gx_lowpower_entry(arg);
			if (ret)
				ret = -EIO;
			break;

		case LOWPOWER_GETWAKEFLAG:
			printk("LOWPOWER_GETWAKEFLAG\n");

			if (lowpower_ops->get_wakeflag != NULL)
			{
				wakeflag = lowpower_ops->get_wakeflag(info);
			}

			ret = __put_user(wakeflag, (unsigned long*)arg);
			if (ret)
				ret = -EIO;
			break;

		default:
			return -ENOTTY;
	}

	return ret;
}

static struct file_operations gxlowpower_fops = {
	.owner = THIS_MODULE,
	.open = gxlowpower_open,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
	.unlocked_ioctl = gxlowpower_ioctl,
#else
	.ioctl = gxlowpower_ioctl,
#endif
};

static struct miscdevice gxlowpower_miscdev = {
	.minor = MISC_LOWPOWER_MINOR,
	.name = "gxlowpower",
	.fops = &gxlowpower_fops,
};

static int get_source(struct gx_lowpower_info *info)
{
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *lowpower_node = info->dev->of_node;
	if (!lowpower_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	if (of_irq_count(lowpower_node) > 0)
		info->irq = platform_get_irq(to_platform_device(info->dev), 0);

	info->reg_addr = reg->start;
	info->reg_size = reg->end - reg->start + 1;

	info->regs = devm_ioremap_resource(info->dev, reg);
	if (!info->regs) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}

	ret = 0;
	return ret;
}

static irqreturn_t gxlowpower_isr(int irq, void *data)
{
	return 0;
}

static int gxlowpower_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct gx_lowpower_info *info = &lowpower_info;
	unsigned int chip_id = 0;

	memset(info, 0, sizeof(struct gx_lowpower_info));
	info->dev = &pdev->dev;

	ret = get_source(info);
	if (ret < 0)
		return ret;

	if (info->irq > 0)
		if(request_irq(info->irq, gxlowpower_isr, IRQF_SHARED, info->dev->driver->name, info)) {
			printk("error: [%s %d] failed to request lowpower irq\n", __func__, __LINE__);
			return ret;
		}

	ret = misc_register(&gxlowpower_miscdev);
	if (ret != 0) {
		printk("error: [%s %d] failed to register misc device\n", __func__, __LINE__);
	}

	chip_id = gx_chip_id_probe();
	switch (chip_id) {
		case 0x6612: // sirius
			lowpower_ops = &gx_lowpower_pmu_ops;
			lowpower_ops->set_utctime(info);
			break;
		case 0x6616: // taurus
		case 0x3211:
			lowpower_ops = &gx_lowpower_mem_ops;
			break;
		default:
			lowpower_ops = NULL;
			break;
	}

	return ret;
}

static int gxlowpower_remove(struct platform_device *pdev)
{
	struct gx_lowpower_info *info = &lowpower_info;

	if (info->irq > 0)
		free_irq(info->irq, info);
	if (info->regs)
		devm_iounmap(info->dev, info->regs);

	kfree(info);
	misc_deregister(&gxlowpower_miscdev);
	return 0;
}

#ifdef CONFIG_PM
int gxlowpower_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

int gxlowpower_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static struct of_device_id gxlowpower_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-lowpower",
		.data = NULL,
	},
};

static struct platform_driver gxlowpower_driver = {
	.probe = gxlowpower_probe,
	.remove = gxlowpower_remove,
#ifdef CONFIG_PM
	.suspend = gxlowpower_suspend,
	.resume = gxlowpower_resume,
#endif
	.driver = {
		.name = "gxlowpower",
		.of_match_table = gxlowpower_device_match,
	},
};

module_platform_driver(gxlowpower_driver);

MODULE_DESCRIPTION("support for NationalChilp lowpower modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
