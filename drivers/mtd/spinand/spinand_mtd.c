/*
spinand_mtd.c

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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/spinand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "spinand_bbm.h"

extern void spinand_dump(uint8_t *buf, int size);

static int ff_memcmp(const void *cs, size_t count)
{
	const unsigned char *su1;
	int res = 0;

	for (su1 = cs; 0 < count; ++su1, count--)
		if ((res = 0xff - *su1) != 0)
			break;
	return res;
}

static int check_offs_len(struct mtd_info *mtd,
					loff_t ofs, uint64_t len)
{
	struct spinand_chip *chip = mtd->priv;
	int ret = 0;

	/* Start address must align on block boundary */
	if (ofs & ((1 << chip->phys_erase_shift) - 1)) {
		printk(KERN_ERR"%s: Unaligned address\n", __func__);
		ret = -EINVAL;
	}

	/* Length must align on block boundary */
	if (len & ((1 << chip->phys_erase_shift) - 1)) {
		printk(KERN_ERR"%s: Length not block aligned\n",
					__func__);
		ret = -EINVAL;
	}

	/* Do not allow past end of device */
	if (ofs + len > mtd->size) {
		printk(KERN_ERR"%s: Past end of device\n",
					__func__);
		ret = -EINVAL;
	}

	return ret;
}
/**
 * spinand_get_device - [GENERIC] Get chip for selected access
 * @param mtd		MTD device structure
 * @param new_state	the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int spinand_get_device(struct mtd_info *mtd, int new_state)
{
	struct spinand_chip *this = mtd->priv;
	DECLARE_WAITQUEUE(wait, current);

	/*
	 * Grab the lock and see if the device is available
	 */
	while (1) {
		spin_lock(&this->chip_lock);
		if (this->state == SPINAND_FL_READY) {
			this->state = new_state;
			spin_unlock(&this->chip_lock);
			break;
		}
		if (new_state == SPINAND_FL_PM_SUSPENDED) {
			spin_unlock(&this->chip_lock);
			return (this->state == SPINAND_FL_PM_SUSPENDED) ? 0 : -EAGAIN;
		}
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&this->wq, &wait);
		spin_unlock(&this->chip_lock);
		schedule();
		remove_wait_queue(&this->wq, &wait);
	}
	return 0;
}

/**
 * spinand_release_device - [GENERIC] release chip
 * @param mtd		MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void spinand_release_device(struct mtd_info *mtd)
{
	struct spinand_chip *this = mtd->priv;

	/* Release the chip */
	spin_lock(&this->chip_lock);
	this->state = SPINAND_FL_READY;
	wake_up(&this->wq);
	spin_unlock(&this->chip_lock);
}

static int spinand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	struct spinand_chip *chip = mtd->priv;
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int block_id;
	u8 is_bad = 0x00;
	int ret = 0;
	
	if (chip->options & NAND_BBT_SCANLASTPAGE)
		ofs += mtd->erasesize - mtd->writesize;

	if(getchip)
		spinand_get_device(mtd, SPINAND_FL_READING);

	block_id = ofs >> info->block_shift;
	
	ret = chip->read_page(spi_nand, info, block_id*info->page_num_per_block, info->page_main_size,1, &is_bad);
	if(ret != 0)
		printk("read page: fail, page id =%d %llx!\n", block_id*info->page_num_per_block,ofs);

	if (ret || is_bad != 0xFF)
	{
		ret =  1;	
		printk("\033[0;32;32m""[%02x]block id %d address %llx is bad.""\033[m""\n",is_bad,block_id,ofs);
	} else {
		ret = 0;
	}
	
	if(getchip)
		spinand_release_device(mtd);

	return ret; 
}

static int spinand_block_checkbad(struct mtd_info *mtd, loff_t ofs, int getchip,
			       int allowbbt)
{
	struct spinand_chip *chip = mtd->priv;

	if (!chip->bbt)
		return spinand_block_bad(mtd, ofs,getchip);

	/* Return info from the table */
	return spinand_isbad_bbt(mtd, ofs, allowbbt);
}

static int spinand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	/* Check for invalid offset */
	if (offs > mtd->size)
		return -EINVAL;

	return spinand_block_checkbad(mtd, offs, 1, 0);
}

/**
 * nand_default_block_markbad - [DEFAULT] mark a block bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int spinand_default_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct spinand_chip *chip = mtd->priv;
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int block_id;
	u8 is_bad = 0x00;
	u8 ret = 0;

	if (chip->options & NAND_BBT_SCANLASTPAGE)
		ofs += mtd->erasesize - mtd->writesize;

	/* Get block number */
	block_id = (int)(ofs >> chip->bbt_erase_shift);
	if (chip->bbt)
		chip->bbt[block_id >> 2] |= 0x01 << ((block_id & 0x03) << 1);

	/* Do we have a flash based bad block table ? */
	if (chip->options & NAND_USE_FLASH_BBT)
		ret = spinand_update_bbt(mtd, ofs);
	else {
		spinand_get_device(mtd, SPINAND_FL_WRITING);
					
		ret = chip->program_page(spi_nand, info, block_id*info->page_num_per_block, info->page_main_size, 1, &is_bad);
		if(ret != 0)
			printk("program page: fail, page id =%d %llx!\n", block_id*info->page_num_per_block,ofs);

		spinand_release_device(mtd);
	}
	if (!ret)
		mtd->ecc_stats.badblocks++;

	return ret;
}

/**
 * spinand_block_markbad - [MTD Interface] Mark bad block
 * @param mtd		MTD device structure
 * @param ofs       Bad block number
 */
static int spinand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	int ret;

	ret = spinand_block_isbad(mtd, ofs);
	if (ret) {
		/* If it was bad already, return success and do nothing. */
		if (ret > 0)
			return 0;
		return ret;
	}

	return spinand_default_block_markbad(mtd, ofs);
}

#define PATCH_OOB_CACHE

static uint8_t *spinand_transfer_oob(struct spinand_chip *chip, uint8_t *oob,
				  struct mtd_oob_ops *ops, size_t len)
{
	struct spinand_info *info = chip->info;

