#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

enum
{
	I2CON = 0,
	I2DATA,
	I2DIV,
};

#define I2CON_W_ACK      (1 << 1) //write ack
#define I2CON_R_ACK      (1 << 2) //read ack
#define I2CON_SI         (1 << 3) //operatin finish
#define I2CON_STO        (1 << 4) //stop
#define I2CON_STA        (1 << 5) //start
#define I2CON_I2EN       (1 << 6) //module enable
#define I2CON_I2IE       (1 << 7) //interrupt enable

#define GX3XXX_SYS_CLK 27000000UL
#define GX3XXX_I2C_CLK 100000UL

#define GXI2C_ACK_TIMEOUT 120 /* for 100kHz (>10*9us) */

enum
{
    I2C_READ = 0,
    I2C_WRITE
};

#if GX3XXX_I2C_CLK == 65000UL
#define I2C_DBG(fmt, args...)		udelay(100)
#else
#define I2C_DBG(fmt, args...)		if (g_debugon) printk(fmt, ##args)
#endif

struct gx31x0_i2c
{
#ifdef GX_I2C_LOCK
	struct semaphore lock;
#endif
	int i2c_count;
	unsigned char chip_address;
	unsigned int i2c_base;
	unsigned int i2c_pclk;
	unsigned int i2c_bus_freq;
	unsigned long __iomem *regs;
	unsigned long delay;
	struct i2c_adapter adap;
};

struct gx31x0_info {
	int i2c_count;
	struct device *dev;
	struct gx31x0_i2c *i2c;
};

static int g_debugon = 0;
static unsigned long gx_i2cretry = 10;

static int gx31x0_i2c_init(struct gx31x0_i2c *i2c);
static int gx31x0_i2c_remove(struct platform_device *pdev);

static int i2c_dont_care_ack = 0;

/* NOTE: In USER space, you can read()/write() via i2c-device interface in NONE-RESTART mode.
   If you want RESTART mode, you MUST use iotcl by I2C_RDWR.
   Define your own character device if you're NOT using i2c-device interface.
   I2C device node: /dev/i2c-0, i2c-1, i2c-2, etc. */

static int gxi2c_waitforack(struct gx31x0_i2c *i2c)
{
	u32 status = 0;
	u32 i = 0;

	I2C_DBG("%s, %p.\n", __func__, i2c->regs);
#if 1 /* the slave device MUST send ACK back in the 9th clk, here we just check */
	if(i2c->chip_address == 0xBA/*TVP51501_ADDR*/)
	{
		I2C_DBG("[%s:%d]TVP51501_ADDR\n",__func__,__LINE__);
		udelay(i2c->delay*4);
	}
	else
		udelay(i2c->delay);

	for (i=0; i<1000; ++i)
	{
		status = readl(i2c->regs+I2CON);
		if (status&I2CON_SI && !(status&I2CON_W_ACK))
			return 0;
		udelay(10);
	}
#else /* NEVER use time_before(when <10ms), since system scheduling MAYBE let registers not updated in time */
	unsigned long timeout = jiffies + msecs_to_jiffies(gx_i2ctimeout);

	do
	{
		status = readl(i2c->regs+I2CON);

		/* wait for SI & ACK */
		if (status&I2CON_SI && !(status&I2CON_W_ACK))
			return 0;

	} while (time_before(jiffies, timeout));
#endif

	if(i2c_dont_care_ack)
	{
		i2c_dont_care_ack = 0;
		return 0;
	}
	
	printk("%s(line %d): i2c wait for ACK failed (status %02x)\n", __func__, __LINE__, status);
	return -EAGAIN;
}

static int gx31x0_i2c_stop(struct gx31x0_i2c *i2c)
{
	u32 status = 0;
	u32 count = 1000;

	if(i2c->chip_address == 0xBA/*TVP51501_ADDR*/)
	{
		I2C_DBG("[%s:%d]TVP51501_ADDR\n",__func__,__LINE__);
		udelay(74);
	}
	
	writel(I2CON_I2EN|I2CON_STO, i2c->regs+I2CON);

	/* STOP need tiny delay */
	udelay(20);

	while(count) {
		status = readl(i2c->regs+I2CON);
		if (status&I2CON_SI)
			return 0;
		udelay(10);
		count--;
	}

	printk("%s(line %d): i2c send stop signal failed\n", __func__, __LINE__);
	return -EAGAIN;
}

