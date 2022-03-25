/*
 * Memory-mapped interface driver for DW SPI Core
 *
 * Copyright (c) 2010, Octasic semiconductor.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/dmaengine.h>

#include <linux/of_platform.h>
#include <linux/property.h>

#include <linux/dma/dw.h>


#include "spi-dw.h"

#define DRIVER_NAME "dw_spi_gx"

#define RX_BUSY		0
#define TX_BUSY		1

#define APB_CLK_FRE (1024*1024*118)



struct dw_spi_gx {
	struct dw_spi  dws;
	struct device     *dma_dev;
};

struct mid_dma {
	struct dw_dma_slave dmas_tx;
	struct dw_dma_slave dmas_rx;
    struct dma_slave_config	rxconf;
	struct dma_slave_config	txconf;

};

struct mid_dma mid_dmas;


static bool mid_spi_dma_chan_filter(struct dma_chan *chan, void *param)
{
	struct dw_dma_slave *s = param;
	
	s->dma_dev = chan->device->dev;
	chan->private = s;
	return true;

}

static int dw_spi_dma_init(struct dw_spi *dws)
{
	struct mid_dma *dw_dma = dws->priv;
	struct dw_dma_slave *rxs, *txs;
	dma_cap_mask_t mask;

	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	txs = &dw_dma->dmas_tx;
    dws->dma_tx = &dw_dma->dmas_tx;
	txs->dst_id = 0;
	txs->src_id = 0;

	rxs = &dw_dma->dmas_rx;
	dws->dma_rx = &dw_dma->dmas_rx;
	rxs->dst_id = 1;
	rxs->src_id = 1;

	/* 1. Init tx channel */
	dws->txchan = dma_request_channel(mask, mid_spi_dma_chan_filter, txs);
	if (!dws->txchan)
		goto err_exit;
	dws->master->dma_tx = dws->txchan;

	/* 2. Init rx channel */
	dws->rxchan = dma_request_channel(mask, mid_spi_dma_chan_filter, rxs);
	if (!dws->rxchan)
		goto free_txchan;
	dws->master->dma_rx = dws->rxchan;

	dws->dma_inited = 1;
	return 0;

free_txchan:
	dma_release_channel(dws->txchan);
err_exit:
	return -1;

}

static void dw_spi_dma_exit(struct dw_spi *dws)
{
	if (!dws->dma_inited)
		return;

	dmaengine_terminate_sync(dws->txchan);
	dma_release_channel(dws->txchan);

	dmaengine_terminate_sync(dws->rxchan);
	dma_release_channel(dws->rxchan);
}
#if 0
#else
static irqreturn_t dma_transfer(struct dw_spi *dws)
{
	u16 irq_status = dw_readl(dws, DW_SPI_ISR);

	if (!irq_status)
		return IRQ_NONE;

	dw_readl(dws, DW_SPI_ICR);
	spi_reset_chip(dws);

	dev_err(&dws->master->dev, "%s: FIFO overrun/underrun\n", __func__);
	dws->master->cur_msg->status = -EIO;
	spi_finalize_current_transfer(dws->master);
	return IRQ_HANDLED;
}

static bool dw_spi_can_dma(struct spi_master *master, struct spi_device *spi,
		struct spi_transfer *xfer)
{
	struct dw_spi *dws = spi_master_get_devdata(master);

	if (!dws->dma_inited)
		return false;

	return xfer->len > dws->tx_fifo_len;
}

static enum dma_slave_buswidth convert_dma_width(u32 dma_width) {
	if (dma_width == 1)
		return DMA_SLAVE_BUSWIDTH_1_BYTE;
	else if (dma_width == 2)
		return DMA_SLAVE_BUSWIDTH_2_BYTES;

	return DMA_SLAVE_BUSWIDTH_UNDEFINED;
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for tx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_tx_done(void *arg)
{
	struct dw_spi *dws = arg;

	clear_bit(TX_BUSY, &dws->dma_chan_busy);
	if (test_bit(RX_BUSY, &dws->dma_chan_busy))
		return;
	spi_finalize_current_transfer(dws->master);
}

/*
 * dws->dma_chan_busy is set before the dma transfer starts, callback for rx
 * channel will clear a corresponding bit.
 */
static void dw_spi_dma_rx_done(void *arg)
{
	struct dw_spi *dws = arg;

	clear_bit(RX_BUSY, &dws->dma_chan_busy);
	if (test_bit(TX_BUSY, &dws->dma_chan_busy))
		return;
	spi_finalize_current_transfer(dws->master);
}

static struct dma_async_tx_descriptor *dw_spi_dma_prepare_tx(struct dw_spi *dws,
		struct spi_transfer *xfer)
{
	struct dma_slave_config txconf;
	struct dma_async_tx_descriptor *txdesc;

