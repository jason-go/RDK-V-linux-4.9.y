#include <linux/io.h>
#include <linux/gx_gpio.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#ifndef NULL
#define NULL 0
#endif

#define ENOERR           0     /* No error */
#define EPERM            1     /* Not permitted */
#define ENOENT           2     /* No such entity */
#define ESRCH            3     /* No such process */
#define EINTR            4     /* Operation interrupted */
#define EIO              5     /* I/O error */
#define EBADF            9     /* Bad file handle */
#define EAGAIN           11    /* Try again later */
#define EWOULDBLOCK      EAGAIN
#define ENOMEM           12    /* Out of memory */
#define EBUSY            16    /* Resource busy */
#define EXDEV            18    /* Cross-device link */    
#define ENODEV           19    /* No such device */
#define ENOTDIR          20    /* Not a directory */
#define EISDIR           21    /* Is a directory */    
#define EINVAL           22    /* Invalid argument */
#define ENFILE           23    /* Too many open files in system */
#define EMFILE           24    /* Too many open files */
#define EFBIG            27    /* File too large */    
#define ENOSPC           28    /* No space left on device */
#define ESPIPE           29    /* Illegal seek */
#define EROFS            30    /* Read-only file system */    
#define EDOM             33    /* Argument to math function outside valid */

// gx gpio v1 register
#define		GPIO_EPDDR			0x00
#define		GPIO_EPDR			0x04
#define		GPIO_EPBSET			0x08
#define		GPIO_EPBCLR			0x0c
#define		GPIO_EPFER			0x10
#define		GPIO_EPFR			0x14
#define		GPIO_EPPAR			0x18
#define		GPIO_EPINV			0x1c
#define		GPIO_EPODR			0x20
#define		GPIO_PDM_SEL			0x24
#define		GPIO_PDM_DACEN			0x80
#define		GPIO_PDM_CLKSEL			0x84
#define		GPIO_PDM_DACIN			0x88

// gx gpio v1 register
#define		GPIO_DIR			0x00
#define		GPIO_SET_OUT			0x04
#define		GPIO_SET_IN			0x08
#define		GPIO_DOUT			0x10
#define		GPIO_SET_HI			0x14
#define		GPIO_SET_LO			0x18
#define		GPIO_PIN_VAL			0x1c
#define		GPIO_PWMSEL00_07		0x20
#define		GPIO_PWMSEL08_15		0x24
#define		GPIO_PWMSEL16_23		0x28
#define		GPIO_PWMSEL24_31		0x2c
#define		GPIO_USE_PWM			0x30
#define		GPIO_DS_EN			0x34
#define		GPIO_DS_CNT			0x38
#define		GPIO_MASK			0x3c
#define		GPIO_INT_EN			0x40
#define		GPIO_INT_STATUS			0x44
#define		GPIO_INT_MASK			0x48
#define		GPIO_INT_HI_EN			0x50
#define		GPIO_INT_LO_EN			0x54
#define		GPIO_INT_RISE_EN		0x58
#define		GPIO_INT_FAIL_EN		0x5c
#define		GPIO_PWM_DUTY0			0x80
#define		GPIO_PWM_DUTY1			0x84
#define		GPIO_PWM_DUTY2			0x88
#define		GPIO_PWM_DUTY3			0x8c
#define		GPIO_PWM_DUTY4			0x90
#define		GPIO_PWM_DUTY5			0x94
#define		GPIO_PWM_DUTY6			0x98
#define		GPIO_PWM_DUTY7			0x9c
#define		GPIO_PWM_CYC0			0xc0
#define		GPIO_PWM_CYC1			0xc4
#define		GPIO_PWM_CYC2			0xc8
#define		GPIO_PWM_CYC3			0xcc
#define		GPIO_PWM_CYC4			0xd0
#define		GPIO_PWM_CYC5			0xd4
#define		GPIO_PWM_CYC6			0xd8
#define		GPIO_PWM_CYC7			0xdc
#define		GPIO_PWM_PAUSE			0xe0
#define		GPIO_PWM_START			0xe4
#define		GPIO_PWM_PREDIV			0xf0