static int gx31x0_i2c_start(struct gx31x0_i2c *i2c, struct i2c_msg *msg, unsigned long direction)
{
	u8 address = msg->addr;

	i2c->chip_address = address;
	
	switch (direction) {
		case I2C_READ:
			address |= 1;
			break;

		case I2C_WRITE:
			address &= ~1;
			break;

		default:
			return -EINVAL;
	}

	I2C_DBG("   (chipaddr|R/W: %02x)\n", address);

	writel(address, i2c->regs+I2DATA);
	/* start */
	writel(I2CON_I2EN|I2CON_STA, i2c->regs+I2CON);

	return gxi2c_waitforack(i2c);
}

static int gx31x0_i2c_read(struct gx31x0_i2c *i2c, struct i2c_msg *msg)
{
	u32 status = 0;
	int i = 0;
	int j = 0;

	I2C_DBG("\n------------- ALL READ DATA START (%d bytes) -------------\n", msg->len);

	/* NOTE: ACK/NOACK must be sent before reading data, this makes DATA updated,
	   or the 1st data is incorrect (device address). */
	for (i = 0; i < msg->len; i++) {
		/* send ACK/NOACK back to device */
		if ((i == msg->len - 1) && ((msg->flags & I2C_M_NO_RD_ACK) == I2C_M_NO_RD_ACK)) /* LAST byte: HIGH ack indicating the last one */
			writel(I2CON_I2EN | I2CON_R_ACK, i2c->regs + I2CON);
		else
			writel(I2CON_I2EN, i2c->regs + I2CON);

		//udelay(i2c->delay);

		//for (j=0; j<1000; ++j)
		for (j=0; j<200000; ++j)
		{
			status = readl(i2c->regs+I2CON);
			if (status&I2CON_SI){
				goto readok;
			}
			//udelay(10);
		}

		printk("%s(line %d): i2c read msg->buf[%d]failed\n", __func__, __LINE__, i);
		return -EAGAIN;
readok:
		/* read data out */
		msg->buf[i] = readl(i2c->regs+I2DATA) & 0xff;

		if (i && !(i%32)) I2C_DBG("\n");
		I2C_DBG("%02x ", msg->buf[i]);
	}
	I2C_DBG("\n------------- ALL READ DATA END   --------------\n");

	return 0;
}

static int gx31x0_i2c_write(struct gx31x0_i2c *i2c, struct i2c_msg *msg)
{
	int i = 0;

	I2C_DBG("\n------------- ALL WRITTEN DATA START (%d bytes) ---\n", msg->len);

	for (i=0; i<msg->len; ++i) {
		if (i && !(i%32)) I2C_DBG("\n");
		I2C_DBG("%02x ", msg->buf[i]);

		writel(msg->buf[i], i2c->regs+I2DATA);
		writel(I2CON_I2EN, i2c->regs+I2CON);

		if (gxi2c_waitforack(i2c))
		{
			printk("%s(line %d): i2c write msg->buf[%d] : %02x failed\n", __FUNCTION__, __LINE__, i,msg->buf[i]);
			return -EAGAIN;
		}
	}

	I2C_DBG("------------- ALL WRITTEN DATA END   --------------\n");

	return 0;
}

