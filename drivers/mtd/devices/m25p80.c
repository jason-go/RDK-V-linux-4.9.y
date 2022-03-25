/*
 * MTD SPI driver for ST M25Pxx (and similar) serial flash chips
 *
 * Author: Mike Lavender, mike@steroidmicros.com
 *
 * Copyright (c) 2005, Intec Automation Inc.
 *
 * Some parts are based on lart.c by Abraham Van Der Merwe
 *
 * Cleaned up and generalized based on mtd_dataflash.c
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

#include <linux/spi/spi.h>
#include <linux/spi/flash.h>

#include <linux/semaphore.h>
#include <linux/version.h>

/* NOTE: AT 25F and SST 25LF series are very similar,
 * but commands for sector erase and chip id differ...
 */

#ifdef CONFIG_MTD_DEBUG
#define DEBUG(n, args...)				\
	do {						\
		if (n <= CONFIG_MTD_DEBUG_VERBOSE)	\
			printk(KERN_INFO args);		\
	} while(0)
#else /* CONFIG_MTD_DEBUG */
#define DEBUG(n, args...)				\
	do {						\
		if (0)					\
			printk(KERN_INFO args);		\
	} while(0)

#endif /* CONFIG_MTD_DEBUG */
extern int parse_mtd_partitions(struct mtd_info *master, const char * const *types,
			 struct mtd_partitions *pparts,
			 struct mtd_part_parser_data *data);
extern int add_mtd_partitions(struct mtd_info *, const struct mtd_partition *, int);
extern int add_mtd_device(struct mtd_info *mtd);
extern int del_mtd_device(struct mtd_info *mtd);
extern int del_mtd_partitions(struct mtd_info *);


#define FLASH_PAGESIZE		256

/* Flash opcodes. */
#define	OPCODE_WREN		0x06	/* Write enable */
#define	OPCODE_RDSR		0x05	/* Read status register */
#define	OPCODE_WRSR		0x01	/* Write status register 1 byte */
#define	OPCODE_NORM_READ	0x03	/* Read data bytes (low frequency) */
#define	OPCODE_FAST_READ	0x0b	/* Read data bytes (high frequency) */
#define	OPCODE_PP		0x02	/* Page program (up to 256 bytes) */
#define	OPCODE_BE_4K 		0x20	/* Erase 4KiB sector */
#define	OPCODE_BE_32K		0x52	/* Erase 32KiB block */
#define	OPCODE_SE		0xd8	/* Block erase (usually 64KiB) */
#define	OPCODE_RDID		0x9f	/* Read JEDEC ID */

#define CFI_MFR_MACRONIX    0x00C2
/* Used for SST flashes only. */
#define OPCODE_BP       0x02    /* Byte program */
#define OPCODE_WRDI     0x04    /* Write disable */
#define OPCODE_AAI_WP       0xad    /* Auto address increment word program */

/* Used for Macronix and Winbond flashes. */
#define OPCODE_EN4B     0xb7    /* Enter 4-byte mode */
#define OPCODE_EX4B     0xe9    /* Exit 4-byte mode */
#define OPCODE_WREX     0xC5    /* Write Extended Address Register */

/* Used for Spansion flashes only. */
#define OPCODE_BRWR     0x17    /* Bank register write */

/* Used for Issi flashes only. */
#define OPCODE_RDFR     0x48    /* Read Function Register */
#define OPCODE_WRFR     0x42    /* Write Function Register */

/* Status Register bits. */
#define	SR_WIP			1	/* Write in progress */
#define	SR_WEL			2	/* Write enable latch */
#define	SR_BP0			4	/* Block protect 0 */
#define	SR_BP1			8	/* Block protect 1 */
#define	SR_BP2			0x10	/* Block protect 2 */
#define	SR_SRWD			0x80	/* SR write protect */

/* Define max times to check status register before we give up. */
#define MAX_READY_WAIT_JIFFIES  (40 * HZ)       /* M25P16 specs 40s max chip erase */
#define	CMD_SIZE		5

#ifdef CONFIG_M25PXX_USE_FAST_READ
#define OPCODE_READ 	OPCODE_FAST_READ
#define FAST_READ_DUMMY_BYTE 1
#else
#define OPCODE_READ 	OPCODE_NORM_READ
#define FAST_READ_DUMMY_BYTE 0
#endif

#define JEDEC_MFR(_jedec_id)    ((_jedec_id) >> 16)

#define CONFIG_MTD_PARTITIONS 1

#ifdef CONFIG_MTD_PARTITIONS
#define	mtd_has_partitions()	(1)
#else
#define	mtd_has_partitions()	(0)
#endif

/****************************************************************************/

typedef enum {
	UNKNOW_MARKID,

	GD25Q128C,
	GD25Q127C,

	MX25L6406E,
	MX25L6433F,
	KH25L6436F,
} sflash_markid;

struct m25p {
	struct spi_device	*spi;
	struct semaphore	lock;
	struct mtd_info		mtd;
	unsigned		partitioned;
	u16         addr_width;
	u8			command[CMD_SIZE + FAST_READ_DUMMY_BYTE];
};

static u16 spi_addr_width = 0;

static void mxic_read_sfdp(struct spi_device *spi, unsigned int addr, unsigned char *rbuf, unsigned int rbuf_len)
{
	unsigned char tx_buf[5];

	tx_buf[0]=0x5A;
	tx_buf[1] = addr >> 16;
	tx_buf[2] = addr >> 8;
	tx_buf[3] = addr >> 0;
	spi_write_then_read(spi, tx_buf, 5, rbuf, rbuf_len);
}

