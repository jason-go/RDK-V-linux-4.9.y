/* linux/drivers/mtd/nand/gx31x0.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <asm/dma.h>
#include <asm/io.h>
#include <linux/semaphore.h>
#include "3201_nand.h"

//#define CONFIG_NAND_DEBUG

#define TIME_PARAMS_8 0x00010401
#define TIME_PARAMS 0x03010401
#define WDMA_TIME_PARAMS 0x03010401
#define RDMA_TIME_PARAMS 0x03010401
#ifdef CONFIG_NAND_LITTLE_ENDIAN
#define __nand_bswap_constant_32(x)     \
	(u32)((((u32)(x) & 0xff000000u) >> 24) |    \
			(((u32)(x) & 0x00ff0000u) >>  8) |  \
			(((u32)(x) & 0x0000ff00u) <<  8) |  \
			(((u32)(x) & 0x000000ffu) << 24))
#else
#define __nand_bswap_constant_32(x) (x)
#endif

//#define CONFIG_MTD_NAND_GX320X_DMAMODE

#define LOCK() do {\
}while(0);

#define UNLOCK() do {\
}while(0);

#ifdef CONFIG_NAND_DEBUG
#define nand_debug(FORMAT, ARGS...) printk("<debug>: ""%s()""[%d]   "FORMAT, __FUNCTION__, __LINE__, ##ARGS)
#else
#define nand_debug(FORMAT, ARGS...)do {} while (0)
#endif


#ifdef CONFIG_NAND_DEBUG
static void dump_regs(struct gx320x_nand_info *dev)
{
	struct nand_regs __iomem *regs = dev->regs;

	printk("\ndump regs:\n");
}
#endif

#define WATI_READY_POLLED(_dev_, _bit_, _t_) do {               \
	int sta,i = 0;                                          \
	do {                                                    \
		sta = (_dev_->regs->status_reg1>>_bit_)&0x01;   \
		i++;                                            \
		if (i > _t_){                                   \
			printk("bit %d poll timeout.\n",_bit_); \
			break;                                  \
		}                                               \
	}while(!(sta));                                         \
	_dev_->regs->status_reg1 = 1<<_bit_;                    \
} while(0);

#define WAIT_OPERATION_POLLED(_dev_)    WATI_READY_POLLED(_dev_, 7, 100000)
#define WAIT_NAND_READY(_dev_)          WATI_READY_POLLED(_dev_, 8, 100000)
#define WAIT_SEND_POLLED(_dev_)         WATI_READY_POLLED(_dev_, 0, 100000)
#define WAIT_RECEIVE_POLLED(_dev_)      WATI_READY_POLLED(_dev_, 1, 100000)
#define WAIT_DMA_POLLED(_dev_)          WATI_READY_POLLED(_dev_, 23,100000)
#define WAIT_ECC_POLLED(_dev_)          WATI_READY_POLLED(_dev_, 6, 100000)

static inline u32 WAIT_FIFO_EMPTY(struct gx320x_nand_info *dev)
{
	u32 delay = 100000, status = 0;
	while (--delay) {
		status = dev->regs->status_reg1;
		if (((status>>0x4) & 0x1))
			return 0;
	}
	return EAGAIN;
}

static inline u32 WAIT_FIFO_FULL(struct gx320x_nand_info *dev)
{
	u32 delay = 100000, status = 0;
	while (--delay) {
		status = dev->regs->status_reg1;
		if (!((status>>0x4) & 0x1))
			return 0;
	}
	return EAGAIN;
}

#define	NFC_EN(_dev_)       do{_dev_->regs->cfg_module |= 0xFF010000;}while(0);		// en NFC module
#define	NAND_EN(_dev_)      do{_dev_->regs->sel |= 0x1;}while(0);			// select NAND Flash
#define	NAND_DIS(_dev_)     do{_dev_->regs->sel &= 0xFFFFFFFE;}while(0);		// diselect NAND Flash
#define	FIFO_CLEAR(_dev_)   do{_dev_->regs->cfg_dma_ctrl |= (1<<28);}while(0);	// clear fifo

#define write_cmd(_dev_, _cmd_) do {                 \
	_dev_->regs->cmd_reg = _cmd_;                \
	while(!(_dev_->regs->status_reg1 & (1<<7))); \
	_dev_->regs->status_reg1 = (1<<7 | 1<<17);   \
}while(0);

#define write_addr(_dev_, _addr_) do {               \
	_dev_->regs->addr_reg = _addr_;              \
	while(!(_dev_->regs->status_reg1 & (1<<7))); \
	_dev_->regs->status_reg1 = (1<<7 | 1<<17);   \
}while(0);

static inline u32 ecc_config_value(struct nand_chip *dev,
		u8 load, u8 decode, u8 start_new_ecc)
{
	struct nand_ecc_config config;

	config.type         = dev->ecc.bytes/7*4;
	//config.data_len     = dev->ecc->data_size;
	if(dev->ecc.size == 1024)
		config.data_len = 2;
	else
		config.data_len = 1;

	config.start_sector = 0;
	config.codes_per_sector = dev->ecc.bytes;

	config.load   = load;
	config.decode = decode;
	config.start_new_ecc = start_new_ecc;

	return (config.data_len << 30)
		| (config.codes_per_sector << 16)
		| (config.type << 10)
		| (config.start_sector << 8)
		| (config.load << 2)
		| (config.decode << 1)
		| (config.start_new_ecc << 0);
}

/* Send 4 byte */
static inline void write_data_32(struct gx320x_nand_info *dev, u32 wrdata)
{
	struct nand_regs __iomem *regs = dev->regs;
	regs->wrdata_fifo = wrdata;
}

