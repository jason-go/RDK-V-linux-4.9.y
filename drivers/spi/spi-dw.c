/*
 * Designware SPI core controller driver (refer pxa2xx_spi.c)
 *
 * Copyright (c) 2009, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>

#include "spi-dw.h"

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif

/* Slave spi_dev related */
struct chip_data {
	u8 cs;			/* chip select pin */
	u8 tmode;		/* TR/TO/RO/EEPROM */
	u8 type;		/* SPI/SSP/MicroWire */

	u8 poll_mode;		/* 1 means use poll mode */

	u8 enable_dma;
	u16 clk_div;		/* baud rate divider */
	u32 speed_hz;		/* baud rate */
	void (*cs_control)(u32 command);
};

#define MRST_SPI_DISABLE   0
#define MRST_SPI_ENABLE    1

extern unsigned int gx_chip_id_probe(void);
//cs is gpio 64
static void spi_cs_init(void)
{
	int value = 0;
	char __iomem *base = NULL;
	unsigned int gx_chipid = gx_chip_id_probe();

	printk("spi_cs_init is call gx_chipid (0x%x)\n",gx_chipid);
	switch(gx_chipid){
		case 0x3211:
			base = ioremap(0x0030a14c, 4);
			if (!base) {
				printk ("%s, %d ioremap failed\n", __func__, __LINE__);
				break;
			}
			value = readl(base);
			value |= 0xff;
			writel(value, base);
			iounmap(base);

			base = ioremap(0x00307000, 0x10);
			if (!base) {
				printk ("%s, %d ioremap failed\n", __func__, __LINE__);
				break;
			}
			value  = readl(base + 0x0);
			value |= (1 << 0);
			writel(value, base + 0x0);
			value  = readl(base + 0x8);
			value |= (1 << 0);
			writel(value, base + 0x8);
			iounmap(base);
			break;
		case 0x6612:
			// spi pin mux
			base = ioremap(0x89400140, 0x10);
			if (!base) {
				printk("error: [%s %d]\n", __func__, __LINE__);
				return;
			}
			value = readl(base);
			value |= 0x000001C0;
			writel(value, base);
			value = readl(base + 0x4);
			value &= ~(1 << 23);
			writel(value, base + 0x4);
			value = readl(base + 0x8);
			value |= 0x0000001E;
			writel(value, base + 0x8);
			iounmap(base);

			// spi cs init
			base = ioremap(0x82406000, 0x20);
			if (!base) {
				printk("error: [%s %d]\n", __func__, __LINE__);
				return;
			}
			value = readl(base + 0x04);
			value |= (1 << 31);
			writel(value, base + 0x04);
			iounmap(base);

			break;
		default:
			break;
	}
}

static char __iomem *cs_base = NULL;
static void spi_cs_control(u32 flag)
{
	int value = 0;
	unsigned int gx_chipid = gx_chip_id_probe();
	char __iomem *regs = NULL;

	switch(flag) {
		case MRST_SPI_DISABLE:
			switch (gx_chipid) {
				case 0x3211:
					regs = ioremap(0x00307008, 4);
					if (!regs) {
						printk ("%s, %d ioremap failed\n", __func__, __LINE__);
						break;
					}
					value  = readl(regs);
					value |= (1 << 0);
					writel(value, regs);
					iounmap(regs);
					break;
				case 0x6612:
					if (!cs_base) {
						cs_base = ioremap(0x82406000, 0x20);
						if (!cs_base) {
							printk("error: [%s %d]\n", __func__, __LINE__);
							return;
						}
					}
					value = readl(cs_base + 0x14);
					value |= (1 << 31);
					writel(value, cs_base + 0x14);
				default:
					break;
			}
			break;
		case MRST_SPI_ENABLE:
			switch (gx_chipid) {
				case 0x3211:
					regs = ioremap(0x0030700c, 4);
					if (!regs) {
						printk ("%s, %d ioremap failed\n", __func__, __LINE__);
						break;
					}
					value  = readl(regs);
					value |= (1 << 0);
					writel(value, regs);
					iounmap(regs);
					break;
				case 0x6612:
					if (!cs_base) {
						cs_base = ioremap(0x82406000, 0x20);
						if (!cs_base) {
							printk("error: [%s %d]\n", __func__, __LINE__);
							return;
						}
					}
					value = readl(cs_base + 0x18);
					value |= (1 << 31);
					writel(value, cs_base + 0x18);

				default:
					break;
			}
			break;
		default:
			return;
	}
}

