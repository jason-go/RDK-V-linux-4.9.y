#ifndef _FRAME_CONVERT_H
#define _FRAME_CONVERT_H

#include "decode_core.h"

typedef enum {
	FR_SKIP   = 0,
	FR_COMMON = 1,
	FR_REPEAT = 2,
}FR_State;

typedef enum {
	FRMODE_NORMAL,
	FRMODE_BYPASS,
}FrMode;

struct frame_rate_convert
{
	FrMode       mode;
	unsigned int frame_cnt;
	unsigned int pp_enable;
	frame_rate   src_framerate;
	frame_rate   dst_framerate;
};

extern struct frame_rate_convert fr_con;

unsigned int video_framerate_double(frame_rate rate);

void     video_frame_convert_init    (struct frame_rate_convert *fr_con);
void     video_frame_convert_config  (struct frame_rate_convert *fr_con, unsigned pp_enable, unsigned src_fps);
void     video_frame_convert_set_mode(struct frame_rate_convert *fr_con, FrMode mode);
FR_State video_frame_convert_run     (struct frame_rate_convert *fr_con, unsigned int frame_cnt);

#endif

