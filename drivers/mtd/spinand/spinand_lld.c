/*
spinand_lld.c

Copyright (c) 2009-2010 Micron Technology, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spinand.h>
#include <linux/vmalloc.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "spinand_bbm.h"

/****************************************************************************/
/**
   OOB area specification layout:  Total 32 available free bytes.
*/
static struct nand_ecclayout spinand_oob_soft = {
	.eccbytes = 12,
	.eccpos = {
		   8, 9, 10, 11,12,13,
		   24, 25, 26, 27,28,29},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 16,
		 .length = 8},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_soft_bch = {
	.eccbytes = 28,
	.eccpos = {
		   8, 9, 10, 11,12,13,14,
		   24, 25, 26, 27,28,29,30,
		   40, 41, 42, 43,44,45,46,
		   56, 57, 58, 59, 60,61,62},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 16,
		 .length = 8},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_64 = {
	.eccbytes = 24,
	.eccpos = {
		   8, 9, 10, 11,12,13,
		   24, 25, 26, 27,28,29,
		   40, 41, 42, 43,44,45,
		   56, 57, 58, 59, 60,61},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 16,
		 .length = 8},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 6},}
};
static struct nand_ecclayout spinand_oob_128 = {
	.eccbytes = 24,
	.eccpos = {
		   8, 9, 10, 11,12,13,
		   24, 25, 26, 27,28,29,
		   40, 41, 42, 43,44,45,
		   56, 57, 58, 59, 60,61},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 16,
		 .length = 8},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_64_PSU1GS20BN = {
	.eccbytes = 24,
	.eccpos = {
		   1, 2, 3, 4,5,6,7,
		   17,18,19, 20, 21,22,23,
		   33, 34,35,36,37,38,39,
		   49, 50, 51},
	.oobavail = 28,
	.oobfree = {
		{.offset = 8,
		 .length = 8},
		{.offset = 24,
		 .length = 8},
		{.offset = 40,
		 .length = 8},
		{.offset = 56,
		 .length = 4},}
};