	switch (ops->mode) {

	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_RAW:
		memcpy(oob, chip->oob_poi + ops->ooboffs, len);
		return oob + len;

	case MTD_OPS_AUTO_OOB: {
		struct nand_oobfree *free = info->ecclayout->oobfree;
		uint32_t boffs = 0, roffs = ops->ooboffs;
		size_t bytes = 0;

		for (; free->length && len; free++, len -= bytes) {
			/* Read request not from offset 0 ? */
			if (unlikely(roffs)) {
				if (roffs >= free->length) {
					roffs -= free->length;
					continue;
				}
				boffs = free->offset + roffs;
				bytes = min_t(size_t, len,
					      (free->length - roffs));
				roffs = 0;
			} else {
				bytes = min_t(size_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(oob, chip->oob_poi + boffs, bytes);
			oob += bytes;
		}
		return oob;
	}
	default:
		BUG();
	}
	return NULL;
}

static int spinand_read_page_raw(struct mtd_info *mtd, struct spinand_chip *chip,
			      uint8_t *buf, int page)
{
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int retval;
	
	if (!buf)
		return -1;

	retval = chip->read_page(spi_nand, info, page, 0, info->page_main_size, buf);
	if (retval != 0)
	{
		printk(KERN_INFO "spinand_read_page_raw: fail, page=%d!\n", page);
		return retval;
	}

	retval = chip->read_page(spi_nand, info, page, info->page_main_size, info->page_spare_size, chip->oob_poi);
	if (retval != 0)
	{
		printk(KERN_INFO "spinand_read_page_raw: fail, page=%d!\n", page);
		return retval;
	}

	if((info->ecc.mode == NAND_ECC_SOFT) || (info->ecc.mode == NAND_ECC_SOFT_BCH)) 
	{	
		u8 ecccalc[NAND_MAX_OOBSIZE >> 2];
		u8 ecccode[NAND_MAX_OOBSIZE >> 2];
		int i, eccsize = info->ecc.size;
		int eccbytes = info->ecc.bytes;
		int eccsteps = info->ecc.steps;
		uint8_t *p = buf;
		uint8_t *ecc_calc = ecccalc;
		uint8_t *ecc_code = ecccode;
		uint32_t *eccpos = info->ecclayout->eccpos;

		//HEXDUMP(&info->buf[info->page_main_size],info->page_spare_size);
		
		for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
			info->ecc.calculate(mtd,p, &ecc_calc[i]);
		
		for (i = 0; i < info->ecc.total; i++)
			ecc_code[i] = chip->oob_poi[eccpos[i]];
		
		eccsteps = info->ecc.steps;
		p = buf;

		//HEXDUMP(ecc_calc,info->ecc.total);
		//HEXDUMP(ecc_code,info->ecc.total);
		
		for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
			int stat;

			stat = info->ecc.correct(mtd,p, &ecc_code[i], &ecc_calc[i]);
			if (stat < 0) {
				printk("spinand_read_page_raw correct fail, page=%d!\n", page);
				return -1;
			}
		}	
	}			
	return 0;
}

static int spinand_read_subpage_ops(struct mtd_info *mtd, struct spinand_chip *chip,int page,
			uint32_t data_offs, uint32_t readlen, uint8_t *bufpoi)
{
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int retval;

	if((info->ecc.mode == NAND_ECC_SOFT) || (info->ecc.mode == NAND_ECC_SOFT_BCH)) 
	{
		u8 ecccalc[NAND_MAX_OOBSIZE >> 2];
		u8 ecccode[NAND_MAX_OOBSIZE >> 2];
		int start_step, end_step, num_steps;
		uint32_t *eccpos = info->ecclayout->eccpos;
		uint8_t *p;
		int data_col_addr, i, gaps = 0;
		int datafrag_len, eccfrag_len, aligned_len, aligned_pos;
		int index = 0;

		/* Column address wihin the page aligned to ECC size (256bytes). */
		start_step = data_offs / info->ecc.size;
		end_step = (data_offs + readlen - 1) / info->ecc.size;
		num_steps = end_step - start_step + 1;

//		printk("[%s:%d] data_offs = %d readlen = %d start_step = %d end_step = %d num_steps = %d\n", 
//			__func__, __LINE__, data_offs,readlen,start_step,end_step,num_steps);
		
		/* Data size aligned to ECC ecc.size*/
		datafrag_len = num_steps * info->ecc.size;
		eccfrag_len = num_steps * info->ecc.bytes;

		data_col_addr = start_step * info->ecc.size;

//		printk("[%s:%d] datafrag_len = %d eccfrag_len = %d data_col_addr = %d\n", __func__, __LINE__, datafrag_len,eccfrag_len,data_col_addr);
		
		/* If we read not a page aligned data */
		p = chip->buf + data_col_addr;
		chip->read_page(spi_nand, info, page,data_col_addr , datafrag_len, p);
		
		/* Calculate  ECC */
		for (i = 0; i < eccfrag_len ; i += info->ecc.bytes, p += info->ecc.size)
			info->ecc.calculate(mtd, p, &ecccalc[i]);

		/* The performance is faster if to position offsets
		   according to ecc.pos. Let make sure here that
		   there are no gaps in ecc positions */
		for (i = 0; i < eccfrag_len - 1; i++) {
			if (eccpos[i + start_step * info->ecc.bytes] + 1 !=
				eccpos[i + start_step * info->ecc.bytes + 1]) {
				gaps = 1;
				break;
			}
		}
		
//		printk("[%s:%d] gaps = %d\n", __func__, __LINE__, gaps);
		if (gaps) {
			index = start_step * info->ecc.bytes;
			
			chip->read_page(spi_nand, info, page,mtd->writesize , mtd->oobsize, chip->oob_poi);
		} else {
			/* send the command to read the particular ecc bytes */
			/* take care about buswidth alignment in read_buf */
			index = start_step * info->ecc.bytes;
			
			aligned_pos = eccpos[index];
			aligned_len = eccfrag_len;

//			printk("[%s:%d] index = %d aligned_pos = %d aligned_len = %d\n", __func__, __LINE__, index,aligned_pos,aligned_len);

			chip->read_page(spi_nand, info, page,mtd->writesize + aligned_pos , aligned_len, &chip->oob_poi[aligned_pos]);
		}

		for (i = 0; i < eccfrag_len; i++)
			ecccode[i] = chip->oob_poi[eccpos[i + index]];

		p = chip->buf + data_col_addr;
		for (i = 0; i < eccfrag_len ; i += info->ecc.bytes, p += info->ecc.size) {
			int stat;

			stat = info->ecc.correct(mtd, p,&ecccode[i], &ecccalc[i]);
 			if (stat < 0) {				
				printk(KERN_INFO "spinand_read_subpage_ops: fail, from = %x offset = %d len = %d page=%d!\n", 
					(page << info->page_shift),data_offs,readlen,page);				
//				printk("[%s:%d] data_offs = %d readlen = %d start_step = %d end_step = %d num_steps = %d\n", 
//					__func__, __LINE__, data_offs,readlen,start_step,end_step,num_steps);
//				printk("[%s:%d] datafrag_len = %d eccfrag_len = %d data_col_addr = %d\n", 
//					__func__, __LINE__, datafrag_len,eccfrag_len,data_col_addr);				
//				spinand_dump(ecccalc,eccfrag_len);
//				spinand_dump(ecccode,eccfrag_len);
				return -1;
 			}
		}

		memcpy(bufpoi,chip->buf + data_offs,readlen);
	}
	else 
	{
		retval = chip->read_page(spi_nand, info, page,data_offs , readlen, bufpoi);
		if (retval != 0)
		{
			printk(KERN_INFO "spinand_read_subpage_ops: fail, from = %x offset = %d len = %d page=%d!\n", 
					(page << info->page_shift),data_offs,readlen,page);
			return retval;
		}
	}
	return 0;
}

static int spinand_read_page_oob_ops(struct mtd_info *mtd,struct spinand_chip *chip, uint8_t *buf, int page)
{
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int retval;

	if (!buf)
		return -1;

	retval = chip->read_page(spi_nand, info, page, 0, info->page_main_size, buf);
	if (retval != 0)
	{
		printk(KERN_INFO "spinand_read_page_raw: fail, page=%d!\n", page);
		return retval;
	}

	retval = chip->read_page(spi_nand, info, page, info->page_main_size, info->page_spare_size, chip->oob_poi);
	if (retval != 0)
	{
		printk(KERN_INFO "spinand_read_page_raw: fail, page=%d!\n", page);
		return retval;
	}

	if((info->ecc.mode == NAND_ECC_SOFT) || (info->ecc.mode == NAND_ECC_SOFT_BCH)) 
	{	
		u8 ecccalc[NAND_MAX_OOBSIZE >> 2];
		u8 ecccode[NAND_MAX_OOBSIZE >> 2];
		int i, eccsize = info->ecc.size;
		int eccbytes = info->ecc.bytes;
		int eccsteps = info->ecc.steps;
		uint8_t *p = buf;
		uint8_t *ecc_calc = ecccalc;
		uint8_t *ecc_code = ecccode;
		uint32_t *eccpos = info->ecclayout->eccpos;

		//HEXDUMP(&info->buf[info->page_main_size],info->page_spare_size);
		
		for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
			info->ecc.calculate(mtd,p, &ecc_calc[i]);
		
		for (i = 0; i < info->ecc.total; i++)
			ecc_code[i] = chip->oob_poi[eccpos[i]];
		
		eccsteps = info->ecc.steps;
		p = buf;

		//HEXDUMP(ecc_calc,info->ecc.total);
		//HEXDUMP(ecc_code,info->ecc.total);
		
		for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
			int stat;

			stat = info->ecc.correct(mtd,p, &ecc_code[i], &ecc_calc[i]);
			if (stat < 0) {
				printk("spinand_read_page_raw correct fail, page=%d!\n", page);
				return -1;
			}
		}	
	}
	
