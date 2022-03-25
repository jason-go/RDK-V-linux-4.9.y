/*
 *  linux/include/linux/mtd/spinand.h
 *
 Copyright (c) 2009-2010 Micron Technology, Inc.

This software is licensed under the terms of the GNU General Public
License version 2, as published by the Free Software Foundation, and
may be copied, distributed, and modified under those terms.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

 *  Henry Pan <hspan@micron.com>
 *
 *  based on nand.h
 */
#ifndef __LINUX_MTD_SPI_NAND_H
#define __LINUX_MTD_SPI_NAND_H

#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mtd/mtd.h>
#include <mtd/mtd-abi.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/nand_bch.h>
#include <linux/bch.h>
#include <linux/list.h>
#include <linux/mtd/partitions.h>
#include <linux/mutex.h>

#define SPINAND_YAFFS
#define SPINAND_MAX_OOBSIZE	64

#ifdef CONFIG_SPINAND_DEBUG
#define SPINAND_P(x,s...)           printk(KERN_INFO x,##s);
#else
#define SPINAND_P(x,s...)	do {}while(0)
#endif

#define mu_spi_nand_driver_version "Beagle-MTD_01.00_Linux2.6.33_20100507"

#define SPI_NAND_MICRON_DRIVER_KEY 0x1233567

/* cmd */
#define CMD_READ				0x13
#define CMD_READ_RDM			0x03
#define CMD_PROG_PAGE_CLRCACHE	0x02
#define CMD_PROG_PAGE			0x84
#define CMD_PROG_PAGE_EXC		0x10
#define CMD_ERASE_BLK			0xd8
#define CMD_WR_ENABLE			0x06
#define CMD_WR_DISABLE			0x04
#define CMD_READ_ID			0x9f
#define CMD_RESET				0xff
#define CMD_READ_REG			0x0f
#define CMD_WRITE_REG			0x1f

/* feature/ status reg */
#define REG_BLOCK_LOCK		0xa0
#define REG_OTP				0xb0
#define REG_STATUS			0xc0/* timing */

/* status */
#define STATUS_OIP_MASK		0x01
#define STATUS_READY		0 << 0
#define STATUS_BUSY			1 << 0

#define STATUS_E_FAIL_MASK	0x04
#define STATUS_E_FAIL		1 << 2

#define STATUS_P_FAIL_MASK 	0x08
#define STATUS_P_FAIL		1 << 3

#define STATUS_ECC_MASK 	0x30
#define STATUS_ECC_1BIT_CORRECTED	1 << 4
#define STATUS_ECC_ERROR			2 << 4
#define STATUS_ECC_RESERVED			3 << 4


/*ECC enable defines*/
#define OTP_ECC_MASK		0x10
#define OTP_ECC_OFF			0
#define OTP_ECC_ON			1

#define ECC_DISABLED
#define ECC_IN_NAND   
#define ECC_SOFT

/* block lock */
#define BL_ALL_LOCKED      0x38 
#define BL_1_2_LOCKED      0x30
#define BL_1_4_LOCKED      0x28
#define BL_1_8_LOCKED      0x20
#define BL_1_16_LOCKED     0x18
#define BL_1_32_LOCKED     0x10
#define BL_1_64_LOCKED     0x08
#define BL_ALL_UNLOCKED    0

/*
 * Chip requires that BBT is periodically rewritten to prevent
 * bits from adjacent blocks from 'leaking' in altering data.
 * This happens with the Renesas AG-AND chips, possibly others.
 */
#define BBT_AUTO_REFRESH	0x00000080

/* Non chip related options */
/*
 * Use a flash based bad block table. OOB identifier is saved in OOB area.
 * This option is passed to the default bad block table function.
 */
#define NAND_USE_FLASH_BBT	0x00010000
/* This option skips the bbt scan during initialization. */
#define NAND_SKIP_BBTSCAN	0x00020000

/*
 * AND Chip which has 4 banks and a confusing page / block
 * assignment. See Renesas datasheet for further information.
 */
#define NAND_IS_AND		0x00000020
/*
 * If passed additionally to NAND_USE_FLASH_BBT then BBT code will not touch
 * the OOB area.
 */
