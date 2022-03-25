#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/reboot.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/moduleparam.h>
#include <linux/clk.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/slab.h>

static unsigned timer_margin = 10000;
module_param(timer_margin, uint, 0);
MODULE_PARM_DESC(timer_margin, "initial watchdog timeout (in seconds)");

struct wdt_regs {
	unsigned int wdt_ctrl;
	unsigned int wdt_match;
	unsigned int wdt_count;
	unsigned int wdt_wsr;
};

struct wdt_info {
	unsigned int wdt_base;
	struct wdt_regs __iomem *regs;
	struct device *dev;
	unsigned int sys_clk;
	unsigned int wdt_clk;
	unsigned int wdt_isrvec;
};

struct wdt_info *w_info = NULL;

//#define WDT_DEBUG
#ifdef WDT_DEBUG
#define wdt_debug(FORMAT, ARGS...) printk("<debug>: ""%s()""[%d]   "FORMAT, __FUNCTION__, __LINE__, ##ARGS)
#else
#define wdt_debug(FORMAT, ARGS...)do {} while (0)
#endif

static int user_atoi(char __user *ubuf, size_t len)
{
        char buf[16];
        unsigned long ret;

        if (len > 15)
                return -EINVAL;

        if (copy_from_user(buf, ubuf, len))
                return -EFAULT;

        buf[len] = 0;
        ret = simple_strtoul(buf, NULL, 0);
        if (ret > INT_MAX)
                return -ERANGE;
        return ret;
}

static int nc_wdt_users;

static spinlock_t wdt_lock;

static void nc_wdt_ping(void)
{
	unsigned int val;

	val = (w_info->regs->wdt_wsr & 0xFFFF0000);
	w_info->regs->wdt_wsr = (0x5555 | val);
	w_info->regs->wdt_wsr = (0xAAAA | val);
}

static void nc_wdt_enable(void)
{
	w_info->regs->wdt_ctrl = 3;
}

static void nc_wdt_disable(void)
{
	w_info->regs->wdt_ctrl = 0;
	w_info->regs->wdt_match = 0;
	w_info->regs->wdt_count = 0;
	w_info->regs->wdt_wsr = 0;
}

static void nc_wdt_adjust_timeout(unsigned new_timeout)
{
	timer_margin = new_timeout;
}

static void nc_wdt_set_timeout(void)
{
	w_info->regs->wdt_match = ((w_info->sys_clk/w_info->wdt_clk - 1) << 16) | (0x10000 - timer_margin);
}

/*
 *	Allow only one task to hold it open
 */
static int nc_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(1, (unsigned long *)&nc_wdt_users))
		return -EBUSY;
	/* initialize prescaler */
	nc_wdt_disable();
	nc_wdt_set_timeout();
	nc_wdt_enable();
	return nonseekable_open(inode, file);
}

static int nc_wdt_release(struct inode *inode, struct file *file)
{
	//nc_wdt_disable();
	nc_wdt_users = 0;
	return 0;
}

static ssize_t nc_wdt_write(struct file *file, const char __user *data,
		size_t len, loff_t *ppos)
{
	int val;
	if (len) {
		spin_lock(&wdt_lock);
		val = user_atoi((char *)data, len);
		if (val > 0 && val != 44) {
			nc_wdt_adjust_timeout(val);
			nc_wdt_disable();
			nc_wdt_set_timeout();
			nc_wdt_enable();
		} else if (val == 44) {
			nc_wdt_disable();
		}
		nc_wdt_ping();
		spin_unlock(&wdt_lock);
	}
	return len;
}