/* Read status */
static inline u8 read_status(struct gx320x_nand_info *dev)
{
	struct nand_regs __iomem *regs = dev->regs;

	regs->timing_sequence = TIME_PARAMS_8;
	regs->ctl_receive = 0x02;
	WAIT_OPERATION_POLLED(dev);
	return (regs->status_reg1 >> 24);
}

static inline void read_data_bulk(struct gx320x_nand_info *dev, u8 *dp, size_t n)
{
	struct nand_regs __iomem *regs = dev->regs;
	u32 tmp;
	u32 *ip;
	u8 *p;
	u32 i;

	regs->timing_sequence = TIME_PARAMS;
	/* Most of the time, we expect to be dealing with word-aligned
	 * multiples of 4 bytes, so optimise for that case. */
	if (unlikely(n%4)) {
		printk("%s error: data num %d is not word-aligned\n",__FUNCTION__,n);
		return;
	}
	if (likely(((u32)dp&3)==0)) {
		ip = (u32 *)dp;
		regs->ctl_receive = (n << 16)|1;
		for(i = 0; i < n; i+=4) {
			WAIT_FIFO_FULL(dev);
			*(ip++) = regs->rddata_fifo;
		}
		WAIT_RECEIVE_POLLED(dev);
	}
	else {
		p = (u8 *)dp;
		regs->ctl_receive = (n << 16)|1;
		for(i = 0; i < n; i+=4) {
			WAIT_FIFO_FULL(dev);
			tmp = regs->rddata_fifo;
			*(p++) = (tmp>>0)&0xff;
			*(p++) = (tmp>>8)&0xff;
			*(p++) = (tmp>>16)&0xff;
			*(p++) = (tmp>>24)&0xff;
		}
		WAIT_RECEIVE_POLLED(dev);
	}
}

static inline void read_data_bulk_with_dma(struct gx320x_nand_info *dev, u8 *dp, size_t n)
{
	struct nand_regs __iomem *regs = dev->regs;

	dma_addr_t rx_dmabuf = dma_map_single(dev->device, dp, n, DMA_FROM_DEVICE);
	regs->timing_sequence = RDMA_TIME_PARAMS;

	FIFO_CLEAR(dev);
	regs->cfg_dma_trans_startaddr = rx_dmabuf;
	regs->cfg_dma_ctrl |= (1<<30);	//read by DMA
	regs->cfg_dma_ctrl |= n;		//DMA len
	regs->cfg_dma_ctrl |= (1<<31);	//enable DMA
	regs->ctl_receive = (n << 16) | 1;	//receive data len
	WAIT_RECEIVE_POLLED(dev);
	WAIT_DMA_POLLED(dev);
	regs->cfg_dma_ctrl = 0x00000000;//diable DMA
	dma_unmap_single(dev->device, rx_dmabuf, n, DMA_FROM_DEVICE);
}

static inline void write_data_bulk(struct gx320x_nand_info *dev, const u8 *dp, size_t n)
{
	struct nand_regs __iomem *regs = dev->regs;
	u32 tmp;
	u8 *p;
	u32 *ip;
	u32 i;
	u32 res = 0;
	u32 k = 0;
	u8 *res_buf = NULL;

	FIFO_CLEAR(dev);
	regs->timing_sequence = TIME_PARAMS;
	regs->status_reg2 = (n<<16);

	res = n % 4;
	k = n - res;

	if (likely(((u32)dp&3)==0)) {
		ip = (u32 *)dp;
		for(i = 0; i < k; i+=4) {
			while(regs->status_reg1 & (1<<16));       //wait for fifo not full (0x18.16=0)
			regs->wrdata_fifo = *(ip++);
		}

		if (res) {
			res_buf = (u8 *)ip;
			while(regs->status_reg1 & (1<<16));       //wait for fifo not full (0x18.16=0)
			tmp = 0;
			for(i = 0; i < res; i++) {
				tmp |= (*(res_buf + i)) << (i * 8);
			}
			regs->wrdata_fifo = tmp;
		}
	} else {
		p= (u8 *)dp;
		for(i = 0; i < k; i+=4) {
			while(regs->status_reg1 & (1<<16));       //wait for fifo not full (0x18.16=0)
			tmp = 0;
			tmp |= *(p++)<<0;
			tmp |= *(p++)<<8;
			tmp |= *(p++)<<16;
			tmp |= *(p++)<<24;
			regs->wrdata_fifo = tmp;
		}

		if (res) {
			res_buf = (u8 *)p;
			while(regs->status_reg1 & (1<<16));       //wait for fifo not full (0x18.16=0)
			tmp = 0;
			for(i = 0; i < res; i++) {
				tmp |= (*(res_buf + i)) << (i * 8);
			}
			regs->wrdata_fifo = tmp;
		}
	}
}