/* GPIO entry in bootloader */
struct gpio_entry_bootloader {
	unsigned char vir_gpio;  /* gpio virtual number */
	unsigned char phy_gpio;  /* gpio physical number */
	unsigned char config_valid:1;
	unsigned char io_mode:1;  /* 0: input, 1: output */
	unsigned char output_value:1;  /* 0: low, 1: high (only for output mode) */
	unsigned char reserved:5;
	unsigned char reserverd2;
};

/* GPIO table header in SRAM */
struct gpio_table_header {
	unsigned int   magic;
	unsigned char  valid;
	unsigned char  entry_num;
};

/* final GPIO entry in SDRAM */
struct gpio_entry
{
	unsigned char valid;
	unsigned char vir_gpio; /* gpio logical number */
	unsigned char phy_gpio; /* gpio physical number */
	unsigned char io_mode:1;  /* 0: input, 1: output */
	unsigned char output_value:1; /* 0: low, 1: high (only for output mode) */
	unsigned char reserved:6;
};

//#define DEBUG
#ifdef DEBUG
#define GPIO_PRINTF			printk
#else
#define GPIO_PRINTF(...)		do{}while(0);
#endif

#define GX_GPIO_PHY_NUM      (32*3)
#define GX_GPIO_TOTAL_NUM    (255)
#define GX_GPIO_MAX          (3)
#define GPIO_MAGIC       0x6770696f      // 'g' 'p' 'i' 'o'

struct gx_gpio_pri_info_s{
	struct mutex      lock;
	struct gpio_entry table[GX_GPIO_TOTAL_NUM];
	unsigned int __iomem *reg_base[GX_GPIO_MAX];
	unsigned int __iomem *sram_mapped_addr;
	struct device *dev;

	int output_read_support;  /* 0: not support. 1: support read output status from reg */
};

static struct gx_gpio_pri_info_s gx_gpio_pri_info;
static struct gx_gpio_pri_info_s *p_info = &gx_gpio_pri_info;

static int gx_gpio_convert(unsigned long vir_gpio, unsigned long *phy_gpio,  unsigned int *reg_base, unsigned long *offs)
{
	if (vir_gpio >= GX_GPIO_TOTAL_NUM)
		return -EINVAL;

	if(vir_gpio == p_info->table[vir_gpio].vir_gpio) {
		if (!p_info->table[vir_gpio].valid)
			return -ENOENT;
		*phy_gpio = p_info->table[vir_gpio].phy_gpio;
	} else
		return -EINVAL;

	if (*phy_gpio < 32) {
		*reg_base = (unsigned int)p_info->reg_base[0];
		*offs = *phy_gpio;
	} else if (*phy_gpio < 64) {
		*reg_base = (unsigned int)p_info->reg_base[1];
		*offs = *phy_gpio - 32;
	} else if (*phy_gpio < 96) {
		*reg_base = (unsigned int)p_info->reg_base[2];
		*offs = *phy_gpio - 64;
	} else
		return -EINVAL;

	return ENOERR;
}

/* setting gpio input/output mode, 0: input, 1: output */
int gx_gpio_setio(unsigned long gpio, unsigned long io)
{
	int ret = ENOERR;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base = 0;
	unsigned int tmp = 0;

	tmp = 0;
	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	p_info->table[gpio].io_mode = io;
#ifdef CONFIG_GX_GPIO_V1
	tmp = readl((const volatile void *)(reg_base + GPIO_EPDDR));
	if (io)
		tmp |= 1 << offs;
	else
		tmp &= ~(1 << offs);
	writel(tmp, (volatile void *)(reg_base + GPIO_EPDDR));
#elif defined(CONFIG_GX_GPIO_V2)
	if (io)
		writel(1 << offs, (volatile void *)(reg_base + GPIO_SET_OUT));
	else
		writel(1 << offs, (volatile void *)(reg_base + GPIO_SET_IN));
#endif

	return ENOERR;
}

/* getting gpio input/output mode, returns 0: input, 1: output */
int gx_gpio_getio(unsigned long gpio)
{
	int ret = ENOERR;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base = 0;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;
#ifdef CONFIG_GX_GPIO_V1
	return (readl((const volatile void *)(reg_base + GPIO_EPDDR)) & (1 << offs)) ? 1 : 0;
#elif defined(CONFIG_GX_GPIO_V2)
	return (readl((const volatile void *)(reg_base + GPIO_DIR)) & (1 << offs)) ? 1 : 0;
#endif
}