static struct nand_ecclayout spinand_oob_64_gd5f1gq4xcxig = {
	.eccbytes = 24,
	.eccpos = {
		   24, 25, 26, 27,28,29,
		   30, 31, 40, 41,42,43,
		   44, 45, 46, 47,54,55,
		   56, 57, 58, 59, 60,61},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 16,
		 .length = 8},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_120_em73c044snb = {
	.eccbytes = 24,
	.eccpos = {
		   16, 17, 18, 19,20,21,
		   22, 23, 24, 25,26,27,
		   28, 29, 46, 47,48,49,
		   50, 51, 52, 53, 54,55},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 8,
		 .length = 8},
		{.offset = 30,
		 .length = 8},
		{.offset = 38,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_128_xtx_pn26g01a = {
	.eccbytes = 24,
	.eccpos = {
		   6, 7, 8, 9,10,11,
		   12, 13, 14, 15,16,17,
		   18, 36, 37, 38,39,40,
		   41, 42, 43, 44, 45,46},
	.oobavail = 28,
	.oobfree = {
		{.offset = 64,
		 .length = 6},
		{.offset = 70,
		 .length = 8},
		{.offset = 78,
		 .length = 8},
		{.offset = 86,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_64_fs35nd01g = {
	.eccbytes = 24,
	.eccpos = {
			32,33,34,35,36,37,
			40,41,42,43,44,45,
			48,49,50,51,52,53,
			56,57,58,59,60,61},
	.oobavail = 28,
	.oobfree = {
		{.offset = 2,
		 .length = 6},
		{.offset = 8,
		 .length = 8},
		{.offset = 16,
		 .length = 8},
		{.offset = 24,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_128_xtx_xt26g01c = {
	.eccbytes = 24,
	.eccpos = {
		   64, 65, 66, 67,68,69,
		   70, 71, 72, 73,74,75,
		   76, 77, 78, 79,80,81,
		   82, 83, 84, 85, 86,87},
	.oobavail = 28,
	.oobfree = {
		{.offset = 16,
		 .length = 6},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 8},
		{.offset = 56,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_128_gd5f1gq5xexxg = {
	.eccbytes = 24,
	.eccpos = {
		   64, 65, 66, 67,68,69,
		   70, 71, 72, 73,74,75,
		   76, 77, 78, 79,80,81,
		   82, 83, 84, 85, 86,87},
	.oobavail = 28,
	.oobfree = {
		{.offset = 16,
		 .length = 6},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 8},
		{.offset = 56,
		 .length = 6},}
};

static struct nand_ecclayout spinand_oob_128_fm25ls01 = {
	.eccbytes = 24,
	.eccpos = {
		   64, 65, 66, 67,68,69,
		   70, 71, 72, 73,74,75,
		   76, 77, 78, 79,80,81,
		   82, 83, 84, 85, 86,87},
	.oobavail = 28,
	.oobfree = {
		{.offset = 16,
		 .length = 6},
		{.offset = 32,
		 .length = 8},
		{.offset = 48,
		 .length = 8},
		{.offset = 56,
		 .length = 6},}
};

void spinand_dump(uint8_t *buf, int size)
{
	int len, i, j, c;
#define PRINT(...) do { printk(__VA_ARGS__); } while(0)

	for(i=0;i<size;i+=16) {
		len = size - i;
		if (len > 16)
			len = 16;
		PRINT("%08x ", i);
		for(j=0;j<16;j++) {
			if (j < len)
				PRINT(" %02x", buf[i+j]);
			else
				PRINT("   ");
		}
		PRINT(" ");
		for(j=0;j<len;j++) {
			c = buf[i+j];
			if (c < ' ' || c > '~')
				c = '.';
			PRINT("%c", c);
		}
		PRINT("\n");
	}
#undef PRINT
}

/**
 * spinand_cmd - to process a command to send to the SPI Nand
 * 
 * Description:
 *    Set up the command buffer to send to the SPI controller.
 *    The command buffer has to initized to 0
 */
int spinand_cmd(struct spi_device *spi, struct spinand_cmd *cmd)
{
	int					ret;
	struct spi_message	message;
	struct spi_transfer		x[4];
	u8 dummy = 0xff;


	spi_message_init(&message);
	memset(x, 0, sizeof x);
	
	x[0].len = 1;
	x[0].tx_buf = &cmd->cmd;
	spi_message_add_tail(&x[0], &message);
	
	if (cmd->n_addr)
	{
		x[1].len = cmd->n_addr;
		x[1].tx_buf = cmd->addr;
		spi_message_add_tail(&x[1], &message);
	}

	if (cmd->n_dummy)
	{
		x[2].len = cmd->n_dummy;
		x[2].tx_buf = &dummy;
		spi_message_add_tail(&x[2], &message);		
	}

	if (cmd->n_tx)
	{
		x[3].len = cmd->n_tx;
		x[3].tx_buf = cmd->tx_buf;
		spi_message_add_tail(&x[3], &message);		
	}

	if (cmd->n_rx)
	{
		x[3].len = cmd->n_rx;
		x[3].rx_buf = cmd->rx_buf;
		spi_message_add_tail(&x[3], &message);	
	}
	
	ret = spi_sync(spi, &message);

	return ret; 
}

int spinand_cmd_dummy(struct spi_device *spi, struct spinand_cmd *cmd,u8 dummy)
{
	int					ret;
	struct spi_message	message;
	struct spi_transfer		x[4];

	spi_message_init(&message);
	memset(x, 0, sizeof x);
	
	x[0].len = 1;
	x[0].tx_buf = &cmd->cmd;
	spi_message_add_tail(&x[0], &message);
	
	if (cmd->n_addr) {
		x[1].len = cmd->n_addr;
		x[1].tx_buf = cmd->addr;
		spi_message_add_tail(&x[1], &message);
	}

	if (cmd->n_dummy) {
		x[2].len = cmd->n_dummy;
		x[2].tx_buf = &dummy;
		spi_message_add_tail(&x[2], &message);		
	}

	if (cmd->n_tx) {
		x[3].len = cmd->n_tx;
		x[3].tx_buf = cmd->tx_buf;
		spi_message_add_tail(&x[3], &message);		
	}

	if (cmd->n_rx) {
		x[3].len = cmd->n_rx;
		x[3].rx_buf = cmd->rx_buf;
		spi_message_add_tail(&x[3], &message);	
	}
	
	ret = spi_sync(spi, &message);

	return ret; 
}

/**
 * spinand_read_id- Read SPI Nand ID
 * 
 * Description:
 *    Read ID: read two ID bytes from the SPI Nand device
 */
static int spinand_read_id(struct spi_device *spi_nand, u8 *id)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_READ_ID;
	cmd.n_dummy = 1;
	cmd.n_rx = 2;
	cmd.rx_buf = id;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "<0>error %d reading id\n",
				(int) retval);
		return retval;
	}

	/*patch for EM73C044SNB */
	if ((id[0]==0 && id[1]==0) || (id[0]==0xff && id[1]==0xff)) {
		u8 dummy;
		memset(&cmd,0,sizeof(struct spinand_cmd));
		cmd.cmd = CMD_READ_ID;
		cmd.n_dummy = 1;
		cmd.n_rx = 1;
		cmd.rx_buf = &id[0];	
		dummy = 0x00;
		retval = spinand_cmd_dummy(spi_nand, &cmd,dummy);
		if (retval != 0) {
			dev_err(&spi_nand->dev, "<1>error %d reading id\n",
					(int) retval);
			return retval;
		}
		memset(&cmd,0,sizeof(struct spinand_cmd));
		cmd.cmd = CMD_READ_ID;
		cmd.n_dummy = 1;
		cmd.n_rx = 1;
		cmd.rx_buf = &id[1];	
		dummy = 0x01;
		retval = spinand_cmd_dummy(spi_nand, &cmd,dummy);
		if (retval != 0) {
			dev_err(&spi_nand->dev, "<2>error %d reading id\n",
					(int) retval);
			return retval;
		}
	}
	
	return 0;	
}

/**
 * spinand_lock_block- send write register 0x1f command to the Nand device
 * 
 * Description:
 *    After power up, all the Nand blocks are locked.  This function allows
 *    one to unlock the blocks, and so it can be wriiten or erased.
 */
static int spinand_lock_block(struct spi_device *spi_nand, struct spinand_info *info, u8 lock)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_BLOCK_LOCK;
	cmd.n_tx = 1;
	cmd.tx_buf = &lock;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "error %d lock block\n",
				(int) retval);
		return retval;
	}
	
	return 0;	
}

/**
 * spinand_read_status- send command 0xf to the SPI Nand status register
 * 
 * Description:
 *    After read, write, or erase, the Nand device is expected to set the busy status.
 *    This function is to allow reading the status of the command: read, write, and erase.
 *    Once the status turns to be ready, the other status bits also are valid status bits.
 */
static int spinand_read_status(struct spi_device *spi_nand, struct spinand_info *info, u8 *status)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_STATUS;
	cmd.n_rx = 1;
	cmd.rx_buf = status;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "error %d reading status register\n",
				(int) retval);
		return retval;
	}

	//printk("func(%s) line(%d):status=0x%x\n", __func__, __LINE__, *status);

	return 0;
}

static int spinand_read_status2(struct spi_device *spi_nand, struct spinand_info *info, u8 *status)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_OTP;
	cmd.n_rx = 1;
	cmd.rx_buf = status;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "error %d reading status register\n",
				(int) retval);
		return retval;
	}

	printk("[%s]reg:%02x data:%02x\n",__func__,REG_OTP,*status);
	return 0;
}

static int spinand_read_status3(struct spi_device *spi_nand, struct spinand_info *info, u8 *status)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_BLOCK_LOCK;
	cmd.n_rx = 1;
	cmd.rx_buf = status;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "error %d reading status register\n",
				(int) retval);
		return retval;
	}

	printk("[%s]reg:%02x data:%02x\n",__func__,REG_BLOCK_LOCK,*status);
	return 0;
}

