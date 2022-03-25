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

#include "guestbus.h"

/* Register access macros */
#define guestbus_readl(base, reg)         __raw_readl(base + reg)
#define guestbus_writel(base, reg, value) __raw_writel((value), base + reg)

static struct guestbus_device_info guestbus_info;

static int guestbus_wait_buf_empty(char *load_mem)
{
	unsigned int wait_time = 0xFFFFFFFF;

	while(!(guestbus_readl(load_mem, SHAREMEM_ACPU2IOCPU+0x20) & 0x02)) {
		if (wait_time-- == 0) {
			return -1;
		}
	}

	return 0;
}

static int guestbus_load_firmware(char *firmware_name, struct device *dev, char *load_mem)
{
	int err = 0;
	const struct firmware *fw_entry = NULL;
	unsigned int firmware_len = 0;
	const char* firmware_data = NULL;
	unsigned int write_data = 0;
	unsigned int need_write_len = 0;
	int i = 0;
	int j = 0;

	err = request_firmware(&fw_entry, firmware_name, dev);
	if (err) {
		printk("error: [%s %d] requet firmware failed. err = %d.\n", __func__, __LINE__, err);
		return err;
	}

	firmware_len = fw_entry->size;
	firmware_data = fw_entry->data;

	if ((firmware_len % 32) != 0)
		need_write_len = firmware_len + 32 - (firmware_len % 32);
	else
		need_write_len = firmware_len;


	guestbus_writel(load_mem, IOCPU_ENABLE_REG, 0); /* enable io cpu */
	guestbus_writel(load_mem, IOCPU_ENABLE_REG, 1); /* enable io cpu */
	guestbus_writel(load_mem, SHAREMEM_ACPU2IOCPU, need_write_len);
	guestbus_writel(load_mem, SHAREMEM_ACPU2IOCPU+0x1c, 1);

	for (i = 0; i < firmware_len;) {
		guestbus_wait_buf_empty(load_mem);

		for (j = 0; j < SHAREMEM_BUF_LENGTH; j++) {
			if (i < firmware_len) {
				write_data = *((unsigned int*)&firmware_data[i]);
				i += 4;
			} else
				write_data = 0;
			guestbus_writel(load_mem, SHAREMEM_ACPU2IOCPU + (j * 4), write_data);
		}
	}

	return err;
}


static int guestbus_entry(unsigned long arg)
{
	int err = 0;
	struct guestbus_device_info *info = &guestbus_info;

	err = guestbus_load_firmware(GUESTBUS_FW_NAME, info->dev, info->regs);

	if (err)
		return err;

	return 0;
}

static int guestbus_open(struct inode *inode, struct file *filp)
{
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
static long guestbus_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int guestbus_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
	int ret = 0;

	switch(cmd) {
		case GUESTBUS_ENTRY:
			ret = guestbus_entry(arg);
			if (ret)
				ret = -EIO;
			break;
		default:
			return -ENOTTY;
	}

	return ret;
}

static struct file_operations guestbus_fops = {
	.owner = THIS_MODULE,
	.open = guestbus_open,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
	.unlocked_ioctl = guestbus_ioctl,
#else
	.ioctl = guestbus_ioctl,
#endif
};

static struct miscdevice guestbus_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "guestbus",
	.fops = &guestbus_fops,
};

static int get_source(struct guestbus_device_info *info)
{
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *guestbus_node = info->dev->of_node;
	if (!guestbus_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

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

static int guestbus_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct guestbus_device_info *info = &guestbus_info;

	memset(info,  0,  sizeof(struct guestbus_device_info));
	info->dev = &pdev->dev;

	ret = get_source(info);
	if (ret < 0)
		return ret;

	ret = misc_register(&guestbus_miscdev);
	if (ret != 0) {
		printk("error: [%s %d] failed to register misc device\n", __func__, __LINE__);
	}

	printk("guestbus_probe\n");

	return ret;
}

static int guestbus_remove(struct platform_device *pdev)
{
	struct guestbus_device_info *info = &guestbus_info;

	if (info->regs)
		devm_iounmap(info->dev, info->regs);

	kfree(info);
	misc_deregister(&guestbus_miscdev);
	printk("guestbus_remove\n");
	return 0;
}

#ifdef CONFIG_PM
int guestbus_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

int guestbus_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static struct of_device_id guestbus_device_match[] = {
	[0] = {
		.compatible = "nationalchip,guestbus",
		.data = NULL,
	},
};

static struct platform_driver guestbus_driver = {
	.probe = guestbus_probe,
	.remove = guestbus_remove,
#ifdef CONFIG_PM
	.suspend = guestbus_suspend,
	.resume = guestbus_resume,
#endif
	.driver = {
		.name = "guestbus",
		.of_match_table = guestbus_device_match,
	},
};

module_platform_driver(guestbus_driver);

MODULE_DESCRIPTION("support for NationalChilp guestbus modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
