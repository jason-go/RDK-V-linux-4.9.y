#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/io.h>

#include "gx_spi.h"

static void gx_spi_write(struct gx3201_spi_driver_data *sdd, unsigned int data)
{
	unsigned int regval;
	writel(data, &sdd->regs->SPI_TX_DATA);
	regval = readl(&sdd->regs->SPI_CR);
	regval |= CR_SPGO;
	writel(regval, &sdd->regs->SPI_CR);

	while(!(readl(&sdd->regs->SPI_SR) & SR_OPE_RDY));

	regval = readl(&sdd->regs->SPI_CR);
	regval &= ~CR_SPGO;
	writel(regval, &sdd->regs->SPI_CR);
}

static u32 gx_spi_read(struct gx3201_spi_driver_data *sdd)
{
	unsigned int regval;
	regval = readl(&sdd->regs->SPI_CR);
	regval |= CR_SPGO;
	writel(regval, &sdd->regs->SPI_CR);

	while(!(readl(&sdd->regs->SPI_SR) & SR_OPE_RDY));

	regval = readl(&sdd->regs->SPI_CR);
	regval &= ~CR_SPGO;
	writel(regval, &sdd->regs->SPI_CR);

	return readl(&sdd->regs->SPI_RX_DATA);
}

static void gx_spi_tx_len(struct gx3201_spi_driver_data *sdd, int size)
{
	int regval;
	regval = readl(&sdd->regs->SPI_CR);
	switch(size){
		case 1:
			if((regval & CR_ICNT_4) != 0){
				regval &= ~CR_ICNT_4;
				writel(regval, &sdd->regs->SPI_CR);
			}
			break;
		case 2:
			if((regval & CR_ICNT_4) != CR_ICNT_2){
				regval &= ~CR_ICNT_4;
				regval |= CR_ICNT_2;
				writel(regval, &sdd->regs->SPI_CR);
			}
			break;
		case 3:
			if((regval & CR_ICNT_4) != CR_ICNT_3){
				regval &= ~CR_ICNT_4;
				regval |= CR_ICNT_3;
				writel(regval, &sdd->regs->SPI_CR);
			}
			break;
		case 4:
			if((regval & CR_ICNT_4) != CR_ICNT_4){
				regval |= CR_ICNT_4;
				writel(regval, &sdd->regs->SPI_CR);
			}
			break;
		default:
			break;
	}
}

static int gx3201_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct gx3201_spi_driver_data *sdd = spi_master_get_devdata(spi->master);
	struct spi_transfer	*xfer;	
	u32 len = 0, data,regval;
	u8 *p;
	int retval;

	msg->actual_length = 0;
	
	if (list_empty(&msg->transfers)) {
		msg->status = -EFAULT;
		dev_err(&spi->dev, "xfer list empty\n");	
		return -EFAULT;
	}

	regval = readl(&sdd->regs->SPI_CR);
	regval |= CR_CSVALID;
	writel(regval, &sdd->regs->SPI_CR);
			
	list_for_each_entry(xfer, &msg->transfers, transfer_list) {
		if(xfer->len == 0 || (xfer->tx_buf && xfer->rx_buf)) {
			dev_err(&spi->dev, "xfer len is zero\n");
			retval = -EINVAL;
			msg->actual_length = 0;
			goto out;
		}		
				
		if (!(xfer->tx_buf || xfer->rx_buf) && xfer->len) {
			dev_err(&spi->dev, "xfer missing rx or tx buf\n");
			retval = -EINVAL;
			msg->actual_length = 0;
			goto out;
		}

		len = xfer->len;

		if(xfer->tx_buf){
			p = (u8 *)xfer->tx_buf;
			
			if(len >= SPI_TX_4BYTES)
				gx_spi_tx_len(sdd, 4);

			while(len >= SPI_TX_4BYTES) {
				data = (*p++) << 24;
				data |= (*p++) << 16;
				data |= (*p++) << 8;
				data |= (*p++);
				gx_spi_write(sdd, data);
				len -= SPI_TX_4BYTES;
			}

			if(len > 0){
				gx_spi_tx_len(sdd, 1);
				while(len--){
					data = *p++;
					gx_spi_write(sdd, data);
				}
			}
		}else if(xfer->rx_buf) {
			p = (u8 *)xfer->rx_buf;
			
			if(len >= SPI_TX_4BYTES)
				gx_spi_tx_len(sdd, 4);

			while(len >= SPI_TX_4BYTES) {
				data = gx_spi_read(sdd);
				*p++ = (data >> 24) & 0xff;
				*p++ = (data >> 16) & 0xff;
				*p++ = (data >>  8) & 0xff;
				*p++ = data & 0xff;
				len -= SPI_TX_4BYTES;
			}

			if(len > 0){
				gx_spi_tx_len(sdd, 1);
				while(len--){
					data = gx_spi_read(sdd);
					*p++ = data & 0xff;
				}
			}
		}

		msg->actual_length += xfer->len;
	}

	retval = 0;
	