static sflash_markid get_flash_markid(struct spi_device *spi, unsigned int jedec_id)
{
	sflash_markid markid = UNKNOW_MARKID;
	unsigned char rbuf;
	switch(jedec_id){
	case 0xc84018:
		mxic_read_sfdp(spi, 0x4a, &rbuf, 1);
		if (rbuf == 0x44)
			markid = GD25Q128C;
		else if (rbuf == 0x00)
			markid = GD25Q127C;
		break;
	case 0xc22017:
		mxic_read_sfdp(spi, 0x32, &rbuf, 1);
		if (rbuf == 0x81)
			markid = MX25L6406E;
		else if (rbuf == 0xF1) {
			mxic_read_sfdp(spi, 0x68, &rbuf, 1);
			if (rbuf == 0xfe)
				markid = MX25L6433F;
			else if (rbuf == 0x85)
				markid = KH25L6436F;
		}
		break;

	default:
		printk("get_flash_markid error, please check.\n");
		break;
	}
	return markid;
}

static inline struct m25p *mtd_to_m25p(struct mtd_info *mtd)
{
	return container_of(mtd, struct m25p, mtd);
}

/****************************************************************************/

/*
 * Internal helper functions
 */

/*
 * Read the status register, returning its value in the location
 * Return the status register value.
 * Returns negative if error occurred.
 */
static int read_sr(struct m25p *flash)
{
	ssize_t retval;
	u8 code = OPCODE_RDSR;
	u8 val;

	retval = spi_write_then_read(flash->spi, &code, 1, &val, 1);

	if (retval < 0) {
		dev_err(&flash->spi->dev, "error %d reading SR\n",
				(int) retval);
		return retval;
	}

	return val;
}

static int read_sr3(struct m25p *flash)
{
	u8 val,code = 0x15;/*Read Status Register-3 */
	ssize_t retval;
	retval = spi_write_then_read(flash->spi, &code, 1, &val, 1);
	if (retval < 0) {
		dev_err(&flash->spi->dev, "error %d reading SR\n",
				(int) retval);
		return retval;
	}
	return val;
}

static int read_sr_extend(struct m25p *flash)
{
	u8 val,code = 0xc8;/*Read Status Register-3 */
	ssize_t retval;
	retval = spi_write_then_read(flash->spi, &code, 1, &val, 1);
	if (retval < 0) {
		dev_err(&flash->spi->dev, "error %d reading SR\n",
				(int) retval);
		return retval;
	}
	return val;
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int write_enable(struct m25p *flash)
{
	u8	code = OPCODE_WREN;

	return spi_write_then_read(flash->spi, &code, 1, NULL, 0);
}

/*
 * Enable/disable 4-byte addressing mode.
 */
static inline int set_4byte(struct m25p *flash, u32 jedec_id, int enable)
{
	switch (JEDEC_MFR(jedec_id)) {
    	case CFI_MFR_MACRONIX:
	case 0xEF /* winbond */:
		flash->command[0] = enable ? OPCODE_EN4B : OPCODE_EX4B;
		if(enable)
		{
			return spi_write(flash->spi, flash->command, 1);
		}
		else
		{
			spi_write(flash->spi, flash->command, 1);
			flash->command[0] = OPCODE_WREN;
			spi_write(flash->spi, flash->command, 1);
			flash->command[0] = OPCODE_WREX;
			flash->command[1] = 0x00;
			spi_write(flash->spi, flash->command, 2);
			return 0;
		}
    	default:
		/* Spansion style */
		flash->command[0] = OPCODE_BRWR;
		flash->command[1] = enable << 7;
		return spi_write(flash->spi, flash->command, 2);
	}
}

/*
 * Service routine to read status register until ready, or timeout occurs.
 * Returns non-zero if error.
 */
static int wait_till_ready(struct m25p *flash)
{
	unsigned long deadline;
	int sr;

	deadline = jiffies + MAX_READY_WAIT_JIFFIES;

	do {
		if ((sr = read_sr(flash)) < 0)
			break;
		else if (!(sr & SR_WIP))
			return 0;

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	return 1;
}

static int write_sr1(struct m25p *flash,u8 val)
{
	u8 code = OPCODE_WRSR;

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	write_enable(flash);

	flash->command[0] = code;
	flash->command[1] = val;
	return spi_write(flash->spi, flash->command, 2);
}

static struct m25p *static_flash;

void m25p80_mutex_lock(void)
{
	struct m25p *flash = static_flash;

	down(&flash->lock);
}
EXPORT_SYMBOL(m25p80_mutex_lock);

void m25p80_mutex_unlock(void)
{
	struct m25p *flash = static_flash;

	up(&flash->lock);
}
EXPORT_SYMBOL(m25p80_mutex_unlock);

int m25p80_halt(void)
{
	struct m25p *flash = static_flash;

	if(!flash)
		return -1;

	if (flash->mtd.size <= 0x1000000)
		return 0;

	wait_till_ready(flash);

	flash->command[0] = 0xe9;
	spi_write(flash->spi, flash->command, 1);

	write_enable(flash);

	flash->command[0] = 0xc5;
	flash->command[1] = 0;
	spi_write(flash->spi, flash->command, 2);

	wait_till_ready(flash);

	printk("read_sr_extend = %x\n",read_sr_extend(flash));
	printk("read_sr3 = %x\n",read_sr3(flash));

	return 0;
}
EXPORT_SYMBOL(m25p80_halt);

static void m25p_addr2cmd(struct m25p *flash, unsigned int addr, u8 *cmd)
{
	/* opcode is in cmd[0] */
	cmd[1] = addr >> (flash->addr_width * 8 -  8);
	cmd[2] = addr >> (flash->addr_width * 8 - 16);
	cmd[3] = addr >> (flash->addr_width * 8 - 24);
	cmd[4] = addr >> (flash->addr_width * 8 - 32);
}

static int m25p_cmdsz(struct m25p *flash)
{
	return 1 + flash->addr_width;
}

/*
 * Erase one sector of flash memory at offset ``offset'' which is any
 * address within the sector which should be erased.
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int erase_sector(struct m25p *flash, u32 offset)
{
	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	/* Send write enable, then erase commands. */
	write_enable(flash);

	/* Set up command buffer. */
	flash->command[0] = OPCODE_SE;
	m25p_addr2cmd(flash, offset, flash->command);

	spi_write(flash->spi, flash->command, m25p_cmdsz(flash));

	return 0;
}

/****************************************************************************/

/*
 * MTD implementation
 */

/*
 * Erase an address range on the flash chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int m25p80_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	u32 addr,len;

	/* sanity checks */
	if (instr->addr + instr->len > flash->mtd.size)
		return -EINVAL;

	if ((uint32_t)instr->addr % mtd->erasesize != 0 || (uint32_t)instr->len % mtd->erasesize != 0)
	{
		return -EINVAL;
	}

	addr = instr->addr;
	len = instr->len;

  	down(&flash->lock);

	/* now erase those sectors */
	while (len) {
		if (erase_sector(flash, addr)) {
			instr->state = MTD_ERASE_FAILED;
			up(&flash->lock);
			return -EIO;
		}

		addr += mtd->erasesize;
		len -= mtd->erasesize;
	}

  	up(&flash->lock);

	instr->state = MTD_ERASE_DONE;
	mtd_erase_callback(instr);

	return 0;
}