static void spi_cs_remove(void)
{
	if (cs_base) {
		iounmap(cs_base);
		cs_base = NULL;
	}
}

#ifdef CONFIG_DEBUG_FS
#define SPI_REGS_BUFSIZE	1024
static ssize_t dw_spi_show_regs(struct file *file, char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct dw_spi *dws = file->private_data;
	char *buf;
	u32 len = 0;
	ssize_t ret;

	buf = kzalloc(SPI_REGS_BUFSIZE, GFP_KERNEL);
	if (!buf)
		return 0;

	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"%s registers:\n", dev_name(&dws->master->dev));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"=================================\n");
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"CTRL0: \t\t0x%08x\n", dw_readl(dws, DW_SPI_CTRL0));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"CTRL1: \t\t0x%08x\n", dw_readl(dws, DW_SPI_CTRL1));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SSIENR: \t0x%08x\n", dw_readl(dws, DW_SPI_SSIENR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SER: \t\t0x%08x\n", dw_readl(dws, DW_SPI_SER));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"BAUDR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_BAUDR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"TXFTLR: \t0x%08x\n", dw_readl(dws, DW_SPI_TXFLTR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"RXFTLR: \t0x%08x\n", dw_readl(dws, DW_SPI_RXFLTR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"TXFLR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_TXFLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"RXFLR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_RXFLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"SR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_SR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"IMR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_IMR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"ISR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_ISR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"DMACR: \t\t0x%08x\n", dw_readl(dws, DW_SPI_DMACR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"DMATDLR: \t0x%08x\n", dw_readl(dws, DW_SPI_DMATDLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"DMARDLR: \t0x%08x\n", dw_readl(dws, DW_SPI_DMARDLR));
	len += snprintf(buf + len, SPI_REGS_BUFSIZE - len,
			"=================================\n");

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
	return ret;
}

static const struct file_operations dw_spi_regs_ops = {
	.owner		= THIS_MODULE,
	.open		= simple_open,
	.read		= dw_spi_show_regs,
	.llseek		= default_llseek,
};

static int dw_spi_debugfs_init(struct dw_spi *dws)
{
	dws->debugfs = debugfs_create_dir("dw_spi", NULL);
	if (!dws->debugfs)
		return -ENOMEM;

	debugfs_create_file("registers", S_IFREG | S_IRUGO,
		dws->debugfs, (void *)dws, &dw_spi_regs_ops);
	return 0;
}

static void dw_spi_debugfs_remove(struct dw_spi *dws)
{
	debugfs_remove_recursive(dws->debugfs);
}

#else
static inline int dw_spi_debugfs_init(struct dw_spi *dws)
{
	return 0;
}

static inline void dw_spi_debugfs_remove(struct dw_spi *dws)
{
}
#endif /* CONFIG_DEBUG_FS */

static void dw_spi_set_cs(struct spi_device *spi, bool enable)
{
	struct dw_spi *dws = spi_master_get_devdata(spi->master);
	struct chip_data *chip = spi_get_ctldata(spi);

	/* Chip select logic is inverted from spi_set_cs() */
	if (chip && chip->cs_control)
		chip->cs_control(!enable);

	if (!enable)
		dw_writel(dws, DW_SPI_SER, BIT(spi->chip_select));
}

