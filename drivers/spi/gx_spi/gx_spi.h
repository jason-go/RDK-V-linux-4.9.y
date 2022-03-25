#ifndef __SPI_GX3110_H__
#define __SPI_GX3110_H__

#include <linux/init.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define CR_NO_INTER_DELAY	(0x1 << 21)
#define CR_LSB				(0x1 << 20)
#define CR_POS_RX			(0x1 << 19)
#define CR_CSEN				(0x1 << 17)
#define CR_CSFORCE			(0x1 << 16)
#define CR_CSVALID			(0x3 << 16)
#define CR_ICNT_2			(0x1 << 14)
#define CR_ICNT_3			(0x2 << 14)
#define CR_ICNT_4			(0x3 << 14)
#define CR_SPIE				(0x1 << 13)
#define CR_SPGO				(0x1 << 12)
#define CR_CSPOL_H			(0x1 << 10)
#define CR_CPOL_H			(0x1 << 9)
#define CR_CPHA_H			(0x1 << 8)
#define CR_CSDL(x)			((x) << 5)
#define CR_SPBR(x)			((x) << 0)

#define SR_BUSY				(0x1 << 1)
#define SR_OPE_RDY			(0x1 << 0)

#define SPI_SIZE			16
#define SZ_MULT				0x10

struct gx3201_spi_regs {
	volatile u32 SPI_CR;
	volatile u32 SPI_SR;
	volatile u32 SPI_TX_DATA;
	volatile u32 SPI_RX_DATA;
};

#define HIGH_LEVEL	1
#define LOW_LEVEL	0

#define SPI_TX_4BYTES	(sizeof(unsigned int))

struct gx3201_spi_driver_data
{
	struct spi_master 				*master;
	struct platform_device       		*pdev;
	struct device     				*dev;
	struct resource   				*ioarea;
	volatile struct gx3201_spi_regs  	*regs;
	volatile u8					*iomap;
};

#endif