/* set the gpio high/low level for output pins, 0: low, 1: high */
int  gx_gpio_setlevel(unsigned long gpio, unsigned long value)
{
	int ret = ENOERR;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base = 0;

	if (value != 0 && value != 1)
		return -EINVAL;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	p_info->table[gpio].output_value= value;

#ifdef CONFIG_GX_GPIO_V1
	if (value)
		writel(1 << offs, (volatile void *)(reg_base + GPIO_EPBSET));
	else
		writel(1 << offs, (volatile void *)(reg_base + GPIO_EPBCLR));
#elif defined(CONFIG_GX_GPIO_V2)
	if (value)
		writel(1 << offs, (volatile void *)(reg_base + GPIO_SET_HI));
	else
		writel(1 << offs, (volatile void *)(reg_base + GPIO_SET_LO));
#endif

	return ENOERR;
}

/* get the gpio level for input/output pins, returns 0: low, 1: high */
int  gx_gpio_getlevel(unsigned long gpio)
{
	int ret = 0;
	unsigned long phy_gpio = 0;
	unsigned long offs = 0;
	unsigned int reg_base = 0;

	ret = gx_gpio_convert(gpio, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	if (p_info->output_read_support) {
#ifdef CONFIG_GX_GPIO_V1
		return (readl((const volatile void *)(reg_base + GPIO_EPDR)) & (1 << offs)) ? 1 : 0;
#elif defined(CONFIG_GX_GPIO_V2)
		return (readl((const volatile void *)(reg_base + GPIO_PIN_VAL)) & (1 << offs)) ? 1 : 0;
#endif
	} else {
#ifdef CONFIG_GX_GPIO_V1
	unsigned long tmp;
		tmp = readl((const volatile void *)(reg_base + GPIO_EPDDR));
		if (tmp & (1 << offs)) {
			return p_info->table[gpio].output_value;
		} else
			return (readl((const volatile void *)(reg_base + GPIO_EPDR)) & (1 << offs)) ? 1 : 0;
#endif
	}

	return -ENOENT;
}

#ifdef CONFIG_GX_GPIO_V1
int gx_gpio_set_pdm(struct gx_gpio_pdm_arg *pdm)
{
	int ret = 0;
	unsigned long offs = 0;
	unsigned long phy_gpio = 0;
	unsigned int reg_base = 0;
	unsigned int tmp;

	ret = gx_gpio_convert(pdm->gpio_num, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	if (pdm->pdm_enable) {
		tmp = readl((const volatile void *)(reg_base + GPIO_PDM_SEL));
		tmp |= 1 << offs;
		writel(tmp, (volatile void *)(reg_base + GPIO_PDM_SEL));

		pdm->div_clock = (pdm->div_clock > 7) ? 7 : pdm->div_clock;
		writel(pdm->div_clock, (volatile void *)(reg_base + GPIO_PDM_CLKSEL));

		pdm->duty_cycle = (pdm->duty_cycle > 100) ? 100 : pdm->duty_cycle;
		tmp = pdm->duty_cycle*0xffff/100;
		writel(tmp, (volatile void *)(reg_base + GPIO_PDM_DACIN));

		tmp = readl((const volatile void *)(reg_base + GPIO_PDM_DACEN));
		tmp |= (1<<0);
		tmp &= ~(1<<1);
		writel(tmp, (volatile void *)(reg_base + GPIO_PDM_DACEN));
	}
	else {
		tmp = readl((const volatile void *)(reg_base + GPIO_PDM_DACEN));
		tmp &= ~(3<<0);
		writel(tmp, (volatile void *)(reg_base + GPIO_PDM_DACEN));

		tmp = readl((const volatile void *)(reg_base + GPIO_PDM_SEL));
		tmp &= ~(1 << offs);
		writel(tmp, (volatile void *)(reg_base + GPIO_PDM_SEL));
	}
	return 0;
}

int gx_gpio_get_pdm(struct gx_gpio_pdm_arg *pdm)
{
	int ret = 0;
	unsigned long offs = 0;
	unsigned long phy_gpio = 0;
	unsigned int reg_base = 0;
	unsigned int tmp;

	ret = gx_gpio_convert(pdm->gpio_num, &phy_gpio, &reg_base, &offs);
	if(ret < 0)
		return ret;

	tmp = readl((const volatile void *)(reg_base + GPIO_PDM_CLKSEL));
	pdm->div_clock = (tmp > 7) ? 7 : tmp;

	tmp = readl((const volatile void *)(reg_base + GPIO_PDM_DACIN));
	pdm->duty_cycle = tmp*100/0xffff;
	pdm->duty_cycle += (tmp*100%0xffff) ? 1 : 0;

	tmp = readl((const volatile void *)(reg_base + GPIO_PDM_DACEN));
	tmp = ((tmp & 0x3) == 0x1) ? 1 : 0;
	pdm->pdm_enable = readl((const volatile void *)(reg_base + GPIO_PDM_SEL)) & (1 << offs) ? tmp : 0;

	return 0;
}
#endif

int  gx_gpio_getphy(unsigned long vir_gpio, unsigned long *phy_gpio)
{
	unsigned long offs = 0;
	unsigned int reg_base = 0;
	return gx_gpio_convert(vir_gpio, phy_gpio, &reg_base, &offs);
}

ssize_t gx_gpio_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
	return 0;
}

static int gx_gpio_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int gx_gpio_release(struct inode *inode, struct file *filp)
{
	return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
static long gx_gpio_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#else
static int gx_gpio_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
#endif
{
	int err;
	long ret = 0;
	struct gx_gpio_arg gpio_arg;
#ifdef CONFIG_GX_GPIO_V1
	struct gx_gpio_pdm_arg pdm_arg;
#endif

	switch(cmd) {
	case GX_GPIO_SET_IO:
		ret = copy_from_user(&gpio_arg, (struct gx_gpio_arg*)arg, sizeof(struct gx_gpio_arg));
		mutex_lock(&p_info->lock);
		err=gx_gpio_setio(gpio_arg.gpio_num, gpio_arg.gpio_io);
		mutex_unlock(&p_info->lock);
		if(err != ENOERR)
			return -EFAULT;
		break;

	case GX_GPIO_GET_IO:
		ret = copy_from_user(&gpio_arg, (struct gx_gpio_arg*)arg, sizeof(struct gx_gpio_arg));
		mutex_lock(&p_info->lock);
		gpio_arg.gpio_io = gx_gpio_getio(gpio_arg.gpio_num);
		mutex_unlock(&p_info->lock);
		if(gpio_arg.gpio_io < 0 )
			return -EFAULT;
		ret = copy_to_user((struct gx_gpio_arg*)arg, &gpio_arg, sizeof(struct gx_gpio_arg));
		break;

	case GX_GPIO_SET_LEVEL:
		ret = copy_from_user(&gpio_arg, (struct gx_gpio_arg*)arg, sizeof(struct gx_gpio_arg));
		mutex_lock(&p_info->lock);
		err=gx_gpio_setlevel(gpio_arg.gpio_num, gpio_arg.gpio_level);
		mutex_unlock(&p_info->lock);
		if(err != ENOERR)
			return -EFAULT;
		break;

	case GX_GPIO_GET_LEVEL:
		ret = copy_from_user(&gpio_arg, (struct gx_gpio_arg*)arg, sizeof(struct gx_gpio_arg));
		mutex_lock(&p_info->lock);
		gpio_arg.gpio_level = gx_gpio_getlevel(gpio_arg.gpio_num);
		mutex_unlock(&p_info->lock);
		if(gpio_arg.gpio_level < 0)
			return -EFAULT;
		ret = copy_to_user((struct gx_gpio_arg*)arg, &gpio_arg, sizeof(struct gx_gpio_arg));
		break;

#ifdef CONFIG_GX_GPIO_V1
	case GX_GPIO_SET_PDM:
		ret = copy_from_user(&pdm_arg, (struct gx_gpio_pdm_arg*)arg, sizeof(struct gx_gpio_pdm_arg));
		mutex_lock(&p_info->lock);
		err = gx_gpio_set_pdm(&pdm_arg);
		mutex_unlock(&p_info->lock);
		if(err != ENOERR)
			return -EFAULT;
		break;

	case GX_GPIO_GET_PDM:
		ret = copy_from_user(&pdm_arg, (struct gx_gpio_pdm_arg*)arg, sizeof(struct gx_gpio_pdm_arg));
		mutex_lock(&p_info->lock);
		err = gx_gpio_get_pdm(&pdm_arg);
		mutex_unlock(&p_info->lock);
		if(err != ENOERR)
			return -EFAULT;
		ret = copy_to_user((struct gx_gpio_pdm_arg*)arg, &pdm_arg, sizeof(struct gx_gpio_pdm_arg));
		break;
#endif

	default:
		break;
	}

	return ENOERR;
}

struct file_operations gx_gpio_fops = {
	.owner = THIS_MODULE,
	.open = gx_gpio_open,
	.read = gx_gpio_read,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))
	.unlocked_ioctl = gx_gpio_ioctl,
#else
	.ioctl = gx_gpio_ioctl,
#endif
	.release = gx_gpio_release,
};