static inline void write_data_bulk_with_dma(struct gx320x_nand_info *dev, const u8 *dp, size_t n)
{
	struct nand_regs __iomem *regs = dev->regs;
	dma_addr_t tx_dmabuf;

	regs->status_reg2 = (n<<16);
	tx_dmabuf = dma_map_single(dev->device, (void *)dp, n, DMA_TO_DEVICE);
	regs->timing_sequence =	WDMA_TIME_PARAMS;
	regs->cfg_dma_trans_startaddr = tx_dmabuf; //dma start addr
	regs->cfg_dma_ctrl = n;//DMA write len
	regs->cfg_dma_ctrl &= ~(1<<30);//write by DMA
	regs->cfg_dma_ctrl |= (1<<31);//enable DMA
	WAIT_DMA_POLLED(dev);
	regs->cfg_dma_ctrl =	0x00000000;
	dma_unmap_single(dev->device, tx_dmabuf, n, DMA_TO_DEVICE);
}

/* conversion functions */
static struct gx320x_nand_mtd *gx320x_nand_mtd_toours(struct nand_chip *chip)
{
	return container_of(chip, struct gx320x_nand_mtd, chip);
}

static struct gx320x_nand_info *gx320x_nand_mtd_toinfo(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd_to_nand(mtd);
	return gx320x_nand_mtd_toours(chip)->info;
}

static struct gx320x_nand_info *gx320x_devto_nand_info(struct platform_device *dev)
{
	return platform_get_drvdata(dev);
}

static void gx320x_nand_write_buf(struct mtd_info *mtd, const u_char *buf, int len)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	struct nand_regs __iomem *regs = dev->regs;

	LOCK();

	regs->cfg_module |= (1<<17); //enable ecc_module

#ifdef CONFIG_MTD_NAND_GX320X_DMAMODE
	if ((((u32)buf & 0x7f)==0)&&(len>0)&&(len%128 == 0)) {
		write_data_bulk_with_dma(dev, buf, len);
	}
	else {
		write_data_bulk(dev, buf, len);
	}
#else
	write_data_bulk(dev, buf, len);
#endif
	while(!(regs->status_reg1 & (1<<0)));         //wait for tx data done (0x18.0=1)
	regs->status_reg1 = 1<<0;                     //clear tx data done status
#ifdef CONFIG_NAND_DEBUG
	int i;
	if (len == mtd->oobsize) {
		printk("\n--------write oob start--------\n");
		for(i = 0; i < len; i++)
			printk("%2.2x ", buf[i]);
		printk("\n--------write oob end--------\n");
	}
#endif
	UNLOCK();
}

static void gx320x_nand_read_buf(struct mtd_info *mtd, u_char *buf, int len)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);

#ifdef CONFIG_MTD_NAND_GX320X_DMAMODE
	if((((u32)buf & 0x7f)==0)&&(len>0)&&(len%128 == 0)) {
		read_data_bulk_with_dma(dev,buf, len);
	}
	else {
		read_data_bulk(dev,buf, len);
	}
#else
	read_data_bulk(dev,buf, len);
#endif

#ifdef CONFIG_MTD_NAND_GX320X_HWECC
	if (len == mtd->writesize)
		WAIT_ECC_POLLED(dev);
#endif

#ifdef CONFIG_NAND_DEBUG
	int i;
	if (len == mtd->oobsize) {
		printk("\n--------read oob data start--------\n");
		for(i = 0; i < len; i++)
			printk("%2.2x ", buf[i]);
		printk("\n--------read oob data end--------\n");
	}
#endif
}

#if 0
static int gx320x_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	uint8_t *read_buf;
	struct nand_chip *this = mtd->priv;

	read_buf = this->buffers->databuf;
	gx320x_nand_read_buf(mtd, read_buf,len);
	for (i = 0; i < len; i++) {
		if (buf[i] != read_buf[i]) {
			return -EFAULT;
		}
	}
	return 0;
}
#endif

/* select chip */
static void gx320x_nand_select_chip(struct mtd_info *mtd, int chip)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);

	if (chip == -1){
		NAND_DIS(dev); //De-select chip
	}
	else{
		NAND_EN(dev); //select chip
	}
}

static int gx320x_nand_inithw(struct gx320x_nand_info *dev, struct platform_device *pdev)
{
	struct nand_regs __iomem *regs = dev->regs;
	int i;

	regs->cfg_module = 0x00010000;//enable nand module
	regs->cfg_module =  0x00010f1f;
#ifdef CONFIG_MTD_NAND_GX320X_HWECC
	regs->cfg_module |= (1<<17);  //ecc module disable.
#endif

	regs->timing_sequence = TIME_PARAMS_8;
	regs->cfg_dma_waittime = 5400000;

	/* It should be reset when flash is started
	 * at first time.
	 */

	for(i = 0; i < dev->mtd_count;i++) {
		regs->sel |= 1 << i; //select chip
		write_cmd(dev,NAND_CMD_RESET);
		WAIT_NAND_READY(dev);
	}
	return 0;
}

/* gx31x0_nand_hwcontrol
 *
 * Issue command and address cycles to the chip
 */