	return 0;
}

static int spinand_read_oob_ops(struct mtd_info *mtd, struct spinand_chip *chip,int page)
{
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	struct spinand_oob *oob,*n;	
	int retval;
	
	if (!chip->oob_poi)
		return -1;

#ifdef PATCH_OOB_CACHE
	list_for_each_entry_safe(oob,n, &chip->oob_list, list_head) 
	{
		if (oob && oob->page == page)
		{
			memcpy(chip->oob_poi,oob->oob,mtd->oobsize);
			printk(KERN_NOTICE"[%s:%d] read oob from list for page %d\n",__func__,__LINE__,page);
			return 0;
		}
	}
#endif

	retval = chip->read_page(spi_nand, info, page, info->page_main_size, info->page_spare_size,chip->oob_poi);
	if (retval != 0)
	{
		printk(KERN_ERR"spinand_read_oob_ops: fail, page=%d!\n", page);
		return retval;
	}

	return 0;
}

static int spinand_read_ops(struct mtd_info *mtd, loff_t from,
			  struct mtd_oob_ops *ops)
{
	struct spinand_chip *chip = mtd->priv;
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int page_id, page_offset, page_num;
	int count,size;
	int main_ok, main_left, main_offset;
	int retval,errcode=0;

	if (!chip->buf || !ops->datbuf)
		return -1;

	SPINAND_P("spinand read from=0x%llx, length=%d\n", from, ops->len);

	page_id = from >> info->page_shift;
	page_offset = from & info->page_mask;
	page_num = (page_offset + ops->len + info->page_main_size -1 ) / info->page_main_size;

	count = main_ok = 0;
	main_left = ops->len;
	main_offset = page_offset;

	SPINAND_P("page_id=%d,page_offset=%d,page_num=%d,main_left=%d,ops->len=%d\n",
			page_id, page_offset, page_num, main_left, ops->len);

	while (count < page_num){
		if ((main_offset + main_left) < info->page_main_size)
			size = main_left;
		else
			size = info->page_main_size - main_offset;
		
//		retval = chip->read_page(spi_nand, info, page_id + count, main_offset, size, ops->datbuf + main_ok);
		retval = spinand_read_subpage_ops(mtd,chip,page_id + count, main_offset, size, ops->datbuf + main_ok);
		if (retval != 0) {
			errcode = -1;
			printk(KERN_ERR"%s fail, page=%d, from=%llx!\n",__func__, page_id,from);
			return errcode;
		}
		
		main_ok += size;
		main_left -= size;
		main_offset = 0;

		ops->retlen = main_ok;

		count++;
	}
	
	return errcode;
}

static int spinand_do_read_oob(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	int page, realpage,ret;
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;
	int readlen = ops->ooblen;
	int len;
	uint8_t *buf = ops->oobbuf;

	if (ops->mode == MTD_OPS_AUTO_OOB)
		len = info->ecclayout->oobavail;
	else
		len = mtd->oobsize;

	if (unlikely(ops->ooboffs >= len)) {
		printk(KERN_ERR"%s: Attempt to start read "
					"outside oob\n", __func__);
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->size ||
		     ops->ooboffs + readlen > ((mtd->size >> info->page_shift) -
					(from >> info->page_shift)) * len)) {
		printk(KERN_ERR"%s: Attempt read beyond end "
					"of device\n", __func__);
		return -EINVAL;
	}

	/* Shift to get page */
	realpage = (int)(from >> info->page_shift);
	page = realpage/* & info->page_mask*/;

	while (1) {
		ret = spinand_read_oob_ops(mtd, chip, page);
		if(ret) {
			ops->oobretlen = 0;
			return -EPERM;
		}
		len = min(len, readlen);
		buf = spinand_transfer_oob(chip, buf, ops, len);

		readlen -= len;
		if (!readlen)
			break;

		/* Increment page address */
		realpage++;

		page = realpage/* & info->page_mask*/;
	}

	ops->oobretlen = ops->ooblen;
	
	return 0;
}

static int spinand_do_read_ops(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	int page, realpage, col, bytes, aligned;
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;	
	int ret = 0;
	uint32_t readlen = ops->len;
	uint32_t oobreadlen = ops->ooblen;
	uint32_t max_oobsize = ops->mode == MTD_OPS_AUTO_OOB ?
		mtd->oobavail : mtd->oobsize;

	uint8_t *bufpoi, *oob, *buf;
	
	realpage = (int)(from >> info->page_shift);
	page = realpage/* & info->page_mask*/;

	col = (int)(from & (mtd->writesize - 1));

	buf = ops->datbuf;
	oob = ops->oobbuf;

	while (1) {
		bytes = min(mtd->writesize - col, readlen);
		aligned = (bytes == mtd->writesize);

		/* Is the current page in the buffer ? */
		if (realpage != chip->pagebuf || oob) {
			bufpoi = aligned ? buf : chip->buf;

			/* Now read the page into the buffer */
			if (unlikely(ops->mode == MTD_OPS_RAW))
				ret = spinand_read_page_raw(mtd, chip,
							      bufpoi, page);
			else if (!aligned && !oob)
				ret = spinand_read_subpage_ops(mtd, chip,page,
							col, bytes, bufpoi);
			else
				ret = spinand_read_page_oob_ops(mtd, chip, bufpoi,page);
			if (ret < 0)
				break;

			/* Transfer not aligned data */
			if (!aligned) {
				memcpy(buf, chip->buf + col, bytes);
			}

			buf += bytes;

			if (unlikely(oob)) {

				int toread = min(oobreadlen, max_oobsize);

				if (toread) {
					oob = spinand_transfer_oob(chip,
						oob, ops, toread);
					oobreadlen -= toread;
				}
			}
		} else {
			memcpy(buf, chip->buf + col, bytes);
			buf += bytes;
		}

		readlen -= bytes;

		if (!readlen)
			break;

		/* For subsequent reads align to page boundary. */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage/* & info->page_mask*/;
	}

	ops->retlen = ops->len - (size_t) readlen;
	if (oob)
		ops->oobretlen = ops->ooblen - oobreadlen;

	if (ret)
		return ret;

	return  0;
}

static struct mtd_info *mtd_info_flash;

static u_char *spinand_cache_map = NULL;
static u32 spinand_cache_from = 0;
static u32 spinand_cache_len = 0;

static ssize_t spinand_proc_cache_write(struct file *filp,const char __user *buffer,
		size_t count, loff_t *data)
{
	char tmp[16];
	u32 from, len;
	struct mtd_info *mtd = mtd_info_flash;
	struct mtd_oob_ops ops = {0};
	
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
				if(spinand_cache_map)
					vfree(spinand_cache_map);
				spinand_get_device(mtd, SPINAND_FL_READING);
				spinand_cache_map = NULL;
				spinand_cache_from = 0;
				spinand_cache_len = 0;
				spinand_release_device(mtd);
				return count;
			}
			/* sanity checks */
			return 0;
		}

		printk("spinand cache from %08x len %08x\n",from,len);

		if (from + len > mtd->size) {
			printk("spinand error from %08x len %08x size %llx\n",from,len,mtd->size);
			return -EINVAL;
		}

		spinand_cache_map = vmalloc(len);
		if(!spinand_cache_map) {
			printk("spinand:no memory\n");
			return -ENOMEM;
		}

		spinand_get_device(mtd, SPINAND_FL_READING);
		
		ops.len = len;
		ops.datbuf = spinand_cache_map;
		spinand_read_ops(mtd, from, &ops);
		
		spinand_release_device(mtd);

		spinand_cache_len = ops.retlen;
		spinand_cache_from = from;

		printk("spinand cache spinand_cache_from %08x spinand_cache_len %08x\n",spinand_cache_from,spinand_cache_len);
	}
	
	return count;
}

