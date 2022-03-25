#include "frame_convert.h"
#include "video.h"
#include "vout_hal.h"
#include "gxav_vout_propertytypes.h"

struct frame_rate_convert fr_con;

unsigned int video_framerate_double(frame_rate rate)
{
	return rate<<1;
}

static int video_get_intfreq(int id)
{
	struct vout_dvemode dvemode;

	dvemode.id = id;
	gxav_videoout_get_dvemode(0, &dvemode);

	switch(dvemode.mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_576I:
	case GXAV_VOUT_576P:
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080P_50HZ:
		return FRAME_RATE_50;
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_480I:
	case GXAV_VOUT_480P:
	case GXAV_VOUT_720P_60HZ:
	case GXAV_VOUT_1080I_60HZ:
	case GXAV_VOUT_1080P_60HZ:
		return FRAME_RATE_5994;
	default:
		return 0;
	}
}

void video_frame_convert_init(struct frame_rate_convert *fc)
{
	gx_memset(fc, 0, sizeof(struct frame_rate_convert));
	fc->mode = FRMODE_NORMAL;
}

void video_frame_convert_config(struct frame_rate_convert *fc, unsigned pp_enable, unsigned src_fps)
{
	frame_rate src_framerate;
	frame_rate dst_framerate;

	if (src_fps == 0)
		return;

	src_framerate = src_fps;
	if(pp_enable)
		src_framerate = video_framerate_double(src_framerate);
	fc->src_framerate = src_framerate;
	fc->pp_enable = pp_enable;

	//get dst framerate
	dst_framerate = video_get_intfreq(ID_VPU);
	if(fc->pp_enable || fc->src_framerate>=FRAME_RATE_50)
		fc->dst_framerate = dst_framerate;
	else
		fc->dst_framerate = dst_framerate/2;
}

void video_frame_convert_set_mode(struct frame_rate_convert *fr_con, FrMode mode)
{
	if(fr_con) {
		fr_con->mode = mode;
	}
}

FR_State video_frame_convert_run(struct frame_rate_convert *fc, unsigned int cnt)
{
	unsigned int frametodeal, framebase;

	if(fc->mode == FRMODE_NORMAL) {
		if(fc->src_framerate && (fc->src_framerate > fc->dst_framerate)) {
			framebase   = fc->src_framerate;
			frametodeal = fc->src_framerate - fc->dst_framerate;
			if(cnt*frametodeal/framebase != (cnt-1)*frametodeal/framebase) {
				return FR_SKIP;
			}
		} else if(fc->src_framerate && (fc->src_framerate < fc->dst_framerate)) {
			framebase   = fc->dst_framerate;
			frametodeal = fc->dst_framerate - fc->src_framerate;
			if(cnt*frametodeal/framebase != (cnt-1)*frametodeal/framebase) {
				return FR_REPEAT;
			}
		}
	}

	return FR_COMMON;
}