static int spinand_wait_till_ready(struct spi_device *spi_nand, struct spinand_info *info)
{
	unsigned long deadline;
	int retval;
	u8 status;

#define	MAX_READY_WAIT_JIFFIES	(40 * HZ)	/* M25P16 specs 40s max chip erase */

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	do {
		retval = spinand_read_status(spi_nand, info, &status);
		if (retval<0) {
			dev_err(&spi_nand->dev, "error %d reading status register\n",
					(int) retval);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY)
			return 0;

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	return -1;
}

/**
 * spinand_get_otp- send command 0xf to read the SPI Nand OTP register
 * 
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_get_otp(struct spi_device *spi_nand, struct spinand_info *info, u8* otp)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_READ_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_OTP;
	cmd.n_rx = 1;
	cmd.rx_buf = otp;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "error %d get otp\n",
				(int) retval);
		return retval;
	}
	
	return 0;	
}


/**
 * spinand_set_otp- send command 0x1f to write the SPI Nand OTP register
 * 
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_set_otp(struct spi_device *spi_nand, struct spinand_info *info, u8* otp)
{
	struct spinand_cmd cmd = {0};
	ssize_t retval;
	
	cmd.cmd = CMD_WRITE_REG;
	cmd.n_addr = 1;
	cmd.addr[0] = REG_OTP;
	cmd.n_tx = 1;
	cmd.tx_buf = otp;
	
	retval = spinand_cmd(spi_nand, &cmd);

	if (retval != 0) {
		dev_err(&spi_nand->dev, "error %d set otp\n",
				(int) retval);
		return retval;
	}
	
	return 0;
}

/**
 * spinand_reset- send reset command "0xff" to the Nand device
 * 
 * Description:
 *    Reset the SPI Nand with the reset command 0xff
 */
static int spinand_reset(struct spi_device *spi_nand)
{
	struct spinand_cmd cmd = {0};
	int retval;
    	u8 status = 0;
        
	cmd.cmd = CMD_RESET;

	retval = spinand_cmd(spi_nand, &cmd);
    	if (retval != 0) {
		printk("error %d reset\n",
				(int) retval);
		return retval;
	}

        	while (1)
	{
		retval = spinand_read_status(spi_nand,NULL,&status);
		if (retval<0) {
			printk("error %d reading status register\n",
					(int) retval);
			return retval;
		}
		if ((status & STATUS_OIP_MASK) == STATUS_READY)
		{
			break;
		}
	}

        	return 0;
}

/**
 * sspinand_enable_ecc- send command 0x1f to write the SPI Nand OTP register
 * 
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int spinand_enable_ecc(struct spi_device *spi_nand, struct spinand_info *info)
{
	ssize_t retval;
	u8 otp = 0;
	
	retval = spinand_get_otp(spi_nand, info, &otp);

	if ((otp & OTP_ECC_MASK) == OTP_ECC_MASK)
	{
		return 0;
	}
	else
	{
		otp |= OTP_ECC_MASK;
		retval = spinand_set_otp(spi_nand, info, &otp);
		retval = spinand_get_otp(spi_nand, info, &otp);
		return retval;
	}
}

static int spinand_disable_ecc(struct spi_device *spi_nand, struct spinand_info *info)
{
	ssize_t retval;
	u8 otp = 0;
	
	retval = spinand_get_otp(spi_nand, info, &otp);

	if ((otp & OTP_ECC_MASK) == OTP_ECC_MASK)
	{
		otp &= ~OTP_ECC_MASK;
		retval = spinand_set_otp(spi_nand, info, &otp);
		retval = spinand_get_otp(spi_nand, info, &otp);
		return retval;
	}
	else
	{
		return 0;
	}
}

/**
 * sspinand_write_enable- send command 0x06 to enable write or erase the Nand cells
 * 
 * Description:
 *   Before write and erase the Nand cells, the write enable has to be set.
 *   After the write or erase, the write enable bit is automatically cleared( status register bit 2 )
 *   Set the bit 2 of the status register has the same effect
 */
static int spinand_write_enable(struct spi_device *spi_nand, struct spinand_info *info)
{
	struct spinand_cmd cmd = {0};

	cmd.cmd = CMD_WR_ENABLE;

	return spinand_cmd(spi_nand, &cmd);
}

static int spinand_read_page_to_cache(struct spi_device *spi_nand, struct spinand_info *info, int page_id)
{
	struct spinand_cmd cmd = {0};
	int row;

	row = page_id;

	cmd.cmd = CMD_READ;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((row&0xff0000)>>16);
	cmd.addr[1] = (u8)((row&0xff00)>>8);
	cmd.addr[2] = (u8)(row&0x00ff);

	return spinand_cmd(spi_nand, &cmd);
}

/**
 * spinand_read_from_cache- send command 0x03 to read out the data from the cache register( 2112 bytes max )
 * 
 * Description:
 *   The read can specify 1 to 2112 bytes of data read at the coresponded locations.
 *   No tRd delay.
 */
static int spinand_read_from_cache(struct spi_device *spi_nand, struct spinand_info *info, u16 byte_id, u16 len, u8* rbuf)
{
	struct spinand_cmd cmd = {0};
	u16 column;

	column = byte_id;

	cmd.cmd = CMD_READ_RDM;
	cmd.n_addr = 2;
	cmd.addr[0] = (u8)((column&0xff00)>>8);
	cmd.addr[1] = (u8)(column&0x00ff);
	cmd.n_dummy = 1;
	cmd.n_rx = len;
	cmd.rx_buf = rbuf;
	
	return spinand_cmd(spi_nand, &cmd);	
}

/**
 * spinand_read_page-to read a page with:
 * @page_id: the physical page number
 * @offset:  the location from 0 to 2111
 * @len:     number of bytes to read
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The read icludes two commands to the Nand: 0x13 and 0x03 commands
 *   Poll to read status to wait for tRD time.
 */
static int spinand_read_page(struct spi_device *spi_nand, struct spinand_info *info, int page_id, u16 offset, u16 len, u8* rbuf)
{
	int retval;
	u8 status = 0;

	if (mutex_lock_interruptible(&info->mutex))
		return -ERESTARTSYS;
	
	/* Wait until finished previous write command. */
	retval = spinand_wait_till_ready(spi_nand,info);
	if (retval) {
		dev_err(&spi_nand->dev, "error %d wait_till_ready\n",
				(int) retval);
		mutex_unlock(&info->mutex);
		return -1;
	}

	retval = spinand_read_page_to_cache(spi_nand, info, page_id);

	while (1)
	{
		retval = spinand_read_status(spi_nand, info, &status);
		if (retval<0) {
			dev_err(&spi_nand->dev, "error %d reading status register\n",
					(int) retval);
			mutex_unlock(&info->mutex);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY)
		{
			if ((status & info->ecc_mask) == info->ecc_error)
			{
				dev_err(&spi_nand->dev, "hw ecc error, page=%d\n", page_id);
				//if (spi_nand == SPI_NAND_MICRON_DRIVER_KEY)
				//	printk(KERN_INFO "Error: reformat or erase your device. \n"); 
				//else
					mutex_unlock(&info->mutex);
					return -1;
			}
			break;
		}
	}

	retval = spinand_read_from_cache(spi_nand, info, offset, len, rbuf);

	mutex_unlock(&info->mutex);
	return 0;		
}

/**
 * spinand_program_data_to_cache--to write a page to cache with:
 * @byte_id: the location to write to the cache
 * @len:     number of bytes to write
 * @rbuf:    read buffer to hold @len bytes
 *
 * Description:
 *   The write command used here is 0x84--indicating that the cache is not cleared first.
 *   Since it is writing the data to cache, there is no tPROG time.
 */
static int spinand_program_data_to_cache(struct spi_device *spi_nand, struct spinand_info *info, u16 byte_id, u16 len, u8* wbuf)
{
	struct spinand_cmd cmd = {0};
	u16 column;

	column = byte_id;

	cmd.cmd = CMD_PROG_PAGE_CLRCACHE;
	cmd.n_addr = 2;
	cmd.addr[0] = (u8)((column&0xff00)>>8);
	cmd.addr[1] = (u8)(column&0x00ff);
	cmd.n_tx = len;
	cmd.tx_buf = wbuf;

	return spinand_cmd(spi_nand, &cmd);
}

/**
 * spinand_program_execute--to write a page from cache to the Nand array with:
 * @page_id: the physical page location to write the page.
 *
 * Description:
 *   The write command used here is 0x10--indicating the cache is writing to the Nand array.
 *   Need to wait for tPROG time to finish the transaction.
 */
static int spinand_program_execute(struct spi_device *spi_nand, struct spinand_info *info, int page_id)
{
	struct spinand_cmd cmd = {0};
	int row;

	row = page_id;

	cmd.cmd = CMD_PROG_PAGE_EXC;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((row&0xff0000)>>16);
	cmd.addr[1] = (u8)((row&0xff00)>>8);
	cmd.addr[2] = (u8)(row&0x00ff);

	return spinand_cmd(spi_nand, &cmd);
}

/**
 * spinand_program_page--to write a page with:
 * @page_id: the physical page location to write the page.
 * @offset:  the location from the cache starting from 0 to 2111
 * @len:     the number of bytes to write 
 * @wbuf:    the buffer to hold the number of bytes
 *
 * Description:
 *   The commands used here are 0x06, 0x84, and 0x10--indicating that the write enable is first
 *   sent, the write cache command, and the write execute command
 *   Poll to wait for the tPROG time to finish the transaction.
 */
static int spinand_program_page(struct spi_device *spi_nand, struct spinand_info *info, int page_id, u16 offset, u16 len, u8* wbuf)
{
	int retval;
	u8 status = 0;

	if (mutex_lock_interruptible(&info->mutex))
		return -ERESTARTSYS;
	
//	printk("[%s:%d] page_id=%d offset=%d len=%d\n",__func__,__LINE__,page_id,offset,len);
	
	/* Wait until finished previous write command. */
	retval = spinand_wait_till_ready(spi_nand,info);
	if (retval) {
		dev_err(&spi_nand->dev, "error %d wait_till_ready\n",
				(int) retval);	
		mutex_unlock(&info->mutex);
		return -1;
	}

	retval = spinand_write_enable(spi_nand, info);
	
	retval = spinand_program_data_to_cache(spi_nand, info, offset, len, wbuf);

	retval = spinand_program_execute(spi_nand, info, page_id);

	while (1)
	{
		retval = spinand_read_status(spi_nand, info, &status);
		if (retval<0) {
			dev_err(&spi_nand->dev, "error %d reading status register\n",
					(int) retval);
			mutex_unlock(&info->mutex);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY)
		{

			if ((status & STATUS_P_FAIL_MASK) == STATUS_P_FAIL)
			{
				dev_err(&spi_nand->dev, "program error, page=%d\n", page_id);
				mutex_unlock(&info->mutex);
				return -1;
			}
			else
				break;
		}
	}
	
	mutex_unlock(&info->mutex);
	return 0;
}

/**
 * spinand_erase_block_erase--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The command used here is 0xd8--indicating an erase command to erase one block--64 pages
 *   Need to wait for tERS.
 */
static int spinand_erase_block_erase(struct spi_device *spi_nand, struct spinand_info *info, int block_id)
{
	struct spinand_cmd cmd = {0};
	int row;

	row = block_id << (info->block_shift - info->page_shift);
	cmd.cmd = CMD_ERASE_BLK;
	cmd.n_addr = 3;
	cmd.addr[0] = (u8)((row&0xff0000)>>16);
	cmd.addr[1] = (u8)((row&0xff00)>>8);
	cmd.addr[2] = (u8)(row&0x00ff);

	return spinand_cmd(spi_nand, &cmd);	
}

/**
 * spinand_erase_block--to erase a page with:
 * @block_id: the physical block location to erase.
 *
 * Description:
 *   The commands used here are 0x06 and 0xd8--indicating an erase command to erase one block--64 pages
 *   It will first to enable the write enable bit ( 0x06 command ), and then send the 0xd8 erase command
 *   Poll to wait for the tERS time to complete the tranaction.
 */
static int spinand_erase_block(struct spi_device *spi_nand, struct spinand_info *info, int block_id)
{
	int retval;
	u8 status= 0;

	if (mutex_lock_interruptible(&info->mutex))
		return -ERESTARTSYS;

	/* Wait until finished previous write command. */
	retval = spinand_wait_till_ready(spi_nand,info);
	if (retval) {
		dev_err(&spi_nand->dev, "error %d wait_till_ready\n",
				(int) retval);	
		mutex_unlock(&info->mutex);
		return -1;
	}
	
	retval = spinand_write_enable(spi_nand, info);
	
	retval = spinand_erase_block_erase(spi_nand, info, block_id);

	while (1)
	{
		retval = spinand_read_status(spi_nand, info, &status);
		if (retval<0) {
			dev_err(&spi_nand->dev, "error %d reading status register\n",
					(int) retval);
			mutex_unlock(&info->mutex);
			return retval;
		}

		if ((status & STATUS_OIP_MASK) == STATUS_READY)
		{
			if ((status & STATUS_E_FAIL_MASK) == STATUS_E_FAIL)
			{
				dev_err(&spi_nand->dev, "erase error, block=%d\n", block_id);
				mutex_unlock(&info->mutex);
				return -1;
			}
			else
				break;
		}
	}
	
	mutex_unlock(&info->mutex);
	return 0;
}

/*
* spinand_get_info: get NAND info, from read id or const value 

 * Description:
 *   To set up the device parameters.
 */
 
static int spinand_calculate_ecc(struct mtd_info *mtd, const unsigned char *buf,
		       unsigned char *code)
{
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;

	__nand_calculate_ecc(buf,
			info->ecc.size, code);

	return 0;
}

static int spinand_correct_data(struct mtd_info *mtd, unsigned char *buf,
		      unsigned char *read_ecc, unsigned char *calc_ecc)
{
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;

	return __nand_correct_data(buf, read_ecc, calc_ecc,
				   info->ecc.size);
}

struct spinand_bch_control {
	struct bch_control   *bch;
	struct nand_ecclayout ecclayout;
	unsigned int         *errloc;
	unsigned char        *eccmask;
};

static void spinand_bch_free(struct spinand_bch_control *nbc)
{
	if (nbc) {
		free_bch(nbc->bch);
		kfree(nbc->errloc);
		kfree(nbc->eccmask);
		kfree(nbc);
	}
}

struct spinand_bch_control *spinand_bch_init(struct spinand_info *info, unsigned int eccsize, unsigned int eccbytes,
	      struct nand_ecclayout **ecclayout)
{
	unsigned int m, t, eccsteps, i;
	struct nand_ecclayout *layout;
	struct spinand_bch_control *nbc = NULL;
	unsigned char *erased_page;

	if (!eccsize || !eccbytes) {
		printk(KERN_WARNING "ecc parameters not supplied\n");
		goto fail;
	}

	m = fls(1+8*eccsize);
	t = (eccbytes*8)/m;

	nbc = kzalloc(sizeof(*nbc), GFP_KERNEL);
	if (!nbc)
		goto fail;

	nbc->bch = init_bch(m, t, 0);
	if (!nbc->bch)
		goto fail;

	/* verify that eccbytes has the expected value */
	if (nbc->bch->ecc_bytes != eccbytes) {
		printk(KERN_WARNING "invalid eccbytes %u, should be %u\n",
		       eccbytes, nbc->bch->ecc_bytes);
		goto fail;
	}

	eccsteps = info->page_main_size/eccsize;

	/* if no ecc placement scheme was provided, build one */
	if (!*ecclayout) {

		/* handle large page devices only */
		if (info->page_spare_size< 64) {
			printk(KERN_WARNING "must provide an oob scheme for "
			       "oobsize %d\n", info->page_spare_size);
			goto fail;
		}

		layout = &nbc->ecclayout;
		layout->eccbytes = eccsteps*eccbytes;

		/* reserve 2 bytes for bad block marker */
		if (layout->eccbytes+2 > info->page_spare_size) {
			printk(KERN_WARNING "no suitable oob scheme available "
			       "for oobsize %d eccbytes %u\n", info->page_spare_size,
			       eccbytes);
			goto fail;
		}
		/* put ecc bytes at oob tail */
		for (i = 0; i < layout->eccbytes; i++)
			layout->eccpos[i] = info->page_spare_size-layout->eccbytes+i;

		layout->oobfree[0].offset = 2;
		layout->oobfree[0].length = info->page_spare_size-2-layout->eccbytes;

		*ecclayout = layout;
	}

	/* sanity checks */
	if (8*(eccsize+eccbytes) >= (1 << m)) {
		printk(KERN_WARNING "eccsize %u is too large\n", eccsize);
		goto fail;
	}
	if ((*ecclayout)->eccbytes != (eccsteps*eccbytes)) {
		printk(KERN_WARNING "invalid ecc layout\n");
		goto fail;
	}

	nbc->eccmask = kmalloc(eccbytes, GFP_KERNEL);
	nbc->errloc = kmalloc(t*sizeof(*nbc->errloc), GFP_KERNEL);
	if (!nbc->eccmask || !nbc->errloc)
		goto fail;
	/*
	 * compute and store the inverted ecc of an erased ecc block
	 */
	erased_page = kmalloc(eccsize, GFP_KERNEL);
	if (!erased_page)
		goto fail;

	memset(erased_page, 0xff, eccsize);
	memset(nbc->eccmask, 0, eccbytes);
	encode_bch(nbc->bch, erased_page, eccsize, nbc->eccmask);
	kfree(erased_page);

	for (i = 0; i < eccbytes; i++)
		nbc->eccmask[i] ^= 0xff;

	return nbc;
fail:
	spinand_bch_free(nbc);
	return NULL;
}

static int spinand_bch_calculate_ecc(struct mtd_info *mtd, const unsigned char *buf,
			   unsigned char *code)
{
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;
	struct spinand_bch_control *nbc = info->ecc.priv;
	unsigned int i;

	memset(code, 0, info->ecc.bytes);
	encode_bch(nbc->bch, buf, info->ecc.size, code);

	/* apply mask so that an erased page is a valid codeword */
	for (i = 0; i < info->ecc.bytes; i++)
		code[i] ^= nbc->eccmask[i];

	return 0;
}

static int spinand_bch_correct_data(struct mtd_info *mtd, unsigned char *buf,
			  unsigned char *read_ecc, unsigned char *calc_ecc)
{
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;
	struct spinand_bch_control *nbc = info->ecc.priv;
	unsigned int *errloc = nbc->errloc;
	int i, count;

	count = decode_bch(nbc->bch, NULL, info->ecc.size, read_ecc, calc_ecc,
			   NULL, errloc);
	if (count > 0) {
		for (i = 0; i < count; i++) {
			if (errloc[i] < (info->ecc.size*8))
				/* error is located in data, correct it */
				buf[errloc[i] >> 3] ^= (1 << (errloc[i] & 7));
			/* else error in ecc, no action needed */

			//DEBUG(MTD_DEBUG_LEVEL0, "%s: corrected bitflip %u\n",__func__, errloc[i]);
		}
	} else if (count < 0) {
		printk(KERN_ERR "ecc unrecoverable error\n");
		count = -1;
	}
	return count;
}

static int spinand_get_info(struct spi_device *spi_nand, struct spinand_info *info, u8* id)
{
	if (id[0]==0x2C && (id[1]==0x11 || id[1]==0x12 || id[1]==0x13))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "MT29F1G01ZAC";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
		
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 0;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR;
		
	}	
	
	if ((id[0] == 0xd1 && id[1] == 0xc8) || ((id[0] == 0xc8) && id[1] == 0xd1) ||
		(id[0] == 0xf1 && id[1] == 0xc8) || ((id[0] == 0xc8) && id[1] == 0xf1))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "GD5F1GQ4UAYIG";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 0;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR;		
	}
	if ((id[0] == 0xd2 && id[1] == 0xc8) || ((id[0] == 0xc8) && id[1] == 0xd2))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "GD5F2GQ4UAYIG";
		info->nand_size = (2048 * 64 * 2176);
		info->usable_size = (2048 * 64 * 2048);
		info->block_size = (2176*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 2048;
		info->page_size = 2176;
		info->page_main_size = 2048;
		info->page_spare_size = 128;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_128;
		info->ecctransfer = &spinand_oob_128;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 0;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR;		
	}

	if (id[0] == 0xef && id[1] == 0xaa)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "W25N01GV";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}

	if (id[0] == 0xc8 && id[1] == 0x21)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "PSU1GS20BNS-ESMTF50L1G41A";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64_PSU1GS20BN;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}

	if (id[0] == 0xc2 && id[1] == 0x12)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "MX35LF1GE4AB";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64_PSU1GS20BN;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 				
	}
	
	if (id[0] == 0xc8 && id[1] == 0xb1)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "GD5F1GQ4xCxIG";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64_gd5f1gq4xcxig;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = 0x70;
		info->ecc_error = (3 << 4); 				
	}

	if (id[0] == 0x98 && id[1] == 0xC2)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "TOSHIBA-TC58CVG0S3HRAIG";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 				
	}
	
	if (id[0] == 0xba && (id[1] == 0x71 || id[1] == 0x21))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "ZD35X1GAXXX";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 0;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 			
	}
	
	if (id[0] == 0x0B && id[1] == 0xE1)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "XT26G01A";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = 0x3c;
		info->ecc_error = (2 << 4);
	}

	if (id[0] == 0xCD && id[1] == 0xA1)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FS35ND01G-D1F1QWHI100";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = 0x70;
		info->ecc_error = (3 << 4); 				
	}

	if (id[0] == 0xCD && id[1] == 0x71)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FS35ND001G";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = 0x30;
		info->ecc_error = (2 << 4);
	}
	
	if (id[0] == 0xCD && id[1] == 0xA2)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FS35ND02G-S2F1QWFI000";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = 0x70;
		info->ecc_error = (3 << 4); 				
	}
	
	if ((id[0] == 0xcd && id[1] == 0xb1) || (id[0] == 0xff  && id[1] == 0xcd))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FS35ND01G-S1F1";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = 0x70;
		info->ecc_error = (3 << 4); 				
	}

	if (id[0] == 0xA1 && id[1] == 0xE1)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "XTX-PN26G01A";
		info->nand_size = (1024 * 64 * 2176);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2176*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2176;
		info->page_main_size = 2048;
		info->page_spare_size = 128;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_128_xtx_pn26g01a;
		info->ecctransfer = &spinand_oob_128_xtx_pn26g01a;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 				
	}

	if (id[0] == 0xd5 && id[1] == 0x11)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "EM73C044SNB";
		info->nand_size = (1024 * 64 * 2168);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2168*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2168;
		info->page_main_size = 2048;
		info->page_spare_size = 120;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_120_em73c044snb;
		info->ecctransfer = &spinand_oob_120_em73c044snb;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;

		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 				
	}

	if (id[0] == 0xa1 && id[1] == 0xe4)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FM25S01ADND";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
	
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}
	
	if (id[0] == 0xc8 && id[1] == 0x01)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "F50L1G41LB";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}

	if ((id[0] == 0xc9 && id[1] == 0x21) || ((id[0] == 0x21) && id[1] == 0xc9) )
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "HYF1GQ4UDACAE";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;
		
		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;
		
		info->ff_page_mode = 1;
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}
	
	if (id[0] == 0xcd && id[1] == 0xea)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FS35ND01G-S1Y2QWFI000";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64_fs35nd01g;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}

	if ((id[0] == 0xe5 && id[1] == 0x71) || ((id[0] == 0x71) && id[1] == 0xe5) ||
			(id[0] == 0xe5 && id[1] == 0x21) || ((id[0] == 0x21) && id[1] == 0xe5))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "DS35Q1GA-1B";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;

		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}

	if ((id[0] == 0x0B && id[1] == 0x11) || (id[0] == 0x11 && id[1] == 0x0B))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "XTX-XT26G01C";
		info->nand_size = (1024 * 64 * 2176);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2176*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2176;
		info->page_main_size = 2048;
		info->page_spare_size = 128;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_128_xtx_xt26g01c;
		info->ecctransfer = &spinand_oob_128_xtx_pn26g01a;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
		
		info->ecc_mask = 0xF0;
		info->ecc_error = (0x0F << 4); 		
	}

	if (id[0] == 0xc8 && (id[1] == 0x51 || id[1] == 0x41))
	{
#if 0	 /*internal ECC off : 2K+128B */
		info->mid = id[0];
		info->did = id[1];
		info->name = "GD5F1GQ5UExxG";
		info->nand_size = (1024 * 64 * 2176);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2176*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2176;
		info->page_main_size = 2048;
		info->page_spare_size = 128;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_128_gd5f1gq5xexxg;
		info->ecctransfer = &spinand_oob_128_xtx_pn26g01a;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
#else /*internal ECC on : 2K + 64B */
		info->mid = id[0];
		info->did = id[1];
		info->name = "GD5F1GQ5UExxG";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64_gd5f1gq4xcxig;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;	
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
#endif
	}

	if (id[0] == 0xA1 && id[1] == 0xA5)
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "FM25LS01";
		info->nand_size = (1024 * 64 * 2176);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2176*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2176;
		info->page_main_size = 2048;
		info->page_spare_size = 128;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_128_fm25ls01;
		info->ecctransfer = &spinand_oob_128_xtx_pn26g01a;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
		
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}
	
	if (id[0] == 0xea && (id[1] == 0xc1 || id[1] == 0xc2 || id[1] == 0xc4))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "WS25D01/02/04GACW";
		info->nand_size = (1024 * 64 * 2112);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2112*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2112;
		info->page_main_size = 2048;
		info->page_spare_size = 64;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;
	
		info->ecclayout = &spinand_oob_64;
		info->ecctransfer = &spinand_oob_64;

		info->ecc.mode = NAND_ECC_HW;
		info->ecc.steps = 4;
		info->ecc.size = 512;
		info->ecc.bytes = -1;
		info->ecc.total = info->ecclayout->eccbytes;

		info->ff_page_mode = 1;
	
		info->ecc_mask = STATUS_ECC_MASK;
		info->ecc_error = STATUS_ECC_ERROR; 		
	}

	if ((id[0] == 0xc2 && id[1] == 0x14) || ((id[0] == 0x14) && id[1] == 0xc2))
	{
		info->mid = id[0];
		info->did = id[1];
		info->name = "MX35LF1G24AD";
		info->nand_size = (1024 * 64 * 2176);
		info->usable_size = (1024 * 64 * 2048);
		info->block_size = (2176*64);
		info->block_main_size = (2048*64);
		info->block_num_per_chip = 1024;
		info->page_size = 2176;
		info->page_main_size = 2048;
		info->page_spare_size = 128;
		info->page_num_per_block = 64;

		info->block_shift = 17;
		info->block_mask = 0x1ffff;

		info->page_shift = 11;
		info->page_mask = 0x7ff;

		info->ecc.mode = NAND_ECC_SOFT_BCH;//NAND_ECC_SOFT
		info->ecc.steps = 4;
		info->ecc.size = 512;

		if(info->ecc.mode == NAND_ECC_SOFT) {
			info->ecclayout = &spinand_oob_soft;
			info->ecctransfer = &spinand_oob_soft;
			info->ecc.bytes = 3;
			info->ecc.calculate = spinand_calculate_ecc;
			info->ecc.correct = spinand_correct_data;
		} 
		else if(info->ecc.mode == NAND_ECC_SOFT_BCH) {
			info->ecclayout = &spinand_oob_soft_bch;
			info->ecctransfer = &spinand_oob_soft_bch;
			info->ecc.bytes = 7;
			info->ecc.calculate = spinand_bch_calculate_ecc;
			info->ecc.correct = spinand_bch_correct_data;
			info->ecc.priv = spinand_bch_init(info,
					       info->ecc.size,
					       info->ecc.bytes,
					       &info->ecclayout);
			if (!info->ecc.priv) {
				printk(KERN_WARNING "BCH ECC initialization failed!\n");
				BUG();
			}
		}
		info->ecc.total = info->ecclayout->eccbytes;
		
		info->ff_page_mode = 0; /*no need use 0xff page mode*/

		info->ecc_mask = 0;
		info->ecc_error = 0xff; 				
	}
	
	return 0;
}