/* Return the max entries we can fill into tx fifo */
static inline u32 tx_max(struct dw_spi *dws)
{
	u32 tx_left, tx_room, rxtx_gap;

	tx_left = (dws->tx_end - dws->tx) / dws->n_bytes;
	tx_room = dws->tx_fifo_len - dw_readl(dws, DW_SPI_TXFLR);

	/*
	 * Another concern is about the tx/rx mismatch, we
	 * though to use (dws->fifo_len - rxflr - txflr) as
	 * one maximum value for tx, but it doesn't cover the
	 * data which is out of tx/rx fifo and inside the
	 * shift registers. So a control from sw point of
	 * view is taken.
	 */
	rxtx_gap = 0;// ((dws->rx_end - dws->rx) - (dws->tx_end - dws->tx))/ dws->n_bytes;

	return min3(tx_left, tx_room, (u32) (dws->tx_fifo_len - rxtx_gap));
}

/* Return the max entries we should read out of rx fifo */
static inline u32 rx_max(struct dw_spi *dws)
{
	u32 rx_left = (dws->rx_end - dws->rx) / dws->n_bytes;

	return min_t(u32, rx_left, dw_readl(dws, DW_SPI_RXFLR));
}

static void dw_writer(struct dw_spi *dws)
{
	u32 max = tx_max(dws);
	u16 txw = 0;

	while (max--) {
		/* Set the tx word if the transfer's original "tx" is not null */
		if (dws->tx_end - dws->len) {
			if (dws->n_bytes == 1)
				txw = *(u8 *)(dws->tx);
			else
				txw = *(u16 *)(dws->tx);
		}
		dw_write_io_reg(dws, DW_SPI_DR, txw);
		dws->tx += dws->n_bytes;
	}
}

static void dw_reader(struct dw_spi *dws)
{
	u32 max = rx_max(dws);
	u16 rxw;

	while (max--) {
		rxw = dw_read_io_reg(dws, DW_SPI_DR);
		/* Care rx only if the transfer's original "rx" is not null */
		if (dws->rx_end - dws->len) {
			if (dws->n_bytes == 1)
				*(u8 *)(dws->rx) = rxw;
			else
				*(u16 *)(dws->rx) = rxw;
		}
		dws->rx += dws->n_bytes;
	}
}

static void int_error_stop(struct dw_spi *dws, const char *msg)
{
	spi_reset_chip(dws);

	dev_err(&dws->master->dev, "%s\n", msg);
	dws->master->cur_msg->status = -EIO;
	spi_finalize_current_transfer(dws->master);
}

static void wait_till_not_busy(struct dw_spi *dws)
{
	unsigned long end = jiffies + 1 + usecs_to_jiffies(1000);
	
	while (time_before(jiffies, end))
	{
		if (!(dw_readw(dws, DW_SPI_SR) & SR_BUSY))
			return;
	}
	dev_err(&dws->master->dev,
			"DW SPI: Status keeps busy for 1000us after a read/write!\n");
}

u32 g_left;

static irqreturn_t interrupt_transfer(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);
	u32 left;
	u32 txint_level = dws->tx_fifo_len / 2;
	u32 rxint_level = dws->rx_fifo_len-1;

	/* Error handling */
	if (irq_status & (SPI_INT_TXOI | SPI_INT_RXOI | SPI_INT_RXUI)) {
		dw_readl(dws, DW_SPI_ICR);
		int_error_stop(dws, "interrupt_transfer: fifo overrun/underrun");
		return IRQ_HANDLED;
	}

	dw_reader(dws);
	if (dws->rx_end == dws->rx) {
		spi_mask_intr(dws, SPI_INT_TXEI|SPI_INT_RXFI);
		spi_finalize_current_transfer(dws->master);
		return IRQ_HANDLED;
	}
	else if ((dws->rx_end > dws->rx) && (irq_status & SPI_INT_RXFI))   
	{
		left = ((dws->rx_end - dws->rx) / dws->n_bytes) - 1;
		left = (left > rxint_level) ? rxint_level : left;
		spi_enable_chip(dws, 0);
		dw_writew(dws, DW_SPI_CTRL1, left);
		spi_enable_chip(dws, 1);
		dw_writew(dws, DW_SPI_RXFLTR, left);
		spi_umask_intr(dws, SPI_INT_RXFI);
		dw_writew(dws, DW_SPI_DR, 0); //启动接收模式
	}

	if (irq_status & SPI_INT_TXEI)
	{
		spi_mask_intr(dws, SPI_INT_TXEI);
		left = (dws->tx_end - dws->tx) / dws->n_bytes;
		left = (left > txint_level) ? txint_level : left;
		g_left = left;
		while (left--)
		{
			dw_writer(dws);
			wait_till_not_busy(dws);
		}
		if (dws->rx)
		{
			dw_reader(dws);
		}
		/* Re-enable the IRQ if there is still data left to tx */
		if (dws->tx_end > dws->tx)
			spi_umask_intr(dws, SPI_INT_TXEI);
		else
			spi_finalize_current_transfer(dws->master);
	}

	return IRQ_HANDLED;
}