static long nc_wdt_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)
{
	int new_margin;
	int options;
	static const struct watchdog_info ident = {
		.identity = "Nationalchip Watchdog",
		.options = WDIOF_SETTIMEOUT,
		.firmware_version = 0,
	};

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		return copy_to_user((struct watchdog_info __user *)arg, &ident,
				sizeof(ident));
	case WDIOC_GETSTATUS:
		return put_user(0, (int __user *)arg);
	case WDIOC_GETBOOTSTATUS:
		return 0;
	case WDIOC_KEEPALIVE:
		spin_lock(&wdt_lock);
		nc_wdt_ping();
		spin_unlock(&wdt_lock);
		return 0;
	case WDIOC_SETTIMEOUT:
		if (get_user(new_margin, (int __user *)arg))
			return -EFAULT;

		nc_wdt_adjust_timeout(new_margin);

		spin_lock(&wdt_lock);
		nc_wdt_disable();
		nc_wdt_set_timeout();
		nc_wdt_enable();

		nc_wdt_ping();
		spin_unlock(&wdt_lock);
		/* Fall */
	case WDIOC_GETTIMEOUT:
		return put_user(timer_margin, (int __user *)arg);
	case WDIOC_SETOPTIONS:
		if (copy_from_user(&options, (int __user *)arg, sizeof(options)))
			return -EFAULT;
		if (options & WDIOS_DISABLECARD){
			spin_lock(&wdt_lock);
			nc_wdt_disable();
			spin_unlock(&wdt_lock);
			return 0;
		}else if (options & WDIOS_ENABLECARD){
			spin_lock(&wdt_lock);
			nc_wdt_disable();
			nc_wdt_set_timeout();
			nc_wdt_enable();
			nc_wdt_ping();
			spin_unlock(&wdt_lock);
			return 0;
		}else
			return -EINVAL;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations nc_wdt_fops = {
	.owner = THIS_MODULE,
	.write = nc_wdt_write,
	.unlocked_ioctl = nc_wdt_ioctl,
	.open = nc_wdt_open,
	.release = nc_wdt_release,
};

static struct miscdevice nc_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &nc_wdt_fops,
};

static int nc_wdt_get_info(struct wdt_info *info)
{
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *wdt_node = info->dev->of_node;
	if (!wdt_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	if (of_irq_count(wdt_node) > 0) {
		info->wdt_isrvec = platform_get_irq(device, 0);
		wdt_debug("wdt intc source: %d\n", info->wdt_isrvec);
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	info->wdt_base = reg->start;
	wdt_debug("wdt regs base addr: 0x%x\n", info->wdt_base);

	info->regs = devm_ioremap_resource(info->dev, reg);
	if (!info->regs) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}
	wdt_debug("wdt regs mapped addr: 0x%p\n", info->regs);

	ret = of_property_read_u32(wdt_node, "clock-frequency", &info->wdt_clk);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	wdt_debug("wdt clock: %d\n", info->wdt_clk);

	ret = of_property_read_u32(wdt_node, "system-clock-frequency", &info->sys_clk);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	wdt_debug("system clock: %d\n", info->sys_clk);

	ret = 0;
	return ret;
}

static int nc_wdt_probe(struct platform_device *pdev)
{
	int ret;
	struct wdt_info *info = NULL;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		printk("error: [%s %d] get mem fail\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto fail;
	}
	w_info = info;
	memset(info, 0, sizeof(*info));
	info->dev = &pdev->dev;

	nc_wdt_users = 0;

	ret = nc_wdt_get_info(info);
	if (ret)
		goto fail;

	nc_wdt_disable();
	nc_wdt_adjust_timeout(timer_margin);

	platform_set_drvdata(pdev, info);
	nc_wdt_miscdev.parent = &pdev->dev;
	ret = misc_register(&nc_wdt_miscdev);
	if (ret)
		goto fail;

	spin_lock_init(&wdt_lock);
	printk("nc watchdog init success.\n");
	return 0;

fail:
	return ret;
}

static void nc_wdt_shutdown(struct platform_device *pdev)
{
	nc_wdt_disable();
}

static int nc_wdt_remove(struct platform_device *pdev)
{
	struct wdt_info *info = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	misc_deregister(&nc_wdt_miscdev);

	kfree(info);

	return 0;
}

#ifdef	CONFIG_PM

/* REVISIT ... not clear this is the best way to handle system suspend; and
 * it's very inappropriate for selective device suspend (e.g. suspending this
 * through sysfs rather than by stopping the watchdog daemon).  Also, this
 * may not play well enough with NOWAYOUT...
 */

static int nc_wdt_suspend(struct platform_device *pdev, pm_message_t state)
{
	if (nc_wdt_users)
		nc_wdt_disable();
	return 0;
}

static int nc_wdt_resume(struct platform_device *pdev)
{
	if (nc_wdt_users) {
		nc_wdt_enable();
		nc_wdt_ping();
	}
	return 0;
}

#else
#define	nc_wdt_suspend	NULL
#define	nc_wdt_resume		NULL
#endif

static struct of_device_id gxwdt_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-wdt",
		.data = NULL,
	},
};

static struct platform_driver gxwdt_driver = {
	.probe		= nc_wdt_probe,
	.remove		= nc_wdt_remove,
	.shutdown	= nc_wdt_shutdown,
#ifdef	CONFIG_PM
	.suspend	= nc_wdt_suspend,
	.resume		= nc_wdt_resume,
#endif
	.driver = {
		.name = "nc_wdt",
		.of_match_table = gxwdt_device_match,
	},
};

module_platform_driver(gxwdt_driver);

MODULE_DESCRIPTION("support for NationalChilp Wdt modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