/* Select the chip by setting nCE to low */
#define NAND_CTL_SETNCE		1
/* Deselect the chip by setting nCE to high */
#define NAND_CTL_CLRNCE		2
/* Select the command latch by setting CLE to high */
#define NAND_CTL_SETCLE		3
/* Deselect the command latch by setting CLE to low */
#define NAND_CTL_CLRCLE		4
/* Select the address latch by setting ALE to high */
#define NAND_CTL_SETALE		5
/* Deselect the address latch by setting ALE to low */
#define NAND_CTL_CLRALE		6
static void gx320x_nand_hwcontrol(struct mtd_info *mtd, int cmd, u32 ctrl)
{
	return;
}

static int gx320x_nand_wait_ready(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	int state = chip->state;

	WAIT_NAND_READY(dev);
	if ((state == FL_ERASING) && (chip->options & NAND_IS_AND))
		chip->cmdfunc(mtd, NAND_CMD_STATUS_MULTI, -1, -1); /* 0x71 */
	else
		chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1); /* 0x70 */

	if (read_status(dev)&0x1) {
		return NAND_STATUS_FAIL;
	}
	return 0;
}

/* over-ride the standard functions for a little more speed. We can
 * use read/write block to move the data buffers to/from the controller
 */
static u8 gx320x_nand_read_byte (struct mtd_info *mtd)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	struct nand_regs __iomem *regs = dev->regs;

	regs->timing_sequence = TIME_PARAMS_8;
	/* Config read count and start read. */
	regs->ctl_receive =0x00010001;
	WAIT_FIFO_FULL(dev);
	return regs->rddata_fifo;
}