static u_char *m25p_cache_map_0 = NULL;
static u32 m25p_cache_from_0 = 0;
static u32 m25p_cache_len_0 = 0;
static u_char *m25p_cache_map_1 = NULL;
static u32 m25p_cache_from_1 = 0;
static u32 m25p_cache_len_1 = 0;

static ssize_t m25p_proc_cache_0_write(struct file *filp,const char __user *buffer,
		size_t count, loff_t *data)
{
	char tmp[16];
	u32 from, len;
	struct spi_transfer t[2];
	struct spi_message m;
	struct m25p *flash = static_flash;
		
	if (count < 2)
	{
		printk("argument size is less than 2\n");
		return -EFAULT;
	}	

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%x %x", &from, &len);

		if (num !=  2) {
			printk("invalid read_reg parameter!\n");
			return count;
		}

		if (!len) {
			if(!from) {
				if(m25p_cache_map_0)
					vfree(m25p_cache_map_0);
				down(&flash->lock);
				m25p_cache_map_0 = NULL;
				m25p_cache_from_0 = 0;
				m25p_cache_len_0 = 0;
				up(&flash->lock);
				return count;
			}
			/* sanity checks */
			return 0;
		}

		printk("m25p80 cache from %08x len %08x\n",from,len);

		if (from + len > flash->mtd.size)
			return -EINVAL;

		m25p_cache_map_0 = vmalloc(len);
		if(!m25p_cache_map_0) {
			printk("m25p80:no memory\n");
			return -ENOMEM;
		}
		
		spi_message_init(&m);
		memset(t, 0, (sizeof t));

		/* NOTE:
		 * OPCODE_FAST_READ (if available) is faster.
		 * Should add 1 byte DUMMY_BYTE.
		 */
		t[0].tx_buf = flash->command;
		t[0].len = m25p_cmdsz(flash) + FAST_READ_DUMMY_BYTE;
		spi_message_add_tail(&t[0], &m);
			
		t[1].rx_buf = m25p_cache_map_0;
		t[1].len = len;
		spi_message_add_tail(&t[1], &m);

		/* Byte count starts at zero. */
		m25p_cache_len_0 = 0;

		down(&flash->lock);

		/* Wait till previous write/erase is done. */
		if (wait_till_ready(flash)) {
			/* REVISIT status return?? */
			up(&flash->lock);
			return 1;
		}

		/* FIXME switch to OPCODE_FAST_READ.  It's required for higher
		 * clocks; and at this writing, every chip this driver handles
		 * supports that opcode.
		 */

		/* Set up the write data buffer. */
		flash->command[0] = OPCODE_READ;
		m25p_addr2cmd(flash, from, flash->command);

		spi_sync(flash->spi, &m);

		m25p_cache_len_0 = m.actual_length - m25p_cmdsz(flash) - FAST_READ_DUMMY_BYTE;
		m25p_cache_from_0 = from;

		printk("m25p80 cache m25p_cache_from_0 %08x m25p_cache_len_0 %08x\n",m25p_cache_from_0,m25p_cache_len_0);
			
		up(&flash->lock);
	}
	
	return count;

}

static int m25p_proc_cache_0_show(struct seq_file *m, void *v)
{
	return 0;
}

static int m25p_proc_cache_0_open(struct inode *inode, struct file *file)
{
	return single_open(file, m25p_proc_cache_0_show, NULL);
}