static int spinand_read(struct mtd_info *mtd, loff_t from, size_t len,
	size_t *retlen, u_char *buf)
{
	struct mtd_oob_ops ops = {0};
	int ret;
	
	/* Do not allow reads past end of device */
	if ((from + len) > mtd->size)
		return -EINVAL;

	if (!len)
		return 0;

	if(spinand_cache_map) {
		if((u32)from >= spinand_cache_from && 
			((u32)from + len) <= (spinand_cache_from + spinand_cache_len)) {
			spinand_get_device(mtd, SPINAND_FL_READING);

			memcpy(buf,&spinand_cache_map[(u32)from - spinand_cache_from],len);
			*retlen = len;
			
			spinand_release_device(mtd);
			return 0;
		}
	}
	
	spinand_get_device(mtd, SPINAND_FL_READING);

	ops.len = len;
	ops.datbuf = buf;

	ret = spinand_read_ops(mtd, from, &ops);
 
 	*retlen = ops.retlen;

	spinand_release_device(mtd);

	return ret;
}

static int spinand_read_oob(struct mtd_info *mtd, loff_t from,
			  struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	ops->retlen = 0;

	if(!ops->datbuf)
		ops->len = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		printk(KERN_ERR"%s: Attempt read "
				"beyond end of device\n", __func__);
		return -EINVAL;
	}

	spinand_get_device(mtd, SPINAND_FL_READING);

	SPINAND_P("[%s:%d] from = %llx,mode=%d,len=%d,ooblen=%d,ooboffs=%d,databuf=%p,oobbuf=%p\n",
		__FUNCTION__,__LINE__,from,ops->mode,ops->len,ops->ooblen,ops->ooboffs,ops->datbuf,ops->oobbuf);

	switch (ops->mode) {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_AUTO_OOB:
	case MTD_OPS_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf)
		ret = spinand_do_read_oob(mtd, from, ops);
	else
		ret = spinand_do_read_ops(mtd, from, ops);
		
out:
	spinand_release_device(mtd);
	return ret;
}

static uint8_t *spinand_fill_oob(struct spinand_chip *chip, uint8_t *oob, size_t len,
						struct mtd_oob_ops *ops)
{
	struct spinand_info *info = chip->info;

	switch (ops->mode) {

	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_RAW:
		memcpy(chip->oob_poi + ops->ooboffs, oob, len);
		return oob + len;

	case MTD_OPS_AUTO_OOB: {
		struct nand_oobfree *free = info->ecclayout->oobfree;
		uint32_t boffs = 0, woffs = ops->ooboffs;
		size_t bytes = 0;

		for (; free->length && len; free++, len -= bytes) {
			/* Write request not from offset 0 ? */
			if (unlikely(woffs)) {
				if (woffs >= free->length) {
					woffs -= free->length;
					continue;
				}
				boffs = free->offset + woffs;
				bytes = min_t(size_t, len,
					      (free->length - woffs));
				woffs = 0;
			} else {
				bytes = min_t(size_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(chip->oob_poi + boffs, oob, bytes);
			oob += bytes;
		}
		return oob;
	}
	default:
		BUG();
	}
	return NULL;
}

static int spinand_write_page_oob_ops(struct mtd_info *mtd, struct spinand_chip *chip,
			   const uint8_t *buf, int page, int cached, int raw)
{
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;

	if (!chip->buf)
		return -1;

	memcpy(chip->buf,buf,info->page_main_size);
		
	memcpy(chip->buf + info->page_main_size,chip->oob_poi,mtd->oobsize);

	if(info->ff_page_mode)
	{
		int ff_page_main = ff_memcmp(chip->buf,info->page_main_size);
		int ff_page_spare = ff_memcmp(chip->buf + info->page_main_size,info->page_spare_size);
		
		if(ff_page_main && ff_page_spare)
		{
			return chip->program_page(spi_nand, info, page, 0, info->page_size, chip->buf);
		}
		else if(ff_page_main)
		{
			return chip->program_page(spi_nand, info, page, 0, info->page_main_size, chip->buf);
		}
		else if(ff_page_spare)
		{
			return chip->program_page(spi_nand, info, page, info->page_main_size, info->page_spare_size,chip->buf + info->page_main_size);
		}
		else
		{
			SPINAND_P("[%s:%d]skip whole 0XFF page\n",__func__,__LINE__);
			return 0;
		}
	}
	else	
	{
		if((info->ecc.mode == NAND_ECC_SOFT) || (info->ecc.mode == NAND_ECC_SOFT_BCH)) 
		{
			int i, eccsize = info->ecc.size;
			int eccbytes = info->ecc.bytes;
			int eccsteps = info->ecc.steps;
			uint8_t *ecc_calc = chip->ecc_calc;
			uint8_t *p = chip->buf;
			uint32_t *eccpos = info->ecclayout->eccpos;

			//printk("func(%s) line(%d):eccsize = %d eccbytes = %d eccsteps = %d\n", __func__, __LINE__, eccsize,eccbytes,eccsteps);
			
			/* Software ecc calculation */
			for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
				info->ecc.calculate(mtd,p, &ecc_calc[i]);
			}
			
			for (i = 0; i < info->ecc.total; i++) {
				chip->buf[info->page_main_size + eccpos[i]] = ecc_calc[i];
			}

			//spinand_dump(&chip->buf[info->page_main_size],info->page_spare_size);
		}
		return chip->program_page(spi_nand, info, page, 0, info->page_size, chip->buf);
	}
}

static int spinand_write_oob_ops(struct mtd_info *mtd, struct spinand_chip *chip,
			      int page)
{
#ifndef PATCH_OOB_CACHE
	struct spi_device *spi_nand = chip->spi_nand;
#endif
	struct spinand_info *info = chip->info;
	struct spinand_oob *oob,*n;
	int ret = 0;
	
	if (!chip->buf)
		return -1;

	memset(chip->buf, 0xFF, info->page_main_size); 

#ifdef PATCH_OOB_CACHE
	list_for_each_entry_safe(oob,n, &chip->oob_list, list_head) 
	{
		if (oob && oob->page == page)
		{
			memcpy(oob->oob,chip->oob_poi,mtd->oobsize);
			printk(KERN_NOTICE"[%s:%d] oob list update for page %d\n",__func__,__LINE__,page);
			return 0;
		}
	}

	oob = vmalloc(sizeof(struct spinand_oob));
	if(!oob)
	{
		printk(KERN_ERR"[%s:%d]vmalloc struct spinand_oob fail\n",__func__,__LINE__);
		return -1;
	}

	oob->page = page;
	oob->len = mtd->oobsize;
	memcpy(oob->oob,chip->oob_poi,mtd->oobsize);
	
	list_add(&oob->list_head, &chip->oob_list);
	
	return ret;
#else	
	if(info->ff_page_mode)
	{
		int ff_page_spare = ff_memcmp(chip->oob_poi,mtd->oobsize);
		
		if(ff_page_spare)
		{
			return chip->program_page(spi_nand, info, page, info->page_main_size, info->page_spare_size,chip->oob_poi);
		}
		else
		{
			SPINAND_P("[%s:%d]skip whole 0XFF page\n",__func__,__LINE__);
			return 0;
		}
	}
	else
	{
		memcpy(chip->buf+info->page_main_size, chip->oob_poi, info->page_spare_size); 

		if((info->ecc.mode == NAND_ECC_SOFT) || (info->ecc.mode == NAND_ECC_SOFT_BCH)) 
		{
			int i, eccsize = info->ecc.size;
			int eccbytes = info->ecc.bytes;
			int eccsteps = info->ecc.steps;
			uint8_t *ecc_calc = chip->ecc_calc;
			uint8_t *p = chip->buf;
			uint32_t *eccpos = info->ecclayout->eccpos;

			//printk("func(%s) line(%d):eccsize = %d eccbytes = %d eccsteps = %d\n", __func__, __LINE__, eccsize,eccbytes,eccsteps);
			
			/* Software ecc calculation */
			for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
				info->ecc.calculate(mtd,p, &ecc_calc[i]);
			}
			
			for (i = 0; i < info->ecc.total; i++) {
				chip->buf[info->page_main_size + eccpos[i]] = ecc_calc[i];
			}

			//HEXDUMP(&info->buf[info->page_main_size],info->page_spare_size);
		}
		
		return chip->program_page(spi_nand, info, page, 0, info->page_size, chip->buf);
	}
#endif
}