static void gx320x_nand_command (struct mtd_info *mtd, unsigned command, int column, int page_addr)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	//struct nand_regs __iomem *regs = dev->regs;
	struct nand_chip *this = mtd->priv;

	/* Begin command latch cycle */
	this->cmd_ctrl (mtd,NAND_CTL_SETCLE, 0);
	/*
	 * Write out the command to the device.
	 */
	if ((command == NAND_CMD_READ1)  ||
			(command == NAND_CMD_READOOB) || /* NOTE: using 0x00 instead, since 0x50 not supported on ST Nand chips */
			(command == NAND_CMD_READ0)) {
		if (mtd->writesize == GX320X_NAND_256_PAGESIZE && column >= GX320X_NAND_256_PAGESIZE) {
			/* 256B page */
			column -= GX320X_NAND_256_PAGESIZE;
			write_cmd(dev, NAND_CMD_READOOB);
		} else if (mtd->writesize == GX320X_NAND_512_PAGESIZE && column >= GX320X_NAND_256_PAGESIZE) {
			/* 512B page */
			if (column < GX320X_NAND_512_PAGESIZE) {
				column -= GX320X_NAND_256_PAGESIZE;
				write_cmd(dev, NAND_CMD_READ1);
			} else {
				column -= GX320X_NAND_512_PAGESIZE;
				write_cmd(dev, NAND_CMD_READOOB);
			}
		} else {
			/* 2KB page */
			write_cmd(dev, NAND_CMD_READ0);
			/* correct column for ST NAND chips */
			if (command == NAND_CMD_READOOB) {    // 0x50
				column = mtd->writesize;
				dev->regs->timing_sequence = TIME_PARAMS;
			}
		}
	} else if (command == NAND_CMD_SEQIN) {     // 0x80
		if(mtd->writesize == GX320X_NAND_512_PAGESIZE) {
			int readcmd;
			if (column >= mtd->writesize) {
				/* OOB area */
				column -= mtd->writesize;
				readcmd = NAND_CMD_READOOB;
			} else if (column < GX320X_NAND_256_PAGESIZE) {
				/* First GX31X0_NAND_256_PAGESIZE bytes --> READ0 */
				readcmd = NAND_CMD_READ0;
			} else {
				column -= GX320X_NAND_256_PAGESIZE;
				readcmd = NAND_CMD_READ1;
			}
			write_cmd(dev, readcmd);
		}
		write_cmd(dev, command);
	}
	else if (command == NAND_CMD_OTP_ENTRY) {
		write_cmd(dev, NAND_CMD_OTP_ENTRY);
		write_cmd(dev, NAND_CMD_OTP_ENTRY2);
		write_cmd(dev, NAND_CMD_OTP_ENTRY3);
		write_cmd(dev, NAND_CMD_OTP_ENTRY4);
	} else if (command == NAND_CMD_UNLOCK1 || command == NAND_CMD_UNLOCK2
			|| command == NAND_CMD_READ_BLS || command == NAND_CMD_READ_BLS2 ) {
		write_cmd(dev, command);
		write_addr(dev, (__u8) (page_addr & 0xff));
		write_addr(dev, (__u8) ((page_addr >> 8) & 0xff));
		write_addr(dev, (__u8) ((page_addr >> 16) & 0xff));
		page_addr = -1;
	}
	else {
		write_cmd(dev, command);
	}
	/* Set ALE and clear CLE to start address cycle */
	this->cmd_ctrl (mtd,NAND_CTL_CLRCLE, 0);

	if (column != -1 || page_addr != -1) {
		this->cmd_ctrl (mtd,NAND_CTL_SETALE, 0);
		/* Serially input address */
		LOCK();
		if (column != -1) {
			/* One more address cycle for higher density devices */
			if (mtd->writesize == GX320X_NAND_2K_PAGESIZE) {
				/* 2KB page */
				write_addr(dev, (column & 0xFF));
				write_addr(dev, ((column >> 8) & 0xFF));
			} else if (mtd->writesize == GX320X_NAND_512_PAGESIZE) {
				/* 512B page */
				write_addr(dev, column);
			} else {
				/* for nand chip read id */
				write_addr(dev, column);
			}
		}
		if (page_addr != -1) {
			write_addr(dev, (__u8) (page_addr & 0xff));
			write_addr(dev, (__u8) ((page_addr >> 8) & 0xff));

			/* One more address cycle for higher density devices */
			if ((this->chip_shift > 27) &&  // 28?
					(mtd->writesize == GX320X_NAND_2K_PAGESIZE)) {
				/* address for overside A12 ~ A27 */
				//printk("[5 CYCLE] sending page index: %d\n", page_addr);
				write_addr(dev, (__u8) ((page_addr >> 16) & 0xff));
			} else if((this->chip_shift > 25) &&
					(mtd->writesize == GX320X_NAND_512_PAGESIZE)) {
				/* address for overside A9 ~ A24 */
				write_addr(dev, (__u8) ((page_addr >> 16) & 0xff));
			}

			if ((this->chip_shift > 35) &&
					(mtd->writesize == GX320X_NAND_2K_PAGESIZE)) {
				/* address for overside A28 ~ A35 */
				write_addr(dev, (__u8) ((page_addr >> 24) & 0x0f));
			} else if ((this->chip_shift > 32) &&
					(mtd->writesize == GX320X_NAND_512_PAGESIZE)) {
				/* address for overside A25 ~ A32 */
				write_addr(dev, (__u8) ((page_addr >> 24) & 0x0f));
			}
		}
		/* Latch in address */
		this->cmd_ctrl (mtd,NAND_CTL_CLRALE, 0);
	}

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay.
	 */
	switch (command) {
		case NAND_CMD_PAGEPROG: /* 0x10 */
		case NAND_CMD_ERASE1:
		case NAND_CMD_ERASE2:
		case NAND_CMD_SEQIN:	/* 0x80 */
		case NAND_CMD_STATUS:	/* 0x70 */
		case NAND_CMD_DEPLETE1:
			return;

			/*
			 * read error status commands require only a short delay
			 */
		case NAND_CMD_STATUS_ERROR:
		case NAND_CMD_STATUS_ERROR0:
		case NAND_CMD_STATUS_ERROR1:
		case NAND_CMD_STATUS_ERROR2:
		case NAND_CMD_STATUS_ERROR3:
			udelay(this->chip_delay);
			return;
		case NAND_CMD_RESET:
			//case NAND_CMD_PAGEPROG: /* 0x10 */
			//case NAND_CMD_ERASE2:
			WAIT_NAND_READY(dev);
			return;
		case NAND_CMD_RNDOUT:
			if (mtd->writesize == GX320X_NAND_2K_PAGESIZE) {
				/* 2K pages */
				/* No ready / busy check necessary */
				write_cmd(dev, NAND_CMD_RNDOUTSTART);
			}
			return;
		case NAND_CMD_READ0:
		case NAND_CMD_READOOB:
			/* One more address cycle for higher density devices */
			if (mtd->writesize == GX320X_NAND_2K_PAGESIZE) {/* for nand chip type about 2K pages */
				/* No ready / busy check necessary */
				write_cmd(dev, NAND_CMD_READSTART);
				WAIT_NAND_READY(dev);
				//printk("in fun %s line %d \n",__FUNCTION__,__LINE__);
			}
		default:
			return;
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	//udelay(100);

	return;
}

#ifdef CONFIG_MTD_NAND_GX320X_HWECC
/* ECC handling functions */
static void gx320x_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	struct nand_regs __iomem *regs = dev->regs;
	struct nand_chip *this = mtd->priv;

	LOCK();

	regs->cfg_module |= (1<<17);

	if (mode == NAND_ECC_WRITE) {
		regs->cfg_ecc = (this->ecc.mode == NAND_ECC_HW) ? ecc_config_value(this, 0, 0, 1) : 0;
	}
	else if (mode == NAND_ECC_READ) {
		regs->cfg_ecc = (this->ecc.mode == NAND_ECC_HW) ? ecc_config_value(this, 0, 1, 1) : 0;
	}

	UNLOCK();
	return;
}

#define NAND_ECC_WRONG_LOCATION(_i_) do{\
	local1 = wrong_location_regs->ecc_wrong_location[_i_][0];\
	local2 = wrong_location_regs->ecc_wrong_location[_i_][1];\
	local3 = wrong_location_regs->ecc_wrong_location[_i_][2];\
	local4 = wrong_location_regs->ecc_wrong_location[_i_][3];\
}while(0)

#define NAND_ECC_FIX_DATA(_reg_, _i_, _j_) do {\
	wrong_bit = (_reg_ >> _i_) & 0xF;\
	wrong_byte = (_reg_>> _j_) & 0x3FF;\
	if (wrong_byte < nbytes) {\
		dat[wrong_byte] ^= (1<<wrong_bit);\
	}\
}while(0)