static const struct file_operations m25p_proc_cache_0_proc_fops = {
	.open		= m25p_proc_cache_0_open,
	.read		= seq_read,
	.write		= m25p_proc_cache_0_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static ssize_t m25p_proc_cache_1_write(struct file *filp,const char __user *buffer,
		size_t count, loff_t *data)
{
	char tmp[16];
	u32 from, len;
	struct spi_transfer t[2];
	struct spi_message m;
	struct m25p *flash = static_flash;
		
	if (count < 2)
	{
		printk("argument size is less than 2\n");
		return -EFAULT;
	}	

	if (buffer && !copy_from_user(tmp, buffer, sizeof(tmp))) {		

		int num = sscanf(tmp, "%x %x", &from, &len);

		if (num !=  2) {
			printk("invalid read_reg parameter!\n");
			return count;
		}

		if (!len) {
			if(!from) {
				if(m25p_cache_map_1)
					vfree(m25p_cache_map_1);
				down(&flash->lock);
				m25p_cache_map_1 = NULL;
				m25p_cache_from_1 = 0;
				m25p_cache_len_1 = 0;
				up(&flash->lock);
				return count;
			}
			/* sanity checks */
			return 0;
		}

		printk("m25p80 cache from %08x len %08x\n",from,len);

		if (from + len > flash->mtd.size)
			return -EINVAL;

		m25p_cache_map_1 = vmalloc(len);
		if(!m25p_cache_map_1) {
			printk("m25p80:no memory\n");
			return -ENOMEM;
		}
		
		spi_message_init(&m);
		memset(t, 0, (sizeof t));

		/* NOTE:
		 * OPCODE_FAST_READ (if available) is faster.
		 * Should add 1 byte DUMMY_BYTE.
		 */
		t[0].tx_buf = flash->command;
		t[0].len = m25p_cmdsz(flash) + FAST_READ_DUMMY_BYTE;
		spi_message_add_tail(&t[0], &m);
			
		t[1].rx_buf = m25p_cache_map_1;
		t[1].len = len;
		spi_message_add_tail(&t[1], &m);

		/* Byte count starts at zero. */
		m25p_cache_len_1 = 0;

		down(&flash->lock);

		/* Wait till previous write/erase is done. */
		if (wait_till_ready(flash)) {
			/* REVISIT status return?? */
			up(&flash->lock);
			return 1;
		}

		/* FIXME switch to OPCODE_FAST_READ.  It's required for higher
		 * clocks; and at this writing, every chip this driver handles
		 * supports that opcode.
		 */

		/* Set up the write data buffer. */
		flash->command[0] = OPCODE_READ;
		m25p_addr2cmd(flash, from, flash->command);

		spi_sync(flash->spi, &m);

		m25p_cache_len_1 = m.actual_length - m25p_cmdsz(flash) - FAST_READ_DUMMY_BYTE;
		m25p_cache_from_1 = from;

		printk("m25p80 cache m25p_cache_from_1 %08x m25p_cache_len_1 %08x\n",m25p_cache_from_1,m25p_cache_len_1);
			
		up(&flash->lock);
	}
	
	return count;

}

static int m25p_proc_cache_1_show(struct seq_file *m, void *v)
{
	return 0;
}

static int m25p_proc_cache_1_open(struct inode *inode, struct file *file)
{
	return single_open(file, m25p_proc_cache_1_show, NULL);
}

static const struct file_operations m25p_proc_cache_1_proc_fops = {
	.open		= m25p_proc_cache_1_open,
	.read		= seq_read,
	.write		= m25p_proc_cache_1_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

/*
 * Read an address range from the flash chip.  The address range
 * may be any size provided it is within the physical boundaries.
 */
static int m25p80_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	struct spi_transfer t[2];
	struct spi_message m;

	/* sanity checks */
	if (!len)
		return 0;

	if (from + len > flash->mtd.size)
		return -EINVAL;
	
	if(m25p_cache_map_0) {
		if((u32)from >= m25p_cache_from_0 && 
			((u32)from + len) <= (m25p_cache_from_0 + m25p_cache_len_0)) {
			down(&flash->lock);

			/* Wait till previous write/erase is done. */
			if (wait_till_ready(flash)) {
				/* REVISIT status return?? */
				up(&flash->lock);
				return 1;
			}

			memcpy(buf,&m25p_cache_map_0[(u32)from - m25p_cache_from_0],len);
			*retlen = len;
			
			up(&flash->lock);

			return 0;
		}
	}

	if(m25p_cache_map_1) {
		if((u32)from >= m25p_cache_from_1 && 
			((u32)from + len) <= (m25p_cache_from_1 + m25p_cache_len_1)) {
			down(&flash->lock);

			/* Wait till previous write/erase is done. */
			if (wait_till_ready(flash)) {
				/* REVISIT status return?? */
				up(&flash->lock);
				return 1;
			}

			memcpy(buf,&m25p_cache_map_1[(u32)from - m25p_cache_from_1],len);
			*retlen = len;
			
			up(&flash->lock);

			return 0;
		}
	}

	spi_message_init(&m);
	memset(t, 0, (sizeof t));

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash) + FAST_READ_DUMMY_BYTE;
	spi_message_add_tail(&t[0], &m);

	t[1].rx_buf = buf;
	t[1].len = len;
	spi_message_add_tail(&t[1], &m);

	/* Byte count starts at zero. */
	if (retlen)
		*retlen = 0;

	down(&flash->lock);

	/* Wait till previous write/erase is done. */
	if (wait_till_ready(flash)) {
		/* REVISIT status return?? */
		up(&flash->lock);
		return 1;
	}

	/* NOTE:  OPCODE_FAST_READ (if available) is faster... */

	/* Set up the write data buffer. */
	flash->command[0] = OPCODE_READ;
    	m25p_addr2cmd(flash, from, flash->command);

	spi_sync(flash->spi, &m);

	*retlen = m.actual_length - m25p_cmdsz(flash) - FAST_READ_DUMMY_BYTE;

  	up(&flash->lock);

	return 0;
}