static u8 spinand_id[2]= {0};

static int spinand_proc_id_show(struct seq_file *m, void *v)
{
	seq_printf(m,"%x %x",spinand_id[0],spinand_id[1]);
	return 0;
}

static int spinand_proc_id_open(struct inode *inode, struct file *file)
{
	return single_open(file, spinand_proc_id_show, NULL);
}

static const struct file_operations spinand_proc_id_fops = {
	.owner = THIS_MODULE,
	.open = spinand_proc_id_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

/**
 * spinand_probe - [spinand Interface] 
* @spi_nand: registered device driver.
 *
 * Description:
 *   To set up the device driver parameters to make the device available.
 */
static int spinand_probe(struct spi_device *spi_nand)
{
	unsigned int i;
	ssize_t retval;
	struct mtd_info *mtd;
	struct spinand_chip *chip; 
	struct spinand_info *info;
	struct flash_platform_data	*data;
	u8 id[2]= {0},status = 0;
	struct proc_dir_entry *proc_id;

	data = spi_nand->dev.platform_data;
	
	retval = spinand_reset(spi_nand);
	retval = spinand_reset(spi_nand);
	retval = spinand_read_id(spi_nand, (u8*)&id);
	if ((id[0]==0 && id[1]==0) || (id[0]==0xff && id[1]==0xff))
	{
		printk(KERN_ERR "SPINAND: read id error! 0x%02x, 0x%02x!\n", id[0], id[1]); 
		return -ENXIO;
	}

	spinand_id[0] = id[0];
	spinand_id[1] = id[1];
	
	info  = kzalloc(sizeof(struct spinand_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;
	
	retval = spinand_get_info(spi_nand, info, (u8*)&id);
	printk(KERN_INFO "SPINAND: 0x%02x, 0x%02x, %s\n", id[0], id[1], info->name); 
	printk(KERN_INFO "%s\n", mu_spi_nand_driver_version);
	retval = spinand_lock_block(spi_nand, info, BL_ALL_UNLOCKED);

#ifdef CONFIG_MTD_SPINAND_ONDIEECC
	if(info->ecc.mode == NAND_ECC_HW)
		retval = spinand_enable_ecc(spi_nand, info);
#else
	retval = spinand_disable_ecc(spi_nand, info);
#endif

	spinand_read_status2(spi_nand,info,&status);
	spinand_read_status3(spi_nand,info,&status);

	chip  = kzalloc(sizeof(struct spinand_chip), GFP_KERNEL);
	if (!chip)
		goto err_info;

	chip->spi_nand = spi_nand;
	chip->info = info;
	chip->reset = spinand_reset;
	chip->read_id = spinand_read_id;
	chip->read_page = spinand_read_page;
	chip->program_page = spinand_program_page;
	chip->erase_block = spinand_erase_block;
	chip->enable_ecc = spinand_enable_ecc;
	chip->disable_ecc = spinand_disable_ecc;
	
	chip->databuf = kzalloc(info->page_size, GFP_KERNEL);
	if (!chip->databuf)
		goto err_chip;
	
	chip->buf = kzalloc(info->page_size, GFP_KERNEL);
	if (!chip->buf)
		goto err_chipdatabuf;
	
	chip->oobbuf = kzalloc(info->ecclayout->oobavail, GFP_KERNEL);
	if (!chip->oobbuf) 
		goto err_chipbuf;

	mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
	if (!mtd)
		goto err_chipoobbuf;

	dev_set_drvdata(&spi_nand->dev, mtd);

	mutex_init(&info->mutex);

	mtd->priv = chip;
	
	retval = spinand_mtd_extern(mtd);
	
	proc_id = proc_create("spinand_id", 
					  S_IRUGO | S_IFREG, NULL,&spinand_proc_id_fops);

	if (data->name)
		mtd->name = data->name;

	chip->oob_poi = chip->buf + mtd->writesize;
	/* Invalidate the pagebuffer reference */
	chip->pagebuf = -1;
	chip->subpagesize = info->page_main_size >> 2; //256Bytes
	chip->pagemask = info->page_mask;
	chip->page_shift = info->page_shift;
	chip->phys_erase_shift = chip->bbt_erase_shift = info->block_shift;
	chip->chipsize = (uint64_t)le32_to_cpu(info->block_num_per_chip) * mtd->erasesize;
	if (chip->chipsize & 0xffffffff)
		chip->chip_shift = ffs((unsigned)chip->chipsize) - 1;
	else {
		chip->chip_shift = ffs((unsigned)(chip->chipsize >> 32));
		chip->chip_shift += 32 - 1;
	}
	chip->badblockpos = NAND_LARGE_BADBLOCK_POS;
	chip->numchips = 1;
	chip->options = 0;
	
	INIT_LIST_HEAD(&chip->oob_list);

	/* partitions should match sector boundaries; and it may be good to
	 * use readonly partitions for writeprotected sectors (BP2..BP0).
	 */
	if (mtd_has_partitions()) {
		struct mtd_partitions parsed;
		int			nr_parts = 0;

#ifdef CONFIG_MTD_CMDLINE_PARTS
		static const char *part_probes[] = { "cmdlinepart", NULL, };
		memset(&parsed, 0, sizeof(parsed));

		nr_parts = parse_mtd_partitions(mtd,
				part_probes, &parsed, NULL/*0*/);
#endif

		if (parsed.nr_parts <= 0 && data && data->parts) {
		    parsed = (struct mtd_partitions){
			    .parts		= data->parts,
			    .nr_parts	= data->nr_parts,
		    };
		}

		if (parsed.nr_parts > 0) {
			chip->parts = kzalloc(sizeof(struct mtd_partition) * nr_parts, GFP_KERNEL);
			if(!chip->parts)
				goto err_mtd;

			memcpy(chip->parts,parsed.parts,sizeof(struct mtd_partition) * nr_parts);
			chip->nr_parts = nr_parts;
			
			for (i = 0; i < nr_parts; i++) {
				DEBUG(MTD_DEBUG_LEVEL2, "partitions[%d] = "
				      "{.name = %s, .offset = 0x%llx, "
				      ".size = 0x%llx (%lldKiB) }\n",
				      i, parsed.parts[i].name,
				      (long long)parsed.parts[i].offset,
				      (long long)parsed.parts[i].size,
				      (long long)(parsed.parts[i].size >> 10));
			}

			retval = add_mtd_partitions(mtd, parsed.parts, parsed.nr_parts);
			if(retval == 1) {
				printk("[%s:%d]mtd_device_register fail\n",__func__,__LINE__);
				goto err_parts;
			}
		}
	} else if (data->nr_parts) {
		printk(KERN_WARNING "ignoring %d default partitions on %s\n",
				data->nr_parts, data->name);
		goto err_parts;
	}

	retval = spinand_default_bbt(mtd);
	if(retval != 0) {
		printk("[%s:%d]spinand_default_bbt fail\n",__func__,__LINE__);
		goto err_mtd_register;
	}

	return retval;

err_mtd_register:
	del_mtd_partitions(mtd);
err_parts:
	kfree(chip->parts);
err_mtd:
	kfree(mtd);
err_chipoobbuf:
	kfree(chip->oobbuf);
err_chipbuf:
	kfree(chip->buf);
err_chipdatabuf:
	kfree(chip->databuf);
err_chip:
	kfree(chip);
err_info:
	kfree(chip->info);
	return -ENOMEM;
}

/**
 * __devexit spinand_remove--Remove the device driver
 * @spi: the spi device.
 *
 * Description:
 *   To remove the device driver parameters and free up allocated memories.
 */
static int spinand_remove(struct spi_device *spi)
{
	struct mtd_info *mtd;
	struct spinand_chip *chip; 

	DEBUG(MTD_DEBUG_LEVEL1, "%s: remove\n", dev_name(&spi->dev));

	mtd = dev_get_drvdata(&spi->dev);

	chip = mtd->priv;
	
	if (mtd_has_partitions() && chip && chip->nr_parts)
		del_mtd_partitions(mtd);
	else
		del_mtd_device(mtd);

	spinand_mtd_release(mtd);

	chip = mtd->priv;

	kfree(chip->parts);
	kfree(chip->info);
	kfree(chip->buf);
	kfree(chip->oobbuf);
	kfree(chip->databuf);

	/* Free bad block table memory */
	kfree(chip->bbt);

	/* Free bad block descriptor memory */
	if (chip->badblock_pattern && chip->badblock_pattern->options
			& NAND_BBT_DYNAMICSTRUCT)
		kfree(chip->badblock_pattern);
	
	kfree(chip);
	kfree(mtd);
	
	return 0;
}

/**
 * Device name structure description
*/
static struct spi_driver spinand_driver = {
	.driver = {
		.name		= "spi_nand",
		.bus		= &spi_bus_type,
		.owner		= THIS_MODULE,
	},

	.probe		= spinand_probe,
	.remove		= spinand_remove,
};

static struct flash_platform_data platform_data = {
	.name = "spi_nand",
	.parts = NULL,
	.nr_parts = 0,
	.type = NULL,
};

static struct spi_board_info spi_flash_board_info[] = {
	{
		.modalias = "spi_nand",
		.platform_data = &platform_data,
		.mode = SPI_CS_HIGH | SPI_MODE_0,
		.max_speed_hz = 0x2,
		.bus_num = 0,
		.chip_select = 0,
	}
};

/**
 * Device driver registration
*/
static int __init spinand_init(void)
{
	printk("Nationalchip SPINand Driver Registration\n");
	spi_register_board_info(spi_flash_board_info, ARRAY_SIZE(spi_flash_board_info));	
	return spi_register_driver(&spinand_driver);
}

/**
 * unregister Device driver.
*/
static void __exit spinand_exit(void)
{
	spi_unregister_driver(&spinand_driver);
}

module_init(spinand_init);
module_exit(spinand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Henry Pan <hspan@micron.com>");
MODULE_DESCRIPTION("SPI NAND driver code");