static int gx320x_nand_correct_data(struct mtd_info *mtd, u_char *dat,
		u_char *sec_pos , u_char *ecc_code)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	struct nand_regs __iomem *regs = dev->regs;
	struct gx_nand_wrong_location_regs __iomem *wrong_location_regs = dev->wrong_location_regs;
	struct nand_chip *this = mtd->priv;

	u32 sector_nr = *sec_pos;
	u32 nbytes = this->ecc.size;
	u32 ecc_status;
	u32 local1, local2, local3, local4;
	u32 err_num;
	u32 wrong_bit, wrong_byte;
	u32 ecclen = this->ecc.bytes;
	int ret = 0;
	int i;

	LOCK();

	switch(sector_nr) {
		case 0:
			ecc_status = regs->ecc_status & 0xff;
			break;
		case 1:
			ecc_status = (regs->ecc_status >> 8) & 0xff;
			break;
		case 2:
			ecc_status = (regs->ecc_status >> 16) & 0xff;
			break;
		case 3:
			ecc_status = (regs->ecc_status >> 24) & 0xff;
			break;
		default:
			printk("sector number exceed normal!\n");
			ret = -1;
			goto exit;
	}

	/* Data_wrong bit and can_correct bit has been set. */
	if ((ecc_status & 0x1) == 0) {
		ret = 0;
		goto exit;
	}

	for (i = 0; i < ecclen; i++)
		if (ecc_code[i] != 0xff)
			break;

	if (i == ecclen) {
		//printk("it is an erased block\n");
		ret = 0;
		goto exit;
	}

	if (((ecc_status >> 1) & 0x1) == 0x0) {
		printk("ecc uncorrectable\n");
		ret = -1;
		goto exit;
	}

	err_num = (ecc_status >> 2) & 0x1F;
	if (err_num == 0) {
		ret = 0;
		goto exit;
	}
	switch(sector_nr) {
		case 0:
			NAND_ECC_WRONG_LOCATION(0);
			break;
		case 1:
			NAND_ECC_WRONG_LOCATION(1);
			break;
		case 2:
			NAND_ECC_WRONG_LOCATION(2);
			break;
		case 3:
			NAND_ECC_WRONG_LOCATION(3);
			break;
		default:
			ret = -1;
			goto exit;
	}
	switch (err_num) {
		case 8:
			NAND_ECC_FIX_DATA(local4, 12, 0);
		case 7:
			NAND_ECC_FIX_DATA(local4, 28, 16);
		case 6:
			NAND_ECC_FIX_DATA(local3, 12, 0);
		case 5:
			NAND_ECC_FIX_DATA(local3, 28, 16);
		case 4:
			NAND_ECC_FIX_DATA(local2, 12, 0);
		case 3:
			NAND_ECC_FIX_DATA(local2, 28, 16);
		case 2:
			NAND_ECC_FIX_DATA(local1, 12, 0);
		case 1: //1 error.
			NAND_ECC_FIX_DATA(local1, 28, 16);
			break;
		default:
			ret = -1;
			goto exit;
	}
	printk("ecc repair ok.\n");
	//repair ok
	ret = 1;
exit:
	//dump_regs(dev);
	UNLOCK();
	return ret;
}

#undef NAND_ECC_WRONG_LOCATION
#undef NAND_ECC_FIX_DATA

static void do_ops_ff(struct nand_chip *this, const u_char* dat, u_char* ecc)
{
	int i, j;
	int eccbytes = this->ecc.bytes;
	int eccsteps = this->ecc.steps;
	int eccsize = this->ecc.size;

	if (unlikely((u32)dat % 4)) {
		for (i=0; i<eccsteps; i++) {
			for (j=0; j<eccsize; j++) {
				if (dat[j+i*eccsize] != 0xff)
					break;
			}
			if (j == eccsize)
				memset(ecc+i*eccbytes, 0xff, eccbytes);
		}
	} else {
		const u32 *p = (const u32 *)dat;
		int size_u32 = eccsize / 4;
		for (i=0; i<eccsteps; i++) {
			for (j=0; j<size_u32; j++) {
				if (p[j+i*size_u32] != 0xffffffff)
					break;
			}
			if (j == size_u32)
				memset(ecc+i*eccbytes, 0xff, eccbytes);
		}
	}
}

static int gx320x_nand_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	struct gx_nand_ecc_regs __iomem *ecc_regs = dev->ecc_regs;
	struct nand_chip *this = mtd->priv;
	u32 ecc_total_bytes = mtd_ooblayout_count_eccbytes(mtd);

	u32 *ecc_code = (u32 *)ecc;
	LOCK();

	/* Not support for small page ecc calculate. */
	if ((mtd->writesize == GX320X_NAND_256_PAGESIZE) ||
			(mtd->writesize == GX320X_NAND_512_PAGESIZE)) {
		UNLOCK();
		return 0;
	}

	WAIT_ECC_POLLED(dev);
	switch(ecc_total_bytes)
	{
		case 56:
			ecc_code[13] = __nand_bswap_constant_32(ecc_regs->ecc_code[0][13]);
			ecc_code[12] = __nand_bswap_constant_32(ecc_regs->ecc_code[0][12]);
			ecc_code[11] = __nand_bswap_constant_32(ecc_regs->ecc_code[0][11]);
			ecc_code[10] = __nand_bswap_constant_32(ecc_regs->ecc_code[0][10]);
			ecc_code[9]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][9]);
			ecc_code[8]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][8]);
			ecc_code[7]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][7]);
		case 28:
			ecc_code[6]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][6]);
			ecc_code[5]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][5]);
			ecc_code[4]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][4]);
			ecc_code[3]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][3]);
			ecc_code[2]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][2]);
			ecc_code[1]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][1]);
			ecc_code[0]  = __nand_bswap_constant_32(ecc_regs->ecc_code[0][0]);
			break;
		default:
			break;
	}

	UNLOCK();

	do_ops_ff(this, dat, (u_char*)ecc_code);

	return 0;
}