	if (!xfer->tx_buf)
		return NULL;

	memset(&txconf,0,sizeof(txconf));

	txconf.direction = DMA_MEM_TO_DEV;
	txconf.dst_addr = dws->dma_addr;
	txconf.dst_addr_width = convert_dma_width(dws->dma_width);
	txconf.device_fc = false;

	dmaengine_slave_config(dws->txchan, &txconf);

	txdesc = dmaengine_prep_slave_sg(dws->txchan,
				xfer->tx_sg.sgl,
				xfer->tx_sg.nents,
				DMA_MEM_TO_DEV,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc)
		return NULL;

	txdesc->callback = dw_spi_dma_tx_done;
	txdesc->callback_param = dws;

	return txdesc;
}

static struct dma_async_tx_descriptor *dw_spi_dma_prepare_rx(struct dw_spi *dws,
		struct spi_transfer *xfer)
{
	struct dma_slave_config rxconf;
	struct dma_async_tx_descriptor *rxdesc;
	struct dma_slave_config txconf;
	struct dma_async_tx_descriptor *txdesc;

	if (!xfer->rx_buf)
		return NULL;

	memset(&rxconf,0,sizeof(rxconf));

	rxconf.direction = DMA_DEV_TO_MEM;
	rxconf.src_addr = dws->dma_addr;
	rxconf.src_addr_width = convert_dma_width(dws->dma_width);
	rxconf.device_fc = false;

	dmaengine_slave_config(dws->rxchan, &rxconf);

	rxdesc = dmaengine_prep_slave_sg(dws->rxchan,
				xfer->rx_sg.sgl,
				xfer->rx_sg.nents,
				DMA_DEV_TO_MEM,
				DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!rxdesc)
		return NULL;

	rxdesc->callback = dw_spi_dma_rx_done;
	rxdesc->callback_param = dws;

	memset(&txconf,0,sizeof(txconf));

	txconf.direction = DMA_MEM_TO_DEV;
	txconf.dst_addr = dws->dma_addr;
	txconf.dst_addr_width = convert_dma_width(dws->dma_width);
	txconf.device_fc = false;

	dmaengine_slave_config(dws->txchan, &txconf);

	txdesc = dmaengine_prep_slave_sg(dws->txchan,
			xfer->rx_sg.sgl,
			xfer->rx_sg.nents,
			DMA_MEM_TO_DEV,
			DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!txdesc)
		return NULL;

	txdesc->callback = dw_spi_dma_tx_done;
	txdesc->callback_param = dws;
	dmaengine_submit(txdesc);
	dma_async_issue_pending(dws->txchan);	