/*
 * Write an address range to the flash chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int m25p80_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct m25p *flash = mtd_to_m25p(mtd);
	u32 page_offset, page_size;
	struct spi_transfer t[2];
	struct spi_message m;

	if (retlen)
		*retlen = 0;

	/* sanity checks */
	if (!len)
		return(0);

	if (to + len > flash->mtd.size)
		return -EINVAL;

	spi_message_init(&m);
	memset(t, 0, (sizeof t));

	t[0].tx_buf = flash->command;
	t[0].len = m25p_cmdsz(flash);
	spi_message_add_tail(&t[0], &m);

	t[1].tx_buf = buf;
	spi_message_add_tail(&t[1], &m);

  	down(&flash->lock);

	/* Wait until finished previous write command. */
	if (wait_till_ready(flash))
		return 1;

	write_enable(flash);

	/* Set up the opcode in the write buffer. */
	flash->command[0] = OPCODE_PP;
   	m25p_addr2cmd(flash, to, flash->command);

	/* what page do we start with? */
	page_offset = to % FLASH_PAGESIZE;

	/* do all the bytes fit onto one page? */
	if (page_offset + len <= FLASH_PAGESIZE) {
		t[1].len = len;

		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash);
	} else {
		u32 i;

		/* the size of data remaining on the first page */
		page_size = FLASH_PAGESIZE - page_offset;

		t[1].len = page_size;
		spi_sync(flash->spi, &m);

		*retlen = m.actual_length - m25p_cmdsz(flash);

		/* write everything in PAGESIZE chunks */
		for (i = page_size; i < len; i += page_size) {
			page_size = len - i;
			if (page_size > FLASH_PAGESIZE)
				page_size = FLASH_PAGESIZE;

			/* write the next page to flash */
            		m25p_addr2cmd(flash, to + i, flash->command);

			t[1].tx_buf = buf + i;
			t[1].len = page_size;

			wait_till_ready(flash);

			write_enable(flash);

			spi_sync(flash->spi, &m);

			if (retlen)
				*retlen += m.actual_length - m25p_cmdsz(flash);
	        }
 	}

	up(&flash->lock);

	return 0;
}


/****************************************************************************/

/*
 * SPI device driver setup and teardown
 */
struct flash_info {
	char		*name;

	/* JEDEC id zero means "no ID" (most older chips); otherwise it has
	 * a high byte of zero plus three data bytes: the manufacturer id,
	 * then a two byte device id.
	 */
	u32		jedec_id;

	/* The size listed here is what works with OPCODE_SE, which isn't
	 * necessarily called a "sector" by the vendor.
	 */
	unsigned	sector_size;
	u16		n_sectors;
	sflash_markid (*get_flash_markid)(struct spi_device *dev, unsigned int jedec_id);
	sflash_markid markid;
	u16		flags;
#define	SECT_4K		0x01		/* OPCODE_BE_4K works uniformly */
    u16     addr_width;
};

/* NOTE: double check command sets and memory organization when you add
 * more flash chips.  This current list focusses on newer chips, which
 * have been converging on command sets which including JEDEC ID.
 */
static struct flash_info /*__devinitdata*/ m25p_data [] = {

	/* Atmel -- some are (confusingly) marketed as "DataFlash" */
	{ "at25fs010",  0x1f6601, 32 * 1024, 4, NULL, 0, SECT_4K, },
	{ "at25fs040",  0x1f6604, 64 * 1024, 8, NULL, 0, SECT_4K, },

	{ "at25df041a", 0x1f4401, 64 * 1024, 8, NULL, 0, SECT_4K, },
	{ "at25df641",  0x1f4800, 64 * 1024, 128, NULL, 0, SECT_4K, },

	{ "at26f004",   0x1f0400, 64 * 1024, 8, NULL, 0, SECT_4K, },
	{ "at26df081a", 0x1f4501, 64 * 1024, 16, NULL, 0, SECT_4K, },
	{ "at26df161a", 0x1f4601, 64 * 1024, 32, NULL, 0, SECT_4K, },
	{ "at26df321",  0x1f4701, 64 * 1024, 64, NULL, 0, SECT_4K, },

	/* Spansion -- single (large) sector size only, at least
	 * for the chips listed here (without boot sectors).
	 */
	{ "s25sl004a", 0x010212, 64 * 1024, 8, NULL, 0, },
	{ "s25sl008a", 0x010213, 64 * 1024, 16, NULL, 0, },
	{ "s25sl016a", 0x010214, 64 * 1024, 32, NULL, 0, },
	{ "s25sl032a", 0x010215, 64 * 1024, 64, NULL, 0, },
	{ "s25sl064a", 0x010216, 64 * 1024, 128, NULL, 0, },

	/* SST -- large erase sizes are "overlays", "sectors" are 4K */
	{ "sst25vf040b", 0xbf258d, 64 * 1024, 8, NULL, 0, SECT_4K, },
	{ "sst25vf080b", 0xbf258e, 64 * 1024, 16, NULL, 0, SECT_4K, },
	{ "sst25vf016b", 0xbf2541, 64 * 1024, 32, NULL, 0, SECT_4K, },
	{ "sst25vf032b", 0xbf254a, 64 * 1024, 64, NULL, 0, SECT_4K, },

	/* ST Microelectronics -- newer production may have feature updates */
	{ "m25p05",  0x202010,  32 * 1024, 2, NULL, 0, },
	{ "m25p10",  0x202011,  32 * 1024, 4, NULL, 0, },
	{ "m25p20",  0x202012,  64 * 1024, 4, NULL, 0, },
	{ "m25p40",  0x202013,  64 * 1024, 8, NULL, 0, },
	/*{ "m25p80",         0,  64 * 1024, 16, },*/
	{ "m25p16",  0x202015,  64 * 1024, 32, NULL, 0, },
	{ "m25p32",  0x202016,  64 * 1024, 64, NULL, 0, },
	{ "m25p64",  0x202017,  64 * 1024, 128, NULL, 0, },
	{ "m25p128", 0x202018, 256 * 1024, 64, NULL, 0, },

	{ "m45pe80", 0x204014,  64 * 1024, 16, NULL, 0, },
	{ "m45pe16", 0x204015,  64 * 1024, 32, NULL, 0, },

	{ "m25pe80", 0x208014,  64 * 1024, 16, NULL, 0, },
	{ "m25pe16", 0x208015,  64 * 1024, 32, NULL, 0, SECT_4K, },