#define NAND_USE_FLASH_BBT_NO_OOB	0x00800000
/* Create an empty BBT with no vendor information if the BBT is available */
#define NAND_CREATE_EMPTY_BBT		0x01000000

/*
 * Internal ECC layout control structure. For historical reasons, there is a
 * similar, smaller struct nand_ecclayout_user (in mtd-abi.h) that is retained
 * for export to user-space via the ECCGETLAYOUT ioctl.
 * nand_ecclayout should be expandable in the future simply by the above macros.
 */
struct nand_ecclayout {
	__u32 eccbytes;
	__u32 eccpos[MTD_MAX_ECCPOS_ENTRIES_LARGE];
	__u32 oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES_LARGE];
};

/*
 * Constants for ECC_MODES
 */
typedef enum {
	NAND_ECC_NONE,
	NAND_ECC_SOFT,
	NAND_ECC_HW,
	NAND_ECC_HW_SYNDROME,
	NAND_ECC_HW_OOB_FIRST,
	NAND_ECC_SOFT_BCH,
} nand_ecc_modes_t;

/**
 * struct nand_ecc_ctrl - Control structure for ECC
 * @mode:	ECC mode
 * @algo:	ECC algorithm
 * @steps:	number of ECC steps per page
 * @size:	data bytes per ECC step
 * @bytes:	ECC bytes per step
 * @strength:	max number of correctible bits per ECC step
 * @total:	total number of ECC bytes per page
 * @prepad:	padding information for syndrome based ECC generators
 * @postpad:	padding information for syndrome based ECC generators
 * @options:	ECC specific options (see NAND_ECC_XXX flags defined above)
 * @priv:	pointer to private ECC control data
 * @hwctl:	function to control hardware ECC generator. Must only
 *		be provided if an hardware ECC is available
 * @calculate:	function for ECC calculation or readback from ECC hardware
 * @correct:	function for ECC correction, matching to ECC generator (sw/hw).
 *		Should return a positive number representing the number of
 *		corrected bitflips, -EBADMSG if the number of bitflips exceed
 *		ECC strength, or any other error code if the error is not
 *		directly related to correction.
 *		If -EBADMSG is returned the input buffers should be left
 *		untouched.
 * @read_page_raw:	function to read a raw page without ECC. This function
 *			should hide the specific layout used by the ECC
 *			controller and always return contiguous in-band and
 *			out-of-band data even if they're not stored
 *			contiguously on the NAND chip (e.g.
 *			NAND_ECC_HW_SYNDROME interleaves in-band and
 *			out-of-band data).
 * @write_page_raw:	function to write a raw page without ECC. This function
 *			should hide the specific layout used by the ECC
 *			controller and consider the passed data as contiguous
 *			in-band and out-of-band data. ECC controller is
 *			responsible for doing the appropriate transformations
 *			to adapt to its specific layout (e.g.
 *			NAND_ECC_HW_SYNDROME interleaves in-band and
 *			out-of-band data).
 * @read_page:	function to read a page according to the ECC generator
 *		requirements; returns maximum number of bitflips corrected in
 *		any single ECC step, 0 if bitflips uncorrectable, -EIO hw error
 * @read_subpage:	function to read parts of the page covered by ECC;
 *			returns same as read_page()
 * @write_subpage:	function to write parts of the page covered by ECC.
 * @write_page:	function to write a page according to the ECC generator
 *		requirements.
 * @write_oob_raw:	function to write chip OOB data without ECC
 * @read_oob_raw:	function to read chip OOB data without ECC
 * @read_oob:	function to read chip OOB data
 * @write_oob:	function to write chip OOB data
 */
struct nand_ecc_ctrl {
	nand_ecc_modes_t mode;
	int steps;
	int size;
	int bytes;
	int total;
	int strength;
	int prepad;
	int postpad;
	unsigned int options;
	void *priv;
	void (*hwctl)(struct mtd_info *mtd, int mode);
	int (*calculate)(struct mtd_info *mtd, const uint8_t *dat,
			uint8_t *ecc_code);
	int (*correct)(struct mtd_info *mtd, uint8_t *dat, uint8_t *read_ecc,
			uint8_t *calc_ecc);
};

/****************************************************************************/
struct spinand_info {
	u8		mid;
	u8		did;
	u8		ff_page_mode;	
	char		*name;
	u64		nand_size;
	u64		usable_size;