static struct miscdevice gx_gpio_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gx_gpio",
	.fops = &gx_gpio_fops,
};

static int gx_gpio_get_info(struct gx_gpio_pri_info_s *info)
{
	int i = 0;
	int ret = -1;
	int reg_num = 0;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *gpio_node = info->dev->of_node;
	if (!gpio_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	ret = of_property_read_u32(gpio_node, "reg-num", &reg_num);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	reg_num = (reg_num > GX_GPIO_MAX) ? GX_GPIO_MAX : reg_num;

	for (i = 0; i < reg_num; i++) {
		reg = platform_get_resource(device, IORESOURCE_MEM, i);
		if (!reg) {
			printk("error: [%s %d]\n", __func__, __LINE__);
			return ret;
		}
		GPIO_PRINTF("gpio regs[%d] base addr: 0x%x len: 0x%x\n", i, reg->start, resource_size(reg));

		info->reg_base[i] = devm_ioremap_nocache(info->dev, reg->start, resource_size(reg));
		if (!info->reg_base) {
			printk("error: [%s %d]\n", __func__, __LINE__);
			return -ENOMEM;
		}
		GPIO_PRINTF("gpio regs[%d] mapped addr: 0x%p\n", i, info->reg_base[i]);
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, reg_num);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	GPIO_PRINTF("gpio sram base addr: 0x%x len: 0x%x\n", reg->start, resource_size(reg));

	info->sram_mapped_addr = devm_ioremap_nocache(info->dev, reg->start, resource_size(reg));
	if (!info->sram_mapped_addr) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}
	GPIO_PRINTF("gpio sram mapped addr: 0x%p\n", info->sram_mapped_addr);

	ret = of_property_read_u32(gpio_node, "output_read_support", &info->output_read_support);
	if (ret) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	GPIO_PRINTF("gpio output_read_support: %d\n", info->output_read_support);

	ret = 0;
	return ret;
}