static int spinand_write_ops(struct mtd_info *mtd, loff_t to,
			  struct mtd_oob_ops *ops)
{
	struct spinand_chip *chip = mtd->priv;
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	int page_id, page_offset, page_num, oob_num;
	uint8_t *oob = ops->oobbuf;
	int count;

	int main_ok, main_left, main_offset;
	int oob_ok, oob_left;
	
	signed int retval;
	signed int errcode=0;

	if (!chip->buf)
		return -1;

	SPINAND_P("\033[0;32;31m""spinand write to=0x%llx, length=%d""\033[m""\n", to, ops->len);

	page_id = to >> info->page_shift;

	/* for main data */
	page_offset = to & info->page_mask;
	page_num = (page_offset + ops->len + info->page_main_size -1 ) / info->page_main_size;

	/* for oob */
	oob_num = (ops->ooblen + info->ecclayout->oobavail -1) / info->ecclayout->oobavail;

	count = 0;

	main_left = ops->len;
	main_ok = 0;
	main_offset = page_offset;

	oob_left = ops->ooblen;
	oob_ok = 0;

	/* If we're not given explicit OOB data, let it be 0xFF */
	if (!oob)
		memset(chip->oob_poi, 0xff, info->page_spare_size);

//	printk("func(%s) line(%d):page_id=%d,page_offset=%d,page_num=%d,main_left=%d,ops->len=%d\n", 
//			__func__, __LINE__, page_id, page_offset, page_num, main_left, ops->len);

	while (1)
	{
		if (count < page_num || count < oob_num) 
		{
			struct spinand_oob *oob, *n;
		
			memset(chip->buf, 0xFF, info->page_size); 
#ifdef PATCH_OOB_CACHE
			if(oob_num && count < oob_num) {
				list_for_each_entry_safe(oob,n, &chip->oob_list, list_head) 
				{
					if (oob && oob->page == (page_id + count))
					{
						memcpy(chip->buf + info->page_main_size, oob->oob, oob->len); 
						list_del(&oob->list_head);
						vfree(oob);
					}
				}
			}
#endif
		}
		else
		{
			break; 
		}

		if (count < page_num && ops->datbuf) 
		{
			int size;

			if ((main_offset + main_left) < info->page_main_size)
			{
				size = main_left;
			}
			else
			{
				size = info->page_main_size - main_offset;
			}

			memcpy (chip->buf, ops->datbuf + main_ok, size);
			
			main_ok += size;
			main_left -= size;
			main_offset = 0;			
		}

		if (count < oob_num && ops->oobbuf && chip->oobbuf)
		{
			int size = 0,bytes = 0,boffs = 0,temp = 0;
			struct nand_oobfree *free = info->ecclayout->oobfree;

			memset(chip->oobbuf, 0xFF, info->ecclayout->oobavail);

			if (oob_left < info->ecclayout->oobavail) 
				size = oob_left;
			else
				size = info->ecclayout->oobavail;

			memcpy (chip->oobbuf, ops->oobbuf + oob_ok, size);

			oob_ok += size;
			oob_left -= size;

			/* repack oob to spare */
			for (; free->length && size; free++, size -= bytes) {
				bytes = min_t(size_t, size, free->length);
				boffs = free->offset;
				memcpy(chip->buf + info->page_main_size + boffs, chip->oobbuf + temp, bytes);
				temp += bytes;
			}
		}

		if (count < page_num || count < oob_num) 
		{
			if(info->ff_page_mode)
			{
				int ff_page_main = ff_memcmp(chip->buf,info->page_main_size);
				int ff_page_spare = ff_memcmp(chip->buf + info->page_main_size,info->page_spare_size);
				
				if(ff_page_main && ff_page_spare)
				{
					retval = chip->program_page(spi_nand, info, page_id + count, 0, info->page_size, chip->buf);
				}
				else if(ff_page_main)
				{
					retval = chip->program_page(spi_nand, info, page_id + count, 0, info->page_main_size, chip->buf);
				}
				else if(ff_page_spare)
				{
					retval = chip->program_page(spi_nand, info, page_id + count, info->page_main_size, info->page_spare_size,chip->buf + info->page_main_size);
				}
				else
				{
					SPINAND_P("[%s:%d]skip whole 0XFF page\n",__func__,__LINE__);
					retval = 0;
				}
			}
			else
			{
				if((info->ecc.mode == NAND_ECC_SOFT) || (info->ecc.mode == NAND_ECC_SOFT_BCH)) 
				{
					int i, eccsize = info->ecc.size;
					int eccbytes = info->ecc.bytes;
					int eccsteps = info->ecc.steps;
					uint8_t *ecc_calc = chip->ecc_calc;
					uint8_t *p = chip->buf;
					uint32_t *eccpos = info->ecclayout->eccpos;

					//printk("func(%s) line(%d):eccsize = %d eccbytes = %d eccsteps = %d\n", __func__, __LINE__, eccsize,eccbytes,eccsteps);
					SPINAND_P(KERN_INFO "spinand_write_ops page=%d\n", page_id + count);
					
					/* Software ecc calculation */
					for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
						info->ecc.calculate(mtd,p, &ecc_calc[i]);
					}
				
					//spinand_dump(ecc_calc,info->ecc.total);
					
					for (i = 0; i < info->ecc.total; i++) {
						chip->buf[info->page_main_size + eccpos[i]] = ecc_calc[i];
					}

					//spinand_dump(&chip->buf[info->page_main_size],info->page_spare_size);
				}
				retval = chip->program_page(spi_nand, info, page_id + count, 0, info->page_size, chip->buf);
			}
			if (retval != 0)
			{
				errcode = -1;
				printk(KERN_INFO "spinand_write_ops: fail, page=%d!\n", page_id);

				return errcode;
			}
		}

		if (count < page_num && ops->datbuf) 
		{
			ops->retlen = main_ok;
		}

		if (count < oob_num && ops->oobbuf && chip->oobbuf)
		{
			ops->oobretlen = oob_ok;
		}

		count++;

	}

	return errcode;
}

static int spinand_do_write_oob(struct mtd_info *mtd, loff_t to,
			  struct mtd_oob_ops *ops)
{
	int ret,page, len;
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;

	if (ops->mode == MTD_OPS_AUTO_OOB)
		len = info->ecclayout->oobavail;
	else
		len = mtd->oobsize;

	/* Do not allow write past end of page */
	if ((ops->ooboffs + ops->ooblen) > len) {
		printk(KERN_ERR"%s: Attempt to write "
				"past end of page\n", __func__);
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= len)) {
		printk(KERN_ERR"%s: Attempt to start "
				"write outside oob\n", __func__);
		return -EINVAL;
	}

	/* Do not allow write past end of device */
	if (unlikely(to >= mtd->size ||
		     ops->ooboffs + ops->ooblen >
			((mtd->size >> info->page_shift) -
			 (to >> info->page_shift)) * len)) {
		printk("%s: Attempt write beyond "
				"end of device\n", __func__);
		return -EINVAL;
	}
	
	/* Shift to get page */
	page = (int)(to >> info->page_shift);

	memset(chip->oob_poi, 0xff, mtd->oobsize);
	spinand_fill_oob(chip, ops->oobbuf, ops->ooblen, ops);
	ret = spinand_write_oob_ops(mtd, chip, page/* & info->page_mask*/);
	memset(chip->oob_poi, 0xff, mtd->oobsize);

	if (ret)
		return ret;

	ops->oobretlen = ops->ooblen;
	
	return 0;
}

