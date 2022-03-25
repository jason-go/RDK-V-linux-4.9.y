#ifndef _MEM_MANAGE_H_
#define _MEM_MANAGE_H_

#include "decode_core.h"
#include "gxav_video_propertytypes.h"

typedef DecoderColbufConfig     DecoderColbufInfo;
typedef DecoderWorkbufConfig    DecoderWorkbufInfo;

typedef struct {
	unsigned int    id;
	unsigned int    addr;
	unsigned int    size;
	unsigned int    used;
} BufInfo;

int mm_init(void);

int mm_colbuf_info_set(DecoderColbufInfo *colbuf_info);
int mm_colbuf_info_get(DecoderColbufInfo *colbuf_info);

int mm_workbuf_info_set(DecoderWorkbufInfo *workbuf_info);
int mm_workbuf_info_get(DecoderWorkbufInfo *workbuf_info);

int mm_framebuf_info_set(DecoderFramebufConfig *framebuf_config);
int mm_framebuf_alloc(FbType type, unsigned int size, unsigned int num, BufInfo *ret_fb);
int mm_framebuf_get_freenum(int size, int *freenum);


#define FB_BOUND_CHECK (0)
#if FB_BOUND_CHECK
	#define MEM_CHECK_SIZE (16)
	typedef enum {
		MM_FB       = (1<<0),
		MM_COL_BUF  = (1<<1),
		MM_WORK_BUF = (1<<2),
		MM_ALL      = (MM_FB|MM_COL_BUF|MM_WORK_BUF),
	} BufSel;

	typedef enum {
		FB_MEM_NORMAL = 0,
		FB_MEM_OVERFLOW,
	}FrameMemStatus;

	int mm_check_init(BufSel sel);
	int mm_check_regist(unsigned start_addr, unsigned size, char *desc);
	FrameMemStatus mm_check_by_startaddr(unsigned start_addr, const char *func, int line);
	FrameMemStatus mm_check_all(const char *func, int line);
#endif

#endif