	/* Winbond -- w25x "blocks" are 64K, "sectors" are 4KiB */
	{ "w25x10", 0xef3011, 64 * 1024, 2, NULL, 0, SECT_4K, },
	{ "w25x20", 0xef3012, 64 * 1024, 4, NULL, 0, SECT_4K, },
	{ "w25x40", 0xef3013, 64 * 1024, 8, NULL, 0, SECT_4K, },
	{ "w25x80", 0xef3014, 64 * 1024, 16, NULL, 0, SECT_4K, },
	{ "w25x16", 0xef3015, 64 * 1024, 32, NULL, 0, SECT_4K, },
	{ "w25x32", 0xef3016, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "w25x64", 0xef3017, 64 * 1024, 128, NULL, 0, SECT_4K, },
	{ "w25q64", 0xef4017, 64 * 1024, 128, NULL, 0, SECT_4K, },
	{ "w25q128", 0xef4018, 64 * 1024, 256, NULL, 0, SECT_4K, },
	{ "w25q256", 0xef4019, 64 * 1024, 512, NULL, 0, SECT_4K, },

	/* Spasion -- s25f "blocks" are 64K, "sectors" are 4KiB */
	{ "s25fl032p", 0x010215, 64 * 1024, 64,  NULL, 0, SECT_4K, },
	{ "s25fl064p", 0x010216, 64 * 1024, 128, NULL, 0, SECT_4K, },
	{ "s25fl128p", 0x012018, 64 * 1024, 256, NULL, 0, SECT_4K, },
	{ "s25fl256p", 0x010219, 64 * 1024, 512, NULL, 0, SECT_4K, },

	{ "mx25l1606e", 0xc22015, 64 * 1024, 32, NULL, 0, SECT_4K, },
	{ "mx25l06435e", 0xc22016, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "mx25l6405d", 0xc22017, 64 * 1024, 128, get_flash_markid, MX25L6406E, SECT_4K, },
	{ "mx25l6433f", 0xc22017, 64 * 1024, 128, get_flash_markid, MX25L6433F, SECT_4K, },
	{ "mx25l12805d", 0xc22018, 64 * 1024, 256, NULL, 0, SECT_4K, },
	{ "mx25l25635e", 0xc22019, 64 * 1024, 512, NULL, 0, SECT_4K, },

	/*  Gigadevice */
	{ "gd25q32", 0xc84016, 64 * 1024, 64,  NULL, 0, SECT_4K, },
	{ "gd25q64", 0xc84017, 64 * 1024, 128, NULL, 0, SECT_4K, },
	{ "gd25q127", 0xc84018, 64 * 1024, 256, get_flash_markid, GD25Q127C, SECT_4K, },
	{ "gd25q128", 0xc84018, 64 * 1024, 256, get_flash_markid, GD25Q128C, SECT_4K, },

	/* Eon */
	{ "en25q32",   0x1c3016, 64 * 1024, 64,  NULL, 0, SECT_4K, },
	{ "en25q64",   0x1c3017, 64 * 1024, 128, NULL, 0, SECT_4K, },
	{ "en25qa64a", 0x1c6017, 64 * 1024, 128, NULL, 0, SECT_4K, },
	{ "en25qa128a",0x1c6018, 64 * 1024, 256, NULL, 0, SECT_4K, },
	{ "en25q128",  0x1c3018, 64 * 1024, 256, NULL, 0, SECT_4K, },
	{ "en25qh128", 0x1c7018, 64 * 1024, 256, NULL, 0, SECT_4K, },
	{ "en25qx128a",0x1c7118, 64 * 1024, 256, NULL, 0, SECT_4K, },

	{ "by25q32as", 0x00684016,64 * 1024, 64,NULL, 0,SECT_4K},
	{ "by25q64as", 0x00684017,64 * 1024, 128,NULL, 0, SECT_4K},
	{ "by25q128as", 0x00684018,64 * 1024, 256,NULL, 0, SECT_4K},
	
	{"xtx25f32b",  0x000b4016,64 * 1024, 64, NULL, 0,SECT_4K},	
	{"xtx25f64b",  0x000b4017,64 * 1024, 128, NULL, 0,SECT_4K},	
	{"xtx25f128b",0x000b4018,64 * 1024, 256, NULL, 0,SECT_4K},	

	{ "pn25f32b",  0x001c7016,64 * 1024, 64, NULL, 0,SECT_4K},
	{ "pn25f64b",  0x001c7017,64 * 1024, 128, NULL, 0,SECT_4K},
	{ "pn25f128b",0x001c7018,64 * 1024, 256, NULL, 0,SECT_4K},

	{ "x25qh32b", 0x00204016,64 * 1024, 64, NULL, 0,SECT_4K},
	{ "x25qh64c", 0x00204017,64 * 1024, 128, NULL, 0,SECT_4K},	
	{ "x25qh128c",0x00204018,64 * 1024, 256, NULL, 0,SECT_4K},	
	{ "x25qh64b", 0x00206017,64 * 1024, 128, NULL, 0,SECT_4K},	
	{ "x25qh128b",0x00206018,64 * 1024, 256, NULL, 0,SECT_4K}, 
	{ "x25qh64a", 0x00207017,64 * 1024, 128, NULL, 0,SECT_4K},	
	{ "x25qh128a",0x00207018,64 * 1024, 256, NULL, 0,SECT_4K}, 

	{ "zb25vq32", 0x005e4016,64 * 1024, 64, NULL, 0,SECT_4K},	
	{ "zb25vq64", 0x005e4017,64 * 1024, 128,NULL, 0, SECT_4K},	
	{ "zb25vq128",0x005e4018,64 * 1024, 256,NULL, 0, SECT_4K},
	
	/* Pmc */
	{ "pm25lq032c", 0x7f9d46, 64 * 1024, 64, NULL, 0, SECT_4K, },

	/* paragon */
	{ "pn25f32",    0xe04016, 64 * 1024, 64,  NULL, 0, SECT_4K, },
	{ "pn25f64",    0xe04017, 64 * 1024, 128,  NULL, 0, SECT_4K, },
	{ "pn25f128",   0xe04018, 64 * 1024, 256,  NULL, 0, SECT_4K, },

