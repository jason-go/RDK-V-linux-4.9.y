#ifndef _MB_FILTER_H_
#define _MB_FILTER_H_

#define MB_FILTER_ENABLE     (1)           ///< 是否开启误码帧丢弃功能
#define MB_FILTER_DEBUG      (0)

#if (MB_FILTER_ENABLE==1)
	#define STRICT_FILTER	(1)
	#if (STRICT_FILTER==1)
		#define MB_FILTER_LEVEL    (0)
		#define MB_FILTER_OUT_CNT  (10*30)///<300 frames
	#else
		#define MB_FILTER_LEVEL     (0xFFFFFFFF)
		#define MB_FILTER_OUT_CNT   (0xFFFFFFFF)
	#endif
#endif

struct mb_filter {
	unsigned int enable;
	enum {
		MB_FILTER_NONE = (0<<0),
		MB_FILTER_SB   = (1<<0),
		MB_FILTER_SIB  = (1<<1),
		MB_FILTER_SI   = (1<<2),
		MB_FILTER_SP   = (1<<3),
	}mode;
	unsigned int       mosaic_drop_gate;
	unsigned int       mosaic_freeze_gate;
	unsigned long long error_frame_cnt;
	unsigned long long filter_frame_cnt;
};

void mb_filter_init                  (struct mb_filter *thiz);
int  mb_filter_run                   (struct mb_filter *thiz, unsigned int type, unsigned int width, unsigned int height, unsigned int err);
void mb_filter_set_enable            (struct mb_filter *thiz, unsigned int enable);
void mb_filter_set_mosaic_freeze_gate(struct mb_filter *thiz, unsigned int frame_num);
void mb_filter_set_mosaic_drop_gate  (struct mb_filter *thiz, unsigned int err_blk_percent);
void mb_filter_get_process_info      (struct mb_filter *thiz, unsigned long long *abandon_cnt, unsigned long long *err_cnt);
#endif

