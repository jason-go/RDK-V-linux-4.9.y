#ifndef __VFRAME_H__
#define __VFRAME_H__

#include "decode_core.h"

struct field_addr {
	int fb_id;
	int is_ref;
	int ready;
	struct{
		unsigned int addry;
		unsigned int addrcb;
		unsigned int addry_2bit;
		unsigned int addrcb_2bit;
	}virt, phys;
};

typedef struct frame_info {
	enum {
		VFRAME_NULL = 0,
		VFRAME_DECODE,
		VFRAME_DEINTERLACE_Y,
		VFRAME_DEINTERLACE_YUV,
		VFRAME_ZOOM,
		VFRAME_ABANDON,
	}type;

	frame_rate        fps;
	scan_type         interlace;
	StoreMode         store_mode;
	unsigned int      stride;
	unsigned int      base_height;
	unsigned int      width;
	unsigned int      height;
	unsigned int      bpp;
	unsigned int      clip_width, clip_height;
	struct field_addr top, bottom;
	unsigned int      pts;
	unsigned int      top_first;
	unsigned int      num_of_err_mb;
	unsigned int      ref_cnt;
	VPicType          pic_type;
	int               repeat_first_field;
	GxAvRect          clip, view;
	int               disp_num;

	struct{
		unsigned int enable;
		unsigned int data;
		unsigned int display;
	}userdata;

	int colorimetry;
	unsigned int stream_id;
}VideoFrameInfo;

#endif