/* copy and construct gpio map from SRAM into SDRAM */
static int gx_gpio_init(struct platform_device *pdev)
{
	int err;
	unsigned int i = 0;
	struct gpio_entry_bootloader *sram_entry = NULL;
	struct gpio_table_header *sram_gpio_table_header = NULL;

	GPIO_PRINTF("\n--------------------------------------------\n");
	GPIO_PRINTF("\t insmode gpio module \n");

	p_info->dev = &pdev->dev;
	err = gx_gpio_get_info(p_info);
	if (err)
		return err;

	sram_gpio_table_header = (struct gpio_table_header*)p_info->sram_mapped_addr;
	sram_entry = (struct gpio_entry_bootloader*)((unsigned int)p_info->sram_mapped_addr + sizeof(struct gpio_table_header));

	/* initialize the vir & phy to make it equal, and default invalid */
	for (i = 0; i < GX_GPIO_TOTAL_NUM; i++) {
		p_info->table[i].valid       = 0;
		p_info->table[i].vir_gpio    = i;
		p_info->table[i].phy_gpio    = 0;
		p_info->table[i].io_mode     = 0;
		p_info->table[i].output_value= 0;
	}

	/* if detected table, update from sram. if not detected, vir & phy is equal, and all valid */
	if ((GPIO_MAGIC == sram_gpio_table_header->magic) && sram_gpio_table_header->valid
			&& (sram_gpio_table_header->entry_num < GX_GPIO_TOTAL_NUM)) {
		for (i = 0; i < sram_gpio_table_header->entry_num; i++) {
			if ((sram_entry[i].phy_gpio >= GX_GPIO_PHY_NUM) || (sram_entry[i].vir_gpio >= GX_GPIO_TOTAL_NUM))
				return -ENOENT;

			p_info->table[sram_entry[i].vir_gpio].valid       = sram_entry[i].config_valid;
			p_info->table[sram_entry[i].vir_gpio].vir_gpio    = sram_entry[i].vir_gpio;
			p_info->table[sram_entry[i].vir_gpio].phy_gpio    = sram_entry[i].phy_gpio;
			p_info->table[sram_entry[i].vir_gpio].io_mode     = sram_entry[i].io_mode;
			p_info->table[sram_entry[i].vir_gpio].output_value= sram_entry[i].output_value;
		}
		GPIO_PRINTF("\n");
		for (i = 0; i < sram_gpio_table_header->entry_num; i++)
			GPIO_PRINTF("[SRAM entry No. %d] vir_gpio:%d, phy_gpio:%d, io_mode:%d, output_value:%d\n",
					i, sram_entry[i].vir_gpio, sram_entry[i].phy_gpio, sram_entry[i].io_mode,\
					sram_entry[i].output_value);
		GPIO_PRINTF("\n");
	} else {
		for (i = 0; i < GX_GPIO_TOTAL_NUM; i++)
			p_info->table[i].valid		= 1;
	}

	for (i = 0; i < GX_GPIO_TOTAL_NUM; i++)
		GPIO_PRINTF("[vir_gpio entry NO. %d] valid:%d, phy_gpio:%d, io_mode:%d, output_value:%d\n",
				p_info->table[i].vir_gpio, p_info->table[i].valid, p_info->table[i].phy_gpio,\
				p_info->table[i].io_mode, p_info->table[i].output_value);
	GPIO_PRINTF("\n");

	GPIO_PRINTF("------------------end-----------------------\n");
	
	mutex_init(&p_info->lock);
	err = misc_register(&gx_gpio_miscdev);
	if (err) {
		printk(KERN_ERR "gx_gpio - misc_register failed, err = %d\n", err);
		mutex_destroy(&p_info->lock);
		return err;
	}

	return ENOERR;
}