static int spinand_do_write_ops(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
#define NOTALIGNED(x)	((x & (chip->subpagesize - 1)) != 0)

	int realpage, page, blockmask, column;
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;	
	uint32_t writelen = ops->len;

	uint32_t oobwritelen = ops->ooblen;
	uint32_t oobmaxlen = ops->mode == MTD_OPS_AUTO_OOB ?
				mtd->oobavail : mtd->oobsize;

	uint8_t *oob = ops->oobbuf;
	uint8_t *buf = ops->datbuf;
	int ret, subpage;

	ops->retlen = 0;
	if (!writelen)
		return 0;

	/* reject writes, which are not page aligned */
	if (NOTALIGNED(to) || NOTALIGNED(ops->len)) {
		printk(KERN_ERR "%s: Attempt to write not "
				"page aligned data\n", __func__);
		return -EINVAL;
	}

	SPINAND_P("[%s:%d] to = %llx,mode=%d,len=%d,ooblen=%d,ooboffs=%d,databuf=%p,oobbuf=%p\n",
		__FUNCTION__,__LINE__,to,ops->mode,ops->len,ops->ooblen,ops->ooboffs,ops->datbuf,ops->oobbuf);

	column = to & (mtd->writesize - 1);
	subpage = column || (writelen & (mtd->writesize - 1));

	if (subpage && oob){
		printk(KERN_ERR "%s: Attempt to write error "
				"subpage data\n", __func__);
		return -EINVAL;
	}

	realpage = (int)(to >> info->page_shift);
	page = realpage/* & info->page_mask*/;
	blockmask = (1 << (info->block_shift - info->page_shift)) - 1;

	/* If we're not given explicit OOB data, let it be 0xFF */
	if (likely(!oob))
		memset(chip->oob_poi, 0xff, mtd->oobsize);

	/* Don't allow multipage oob writes with offset */
	if (oob && ops->ooboffs && (ops->ooboffs + ops->ooblen > oobmaxlen))
		return -EINVAL;

	while (1) {
		int bytes = mtd->writesize;
		int cached = writelen > bytes && page != blockmask;
		uint8_t *wbuf = buf;

		/* Partial page write ? */
		if (unlikely(column || writelen < (mtd->writesize - 1))) {
			cached = 0;
			bytes = min_t(int, bytes - column, (int) writelen);
			chip->pagebuf = -1;
			memset(chip->buf, 0xff, mtd->writesize);
			memcpy(&chip->buf[column], buf, bytes);
			wbuf = chip->buf;
		}

		if (unlikely(oob)) {
			size_t len = min(oobwritelen, oobmaxlen);
			oob = spinand_fill_oob(chip, oob, len, ops);
			oobwritelen -= len;
		}

		ret = spinand_write_page_oob_ops(mtd, chip, wbuf, page, cached,
				       (ops->mode == MTD_OPS_RAW));
		if (ret)
			break;

		writelen -= bytes;
		if (!writelen)
			break;

		column = 0;
		buf += bytes;
		realpage++;

		page = realpage/* & info->page_mask*/;
	}

	ops->retlen = ops->len - writelen;
	if (unlikely(oob))
		ops->oobretlen = ops->ooblen;
	return ret;
}

static int SPINAND_SPEED_WRITE = 0;

static ssize_t spinand_proc_speed_write(struct file *filp,const char __user *buf,
		size_t count, loff_t *data)
{
	char		*myString;
	
	myString = (char *) kmalloc(count + 1, GFP_KERNEL);
	if (!myString)
	    return -ENOMEM;
	    
	if (copy_from_user(myString, buf, count))
		return -EFAULT;

	myString[count] = '\0';

	sscanf(myString, "%d", &SPINAND_SPEED_WRITE);	

	kfree(myString);

	return count;
}

static int spinand_proc_speed_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", SPINAND_SPEED_WRITE);
	return 0;
}

static int spinand_proc_speed_open(struct inode *inode, struct file *file)
{
	return single_open(file, spinand_proc_speed_show, NULL);
}

static const struct file_operations spinand_proc_speed_fops = {
	.owner = THIS_MODULE,
	.open = spinand_proc_speed_open,
	.read = seq_read,
	.write = spinand_proc_speed_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static uint8_t *spinand_transfer_oob_psu1gs20bn(struct spinand_chip *chip,uint8_t *oob)
{
	struct spinand_info *info = chip->info;
	u8 oobavail[info->ecclayout->oobavail];
	u8 *oobfree = oobavail;
	struct nand_oobfree *free = info->ecctransfer->oobfree;
	uint32_t boffs = 0, roffs = 0,woffs = 0;
	size_t bytes = 0, len = info->page_spare_size;

	if(info->ecctransfer->oobfree == info->ecclayout->oobfree)
		return oob;
	
	for (; free->length && len; free++, len -= bytes) 
	{
		if (unlikely(roffs)) {
			if (roffs >= free->length) {
				roffs -= free->length;
				continue;
			}
			boffs = free->offset + roffs;
			bytes = min_t(size_t, len,
				      (free->length - roffs));
			roffs = 0;
		} else {
			bytes = min_t(size_t, len, free->length);
			boffs = free->offset;
		}
		memcpy(oobfree, oob + boffs, bytes);
		oobfree += bytes;
	}

	len = info->page_spare_size;
	memset(oob,0xFF,len);
	oobfree = oobavail;
	free = info->ecclayout->oobfree;
	for (boffs = 0,bytes = 0; free->length && len; free++, len -= bytes) 
	{
		if (unlikely(woffs)) {
			if (woffs >= free->length) {
				woffs -= free->length;
				continue;
			}
			boffs = free->offset + woffs;
			bytes = min(len,
				      (free->length - woffs));
			woffs = 0;
		} else {
			bytes = min( len, free->length);
			boffs = free->offset;
		}
		memcpy(oob + boffs, oobfree, bytes);
		oobfree += bytes;
	}
	return oob;
}

static int spinand_write(struct mtd_info *mtd, loff_t to, size_t len,
	size_t *retlen, const u_char *buf)
{
	struct mtd_oob_ops ops = {0};
	int ret = 0;
	
	/* Do not allow reads past end of device */
	if ((to + len) > mtd->size)
		return -EINVAL;
	if (!len)
		return 0;

	/* patch for spinand speed*/
	if(SPINAND_SPEED_WRITE)
	{
		struct spinand_chip *chip = mtd->priv;
		const u_char *wbuf = buf;
		uint32_t wlen = 0;	
		loff_t wto = to;
		
		if ((len % (mtd->writesize + mtd->oobsize)) != 0) {
			if(len <= mtd->writesize)
			{
//				printk(KERN_NOTICE "[%s]Input is in speed up mode - %d - %d\n",__FUNCTION__,
//					len,mtd->writesize);
				goto next;
			}
			printk(KERN_ERR "[%s]Input is not page aligned - %d - %d\n",__FUNCTION__,
				len,mtd->writesize + mtd->oobsize);
			return -EINVAL;
		}

		spinand_get_device(mtd, SPINAND_FL_WRITING);
		
		while(wlen < len) {
			ops.len = mtd->writesize;
			ops.datbuf = (uint8_t *)wbuf;
			ops.ooboffs = 0;
			ops.ooblen = mtd->oobsize;
			ops.oobbuf = spinand_transfer_oob_psu1gs20bn(chip,(uint8_t *)(wbuf + mtd->writesize));
			ops.mode = MTD_OPS_PLACE_OOB;
			ret = spinand_do_write_ops(mtd, wto, &ops);
			if(ret != 0) {
				printk(KERN_ERR "do write ops fail\n");
				spinand_release_device(mtd);
				return -EINVAL;
			}

			wto += mtd->writesize;
			wlen += mtd->writesize + mtd->oobsize;
			wbuf += mtd->writesize + mtd->oobsize;
		}
		
		*retlen = len;
		spinand_release_device(mtd);
		return ret;
	}

next:	
	spinand_get_device(mtd, SPINAND_FL_WRITING);

	ops.len = len;
	ops.datbuf = (uint8_t *)buf;
		
	ret =  spinand_write_ops(mtd, to, &ops);

	*retlen = ops.retlen;

	spinand_release_device(mtd);
	
	return ret;
}

static int spinand_write_oob(struct mtd_info *mtd, loff_t to,
			  struct mtd_oob_ops *ops)
{
	int ret = -ENOTSUPP;

	ops->retlen = 0;

	if(!ops->datbuf)
	{
		ops->len = 0;
	
//		printk("[%s:%d] to = %llx,mode=%d,len=%d,ooblen=%d,ooboffs=%d,databuf=%p,oobbuf=%p\n",
//			__FUNCTION__,__LINE__,to,ops->mode,ops->len,ops->ooblen,ops->ooboffs,ops->datbuf,ops->oobbuf);	
	}
	
	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) {
		printk(KERN_ERR"%s: Attempt write beyond "
				"end of device\n", __func__);
		return -EINVAL;
	}

	spinand_get_device(mtd, SPINAND_FL_WRITING);

	SPINAND_P("[%s:%d] to = %llx,mode=%d,len=%d,ooblen=%d,ooboffs=%d,databuf=%p,oobbuf=%p\n",
		__FUNCTION__,__LINE__,to,ops->mode,ops->len,ops->ooblen,ops->ooboffs,ops->datbuf,ops->oobbuf);

	switch (ops->mode) {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_AUTO_OOB:
	case MTD_OPS_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf)
		ret = spinand_do_write_oob(mtd, to, ops);
	else
		ret = spinand_do_write_ops(mtd, to, ops);

out:
	spinand_release_device(mtd);
	
	return ret;
}

