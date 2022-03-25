#include "mb_filter.h"
#include "decode_core.h"
#include "v_debug.h"

void mb_filter_init(struct mb_filter *thiz)
{
	thiz->enable           = 1;
	thiz->mode             = MB_FILTER_NONE;
	thiz->error_frame_cnt  = 0;
	thiz->filter_frame_cnt = 0;
}

void mb_filter_set_enable(struct mb_filter *thiz, unsigned int enable)
{
	thiz->enable = enable;
}

void mb_filter_set_mosaic_freeze_gate(struct mb_filter *thiz, unsigned int frame_num)
{
	if(frame_num)
		thiz->mosaic_freeze_gate = frame_num;
	else
		thiz->mosaic_freeze_gate = 0;
}

void mb_filter_set_mosaic_drop_gate(struct mb_filter *thiz, unsigned int err_blk_percent)
{
	if(err_blk_percent)
		thiz->mosaic_drop_gate = err_blk_percent;
	else
		thiz->mosaic_drop_gate = 0;

	VDBG_PRINT("\n[AV-MSC] : more then %u persent drop\n", err_blk_percent);
}

int mb_filter_run(struct mb_filter *thiz, unsigned int type, unsigned int width, unsigned int height, unsigned int err_blk_num)
{
	#define FRAME_IS_ERR (err_mbs > thiz->mosaic_drop_gate*width*height/25600)
	unsigned int err_mbs  = err_blk_num;
	unsigned int pic_type = type;
	static unsigned int err_frame_cnt = 0;

	if(!thiz->enable)
		goto PLAY;

	if(err_blk_num)
		thiz->error_frame_cnt++;

#if (STRICT_FILTER==1)
	if(FRAME_IS_ERR && thiz->mode==MB_FILTER_NONE) {
		err_frame_cnt = 0;
		thiz->mode = MB_FILTER_SI;
	}

	if(thiz->mode==MB_FILTER_SI) {
		if((!FRAME_IS_ERR) && pic_type==I_FRAME) {
			thiz->mode = MB_FILTER_SIB;
			goto PLAY;
		}
		else {
			err_frame_cnt++ ;
			if(err_frame_cnt >= thiz->mosaic_freeze_gate) {
				thiz->mode = MB_FILTER_NONE;
				goto PLAY;
			}
			else
				goto ABANDON;
		}
	}

	if(thiz->mode==MB_FILTER_SIB) {
		if(pic_type==B_FRAME)
			goto ABANDON;
		else {
			if(FRAME_IS_ERR) {
				err_frame_cnt++;
				if(err_frame_cnt >= thiz->mosaic_freeze_gate) {
					thiz->mode = MB_FILTER_NONE;
					goto PLAY;
				}
				goto ABANDON;
			}
			else {
				thiz->mode = MB_FILTER_NONE;
				goto PLAY;
			}
		}
	}

PLAY:
	err_frame_cnt = 0;
	return 0;
ABANDON:
	thiz->filter_frame_cnt++;
	return -1;
#else
	return ;
#endif
}

void mb_filter_get_process_info(struct mb_filter *thiz, unsigned long long *abandon_cnt, unsigned long long* err_cnt)
{
	if (thiz) {
		if (err_cnt)
			*err_cnt = thiz->error_frame_cnt;
		if (abandon_cnt)
			*abandon_cnt = thiz->filter_frame_cnt;
	}
}