static int gx31x0_i2c_doxfer(struct gx31x0_i2c *i2c, struct i2c_msg *msgs, int num)
{
	u32 res = 0, i = 0;

	I2C_DBG("\n======== I2C TRANSFERS (i2c: %p) ========\n", i2c);

	/* NOTE: if i2c transfer interrupted by ISR/signals, the previous transfer maybe fail */
#ifdef GX_I2C_LOCK
	res = down_interruptible(&i2c->lock);
	if (res<0)
	{
		printk("%s(line %d): i2c transfer interrupted by ISR/signals (res: %s)\n", __func__, __LINE__, res==-EINTR?"-EINTR":"UNKNOWN");
		return res;
	}
#endif

	for (i=0; i<num; i++) {

		if ((msgs[i].flags & I2C_M_NOSTART)!=I2C_M_NOSTART) {
			/* send device self address simultaneously */
			I2C_DBG(" [%d] [R/W] start\n", i);
			res = gx31x0_i2c_start(i2c, msgs + i,
					((msgs[i].flags & I2C_M_RD) == I2C_M_RD) ? I2C_READ : I2C_WRITE);
			if (res)
				goto out;
		}

		/* 0 address width operations passed */
		if (!msgs[i].len)
			goto send_stop;

		if ((msgs[i].flags & I2C_M_RD) == I2C_M_RD) {
			/* read operations */
			I2C_DBG(" [%d] [R] read\n", i);
			res = gx31x0_i2c_read(i2c, msgs+i);
			if (res)
				goto out;
		}
		else {
			/* write operations */
			I2C_DBG(" [%d] [W] write\n", i);
			res = gx31x0_i2c_write(i2c, msgs + i);
			if (res)
				goto out;
		}

send_stop:
		if ((msgs[i].flags & I2C_M_REV_DIR_ADDR) != I2C_M_REV_DIR_ADDR) {
			//if(msgs[i].flags != 0)	//tmp debug
			{
				I2C_DBG(" [%d] [S] stop\n\n", i);
				res = gx31x0_i2c_stop(i2c);
			}
			if (res)
				goto out;
		}
	}

#ifdef GX_I2C_LOCK
	up(&i2c->lock);
#endif
	return num;

	/* send stop to make sure the state machine of slave device work properly */
out:
	gx31x0_i2c_stop(i2c);
#ifdef GX_I2C_LOCK
	up(&i2c->lock);
#endif
	return res;
}

extern unsigned int gx_chip_id_probe(void);
static void gx_i2c_mulpin_config(void)
{
	unsigned int gx_chipid = gx_chip_id_probe();
	char __iomem *regs = NULL;
	unsigned int tmp;

	switch(gx_chipid) {
	case 0x3201:
		/* multi pin config for gx3201 gx i2c0 */
		regs = ioremap(0x0030a148, 4);
		if (!regs) {
			printk ("%s, %d, ioremap failed\n", __func__, __LINE__);
			return ;
		}
		tmp = readl(regs);
		tmp &= ~(1 << 7);
		writel(tmp, regs);
		iounmap(regs);
		break;
	case 0x3211:
		regs = ioremap(0x0030a14c, 4);
		if (!regs) {
			printk ("%s, %d, ioremap failed\n", __func__, __LINE__);
			return ;
		}
		tmp = readl(regs);
		tmp &= ~(1 << 20);
		writel(tmp, regs);
		tmp = readl(regs);
		tmp &= ~(1 << 21);
		writel(tmp, regs);
		iounmap(regs);
		break;
	case 0x6616:
		regs = ioremap(0x0030a150, 4);
		if (!regs) {
			printk ("%s, %d, ioremap failed\n", __func__, __LINE__);
			return ;
		}
		tmp = readl(regs);
		tmp &= ~(1 << 20);
		writel(tmp, regs);
		tmp = readl(regs);
		tmp &= ~(1 << 21);
		writel(tmp, regs);
		iounmap(regs);
		break;
	default:
		break;
	}
}

/* gx31x0_i2c_xfer
 *
 * first port of call from the i2c bus code when an message needs
 * transferring across the i2c bus.
 */
static int gx31x0_i2c_xfer(struct i2c_adapter *adap,
		struct i2c_msg *msgs, int num)
{
	struct gx31x0_i2c *i2c = (struct gx31x0_i2c *)adap->algo_data;
	int retry = 0;
	int ret = 0;

	if (adap->nr == 0)
		gx_i2c_mulpin_config();

	I2C_DBG("%s: %x.\n", __func__, i2c->i2c_base);

	for (retry = 0; retry < adap->retries; retry++) {
		ret = gx31x0_i2c_doxfer(i2c, msgs, num);
		/* in case successed or some exceptions occured */
		if (ret != -EAGAIN)
			return ret;

		if (adap->retries < 2) return -EREMOTEIO;

		printk("%s(): Retrying transmission for device 0x%02x (NO. %d, total %d)(operatin: %s, i2cbaseaddress: 0x%08x, i2cid:%d, chipaddress: 0x%02x, address(one byte): 0x%02x)\n",
				__func__, msgs[0].addr, retry, adap->retries, msgs[1].flags&I2C_M_RD?"READ":"WRITE",i2c->i2c_base ,i2c->adap.nr,msgs[0].addr, msgs[0].buf[0]);

		udelay(100);
	}