static int gx320x_nand_read_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	struct gx320x_nand_info *dev = gx320x_nand_mtd_toinfo(mtd);
	struct nand_chip *this = mtd->priv;
	struct nand_regs __iomem *regs = dev->regs;
	u32 eccbytes = mtd_ooblayout_count_eccbytes(mtd);
	int i;
	u32 ecc_code[mtd->oobsize/4];

	regs->status_reg1 = 0x00ffffff;
	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);

	LOCK();

	FIFO_CLEAR(dev);
	regs->cfg_ecc = (this->ecc.mode == NAND_ECC_HW) ? ecc_config_value(this, 1, 1, 0) : 0;
	regs->ctl_receive = (mtd->oobsize << 16)|(eccbytes<<8)|1;
	for(i = 0; i < (mtd->oobsize/4); i++) {
		WAIT_FIFO_FULL(dev);
		ecc_code[i] = regs->rddata_fifo;
	}
	WAIT_RECEIVE_POLLED(dev);

#ifdef CONFIG_NAND_DEBUG
	u8 *buf = (u8 *)ecc_code;
	printk("\n--------read oob start--------\n");
	for(i = 0; i < mtd->oobsize; i++)
		printk("%2.2x ", buf[i]);
	printk("\n--------read oob end--------\n");
#endif
	memcpy(chip->oob_poi, ecc_code, mtd->oobsize);

	UNLOCK();
	return 0;
}
#endif

/* gx31x0_nand_init_chip
 *
 * init a single instance of an chip
 */

static void gx320x_nand_init_chip(struct gx320x_nand_info *info,
		struct gx320x_nand_mtd *nmtd)
{
	struct nand_chip *chip  = &nmtd->chip;

	chip->write_buf     = gx320x_nand_write_buf;
	chip->read_buf      = gx320x_nand_read_buf;
	chip->select_chip   = gx320x_nand_select_chip;
	chip->chip_delay    = 0;
	chip->priv	        = nmtd;
	chip->options	    = 0; //NAND_SKIP_BBTSCAN;
	chip->controller    = &info->controller;
	chip->IO_ADDR_W     = (void  __iomem *)&info->regs->wrdata_fifo;
	chip->IO_ADDR_R     = (void  __iomem *)&info->regs->rddata_fifo;
	info->sel_reg       = (void  __iomem *)&info->regs->sel;
	info->sel_bit	    = 1<<0;
	chip->cmd_ctrl      = gx320x_nand_hwcontrol;
	//chip->dev_ready     = gx320x_nand_dev_ready;
	chip->cmdfunc       = gx320x_nand_command;
	chip->waitfunc      = gx320x_nand_wait_ready; /* NOTE: only used for write operations */
	chip->read_byte     = gx320x_nand_read_byte;
	//chip->read_word     = gx320x_nand_read_word;
	//chip->verify_buf    = gx320x_nand_verify_buf;
	//chip->bbt_td        = &gx320x_bbt_main_descr;
	//chip->bbt_md        = &gx320x_bbt_mirror_descr;

	nmtd->info          = info;
	nmtd->mtd->priv	    = chip;
	nmtd->mtd->owner     = THIS_MODULE;
	nmtd->mtd->name      = "gx320x";
	//nmtd->set	        = set;
#ifdef CONFIG_MTD_NAND_GX320X_HWECC
	chip->ecc.calculate = gx320x_nand_calculate_ecc;
	chip->ecc.correct   = gx320x_nand_correct_data;
	chip->ecc.mode	    = NAND_ECC_HW;
	chip->ecc.size	    = GX320X_NAND_512_PAGESIZE;
	chip->ecc.hwctl	    = gx320x_nand_enable_hwecc;
	chip->ecc.read_oob  = gx320x_nand_read_oob;
#else
	chip->ecc.mode	    = NAND_ECC_NONE;
#endif
}

/* ECC handling functions */
static int gx320x_nand_remove(struct platform_device *pdev)
{
	struct gx320x_nand_info *info = gx320x_devto_nand_info(pdev);
	unsigned int i = 0;

	platform_set_drvdata(pdev, NULL);

	if (info == NULL)
		return 0;
	/* first thing we need to do is release all our mtds
	 * and their partitions, then go through freeing the
	 * resources used
	 */
	if (info->mtds) {
		struct gx320x_nand_mtd *ptr = info->mtds;
		int mtdno;

		for (mtdno = 0; mtdno < info->mtd_count; mtdno++, ptr++) {
			printk("NAND: releasing mtd %d (%p)\n", mtdno, ptr);
			nand_release(ptr->mtd);
		}

		kfree(info->mtds);
	}

	if (info->part) {
		for(i = 0; i < info->npart; i++)
			kfree(info->part[i].name);
		kfree(info->part);
	}

	kfree(info);
	return 0;
}