static irqreturn_t dw_spi_irq(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct dw_spi *dws = spi_master_get_devdata(master);
	u16 irq_status = dw_readl(dws, DW_SPI_ISR) & 0x3f;

	if (!irq_status)
		return IRQ_NONE;

	if (!master->cur_msg) {
		spi_mask_intr(dws, SPI_INT_TXEI);
		return IRQ_HANDLED;
	}

	return dws->transfer_handler(dws);
}

/* Must be called inside pump_transfers() */
static int poll_transfer(struct dw_spi *dws)
{
	do {
		dw_writer(dws);
		dw_reader(dws);
		cpu_relax();
	} while (dws->rx_end > dws->rx);

	return 0;
}

static int dw_spi_transfer_one(struct spi_master *master,
		struct spi_device *spi, struct spi_transfer *transfer)
{
	struct dw_spi *dws = spi_master_get_devdata(master);
	struct chip_data *chip = spi_get_ctldata(spi);
	u8 imask = 0;
	u16 txlevel = 0;
	u16 rxlevel = 0;
	u32 cr0;
	int ret;
	int templen;

	dws->dma_mapped = 0;

	dws->tx = (void *)transfer->tx_buf;
	dws->tx_end = dws->tx + transfer->len;
	dws->rx = transfer->rx_buf;
	dws->rx_end = dws->rx + transfer->len;
	dws->len = transfer->len;

	spi_enable_chip(dws, 0);

	/* Handle per transfer options for bpw and speed */
	if (transfer->speed_hz != dws->current_freq) {
		if (transfer->speed_hz != chip->speed_hz) {
			/* clk_div doesn't support odd number */
			chip->clk_div = (DIV_ROUND_UP(dws->max_freq, transfer->speed_hz) + 1) & 0xfffe;
			chip->speed_hz = transfer->speed_hz;
		}
		dws->current_freq = transfer->speed_hz;
		spi_set_clk(dws, chip->clk_div);
	}
	if (transfer->bits_per_word == 8) {
		dws->n_bytes = 1;
		dws->dma_width = 1;
	} else if (transfer->bits_per_word == 16) {
		dws->n_bytes = 2;
		dws->dma_width = 2;
	} else {
		return -EINVAL;
	}
	/* Default SPI mode is SCPOL = 0, SCPH = 0 */
	cr0 = (transfer->bits_per_word - 1)
		| (chip->type << SPI_FRF_OFFSET)
		| (spi->mode << SPI_MODE_OFFSET)
		| (chip->tmode << SPI_TMOD_OFFSET);

	/*
	 * Adjust transfer mode if necessary. Requires platform dependent
	 * chipselect mechanism.
	 */
	if (chip->cs_control) {
		if (dws->rx && dws->tx)
			chip->tmode = SPI_TMOD_TR;
		else if (dws->rx)
			chip->tmode = SPI_TMOD_RO;
		else
			chip->tmode = SPI_TMOD_TO;

		cr0 &= ~SPI_TMOD_MASK;
		cr0 |= (chip->tmode << SPI_TMOD_OFFSET);
	}

	dw_writel(dws, DW_SPI_CTRL0, cr0);

	/* Check if current transfer is a DMA transaction */
	if (master->can_dma && master->can_dma(master, spi, transfer))
		dws->dma_mapped = master->cur_msg_mapped;

	/* For poll mode just disable all interrupts */
	spi_mask_intr(dws, 0xff);

