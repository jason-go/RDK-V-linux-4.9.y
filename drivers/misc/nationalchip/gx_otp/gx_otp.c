#include <linux/io.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include "gx_otp.h"

//#define OTP_DEBUG
#ifdef OTP_DEBUG
#define OTP_PRINTF    printk
#else
#define OTP_PRINTF(...)    do { }while(0);
#endif

static struct otp_info *o_info = NULL;

int sirius_otp_read_byte(unsigned int addr, unsigned char *data)
{
	int ret = 0;

	while ((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_BUSY) & 0x1);
	SIRIUS_OTP_CTRL &= ~(0xFFFF);
	SIRIUS_OTP_CTRL |= ((addr << 3) & 0xFFFF);
	SIRIUS_OTP_CTRL |= (1 << SIRIUS_OTP_CTRL_RD);

	while ((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_BUSY) & 0x1);
	if ((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_RD_VALID) & 0x1)
		*data = (SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_RD_DATA) & 0xFF;

	SIRIUS_OTP_CTRL &= ~(1 << SIRIUS_OTP_CTRL_RD);

	if ((!((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_RD_VALID) & 0x1)) || ((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_RW_FAIL) & 0x1))
		ret = -1;

	return ret;
}
EXPORT_SYMBOL(sirius_otp_read_byte);

int sirius_otp_read(unsigned int start_addr, int rd_num, unsigned char *data)
{
	int i = 0;
	int ret = 0;
	unsigned char val = 0;

	for (i = 0; i < rd_num; i++) {
		ret = sirius_otp_read_byte(start_addr + i, &val);
		if (ret < 0)
			return ret;
		else
			data[i] = val;
	}

	return rd_num;
}
EXPORT_SYMBOL(sirius_otp_read);

int sirius_otp_write_byte(unsigned int addr, unsigned char val)
{
	while ((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_BUSY) & 0x1);
	SIRIUS_OTP_CTRL &= ~(0xFFFF);
	SIRIUS_OTP_CTRL |= ((addr << 3) & 0xFFFF);

	SIRIUS_OTP_CTRL &= ~(0xFF << SIRIUS_OTP_CTRL_WR_DATA);
	SIRIUS_OTP_CTRL |= (val << SIRIUS_OTP_CTRL_WR_DATA);
	SIRIUS_OTP_CTRL |= (1 << SIRIUS_OTP_CTRL_WR);

	while ((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_BUSY) & 0x1);

	SIRIUS_OTP_CTRL &= ~(1 << SIRIUS_OTP_CTRL_WR);

	if (((SIRIUS_OTP_STATUS >> SIRIUS_OTP_STATUS_RW_FAIL) & 0x1))
		return -1;

	return 0;
}
EXPORT_SYMBOL(sirius_otp_write_byte);

int sirius_otp_write(unsigned int start_addr, int wr_num, unsigned char *data)
{
	int i = 0;
	int val = 0;

	for (i = 0; i < wr_num; i++) {
		val = sirius_otp_write_byte(start_addr + i, data[i]);
		if (val < 0)
			return val;
	}

	return wr_num;
}
EXPORT_SYMBOL(sirius_otp_write);

int taurus_otp_read(u32 start_addr,int num, u8 *data)
{
	int i;
	unsigned int otp_con_reg;

	otp_con_reg = 0x00000000; // set all control signals to be invalid, address to 0x1800(default)
	while (!(TAURUS_OTP_STA_REG & (1<<10))); // check if OTP Controller is ready
	for (i = 0; i < num; i++) {
		otp_con_reg = (0x7ff & (start_addr+i))<<3;  // start address
		while (TAURUS_OTP_STA_REG & 0x100); // check if OTP Controller is busy now; 1 for busy
		TAURUS_OTP_CON_REG = otp_con_reg | (0x1 << 14); // set READEN
		udelay(64);
		TAURUS_OTP_CON_REG &= ~(0x1 << 14); // clear READEN
		while (!(TAURUS_OTP_STA_REG & 0x200)); // chekc if OTP data is ready
		data[i] = (unsigned char)(TAURUS_OTP_STA_REG & 0xFF);
		TAURUS_OTP_CON_REG = 0x00000000;
	}
	TAURUS_OTP_CON_REG = 0x00000000; // set all control signals to be invalid, address to 0x1800(default)

	return num;
}
EXPORT_SYMBOL(taurus_otp_read);