static int gx_gpio_exit(struct platform_device *pdev)
{
	misc_deregister(&gx_gpio_miscdev);
	return 0;
}

#ifdef CONFIG_PM
static int gx_gpio_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int gx_gpio_resume(struct platform_device *pdev)
{
	int i;
	int err;
	for (i = 0; i < GX_GPIO_TOTAL_NUM; i++) {
		if (p_info->table[i].valid) {
			mutex_lock(&p_info->lock);
			err=gx_gpio_setio(p_info->table[i].vir_gpio, p_info->table[i].io_mode);
			mutex_unlock(&p_info->lock);
			if(err != ENOERR)
				return -EFAULT;

			if (p_info->table[i].io_mode) {
				mutex_lock(&p_info->lock);
				err=gx_gpio_setlevel(p_info->table[i].vir_gpio,p_info->table[i].output_value);
				mutex_unlock(&p_info->lock);
				if(err != ENOERR)
					return -EFAULT;
			}
		}
	}
	return 0;
}
#endif

EXPORT_SYMBOL(gx_gpio_setio);
EXPORT_SYMBOL(gx_gpio_getio);
EXPORT_SYMBOL(gx_gpio_setlevel);
EXPORT_SYMBOL(gx_gpio_getlevel);
EXPORT_SYMBOL(gx_gpio_getphy);

static struct of_device_id gxgpio_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-gpio",
		.data = NULL,
	},
};

static struct platform_driver gxgpio_driver = {
	.probe		= gx_gpio_init,
	.remove		= gx_gpio_exit,
#ifdef	CONFIG_PM
	.suspend	= gx_gpio_suspend,
	.resume		= gx_gpio_resume,
#endif
	.driver = {
		.name = "gx_gpio",
		.of_match_table = gxgpio_device_match,
	},
};

module_platform_driver(gxgpio_driver);

MODULE_DESCRIPTION("support for NationalChilp Gpio modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