	/* in case all retries failed */
	return -EREMOTEIO;
}

/* declare our i2c functionality */
static u32 gx31x0_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

static const struct i2c_algorithm gx31x0_i2c_algorithm = {
	.master_xfer   = gx31x0_i2c_xfer,
	.functionality = gx31x0_i2c_func,
};

static int gx31x0_i2c_init(struct gx31x0_i2c *i2c)
{
	unsigned long div = 0;

	/* i2c div set */
	div = i2c->i2c_pclk/i2c->i2c_bus_freq/4 - 1;
	div = (div<<16) + div;
	writel(div, i2c->regs+I2DIV);

	/* enable the i2c controller, disabling interrupt now */
	writel(I2CON_I2EN, i2c->regs+I2CON);

	return 0;
}

static int gx_i2c_get_info(struct gx31x0_info *info)
{
	int i = 0;
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *i2c_node = info->dev->of_node;
	if (!i2c_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	ret = of_property_read_u32(i2c_node, "i2c-count", &info->i2c_count);
	if (ret || (info->i2c_count < 0)) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	I2C_DBG("gx i2c count: %d\n", info->i2c_count);

	info->i2c = kzalloc(info->i2c_count * sizeof(struct gx31x0_i2c), GFP_KERNEL);
	if (!info->i2c) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}

	for (i = 0; i < info->i2c_count; i++) {
		struct gx31x0_i2c *i2c = &info->i2c[i];
		reg = platform_get_resource(device, IORESOURCE_MEM, i);
		if (!reg) {
			printk("error: [%s %d]\n", __func__, __LINE__);
			ret = -ENOMEM;
			goto exit;
		}

		i2c->regs = devm_ioremap_resource(info->dev, reg);
		if (!i2c->regs) {
			printk("error: [%s %d]\n", __func__, __LINE__);
			ret = -ENOMEM;
			goto exit;
		}

		ret = of_property_read_u32_index(i2c_node, "clock-frequency", i, &(i2c->i2c_bus_freq));
		if (ret) {
			printk("error: [%s %d]\n", __func__, __LINE__);
			goto exit;
		}

		ret = of_property_read_u32_index(i2c_node, "system-clock-frequency", i, &(i2c->i2c_pclk));
		if (ret) {
			printk("error: [%s %d]\n", __func__, __LINE__);
			goto exit;
		}
		I2C_DBG("gx i2c[%d] info: reg base addr [0x%x], reg mapped addr [0x%x], bus clock frequency [%d], system clock frequency [%d]\n",
				i, i2c->i2c_base, (unsigned int)i2c->regs, i2c->i2c_bus_freq, i2c->i2c_pclk);
	}

	ret = 0;
exit:
	if (ret) {
		if (info->i2c) {
			kfree(info->i2c);
			info->i2c = NULL;
		}
	}
	return ret;
}

static ssize_t dvb_i2c_write(struct file *file, const char __user *buf,
		size_t count, loff_t *pos)
{
	char lbuf[32];
	int ret;

	if (count >= sizeof(lbuf))
		return -EINVAL;

	if (copy_from_user(lbuf, buf, count))
		return -EFAULT;
	lbuf[count] = '\0';

	ret = sscanf(lbuf, "%d", &i2c_dont_care_ack);
	if (ret < 1)
		return -EINVAL;
	
	return count;		
}

static int dvb_i2c_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", i2c_dont_care_ack);
	return 0;
}

static int dvb_i2c_open(struct inode *inode, struct file *file)
{
	return single_open(file, dvb_i2c_proc_show, NULL);
}

static const struct file_operations gx31x0_i2c_fops = {
	.owner = THIS_MODULE,
	.open = dvb_i2c_open,
	.read = seq_read,
	.write = dvb_i2c_write,
	.llseek = seq_lseek,
	.release = single_release,
};

