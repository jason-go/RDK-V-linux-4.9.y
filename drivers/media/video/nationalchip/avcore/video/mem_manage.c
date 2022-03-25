#include "mem_manage.h"
#include "kernelcalls.h"
#include "firewall.h"

typedef struct {
	unsigned int fb_num;
	BufInfo      fb[MAX_FB_NUM];
}DecoderFramebufInfo;

typedef struct {
	DecoderColbufInfo   colbuf;
	DecoderWorkbufInfo  workbuf;
	DecoderFramebufInfo framebuf;
}DecoderMemInfo;

static DecoderMemInfo decmem;

int mm_init(void)
{
	gx_memset(&decmem, 0, sizeof(DecoderMemInfo));
#if FB_BOUND_CHECK
	mm_check_init(MM_ALL);
#endif
	return 0;
}

int mm_colbuf_info_set(DecoderColbufInfo *colbuf_info)
{
	if(colbuf_info) {
		decmem.colbuf = *colbuf_info;
#if FB_BOUND_CHECK
		mm_check_init(MM_COL_BUF);
		mm_check_regist(colbuf_info->addr, colbuf_info->size, "col");
#endif
	} else {
		gx_printf("\n%s:%d: config colbuf error!\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

int mm_colbuf_info_get(DecoderColbufInfo *colbuf_info)
{
	colbuf_info->size = decmem.colbuf.size;
	colbuf_info->addr = gx_virt_to_phys(decmem.colbuf.addr);
	return 0;
}

int mm_workbuf_info_set(DecoderWorkbufInfo *workbuf_info)
{
	unsigned phy_addr;

	if (workbuf_info->size > 0) {
		decmem.workbuf = *workbuf_info;
		phy_addr = gx_virt_to_phys(decmem.workbuf.addr);
		gxav_firewall_register_buffer(GXAV_BUFFER_VIDEO_FIRMWARE, phy_addr, CODE_BUF_SIZE);
#if FB_BOUND_CHECK
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_VIDEO_FIRMWARE) == 0) {
			mm_check_init(MM_WORK_BUF);
			mm_check_regist(workbuf_info->addr, workbuf_info->size, "work");
		}
#endif
	}

	return 0;
}

int mm_workbuf_info_get(DecoderWorkbufInfo *workbuf_info)
{
	workbuf_info->size = decmem.workbuf.size;
	workbuf_info->addr = gx_virt_to_phys(decmem.workbuf.addr);
	return 0;
}

int mm_framebuf_info_set(DecoderFramebufConfig *framebuf_config)
{
	unsigned int i, phy_addr;
	unsigned int fb_size = 0;
	unsigned int min_phy_addr = 0, max_phy_addr = 0;

	gx_memset(&decmem.framebuf, 0, sizeof(decmem.framebuf));
	decmem.framebuf.fb_num = framebuf_config->fb_num;

#if FB_BOUND_CHECK
	mm_check_init(MM_FB);
#endif
	for(i = 0; i < framebuf_config->fb_num; i++) {
		if(framebuf_config->fb[i].addr && framebuf_config->fb[i].size) {
			decmem.framebuf.fb[i].id   = i;
			decmem.framebuf.fb[i].used = 0;
			decmem.framebuf.fb[i].size = framebuf_config->fb[i].size;
			decmem.framebuf.fb[i].addr = framebuf_config->fb[i].addr;
			phy_addr = gx_virt_to_phys(decmem.framebuf.fb[i].addr);
			if (min_phy_addr == 0 || phy_addr < min_phy_addr)
				min_phy_addr = phy_addr;
			if (max_phy_addr == 0 || phy_addr >= max_phy_addr)
				max_phy_addr = phy_addr + framebuf_config->fb[i].size;

#if FB_BOUND_CHECK
		if (gxav_firewall_buffer_protected(GXAV_BUFFER_VIDEO_FRAME) == 0) {
			mm_check_regist(framebuf_config->fb[i].addr, framebuf_config->fb[i].size, "fb");
		}
#endif
		} else {
			gx_printf("\n%s:%d: config fb error!\n", __func__, __LINE__);
			return -1;
		}
	}
	fb_size = (max_phy_addr - min_phy_addr) & 0xfffffc00;
	gxav_firewall_register_buffer(GXAV_BUFFER_VIDEO_FRAME, min_phy_addr, max_phy_addr-min_phy_addr);

	return 0;
}

int mm_framebuf_alloc(FbType type, unsigned int size, unsigned int num, BufInfo *ret_fb)
{
	unsigned int i, cnt;

	cnt = 0;
	if(type == FB_FREEZE) {
		if(decmem.framebuf.fb[FREEZE_FB_ID].size >= size) {
			ret_fb[cnt].id   = FREEZE_FB_ID;
			ret_fb[cnt].size = decmem.framebuf.fb[FREEZE_FB_ID].size;
			ret_fb[cnt].addr = gx_virt_to_phys(decmem.framebuf.fb[FREEZE_FB_ID].addr);
		} else {
			gx_printf("\n%s:%d: alloc freeze fb!\n", __func__, __LINE__);
			return -1;
		}
	}

	if(type == FB_NORMAL) {
		for(i = 0; i < decmem.framebuf.fb_num && cnt < num; i++) {
			if(!decmem.framebuf.fb[i].used && decmem.framebuf.fb[i].size >= size) {
				decmem.framebuf.fb[i].used = 1;
				ret_fb[cnt].id   = decmem.framebuf.fb[i].id;
				ret_fb[cnt].size = decmem.framebuf.fb[i].size;
				ret_fb[cnt].addr = gx_virt_to_phys(decmem.framebuf.fb[i].addr);
				cnt ++;
			}
		}
		if(cnt != num) {
			gx_printf("\n%s:%d: alloc fb error!\n", __func__, __LINE__);
			return -1;
		}
	}

	return 0;
}

int mm_framebuf_get_freenum(int size, int *freenum)
{
	unsigned int i;

	*freenum = 0;
	for(i = 0; i < decmem.framebuf.fb_num; i++) {
		if(!decmem.framebuf.fb[i].used && decmem.framebuf.fb[i].size >= size) {
			*freenum += 1;
		}
	}

	return 0;
}


#if FB_BOUND_CHECK
#define MEM_CHECK_WORD (0x12345678)
static struct mem_check_node {
	unsigned used;
	unsigned start_addr;
	unsigned check_addr;
	char     desc[8];
} mem_check[34];

int mm_check_init(BufSel sel)
{
	int i, len = sizeof(mem_check)/sizeof(struct mem_check_node);

	if (sel == MM_ALL) {
		memset(mem_check, 0, sizeof(mem_check));
	} else {
		for(i = 0; i < len; i++) {
			if(mem_check[i].used) {
				if ((sel & MM_COL_BUF) && (memcmp(mem_check[i].desc, "col", 3) == 0)) {
					memset(&mem_check[i], 0, sizeof(mem_check[i]));
				}
				if ((sel & MM_WORK_BUF) && (memcmp(mem_check[i].desc, "work", 4) == 0)) {
					memset(&mem_check[i], 0, sizeof(mem_check[i]));
				}
				if ((sel & MM_FB) && (memcmp(mem_check[i].desc, "fb", 2) == 0)) {
					memset(&mem_check[i], 0, sizeof(mem_check[i]));
				}
			}
		}
	}

	return 0;
}

int mm_check_regist(unsigned start_addr, unsigned size, char *desc)
{
	int i, len = sizeof(mem_check)/sizeof(struct mem_check_node);

	for(i = 0; i < len; i++) {
		if(!mem_check[i].used && start_addr && size > MEM_CHECK_SIZE) {
			int j = 0;
			unsigned *check_buf = NULL;

			mem_check[i].used = 1;
			mem_check[i].start_addr = start_addr;
			mem_check[i].check_addr = start_addr + size - MEM_CHECK_SIZE;
			check_buf = (unsigned*)mem_check[i].check_addr;
			for (j = 0; j < MEM_CHECK_SIZE/sizeof(unsigned); j++)
				check_buf[j] = MEM_CHECK_WORD;

			if(desc) {
				strncpy(mem_check[i].desc, desc, sizeof(mem_check[i].desc)-1);
				mem_check[i].desc[sizeof(mem_check[i].desc)-1] = '\0';
			} else {
				mem_check[i].desc[0] = '\0';
			}
			return 0;
		}
	}

	return -1;
}

FrameMemStatus mm_check_by_startaddr(unsigned start_addr, const char *func, int line)
{
	int i, len = sizeof(mem_check)/sizeof(struct mem_check_node);

	for(i = 0; i < len; i++) {
		if(mem_check[i].used && mem_check[i].start_addr == start_addr) {
			if(*(unsigned*)mem_check[i].check_addr != MEM_CHECK_WORD) {
				int j = 0;
				unsigned *check_buf = (unsigned*)mem_check[i].check_addr;

				gx_printf("%s %d (%s: 0x%p)\n", func, line, mem_check[i].desc, check_buf);
				for (j = 0; j < MEM_CHECK_SIZE/sizeof(unsigned); j++)
					gx_printf("0x%x ", check_buf[j]);
				gx_printf("\n");
				return FB_MEM_OVERFLOW;
			}
			break;
		}
	}

	return FB_MEM_NORMAL;
}

FrameMemStatus mm_check_all(const char *func, int line)
{
	int i, len = sizeof(mem_check)/sizeof(struct mem_check_node);

	for(i = 0; i < len; i++) {
		if(mem_check[i].used && mem_check[i].check_addr) {
			if(*(unsigned*)mem_check[i].check_addr != MEM_CHECK_WORD) {
				int j = 0;
				unsigned *check_buf = (unsigned*)mem_check[i].check_addr;

				gx_printf("%s %d (%s: 0x%p)\n", func, line, mem_check[i].desc, check_buf);
				for (j = 0; j < MEM_CHECK_SIZE/sizeof(unsigned); j++)
					gx_printf("0x%x ", check_buf[j]);
				gx_printf("\n");

				return FB_MEM_OVERFLOW;
			}
		}
	}

	return FB_MEM_NORMAL;
}
#endif