/**
 * nand_erase_nand - [Internal] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 * @allowbbt:	allow erasing the bbt area
 *
 * Erase one ore more blocks
 */
int spinand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
		    int allowbbt)
{
#define BBT_PAGE_MASK	0xffffff3f

	int page, status, pages_per_block, ret,chipnr;
	struct spinand_chip *chip = mtd->priv;
	struct spi_device *spi_nand = chip->spi_nand;
	struct spinand_info *info = chip->info;
	loff_t rewrite_bbt[NAND_MAX_CHIPS] = {0};
	unsigned int bbt_masked_page = 0xffffffff;
	loff_t len;

	if (check_offs_len(mtd, instr->addr, instr->len))
		return -EINVAL;

	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;

	/* Grab the lock and see if the device is available */
	spinand_get_device(mtd, SPINAND_FL_ERASING);

	/* Shift to get first page */
	page = (int)(instr->addr >> chip->page_shift);
	chipnr = (int)(instr->addr >> chip->chip_shift);

	SPINAND_P("\033[0;32;31m""[%s:%d]start = 0x%llx, len = %lld, chipnr = %d""\033[m""\n",
				__func__, __LINE__,(unsigned long long)instr->addr,
				(unsigned long long)instr->len,chipnr);

	/* Calculate pages in each block */
	pages_per_block = 1 << (chip->phys_erase_shift - chip->page_shift);

	/*
	 * If BBT requires refresh, set the BBT page mask to see if the BBT
	 * should be rewritten. Otherwise the mask is set to 0xffffffff which
	 * can not be matched. This is also done when the bbt is actually
	 * erased to avoid recusrsive updates
	 */
	if (chip->options & BBT_AUTO_REFRESH && !allowbbt)
		bbt_masked_page = chip->bbt_td->pages[chipnr] & BBT_PAGE_MASK;

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

	while (len) {
		/*
		 * check if we have a bad block, we do not erase bad blocks !
		 */
		if (spinand_block_checkbad(mtd, ((loff_t) page) <<
					chip->page_shift, 0, allowbbt)) {
			printk(KERN_WARNING "%s: attempt to erase a bad block "
					"at page 0x%08x\n", __func__, page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}

		status = chip->erase_block(spi_nand, info, page/pages_per_block);
		if (status) {
			printk("%s: Failed erase,page %d block %d\n", __func__,page, page/pages_per_block);
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr =
				((loff_t)page << chip->page_shift);
			goto erase_exit;
		}

		/*
		 * If BBT requires refresh, set the BBT rewrite flag to the
		 * page being erased
		 */
		if (bbt_masked_page != 0xffffffff &&
		    (page & BBT_PAGE_MASK) == bbt_masked_page)
			    rewrite_bbt[chipnr] =
					((loff_t)page << chip->page_shift);

		/* Increment page address and decrement length */
		len -= (1 << chip->phys_erase_shift);
		page += pages_per_block;

		/* Check, if we cross a chip boundary */
		if (len && !(page & chip->pagemask)) {
			printk("%s: Failed erase cross a chip boundary\n", __func__);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}
	}
	
	instr->state = MTD_ERASE_DONE;

erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	/* Deselect and wake up anyone waiting on the device */
	spinand_release_device(mtd);

	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/*
	 * If BBT requires refresh and erase was successful, rewrite any
	 * selected bad block tables
	 */
	if (bbt_masked_page == 0xffffffff || ret)
		return ret;

	for (chipnr = 0; chipnr < chip->numchips; chipnr++) {
		if (!rewrite_bbt[chipnr])
			continue;
		/* update the BBT for chip */
		DEBUG(MTD_DEBUG_LEVEL0, "%s: nand_update_bbt "
			"(%d:0x%0llx 0x%0x)\n", __func__, chipnr,
			rewrite_bbt[chipnr], chip->bbt_td->pages[chipnr]);
		spinand_update_bbt(mtd, rewrite_bbt[chipnr]);
	}

	/* Return more or less happy */
	return ret;
}

/**
 * spinand_erase - [MTD Interface] erase block(s)
 * @param mtd		MTD device structure
 * @param instr		erase instruction
 *
 * Erase one ore more blocks
 */
static int spinand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return spinand_erase_nand(mtd, instr, 0);
}
/**
 * spinand_sync - [MTD Interface] sync
 * @param mtd		MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void spinand_sync(struct mtd_info *mtd)
{
	DEBUG(MTD_DEBUG_LEVEL3, "spinand_sync: called\n");

	/* Grab the lock and see if the device is available */
	spinand_get_device(mtd, SPINAND_FL_SYNCING);

	/* Release it and go back */
	spinand_release_device(mtd);
}


/**
 * spinand_suspend - [MTD Interface] Suspend the spinand flash
 * @param mtd		MTD device structure
 */
static int spinand_suspend(struct mtd_info *mtd)
{
	return spinand_get_device(mtd, SPINAND_FL_PM_SUSPENDED);
}

/**
 * spinand_resume - [MTD Interface] Resume the spinand flash
 * @param mtd		MTD device structure
 */
static void spinand_resume(struct mtd_info *mtd)
{
	struct spinand_chip *this = mtd->priv;

	if (this->state == SPINAND_FL_PM_SUSPENDED)
		spinand_release_device(mtd);
	else
		printk(KERN_ERR "resume() called for the chip which is not"
				"in suspended state\n");
}

static LIST_HEAD(proc_sr_list);

struct proc_sr_name {
	char			      name[128];
	struct list_head    list_head;
};

static ssize_t spinand_proc_suspend_write(struct file *filp,const char __user *buf,
		size_t len, loff_t *data)
{
	ssize_t 	ret = -ENOMEM;
	struct proc_sr_name *sr,*n;
	char *temp;
	char* myString = kmalloc(len + 1, GFP_KERNEL);

	if (myString) 
	{
		ret = -EFAULT;
		if (copy_from_user(myString, buf, len))
			goto out;

		temp = strchr(myString,'\n');
		if(temp)
			myString[temp-myString] = '\0';
		else
			myString[len] = '\0';
				
		/* always return count to avoid endless loop */
		ret = len;	

		list_for_each_entry_safe(sr,n, &proc_sr_list, list_head) 
		{
			if (sr && !strcmp(sr->name,myString))
				goto out;
		}

		sr = vmalloc(sizeof(struct proc_sr_name));
		if(!sr)
		{
			printk(KERN_ERR"[%s:%d]vmalloc struct proc_sr_name fail\n",__func__,__LINE__);
			goto out;
		}

		strcpy(sr->name,myString);
		list_add(&sr->list_head, &proc_sr_list);
	}
	
out:
	kfree(myString);
	return ret;
}