static int gx320x_nand_get_info(struct gx320x_nand_info *info)
{
	int ret = -1;
	struct resource *reg = NULL;
	struct platform_device *device = to_platform_device(info->device);

	struct device_node *nand_node = info->device->of_node;
	if (!nand_node) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	reg = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!reg) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return ret;
	}

	info->nand_base = reg->start;
	nand_debug("nand regs base addr: 0x%x\n", info->nand_base);

	info->regs = devm_ioremap_resource(info->device, reg);
	if (!info->regs) {
		printk("error: [%s %d]\n", __func__, __LINE__);
		return -ENOMEM;
	}
	nand_debug("nand regs mapped addr: 0x%p\n", info->regs);

	ret = 0;
	return ret;
}

/* gx320x_nand_probe
 *
 * called by device layer when it finds a device matching
 * one our driver can handled. This code checks to see if
 * it can allocate all necessary resources then calls the
 * nand layer to look for devices
 */
static int gx320x_nand_probe(struct platform_device *pdev)
{
	struct gx320x_nand_info *info = NULL;
	struct gx320x_nand_mtd *nmtd = NULL;
	struct mtd_partition *mtd_parts = NULL;
	int mtd_parts_nb = 0;
	int nr_sets = GX320X_MAX_NAND_DEVICES;
	unsigned long size = 0;
	int setno = 0;
	int err = 0;

	info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		printk("nand: no memory for flash info\n");
		err = -ENOMEM;
		goto err_out;
	}

	memset(info, 0, sizeof(*info));
	platform_set_drvdata(pdev, info);

	spin_lock_init(&info->controller.lock);
	init_waitqueue_head(&info->controller.wq);

	info->device = &pdev->dev;
	err = gx320x_nand_get_info(info);
	if (err)
		goto err_out;

	info->ecc_regs = (void *)info->regs + ECC_REG_OFFSET;
	info->wrong_location_regs = (void *)info->regs + WRONG_LOCATION_REG_OFFSET;
	printk("nand: mapped registers at %p\n", info->regs);

	//sets = (plat != NULL) ? plat->sets : NULL;0xff
	//nr_sets = (plat != NULL) ? plat->nr_sets : 1;
	info->mtd_count = nr_sets;

	/* initialise the hardware */
	err = gx320x_nand_inithw(info, pdev);
	if (err)
		goto err_out;

	/* allocate our information */
	size = nr_sets * sizeof(*info->mtds);
	info->mtds = kmalloc(size, GFP_KERNEL);
	if (!(info->mtds)) {
		printk("nand: failed to allocate mtd storage\n");
		err = -ENOMEM;
		goto err_out;
	}
	memset(info->mtds, 0, size);

	/* initialise all possible chips */
	nmtd = info->mtds;
	for (setno = 0; setno < nr_sets; setno++, nmtd++) {
		struct mtd_info *mtd = nand_to_mtd(&nmtd->chip);
		nmtd->mtd = mtd;
		printk("NAND: initialising set %d (%p, info %p)\n", setno, nmtd, info);
		mtd->dev.parent = &pdev->dev;
		gx320x_nand_init_chip(info, nmtd);
		nmtd->scan_res = nand_scan(nmtd->mtd, 1);
		if (!(nmtd->scan_res)){
#ifdef CONFIG_MTD_CMDLINE_PARTS
			mtd_device_register(nmtd->mtd, mtd_parts, mtd_parts_nb);
#endif
		}
	}

	return 0;
err_out:
	gx320x_nand_remove(pdev);
	if (!err)
		err = -EINVAL;

	return err;
}

#ifdef CONFIG_PM
static int gx320x_nand_suspend(struct platform_device *dev, pm_message_t pm)
{
	return 0;
}

static int gx320x_nand_resume(struct platform_device *dev)
{
	struct gx320x_nand_info *info = platform_get_drvdata(dev);
	gx320x_nand_inithw(info, dev);
	return 0;
}
#else
#define gx320x_nand_suspend    NULL
#define gx320x_nand_resume     NULL
#endif

static struct of_device_id gxnand_device_match[] = {
	[0] = {
		.compatible = "nationalchip,gx-nand",
		.data = NULL,
	},
};

static struct platform_driver gxnand_driver = {
	.probe		= gx320x_nand_probe,
	.remove		= gx320x_nand_remove,
#ifdef	CONFIG_PM
	.suspend	= gx320x_nand_suspend,
	.resume		= gx320x_nand_resume,
#endif
	.driver = {
		.name = "gx_nand",
		.of_match_table = gxnand_device_match,
	},
};

module_platform_driver(gxnand_driver);

MODULE_DESCRIPTION("support for NationalChilp NandFlash modules");
MODULE_AUTHOR("NationalChilp");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("NationalChilp Device");
MODULE_VERSION("V1.0");