	/*
	 * Interrupt mode
	 * we only need set the TXEI IRQ, as TX/RX always happen syncronizely
	 */
	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_setup(dws, transfer);
		if (ret < 0) {
			spi_enable_chip(dws, 1);
			return ret;
		}
	} else if (!chip->poll_mode) {
		/* Set the interrupt mask */
		imask = 0;
		
		if (chip->tmode == SPI_TMOD_RO)
		{
			templen = dws->len / dws->n_bytes - 1;
			rxlevel = dws->rx_fifo_len-1;
			rxlevel = (templen > rxlevel) ? rxlevel : templen;
			imask |= SPI_INT_RXFI;
		}
		else
		{
			templen = dws->len / dws->n_bytes;
			txlevel = dws->tx_fifo_len / 2;
			txlevel = (templen > txlevel) ? txlevel : templen;
			imask |= SPI_INT_TXEI;
		}

		dw_writew(dws, DW_SPI_CTRL1, rxlevel);

		dws->transfer_handler = interrupt_transfer;
	}

	spi_enable_chip(dws, 1);

	if (dws->dma_mapped) {
		ret = dws->dma_ops->dma_transfer(dws, transfer);
		if (ret < 0)
			return ret;
	}
	else
	{
		dw_writel(dws, DW_SPI_RXFLTR, rxlevel);
		dw_writel(dws, DW_SPI_TXFLTR, txlevel);
		if(chip->tmode == SPI_TMOD_RO)
		{
			dw_writew(dws, DW_SPI_DR, 0); //\u542f\u52a8\u63a5\u6536\u6a21\u5f0f
		}
		spi_umask_intr(dws, imask);
	}

	if (chip->poll_mode)
		return poll_transfer(dws);

	return 1;
}

static void dw_spi_handle_err(struct spi_master *master,
		struct spi_message *msg)
{
	struct dw_spi *dws = spi_master_get_devdata(master);

	if (dws->dma_mapped)
		dws->dma_ops->dma_stop(dws);

	spi_reset_chip(dws);
}

/* This may be called twice for each spi dev */
static int dw_spi_setup(struct spi_device *spi)
{
	struct dw_spi_chip *chip_info = NULL;
	struct chip_data *chip;
	int ret;

	/* Only alloc on first setup */
	chip = spi_get_ctldata(spi);
	if (!chip) {
		chip = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!chip)
			return -ENOMEM;
		spi_set_ctldata(spi, chip);
	}

	/*
	 * Protocol drivers may change the chip settings, so...
	 * if chip_info exists, use it
	 */
	chip_info = spi->controller_data;

	/* chip_info doesn't always exist */
	if (chip_info) {
		if (chip_info->cs_control)
			chip->cs_control = chip_info->cs_control;

		chip->poll_mode = chip_info->poll_mode;
		chip->type = chip_info->type;
	}
	else
		chip->cs_control = spi_cs_control;

	chip->tmode = SPI_TMOD_TR;
	spi->mode = 0x00;

	if (gpio_is_valid(spi->cs_gpio)) {
		ret = gpio_direction_output(spi->cs_gpio,
				!(spi->mode & SPI_CS_HIGH));
		if (ret)
			return ret;
	}

	return 0;
}

static void dw_spi_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	kfree(chip);
	spi_set_ctldata(spi, NULL);
}

/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct device *dev, struct dw_spi *dws)
{
	spi_reset_chip(dws);

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec
	 */
	if (!dws->tx_fifo_len)
	{
		u32 fifo;
		for (fifo = 2; fifo <= 257; fifo++)
		{
			dw_writew(dws, DW_SPI_TXFLTR, fifo);
			if (fifo != dw_readw(dws, DW_SPI_TXFLTR))
				break;
		}

		dws->tx_fifo_len = (fifo == 257) ? 0 : fifo;
		dw_writew(dws, DW_SPI_TXFLTR, 0);
	}
	if (!dws->rx_fifo_len)
	{
		u32 fifo;
		for (fifo = 2; fifo <= 257; fifo++)
		{
			dw_writew(dws, DW_SPI_RXFLTR, fifo);
			if (fifo != dw_readw(dws, DW_SPI_RXFLTR))
				break;
		}

		dws->rx_fifo_len = (fifo == 257) ? 0 : fifo;
		dw_writew(dws, DW_SPI_RXFLTR, 0);
	}

	spi_cs_init();
	spi_cs_control(MRST_SPI_DISABLE);
}