static int spinand_proc_suspend_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", 0);
	return 0;
}

static int spinand_proc_suspend_open(struct inode *inode, struct file *file)
{
	return single_open(file, spinand_proc_suspend_show, NULL);
}

static const struct file_operations spinand_proc_suspend_fops = {
	.owner = THIS_MODULE,
	.open = spinand_proc_suspend_open,
	.read = seq_read,
	.write = spinand_proc_suspend_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t spinand_proc_resume_write(struct file *filp,const char __user *buf,
		size_t len, loff_t *data)
{
	ssize_t 	ret = -ENOMEM;
	struct proc_sr_name *sr,*n;
	char *temp;
	char* myString = kmalloc(len + 1, GFP_KERNEL);
	
	if (myString) 
	{
		ret = -EFAULT;
		if (copy_from_user(myString, buf, len))
			goto out;

		temp = strchr(myString,'\n');
		if(temp)
			myString[temp-myString] = '\0';
		else
			myString[len] = '\0';

		list_for_each_entry_safe(sr,n, &proc_sr_list, list_head) 
		{
			if (sr && !strcmp(sr->name,myString))
			{
				list_del(&sr->list_head);
				memset(sr->name,0,sizeof(sr->name));
				vfree(sr);
			}
		}

		/* always return count to avoid endless loop */
		ret = len;	
	}
	
out:
	kfree(myString);
	return ret;
}

static int spinand_proc_resume_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", 0);
	return 0;
}

static int spinand_proc_resume_open(struct inode *inode, struct file *file)
{
	return single_open(file, spinand_proc_resume_show, NULL);
}

static const struct file_operations spinand_proc_resume_fops = {
	.owner = THIS_MODULE,
	.open = spinand_proc_resume_open,
	.read = seq_read,
	.write = spinand_proc_resume_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int spinand_proc_cache_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", 0);
	return 0;
}

static int spinand_proc_cache_open(struct inode *inode, struct file *file)
{
	return single_open(file, spinand_proc_cache_show, NULL);
}

static const struct file_operations spinand_proc_cache_fops = {
	.owner = THIS_MODULE,
	.open = spinand_proc_cache_open,
	.read = seq_read,
	.write = spinand_proc_cache_write,
	.llseek = seq_lseek,
	.release = single_release,
};

int spinand_proc_suspend_by_name(const char *name)
{
	struct proc_sr_name *sr;

	if(list_empty(&proc_sr_list)) 
		return 0;
	
	list_for_each_entry(sr,&proc_sr_list, list_head)  {
		if (sr && !strcmp(sr->name,name)) {
//			printk("SPI Nand Filesystem Write Suspend MTD : %s\n",name);
			return 1;
		}
	}
	return 0;
}
EXPORT_SYMBOL(spinand_proc_suspend_by_name);

static int spinand_create_proc(void)
{
	struct proc_dir_entry *spinand_dir;
	struct proc_dir_entry *spinand_file_yaffs2_speed;
	struct proc_dir_entry *spinand_file_yaffs2_suspend;
	struct proc_dir_entry *spinand_file_yaffs2_resume;
	struct proc_dir_entry *spinand_file_cache;

	spinand_dir = proc_mkdir("spinand", NULL);
	if(spinand_dir == NULL)
		return -ENOMEM;

	/* yaffs2 image read/write faster*/
	spinand_file_yaffs2_speed = proc_create("speed_write", 0644, spinand_dir,&spinand_proc_speed_fops);
	if(spinand_file_yaffs2_speed == NULL) {
		remove_proc_entry("speed_write", spinand_dir);
		return -ENOMEM;
	}
	
	/* afffs2 program mutex app update */
	spinand_file_yaffs2_suspend = proc_create("suspend", 0644, spinand_dir,&spinand_proc_suspend_fops);
	if(spinand_file_yaffs2_suspend == NULL) {
		remove_proc_entry("speed_write", spinand_dir);
		remove_proc_entry("suspend", spinand_dir);
		return -ENOMEM;
	}
	
	spinand_file_yaffs2_resume = proc_create("resume", 0644, spinand_dir,&spinand_proc_resume_fops);
	if(spinand_file_yaffs2_resume == NULL) {
		remove_proc_entry("speed_write", spinand_dir);
		remove_proc_entry("suspend", spinand_dir);
		remove_proc_entry("resume", spinand_dir);
		return -ENOMEM;
	}

	spinand_file_cache = proc_create("cache_map", 0644, spinand_dir,&spinand_proc_cache_fops);
	if(spinand_file_cache == NULL) {
		remove_proc_entry("speed_write", spinand_dir);
		remove_proc_entry("suspend", spinand_dir);
		remove_proc_entry("resume", spinand_dir);
		return -ENOMEM;
	}
	
	return 0;
}

/**
 * spinand_mtd - add MTD device with parameters
 * @param mtd		MTD device structure
 *
 * Add MTD device with parameters.
 */
int spinand_mtd(struct mtd_info *mtd)
{
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;

	chip->state = SPINAND_FL_READY;
	init_waitqueue_head(&chip->wq);
	spin_lock_init(&chip->chip_lock);

	mtd->name = info->name;
	mtd->size = info->usable_size;
	mtd->erasesize = info->block_main_size;
	mtd->writesize = info->page_main_size;
	mtd->oobavail  = info->ecclayout->oobavail;
	mtd->oobsize = info->page_spare_size;
	mtd->owner = THIS_MODULE;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
		
	mtd->_erase = spinand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = spinand_read;
	mtd->_write = spinand_write;
	mtd->_read_oob = spinand_read_oob;
	mtd->_write_oob = spinand_write_oob;
	mtd->_sync = spinand_sync;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_suspend = spinand_suspend;
	mtd->_resume = spinand_resume;
	mtd->_block_isbad = spinand_block_isbad;
	mtd->_block_markbad = spinand_block_markbad;
	
	return add_mtd_device(mtd) == 1 ? -ENODEV : 0;
}

/**
 * spinand_mtd - add MTD device with parameters
 * @param mtd		MTD device structure
 *
 * only set parameters. not add_mtd_device.
 */
int spinand_mtd_extern(struct mtd_info *mtd)
{
	struct spinand_chip *chip = mtd->priv;
	struct spinand_info *info = chip->info;

	chip->state = SPINAND_FL_READY;
	init_waitqueue_head(&chip->wq);
	spin_lock_init(&chip->chip_lock);

	mtd->name = info->name;
	mtd->size = info->usable_size;
	mtd->erasesize = info->block_main_size;
	mtd->writesize = info->page_main_size;
	mtd->oobavail  = info->ecclayout->oobavail;
	mtd->oobsize = info->page_spare_size;
	mtd->owner = THIS_MODULE;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->writebufsize = mtd->writesize;
	mtd->subpage_sft = 0;
		
	mtd->_erase = spinand_erase;
	mtd->_point = NULL;
	mtd->_unpoint = NULL;
	mtd->_read = spinand_read;
	mtd->_write = spinand_write;
	mtd->_writev = NULL;
	mtd->_read_oob = spinand_read_oob;
	mtd->_write_oob = spinand_write_oob;
	mtd->_sync = spinand_sync;
	mtd->_lock = NULL;
	mtd->_unlock = NULL;
	mtd->_suspend = spinand_suspend;
	mtd->_resume = spinand_resume;
	mtd->_block_isbad = spinand_block_isbad;
	mtd->_block_markbad = spinand_block_markbad;

	mtd_info_flash = mtd;
	
	return spinand_create_proc();
}

void spinand_mtd_release(struct mtd_info *mtd)
{
	del_mtd_device(mtd);
}

EXPORT_SYMBOL_GPL(spinand_mtd);
EXPORT_SYMBOL_GPL(spinand_mtd_extern);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Henry Pan <hspan@micron.com>");
MODULE_DESCRIPTION("SPI NAND driver code");