out:
	regval = readl(&sdd->regs->SPI_CR);
	regval &= ~CR_CSVALID;
	writel(regval, &sdd->regs->SPI_CR);

	if(msg->complete != NULL)
		msg->complete(msg->context);

	msg->status = retval;

	return retval;
}

static int gx3201_spi_setup(struct spi_device *spi)
{
	struct gx3201_spi_driver_data *sdd;

	sdd = spi_master_get_devdata(spi->master);
	
	writel(0x8420c002, &sdd->regs->SPI_CR);
	return 0;
}

static void gx3201_spi_cleanup(struct spi_device *spi)
{
	spi = spi;
}

extern unsigned int gx_chip_id_probe(void);
static int gx3201_spi_get_info(struct gx3201_spi_driver_data *sdd)
{
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(sdd->dev);
	unsigned int chip_id = 0;
	char __iomem *base = NULL;
	int ret = -1;

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}

	sdd->regs = devm_ioremap_resource(sdd->dev, reg);
	if (!sdd->regs) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}

	chip_id = gx_chip_id_probe();
	switch(chip_id) {
		case 0x6612:
			base = ioremap(0x89400140, 0x14);
			if (!base) {
				printk("error: [%s %d]\n", __func__, __LINE__);
				return -ENOMEM;
			}
			ret = readl(base);
			ret |= 0x000001C0;
			writel(ret, base);
			ret = readl(base + 0x4);
			ret &= ~(1 << 23);
			writel(ret, base + 0x4);
			ret = readl(base + 0x8);
			ret &= ~(0x0000001E);
			writel(ret, base + 0x8);
			iounmap(base);
			break;
		default:
			break;
	}
	
	return 0;
}

static int /*__init*/ gx3201_spi_probe(struct platform_device *pdev)
{
	struct gx3201_spi_driver_data *sdd;
	struct spi_master *master;
	int ret;

	master = spi_alloc_master(&pdev->dev,sizeof(struct gx3201_spi_driver_data));
	if(master == NULL)
	{
		dev_err(&pdev->dev, "Unable to allocate SPI Master\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, master);
	
	sdd = spi_master_get_devdata(master);
	memset(sdd, 0, sizeof(struct gx3201_spi_driver_data));
	
	sdd->master = spi_master_get(master);
	sdd->master->bus_num = 0;
	sdd->master->num_chipselect = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39)
	sdd->master->mode_bits = SPI_CS_HIGH | SPI_MODE_0;
#endif
	
	sdd->master->setup = gx3201_spi_setup;
	sdd->master->transfer = gx3201_spi_transfer;
	sdd->master->cleanup = gx3201_spi_cleanup;
	sdd->pdev = pdev;
	sdd->dev = &pdev->dev;

	ret = gx3201_spi_get_info(sdd);
	if (ret) {
		dev_err(&pdev->dev, "Unable to remap IO\n");
		ret = -ENXIO;
		goto err;
	}

	ret =  spi_register_master(master);
	if(ret) {
		dev_err(&pdev->dev, "cannot register SPI master\n");
		goto err1;
	}
	
	dev_printk(KERN_INFO,&pdev->dev, "SPI Driver for Bus SPI-%d "
					"with %d Slaves\n",
					pdev->id, master->num_chipselect);
	
	return 0;

err1:
	devm_iounmap(sdd->dev,(void *)sdd->regs);
err:
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return ret;
}

static int gx3201_spi_remove(struct platform_device *pdev)
{
	struct spi_master *master = spi_master_get(platform_get_drvdata(pdev));
	struct gx3201_spi_driver_data *sdd = spi_master_get_devdata(master);

	spi_unregister_master(master);

	devm_iounmap(sdd->dev,(void *)sdd->regs);
	
	platform_set_drvdata(pdev, NULL);
	spi_master_put(master);

	return 0;
}

static struct of_device_id gx3201_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-spi",
		.data = NULL,
	},
};

static struct platform_driver gx3201_spi_driver = {
	.probe		= gx3201_spi_probe,
	.remove		= gx3201_spi_remove,
	.driver = {
		.name = "nc-spi",
		.of_match_table = gx3201_match,
	},
};

static int __init gx3201_spi_init(void)
{
	printk("Nationalchip SPI Controller Driver Registration\n");
	return platform_driver_register(&gx3201_spi_driver);
}

static void __exit gx3201_spi_exit(void)
{
	platform_driver_unregister(&gx3201_spi_driver);
}

postcore_initcall(gx3201_spi_init);
module_exit(gx3201_spi_exit);

MODULE_DESCRIPTION("support for NationalChilp SPI modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