/* gx31x0_i2c_probe: called by the bus driver when a suitable device is found */
static int gx31x0_i2c_probe(struct platform_device *pdev)
{
#define I2C_NAME_LEN (10)
	struct gx31x0_info *info = NULL;
	unsigned char buf[I2C_NAME_LEN];
	int i = 0, ret = 0;
	struct proc_dir_entry *dvb_dir;
	struct proc_dir_entry *dvb_file;

	printk("Nationalchip I2C Controller Driver Registration\n");
	
	dvb_dir = proc_mkdir("i2c_ack", NULL);
	if(dvb_dir == NULL){
		return -ENOMEM;
	}
	dvb_file = proc_create("i2c_ack", 0644, dvb_dir,&gx31x0_i2c_fops);
	if(dvb_file == NULL){
		return -ENOMEM;
	}

	info = kzalloc(sizeof(struct gx31x0_info), GFP_KERNEL);
	if (!info) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}

	info->dev = &pdev->dev;
	platform_set_drvdata(pdev, info);

	ret = gx_i2c_get_info(info);
	if (ret)
		goto error;

	if (gx_i2cretry < 1) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		ret = -EINVAL;
		goto error;
	}

	for (i = 0; i < info->i2c_count; i++) {
		struct gx31x0_i2c *i2c = &info->i2c[i];
#ifdef GX_I2C_LOCK
		sema_init(&(i2c->lock), 1);
#endif
		i2c->delay     = 1000000UL/i2c->i2c_bus_freq*9; /* 90us for 100kHz */
		/* setup info block for the i2c core */

		sprintf(buf, "gx-i2c-%d", i);
		memcpy(&(i2c->adap.name), buf, I2C_NAME_LEN);
		i2c->adap.owner = THIS_MODULE;
		i2c->adap.algo = &gx31x0_i2c_algorithm;
		i2c->adap.retries = gx_i2cretry;
		i2c->adap.class = I2C_CLASS_HWMON;
		i2c->adap.algo_data = i2c;
		i2c->adap.dev.parent = &pdev->dev;

		/* initialise the i2c controller (NOTE: not using IRQ) */
		gx31x0_i2c_init(i2c);

		i2c->adap.nr = i;
		ret = i2c_add_numbered_adapter(&(i2c->adap));

		if (ret < 0) {
			dev_err(&pdev->dev, "failed to add bus to i2c core\n");
			goto error;
		}
	}

	return 0;
error:
	gx31x0_i2c_remove(pdev);
	return ret;
#undef I2C_NAME_LEN
}

static int gx31x0_i2c_remove(struct platform_device *pdev)
{
	struct gx31x0_info *info = platform_get_drvdata(pdev);
	int i = 0;

	if (info) {
		for (i = 0; i < info->i2c_count; i++) {
			struct gx31x0_i2c *i2c = &(info->i2c[i]);
			i2c_del_adapter(&(i2c->adap));
		}

		platform_set_drvdata(pdev, NULL);
		if (info) {
			if (info->i2c) {
				kfree(info->i2c);
				info->i2c = NULL;
			}
			kfree(info);
			info = NULL;
		}
	}

	return 0;
}

#ifdef CONFIG_PM
static int gx31x0_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int gx31x0_i2c_resume(struct platform_device *pdev)
{
	struct gx31x0_info *info = platform_get_drvdata(pdev);
	int i2c_count = 0, i = 0;

	if (info) {
		i2c_count = info->i2c_count;
		for (i = 0; i < i2c_count; i++)
			gx31x0_i2c_init(&info->i2c[i]);
	}

	return 0;
}
#else
#define gx31x0_i2c_suspend NULL
#define gx31x0_i2c_resume  NULL
#endif

static struct of_device_id gxi2c_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-i2c",
		.data = NULL,
	},
};

static struct platform_driver gxi2c_driver = {
	.probe		= gx31x0_i2c_probe,
	.remove		= gx31x0_i2c_remove,
	.suspend	= gx31x0_i2c_suspend,
	.resume		= gx31x0_i2c_resume,
	.driver = {
		.name = "gx-i2c",
		.of_match_table = gxi2c_device_match,
	},
};

module_platform_driver(gxi2c_driver);

MODULE_DESCRIPTION("support for NationalChilp I2C modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");

module_param(g_debugon, int, S_IRUGO);
module_param(gx_i2cretry, ulong, S_IRUGO);