int taurus_otp_write(u32 start_addr, int num, u8 *data)
{
	int i;
	unsigned int otp_con_reg;

	otp_con_reg = 0x00000000; // set all control signals to be invalid, address to 0x1800(default)
	while (!(TAURUS_OTP_STA_REG & (1<<10))); // check if OTP Controller is ready
	for (i = 0; i < num; i++) {
		otp_con_reg = (0x7ff & (start_addr+i))<<3; // start address
		otp_con_reg &= ~(0xff<<16);
		otp_con_reg |= data[i] << 16;
		while (TAURUS_OTP_STA_REG & 0x100); // check if OTP Controller is busy now; 1 for busy
		TAURUS_OTP_CON_REG = otp_con_reg | (0x1 << 15); // set BURN
		udelay(64);
		TAURUS_OTP_CON_REG = otp_con_reg; // clear BURN
		udelay(64);
	}
	otp_con_reg = 0x00000000; // set all control signals to be invalid, address to 0x1800(default)
	return num;
}
EXPORT_SYMBOL(taurus_otp_write);

static struct otp_table all_otp_info[] = {
	{0x6612, 512, sirius_otp_read,  sirius_otp_write},
	{0x6616, 288, taurus_otp_read, taurus_otp_write},
	{0x0,    0x0,    NULL,             NULL},
};

static int gx_otp_get_info(struct otp_info *info)
{
	int i = 0;
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->dev);

	struct device_node *otp_node = info->dev->of_node;
	if (!otp_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}
	OTP_PRINTF("otp regs base addr: 0x%x len: 0x%x\n", reg->start, resource_size(reg));

	info->reg_base = devm_ioremap_nocache(info->dev, reg->start, resource_size(reg));
	if (!info->reg_base) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}
	printk("otp regs mapped addr: 0x%x\n", (unsigned int)info->reg_base);

	info->table.chip_id = gx_chip_id_probe();
	if (info->table.chip_id < 0) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOENT;
	}

	for (i = 0; i < sizeof(all_otp_info)/sizeof(struct otp_table); i++) {
		if (all_otp_info[i].chip_id == info->table.chip_id) {
			info->table.max_size = all_otp_info[i].max_size;
			info->table.read = all_otp_info[i].read;
			info->table.write = all_otp_info[i].write;
			break;
		}
	}
	if (i == sizeof(all_otp_info)/sizeof(struct otp_table)) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOENT;
	}

	ret = 0;
	return ret;
}

static int gx_otp_probe(struct platform_device *pdev)
{
	int err = -1;
	
	struct otp_info *info = kmalloc(sizeof(struct otp_info), GFP_KERNEL);
	if (!info) {
		printk("error:[%s %d]\n", __func__, __LINE__);
		err = -ENOMEM;
		goto exit;
	}
	o_info = info;

	info->dev = &pdev->dev;

	err = gx_otp_get_info(info);
	if (err)
		goto exit;

	platform_set_drvdata(pdev, info);

	err = 0;
exit:
	return err;
}

static int gx_otp_remove(struct platform_device *pdev)
{
	struct otp_info *info = platform_get_drvdata(pdev);
	if (!info) {
		printk("error:[%s %d] no mem!\n", __func__, __LINE__);
		return -1;
	}

	platform_set_drvdata(pdev, NULL);

	kfree(info);
	info = NULL;

	return 0;
}

static struct of_device_id gxotp_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-otp",
		.data = NULL,
	},
};

static struct platform_driver gxotp_driver = {
	.probe		= gx_otp_probe,
	.remove		= gx_otp_remove,
	.driver = {
		.name = "gx_otp",
		.of_match_table = gxotp_device_match,
	},
};

module_platform_driver(gxotp_driver);

MODULE_DESCRIPTION("support for NationalChilp OTP modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