	return rxdesc;
}

#endif
#define INVALID_DMA_ADDRESS    0x00 
int dw_spi_dma_map(struct spi_master *master, struct spi_transfer *xfer)
{
	xfer->tx_dma = xfer->rx_dma = INVALID_DMA_ADDRESS;
	if (xfer->tx_buf) 
	{
		xfer->tx_dma = dma_map_single(master->dev.parent,
				(void *) xfer->tx_buf, xfer->len, DMA_TO_DEVICE);
		if (dma_mapping_error(master->dev.parent, xfer->tx_dma))
			return -ENOMEM;
	}
	if (xfer->rx_buf) 
	{
		xfer->rx_dma = dma_map_single(master->dev.parent,
				xfer->rx_buf, xfer->len, DMA_FROM_DEVICE);
		if (dma_mapping_error(master->dev.parent, xfer->rx_dma)) 
		{
			if (xfer->tx_buf)
				dma_unmap_single(master->dev.parent,
						xfer->tx_dma, xfer->len, DMA_TO_DEVICE);
			return -ENOMEM;
		}
	}
	return 0;
}

static int dw_spi_dma_setup(struct dw_spi *dws, struct spi_transfer *xfer)
{
	u16 dma_ctrl = 0;

	dw_writel(dws, DW_SPI_DMARDLR, 7);
	dw_writel(dws, DW_SPI_DMATDLR, 0x10);

	if (xfer->tx_buf)
		dma_ctrl |= SPI_DMA_TDMAE;
	if (xfer->rx_buf)
		{
		dma_ctrl |= SPI_DMA_RDMAE;
		dma_ctrl |= SPI_DMA_TDMAE;
		}
	dw_writel(dws, DW_SPI_DMACR, dma_ctrl);

	/* Set the interrupt mask */
	spi_umask_intr(dws, SPI_INT_TXOI | SPI_INT_RXUI | SPI_INT_RXOI);

	dws->transfer_handler = dma_transfer;

	return 0;
}

static int dw_spi_dma_transfer(struct dw_spi *dws, struct spi_transfer *xfer)
{
	struct dma_async_tx_descriptor *txdesc, *rxdesc;

	/* Prepare the TX dma transfer */
	txdesc = dw_spi_dma_prepare_tx(dws, xfer);

	/* Prepare the RX dma transfer */
	rxdesc = dw_spi_dma_prepare_rx(dws, xfer);

	/* rx must be started before tx due to spi instinct */
	if (rxdesc) {
		set_bit(RX_BUSY, &dws->dma_chan_busy);
		dmaengine_submit(rxdesc);
		dma_async_issue_pending(dws->rxchan);
	}

	if (txdesc) {
		set_bit(TX_BUSY, &dws->dma_chan_busy);
		dmaengine_submit(txdesc);
		dma_async_issue_pending(dws->txchan);
	}

	return 0;
}

static void dw_spi_dma_stop(struct dw_spi *dws)
{
	if (test_bit(TX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_all(dws->txchan);
		clear_bit(TX_BUSY, &dws->dma_chan_busy);
	}
	if (test_bit(RX_BUSY, &dws->dma_chan_busy)) {
		dmaengine_terminate_all(dws->rxchan);
		clear_bit(RX_BUSY, &dws->dma_chan_busy);
	}
}

static const struct dw_spi_dma_ops mid_dma_ops = {
	.dma_init	= dw_spi_dma_init,
	.dma_exit	= dw_spi_dma_exit,
	.dma_setup	= dw_spi_dma_setup,
	.can_dma	= dw_spi_can_dma,
	.dma_transfer	= dw_spi_dma_transfer,
	.dma_stop	= dw_spi_dma_stop,
};

static int dw_spi_gx_probe(struct platform_device *pdev)
{
	struct dw_spi_gx *dws_gx;
	struct dw_spi *dws;
	struct resource *mem;
	int ret;
	int num_cs;

	dws_gx = devm_kzalloc(&pdev->dev, sizeof(struct dw_spi_gx),
			GFP_KERNEL);
	if (!dws_gx)
		return -ENOMEM;

	dws = &dws_gx->dws;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dws->paddr = mem->start;
	dws->regs = devm_ioremap_resource(&pdev->dev, mem);
	
	if (IS_ERR(dws->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		return PTR_ERR(dws->regs);
	}

	dws->irq = platform_get_irq(pdev, 0);
	if (dws->irq < 0) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return dws->irq; /* -ENXIO */
	}
	
	device_property_read_u16(&pdev->dev, "bus-num", &dws->bus_num);
	device_property_read_u32(&pdev->dev, "reg-io-width", &dws->reg_io_width);
	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);
	dws->max_freq = APB_CLK_FRE;

	dws->num_cs = num_cs;
	dws->dma_ops = &mid_dma_ops;
	dws->priv = &mid_dmas;

	ret = dw_spi_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dws_gx);
	return 0;

out:
	dw_spi_remove_host(dws);
	return ret;
}

static int dw_spi_gx_remove(struct platform_device *pdev)
{
	struct dw_spi_gx *dws_gx = platform_get_drvdata(pdev);

	dw_spi_remove_host(&dws_gx->dws);

	return 0;
}

static const struct of_device_id dw_spi_gx_of_match[] = {
	{ .compatible = "nationalchip-dw-spi", },
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, dw_spi_gx_of_match);

static struct platform_driver dw_spi_gx_driver = {
	.probe		= dw_spi_gx_probe,
	.remove		= dw_spi_gx_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = dw_spi_gx_of_match,
	},
};
module_platform_driver(dw_spi_gx_driver);

MODULE_AUTHOR("gx");
MODULE_DESCRIPTION("gx");
MODULE_LICENSE("GPL v2");