	/* issi */
	{ "ic25lp064", 0x9d6017, 64 * 1024, 128, NULL, 0, SECT_4K},

	/* p25q* */
	{ "p25q32h", 0x856016,  64 * 1024, 64,  NULL, 0, SECT_4K, },
	{ "p25q64h", 0x856017,  64 * 1024, 128,  NULL, 0, SECT_4K, },
	{ "p25q128h", 0x856018, 64 * 1024, 256,  NULL, 0, SECT_4K, },

	{ "py25q32h", 0x852016,  64 * 1024, 64,  NULL, 0, SECT_4K, },
	{ "py25q64h", 0x852017,  64 * 1024, 128,  NULL, 0, SECT_4K, },
	{ "py25q128h", 0x852018, 64 * 1024, 256,  NULL, 0, SECT_4K, },
	
	{ "fm25q32", 0x00a14016, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "fm25q64",  0x00a14017, 64*1024,  128,NULL, 0, SECT_4K, },		
	{ "fm25q128",  0x00a14018, 64*1024,  256,NULL, 0, SECT_4K, },
	{ "fm25sq128",  0x00a12818, 64*1024,  256,NULL, 0, SECT_4K, },

	{ "zd25q32b", 0x00ba3216, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "zd25q64b",  0x00ba3217, 64*1024,  128,NULL, 0, SECT_4K, },		
	{ "zd25q128b",  0x00ba3218, 64*1024,  256,NULL, 0, SECT_4K, },

	{ "nm25q32b", 0x00522216, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "nm25q64b",  0x00522217, 64*1024,  128,NULL, 0, SECT_4K, },		
	{ "nm25q128b",  0x00522218, 64*1024,  256,NULL, 0, SECT_4K, },

	{ "nm25q32evb", 0x00522116, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "nm25q64evb",  0x00522117, 64*1024,  128,NULL, 0, SECT_4K, },		
	{ "nm25q128evb",  0x00522118, 64*1024,  256,NULL, 0, SECT_4K, },

	{ "sk25p32", 0x00256016, 64 * 1024, 64, NULL, 0, SECT_4K, },
	{ "sk25p64", 0x00256017, 64*1024,  128,NULL, 0, SECT_4K, },		
	{ "sk25p128",0x00256018, 64*1024,  256,NULL, 0, SECT_4K, },
};

static struct flash_info * /*__devinit*/ jedec_probe(struct spi_device *spi)
{
	int			tmp;
	u8			code = OPCODE_RDID;
	u8			id[3];
	u32			jedec;
	struct flash_info	*info;

	/* JEDEC also defines an optional "extended device information"
	 * string for after vendor-specific data, after the three bytes
	 * we use here.  Supporting some chips might require using it.
	 */
	tmp = spi_write_then_read(spi, &code, 1, id, 3);
	if (tmp < 0) {
		DEBUG(MTD_DEBUG_LEVEL0, "error %d reading JEDEC ID\n",tmp);
		return NULL;
	}
	jedec = id[0];
	jedec = jedec << 8;
	jedec |= id[1];
	jedec = jedec << 8;
	jedec |= id[2];


	for (tmp = 0, info = m25p_data;
			tmp < ARRAY_SIZE(m25p_data);
			tmp++, info++) {
		if ((info->jedec_id == jedec) && ((info->get_flash_markid == NULL) || \
					(info->markid == info->get_flash_markid(spi, info->jedec_id)))) {
			return info;
        }
	}
	dev_err(&spi->dev, "unrecognized JEDEC id %06x\n", jedec);
	return NULL;
}
/*
 * board specific setup should have ensured the SPI clock used here
 * matches what the READ command supports, at least until this driver
 * understands FAST_READ (for clocks over 25 MHz).
 */
