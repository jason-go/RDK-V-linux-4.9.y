#include <linux/mtd/mtd.h>

#include "minifs_def.h"
#include "minifs_diskif.h"
#include "compr.h"

#define PERCENTAGE(x,total) (((x) * 100) / (total))
#define INT32_MAX 0x7fffffff

struct mtd_info *g_mtd_info;

static int minifsMtd_ReadPage(minifs_device *dev, int chunk, uint8_t *data, minifs_PageTags *tags)
{
	loff_t offset = (chunk - 1) * MINIFS_HW_PAGE_SIZE;
	uint32_t len, ret_len;

	if (data) {
		len = MINIFS_BYTES_PER_CHUNK;
		mtd_read(g_mtd_info, offset, len, &ret_len, data);
		if (ret_len != len) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			return MINIFS_FAIL;
		}
	}

	if (tags) {
		len = sizeof(minifs_PageTags);
		offset += MINIFS_BYTES_PER_CHUNK;
		mtd_read(g_mtd_info, offset, len, &ret_len, (uint8_t *)tags);
		if(ret_len != len) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			return MINIFS_FAIL;
		}
	}

	return MINIFS_OK;
}

static int minifsMtd_WritePage(minifs_device *dev, int chunk, const uint8_t *data, minifs_PageTags *tags)
{
	loff_t offset = (chunk - 1) * MINIFS_HW_PAGE_SIZE;
	uint32_t len, ret_len;

	if (data) {
		len = MINIFS_BYTES_PER_CHUNK;
		mtd_write(g_mtd_info, offset, len, &ret_len, data);
		if (ret_len != len) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			return MINIFS_FAIL;
		}
	}

	if (tags) {
		len = sizeof(minifs_PageTags);
		offset += MINIFS_BYTES_PER_CHUNK;
		mtd_write(g_mtd_info, offset, len, &ret_len, (uint8_t *)tags);
		if (ret_len != len ) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			return MINIFS_FAIL;
		}
	}

	return MINIFS_OK;
}

static int minifsMtd_EraseBlock(minifs_device *dev, int blockNumber)
{
	int start_page = device_BlockStartPage(dev, blockNumber);
	struct erase_info ei;
	size_t blocks, i;
	uint64_t tmp;

	ei.mtd = g_mtd_info;
	ei.addr = (start_page - 1) * MINIFS_HW_PAGE_SIZE;

	ei.len = (device_BlockPageCount(dev, blockNumber)) * MINIFS_HW_PAGE_SIZE;
	ei.len = g_mtd_info->erasesize;
	tmp = ei.len;
	do_div(tmp, g_mtd_info->erasesize); // 直接除,64位被除数缺少__aeabi_uldivmod编译不过去
	blocks = tmp;
	ei.callback = NULL;

	for (i=0; i<blocks; i++) {
		if (mtd_erase(ei.mtd, &ei)) {
			T("minifs error: %s %d\n", __func__, __LINE__);
			return MINIFS_FAIL;
		}
		ei.addr += g_mtd_info->erasesize;
	}

	return MINIFS_OK;
}

int minifsMtd_Init(minifs_device *dev)
{
	int i, pageCount;
	uint64_t tmp;

	dev->writePage  = minifsMtd_WritePage;
	dev->readPage   = minifsMtd_ReadPage;
	dev->eraseBlock = minifsMtd_EraseBlock;

	tmp = g_mtd_info->size;
	do_div(tmp, g_mtd_info->erasesize);
	dev->block_count = tmp;
	dev->ChunksPerBlock = g_mtd_info->erasesize / MINIFS_HW_PAGE_SIZE;

	dev->blocks = (minifs_Block*)YMALLOC((dev->block_count + 1) * sizeof(minifs_Block));
	memset(dev->blocks, 0, (dev->block_count + 1) * sizeof(minifs_Block));

	pageCount = 1;
	dev->PagesMaxBlock = 0;
	dev->PagesMinBlock = INT32_MAX;
	for (i=1; i<= dev->block_count; i++) {
		dev->blocks[i].page_count = g_mtd_info->erasesize / MINIFS_HW_PAGE_SIZE;
		dev->blocks[i].start_page = pageCount;

		if (dev->blocks[i].page_count > dev->PagesMaxBlock)
			dev->PagesMaxBlock = dev->blocks[i].page_count;
		if (dev->blocks[i].page_count < dev->PagesMinBlock)
			dev->PagesMinBlock = dev->blocks[i].page_count;
		pageCount += dev->blocks[i].page_count;
	}

	dev->page_count = pageCount - 1;

	if (dev->EndBlock > dev->block_count)
		dev->EndBlock = dev->block_count;

	return MINIFS_OK;
}