int dw_spi_add_host(struct device *dev, struct dw_spi *dws)
{
	struct spi_master *master;
	int ret;

	BUG_ON(dws == NULL);

	master = spi_alloc_master(dev, 0);
	if (!master)
		return -ENOMEM;

	dws->master = master;
	dws->type = SSI_MOTO_SPI;
	dws->dma_inited = 0;
	dws->dma_addr = (dma_addr_t)(dws->paddr + DW_SPI_DR);
	snprintf(dws->name, sizeof(dws->name), "dw_spi%d", dws->bus_num);

	ret = request_irq(dws->irq, dw_spi_irq, IRQF_SHARED, dws->name, master);
	if (ret < 0) {
		dev_err(dev, "can not get IRQ\n");
		goto err_free_master;
	}

	master->mode_bits = SPI_CS_HIGH | SPI_MODE_0;//SPI_CPOL | SPI_CPHA | SPI_LOOP;
	master->bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(16);
	master->bus_num = dws->bus_num;
	master->num_chipselect = dws->num_cs;
	master->setup = dw_spi_setup;
	master->cleanup = dw_spi_cleanup;
	master->set_cs = dw_spi_set_cs;
	master->transfer_one = dw_spi_transfer_one;
	master->handle_err = dw_spi_handle_err;
	master->max_speed_hz = dws->max_freq;
	master->dev.of_node = dev->of_node;

	/* Basic HW init */
	spi_hw_init(dev, dws);

	if (dws->dma_ops && dws->dma_ops->dma_init) {
		ret = dws->dma_ops->dma_init(dws);
		if (ret) {
			dev_warn(dev, "DMA init failed\n");
			dws->dma_inited = 0;
		} else {
			master->can_dma = dws->dma_ops->can_dma;
		}
	}

	spi_master_set_devdata(master, dws);
	ret = devm_spi_register_master(dev, master);
	if (ret) {
		dev_err(&master->dev, "problem registering spi master\n");
		goto err_dma_exit;
	}

	dw_spi_debugfs_init(dws);
	return 0;

err_dma_exit:
	if (dws->dma_ops && dws->dma_ops->dma_exit)
		dws->dma_ops->dma_exit(dws);
	spi_enable_chip(dws, 0);
	free_irq(dws->irq, master);
err_free_master:
	spi_master_put(master);
	return ret;
}
EXPORT_SYMBOL_GPL(dw_spi_add_host);

void dw_spi_remove_host(struct dw_spi *dws)
{
	dw_spi_debugfs_remove(dws);

	if (dws->dma_ops && dws->dma_ops->dma_exit)
		dws->dma_ops->dma_exit(dws);

	spi_shutdown_chip(dws);

	free_irq(dws->irq, dws->master);
	spi_cs_remove();
}
EXPORT_SYMBOL_GPL(dw_spi_remove_host);

int dw_spi_suspend_host(struct dw_spi *dws)
{
	int ret;

	ret = spi_master_suspend(dws->master);
	if (ret)
		return ret;

	spi_shutdown_chip(dws);
	return 0;
}
EXPORT_SYMBOL_GPL(dw_spi_suspend_host);

int dw_spi_resume_host(struct dw_spi *dws)
{
	int ret;

	spi_hw_init(&dws->master->dev, dws);
	ret = spi_master_resume(dws->master);
	if (ret)
		dev_err(&dws->master->dev, "fail to start queue (%d)\n", ret);
	return ret;
}
EXPORT_SYMBOL_GPL(dw_spi_resume_host);

MODULE_AUTHOR("Feng Tang <feng.tang@intel.com>");
MODULE_DESCRIPTION("Driver for DesignWare SPI controller core");
MODULE_LICENSE("GPL v2");