static int /*__devinit*/ m25p_probe(struct spi_device *spi)
{
	struct flash_platform_data	*data;
	struct m25p			*flash;
	struct flash_info		*info;
	u8 temp;
	unsigned			i;

	data = spi->dev.platform_data;
	if (data && data->type) {
		for (i = 0, info = m25p_data;
				i < ARRAY_SIZE(m25p_data);
				i++, info++) {
			if (strcmp(data->type, info->name) == 0)
				break;
		}

		/* unrecognized chip? */
		if (i == ARRAY_SIZE(m25p_data)) {
			DEBUG(MTD_DEBUG_LEVEL0, "unrecognized id %s\n",data->type);
			info = NULL;

		/* recognized; is that chip really what's there? */
		} else if (info->jedec_id) {
			struct flash_info	*chip = jedec_probe(spi);

			if (!chip || chip != info) {
				dev_warn(&spi->dev, "found %s, expected %s\n",
						chip ? chip->name : "UNKNOWN",
						info->name);
				info = NULL;
			}
		}
	} else
		info = jedec_probe(spi);

	if (!info)
		return -ENODEV;

	flash = kzalloc(sizeof *flash, GFP_KERNEL);
	if (!flash)
		return -ENOMEM;

	static_flash = flash;
	
	flash->spi = spi;
	sema_init(&flash->lock, 1);
	dev_set_drvdata(&spi->dev, flash);

	if (data->name)
		flash->mtd.name = data->name;
	else
		flash->mtd.name = info->name;

	flash->mtd.type = MTD_NORFLASH;
	flash->mtd.writesize = 1;
	flash->mtd.flags = MTD_CAP_NORFLASH;
	flash->mtd.size = info->sector_size * info->n_sectors;
	flash->mtd.erasesize = info->sector_size;
	flash->mtd._erase = m25p80_erase;
	flash->mtd._read = m25p80_read;
	flash->mtd._write = m25p80_write;

	if (info->addr_width) {
		flash->addr_width = info->addr_width;
	}
	else {
		/* enable 4-byte addressing if the device exceeds 16MiB */
		if (flash->mtd.size > 0x1000000) {
			spi_addr_width = flash->addr_width = 4;
			set_4byte(flash, info->jedec_id, 1);
		} else
			spi_addr_width = flash->addr_width = 3;
	}
	
	/*write flash unprotect*/
	temp = read_sr(flash);
	dev_info(&spi->dev, "%s sr1 : %02x\n", info->name,temp);
	temp &= ~0xFC;
	write_sr1(flash,temp);
	wait_till_ready(flash);
	
	dev_info(&spi->dev, "%s (%lld Kbytes)\n", info->name,
			flash->mtd.size / 1024);

	DEBUG(MTD_DEBUG_LEVEL2,
		"mtd .name = %s, .size = 0x%.8llx (%lluM) "
			".erasesize = 0x%.8x (%uK) .numeraseregions = %d\n",
		flash->mtd.name,
		flash->mtd.size, flash->mtd.size / (1024*1024),
		flash->mtd.erasesize, flash->mtd.erasesize / 1024,
		flash->mtd.numeraseregions);

	if (flash->mtd.numeraseregions)
		for (i = 0; i < flash->mtd.numeraseregions; i++)
			DEBUG(MTD_DEBUG_LEVEL2,
				"mtd.eraseregions[%d] = { .offset = 0x%.8llx, "
				".erasesize = 0x%.8x (%uK), "
				".numblocks = %d }\n",
				i, flash->mtd.eraseregions[i].offset,
				flash->mtd.eraseregions[i].erasesize,
				flash->mtd.eraseregions[i].erasesize / 1024,
				flash->mtd.eraseregions[i].numblocks);


	/* partitions should match sector boundaries; and it may be good to
	 * use readonly partitions for writeprotected sectors (BP2..BP0).
	 */
	if (mtd_has_partitions()) {
		//struct mtd_partition	*parts = NULL;
		struct mtd_partitions parsed;
		int			nr_parts = 0;

#ifdef CONFIG_MTD_CMDLINE_PARTS
		static const char *part_probes[] = { "cmdlinepart", NULL, };
		memset(&parsed, 0, sizeof(parsed));

		nr_parts = parse_mtd_partitions(&flash->mtd,
				part_probes, &parsed, NULL/*0*/);
#endif

		if (parsed.nr_parts <= 0 && data && data->parts) {
		    parsed = (struct mtd_partitions){
			    .parts		= data->parts,
			    .nr_parts	= data->nr_parts,
		    };
		}

		if (parsed.nr_parts > 0) {
			for (i = 0; i < parsed.nr_parts; i++) {
				DEBUG(MTD_DEBUG_LEVEL2, "partitions[%d] = "
					"{.name = %s, .offset = 0x%.8llx, "
						".size = 0x%.8llx (%lluK) }\n",
					i, parsed.parts[i].name,
					parsed.parts[i].offset,
					parsed.parts[i].size,
					parsed.parts[i].size / 1024);
			}
			flash->partitioned = 1;

			{
				struct proc_dir_entry *m25p_dir;
				struct proc_dir_entry *m25p_file_0;
				struct proc_dir_entry *m25p_file_1;

				m25p_dir = proc_mkdir("m25p", NULL);
				m25p_file_0 = proc_create("cache_map_0", 0644, m25p_dir,&m25p_proc_cache_0_proc_fops);
				m25p_file_1 = proc_create("cache_map_1", 0644, m25p_dir,&m25p_proc_cache_1_proc_fops);
			}
			return add_mtd_partitions(&flash->mtd, parsed.parts, parsed.nr_parts);
		}
	} else if (data->nr_parts)
		dev_warn(&spi->dev, "ignoring %d default partitions on %s\n",
				data->nr_parts, data->name);
	return 0;
}


static int /*__devexit*/ m25p_remove(struct spi_device *spi)
{
	struct m25p	*flash = dev_get_drvdata(&spi->dev);
	int		status;

	/* Clean up MTD stuff. */
	if (mtd_has_partitions() && flash->partitioned)
		status = del_mtd_partitions(&flash->mtd);
	else
		status = del_mtd_device(&flash->mtd);
	if (status == 0)
		kfree(flash);
	return 0;
}

static void m25p_shutdown(struct spi_device *spi)
{
	struct m25p *flash = dev_get_drvdata(&spi->dev);
	struct flash_info  *info;
	info = jedec_probe(spi);
	if (!info)
		return;
	if (flash->mtd.size > 0x1000000)
	{
		set_4byte(flash, info->jedec_id, 0);
	}
}

static struct spi_driver m25p80_driver = {
	.driver = {
		.name	= "m25p80",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe	= m25p_probe,
	.remove	= m25p_remove,//__devexit_p(m25p_remove),
	.shutdown =m25p_shutdown,
	/* REVISIT: many of these chips have deep power-down modes, which
	 * should clearly be entered on suspend() to minimize power use.
	 * And also when they're otherwise idle...
	 */
};

static struct flash_platform_data platform_data = {
	.name = "m25p80",
	.parts = NULL,
	.nr_parts = 0,
	.type = NULL,
};

static struct spi_board_info spi_flash_board_info[] = {
	{
		.modalias = "m25p80",
		.platform_data = &platform_data,
		.mode = SPI_CS_HIGH | SPI_MODE_0,
		.max_speed_hz = 30000000,
		.bus_num = 0,
		.chip_select = 0,
	}
};

static int m25p80_init(void)
{
	printk("Nationalchip M25P80 Controller Driver Registration\n");
	/* it must be separated with spi_nand flash driver, or will confict. */
	spi_register_board_info(spi_flash_board_info, ARRAY_SIZE(spi_flash_board_info));
	return spi_register_driver(&m25p80_driver);
}


static void m25p80_exit(void)
{
	spi_unregister_driver(&m25p80_driver);
}

module_init(m25p80_init);
module_exit(m25p80_exit);