	u32		block_size;
	u32		block_main_size;
	/*u32		block_spare_size; */
	u16		block_num_per_chip;
	
	u16		page_size;
	u16		page_main_size;
	u16		page_spare_size;
	u16		page_num_per_block;
	
	u8		block_shift;
	u32		block_mask;

	u8		page_shift;
	u16		page_mask;

	u8		ecc_mask;
	u8		ecc_error;

	struct nand_ecc_ctrl ecc;
	struct nand_ecclayout *ecclayout;
	struct nand_ecclayout *ecctransfer;

	struct mutex mutex;
};

typedef enum {
	SPINAND_FL_READY,
	SPINAND_FL_READING,
	SPINAND_FL_WRITING,
	SPINAND_FL_ERASING,
	SPINAND_FL_SYNCING,
	SPINAND_FL_LOCKING,
	SPINAND_FL_RESETING,
	SPINAND_FL_OTPING,
	SPINAND_FL_PM_SUSPENDED,
} spinand_state_t;

/*
 * This constant declares the max. oobsize / page, which
 * is supported now. If you add a chip with bigger oobsize/page
 * adjust this accordingly.
 */
#define NAND_MAX_OOBSIZE	576
#define NAND_MAX_PAGESIZE	8192
	
struct spinand_chip { /* used for multi chip */
	spinlock_t		chip_lock;
	wait_queue_head_t wq;	
	spinand_state_t	state;
	struct spi_device	*spi_nand;
	struct spinand_info *info;
	struct mtd_partition *parts;
	int nr_parts;

	int (*reset) (struct spi_device *spi_nand);
	int (*read_id) (struct spi_device *spi_nand, u8* id);
	int (*read_page) (struct spi_device *spi_nand, struct spinand_info *info, int page_id, u16 offset, u16 len, u8* rbuf);
	int (*program_page) (struct spi_device *spi_nand, struct spinand_info *info, int page_id, u16 offset, u16 len, u8* wbuf);
	int (*erase_block) (struct spi_device *spi_nand, struct spinand_info *info, int block_id);
	int (*enable_ecc) (struct spi_device *spi_nand, struct spinand_info *info);
	int (*disable_ecc) (struct spi_device *spi_nand, struct spinand_info *info);

	u8 *databuf;
	u8 *buf;
	u8 *oob_poi;
	u8 *oobbuf; /* temp buffer */

	int pagebuf;
	int subpagesize;
		
	u8 ecc_calc[NAND_MAX_OOBSIZE >> 2];
	u8 ecc_code[NAND_MAX_OOBSIZE >> 2];

	struct list_head oob_list;

	int numchips;
	unsigned int options;
	uint64_t chipsize;
	int chip_shift;
	int page_shift;
	int pagemask;
	int phys_erase_shift;
	int bbt_erase_shift;
	int badblockpos;
	
	uint8_t *bbt;
	struct nand_bbt_descr *bbt_td;
	struct nand_bbt_descr *bbt_md;

	struct nand_bbt_descr *badblock_pattern;
};

struct spinand_oob{
	u16 page;
	u8 oob[SPINAND_MAX_OOBSIZE];
	u8 len;
	struct list_head list_head;
};

struct spinand_cmd {
	u8 cmd;
	unsigned n_addr;	
	u8 addr[3];
	unsigned n_dummy;
	unsigned n_tx;	
	u8 *tx_buf;
	unsigned n_rx;
	u8 *rx_buf;
};

#define CONFIG_MTD_PARTITIONS 1
#ifdef CONFIG_MTD_PARTITIONS
#define	mtd_has_partitions()	(1)
#else
#define	mtd_has_partitions()	(0)
#endif

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

extern int spinand_mtd(struct mtd_info *mtd);
extern int spinand_mtd_extern(struct mtd_info *mtd);
extern void spinand_mtd_release(struct mtd_info *mtd);
extern int spinand_update_bbt(struct mtd_info *mtd, loff_t offs);
extern int spinand_default_bbt(struct mtd_info *mtd);
extern int spinand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt);
extern int spinand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
		    int allowbbt);

#endif /* __LINUX_MTD_SPI_NAND_H */
