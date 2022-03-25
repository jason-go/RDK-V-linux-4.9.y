#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gx3211_vpu.h"
#include "vpu_color.h"
#include "vout_hal.h"
#include "porting.h"
#include "video.h"
#include "gx3211_vpu_internel.h"

//正常情况下的缩放系数表
static unsigned int sPhaseNomal[] = {
	0x10909010,0x00ff0100,0x01ff0200,0x02ff0300,
	0x03ff0400,0x04ff0500,0x05fe0700,0x06fe0800,
	0x07fd0a00,0x07fc0b00,0x08fc0c00,0x09fb0e00,
	0x09fa1001,0x0af91201,0x0bf81401,0x0bf71501,
	0x0cf61701,0x0cf51801,0x0df41b02,0x0df31c02,
	0x0ef11f02,0x0ef02002,0x0fef2303,0x0fed2503,
	0x10ec2703,0x10ea2903,0x10e92b04,0x10e72d04,
	0x11e53004,0x11e33305,0x11e13505,0x11df3705,
	0x12de3a06,0x12dc3c06,0x12da3e06,0x12d74106,
	0x12d54407,0x12d34607,0x12d14807,0x12cf4b08,
	0x12cd4d08,0x12ca5008,0x12c85309,0x12c65509,
	0x12c35809,0x12c15b0a,0x12bf5d0a,0x12bc600a,
	0x12ba630b,0x12b7660b,0x12b5680b,0x12b26b0b,
	0x12b06e0c,0x12ad710c,0x12aa750d,0x11a8760d,
	0x11a5790d,0x11a27d0e,0x11a07f0e,0x119d820e,
	0x109a840e,0x1098870f,0x10958a0f,0x10928d0f,
};

#define VPU_CLOSED() (gx3211vpu_info == NULL)

static int Gx3211Vpu_LetterBox(GxLayerID layer, int aspect, GxAvRect *view_rect)
{
	int full_screen;
	GxVpuProperty_Resolution actual_resolution;

	gx3211_vpu_GetActualResolution(&actual_resolution);
	if(actual_resolution.xres==view_rect->width && actual_resolution.yres==view_rect->height)
		full_screen = 1;
	else
		full_screen = 0;

	if(full_screen) {
		if(gx3211vpu_info->layer[layer].screen == SCREEN_16X9) {
			if(aspect == ASPECTRATIO_4BY3) {
				int fix_width = view_rect->width*3/4;
				view_rect->x       = view_rect->x + ((view_rect->width - fix_width) >> 1);
				view_rect->width   = fix_width;
			}
		}
		else if(gx3211vpu_info->layer[layer].screen == SCREEN_4X3) {
			if(aspect == ASPECTRATIO_16BY9) {
				int fix_height = (view_rect->height*3)/4;
				view_rect->y = view_rect->y + ((view_rect->height - fix_height) >> 1);
				view_rect->height  = fix_height;
			}
		}
	}
	else {
		int ah = 0, aw = 0;
		if(gx3211vpu_info->layer[layer].screen == SCREEN_16X9) {
			if(aspect == ASPECTRATIO_4BY3) {
				aw = 4 * actual_resolution.xres /16;
				ah = 3 * actual_resolution.yres /9;
			}
		}
		else if(gx3211vpu_info->layer[layer].screen == SCREEN_4X3) {
			if(aspect == ASPECTRATIO_16BY9) {
				aw = 16 * actual_resolution.xres /4;
				ah = 9 * actual_resolution.yres /3;
			}
		}

		if(ah && aw){
			if(view_rect->width * ah <= view_rect->height * aw) {
				int fix_height = view_rect->width *ah / aw;
				view_rect->y = view_rect->y + ((view_rect->height - fix_height) >> 1);
				view_rect->height  = fix_height;
			}
			else {
				int fix_width = view_rect->height * aw / ah;
				view_rect->x = view_rect->x + ((view_rect->width - fix_width) >> 1);
				view_rect->width = fix_width;
			}
		}
	}

	return 0;
}

static int Gx3211Vpu_PanScan(GxLayerID layer, int aspect, GxAvRect *clip_rect)
{
	if(gx3211vpu_info->layer[layer].screen == SCREEN_16X9) {
		if(aspect == ASPECTRATIO_4BY3) {
			int i = (clip_rect->height*3)>>2;
			clip_rect->y      += ((clip_rect->height - i) >> 1);
			clip_rect->height  = i;
		}
	}
	else if(gx3211vpu_info->layer[layer].screen == SCREEN_4X3) {
		if(aspect == ASPECTRATIO_16BY9) {
			clip_rect->x = (clip_rect->width >> 3);
			clip_rect->width -= (clip_rect->x << 1);
		}
	}

	return 0;
}

static int Gx3211Vpu_RawSize(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(clip_rect->width > view_rect->width || clip_rect->height > view_rect->height) {
		GxAvRect raw_view = {0};

		if(clip_rect->width <= view_rect->width && clip_rect->height > view_rect->height) {
			unsigned int h_times = (clip_rect->height * 1000) / (view_rect->height);
			raw_view.height = view_rect->height;
			raw_view.width  = (clip_rect->width * 1000)/(h_times);
		}
		else if(clip_rect->width > view_rect->width && clip_rect->height <= view_rect->height) {
			unsigned int w_times = (clip_rect->width * 1000) / (view_rect->width);
			raw_view.width  = view_rect->width;
			raw_view.height = (clip_rect->height * 1000)/(w_times);
		}
		else {
			unsigned int w_times = 0,h_times = 0;
			w_times = (clip_rect->width * 1000) / (view_rect->width);
			h_times = (clip_rect->height * 1000) / (view_rect->height);
			if(w_times >= h_times) {
				raw_view.width = view_rect->width;
				raw_view.height = (clip_rect->height * 1000)/w_times;
			}
			else {
				raw_view.height = view_rect->height;
				raw_view.width = (clip_rect->width * 1000)/h_times;
			}
		}

		VPU_DBG("raw_view : w : %d,h : %d \n",raw_view.width,raw_view.height);
		view_rect->x = view_rect->x + ((view_rect->width - raw_view.width) >> 1);
		view_rect->y = view_rect->y + ((view_rect->height - raw_view.height) >> 1);
		view_rect->width  = raw_view.width;
		view_rect->height = raw_view.height;
	}
	else {
		view_rect->x      = view_rect->x + ((view_rect->width - clip_rect->width) >> 1);
		view_rect->y      = view_rect->y + ((view_rect->height - clip_rect->height) >> 1);
		view_rect->width  = clip_rect->width;
		view_rect->height = clip_rect->height;
	}

	return 0;
}

static int Gx3211Vpu_RawRatio(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	GxAvRect raw_view = {0};

	if(clip_rect->width <= view_rect->width && clip_rect->height <= view_rect->height) {
		unsigned int times = 0,w_times = 0,h_times = 0;
		w_times = (view_rect->width * 1000) / (clip_rect->width);
		h_times = (view_rect->height * 1000) / (clip_rect->height);

		times = GX_MIN(w_times,h_times);
		raw_view.width  = (clip_rect->width * times) / 1000;
		raw_view.height = (clip_rect->height * times) / 1000;
	}
	else if(clip_rect->width <= view_rect->width && clip_rect->height > view_rect->height) {
		unsigned int h_times = (clip_rect->height * 1000) / (view_rect->height);
		raw_view.height = view_rect->height;
		raw_view.width  = (clip_rect->width * 1000) / h_times;
	}
	else if(clip_rect->width > view_rect->width && clip_rect->height <= view_rect->height) {
		unsigned int w_times = (clip_rect->width * 1000) / (view_rect->width);
		raw_view.width  = view_rect->width;
		raw_view.height = (clip_rect->height * 1000) / w_times;
	}
	else {
		unsigned int w_times = 0,h_times = 0;
		w_times = (clip_rect->width * 1000) / (view_rect->width);
		h_times = (clip_rect->height * 1000) / (view_rect->height);
		if(w_times >= h_times) {
			raw_view.width  = view_rect->width;
			raw_view.height = (clip_rect->height * 1000) / w_times;
		}
		else {
			raw_view.height = view_rect->height;
			raw_view.width  = (clip_rect->width * 1000) / h_times;
		}
	}

	VPU_DBG("raw_view : w : %d,h : %d \n",raw_view.width,raw_view.height);
	view_rect->x      = view_rect->x + ((view_rect->width - raw_view.width) >> 1);
	view_rect->y      = view_rect->y + ((view_rect->height - raw_view.height) >> 1);
	view_rect->width  = raw_view.width;
	view_rect->height = raw_view.height;

	return 0;
}

static int Gx3211Vpu_4X3PullDown(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_4BY3) {
		Gx3211Vpu_RawRatio(clip_rect,view_rect);
	}
	else {
		GxAvRect raw_view = {0};
		if(clip_rect->width <= view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.height = (clip_rect->width * 3)>>2;
			raw_view.width  = clip_rect->width;
			if(raw_view.height < clip_rect->height) {
				raw_view.height = clip_rect->height;
				raw_view.width  = (raw_view.height<<2) / 3;
			}
			if(raw_view.height > view_rect->height) {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<2) / 3;
			}
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
			}
		}
		else if(clip_rect->width <= view_rect->width && clip_rect->height > view_rect->height) {
			raw_view.height = view_rect->height;
			raw_view.width  = (raw_view.height<<2) / 3;
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
			}
		}
		else if(clip_rect->width > view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.width  = view_rect->width;
			raw_view.height = (raw_view.width * 3)>>2;
			if(raw_view.height > view_rect->height) {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<2) / 3;
			}
		}
		else {
			unsigned int w_times = 0,h_times = 0;
			w_times = (clip_rect->width * 1000) / (view_rect->width);
			h_times = (clip_rect->height * 1000) / (view_rect->height);
			if(w_times >= h_times) {
				raw_view.width  = view_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
				if(raw_view.height > view_rect->height) {
					raw_view.height = view_rect->height;
					raw_view.width  = (raw_view.height<<2) / 3;
				}
			}
			else {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<2) / 3;
				if(raw_view.width > view_rect->width) {
					raw_view.width = view_rect->width;
					raw_view.height = (raw_view.width * 3)>>2;
				}
			}
		}
		//全屏，原始比例处理
		Gx3211Vpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int Gx3211Vpu_4X3CutOut(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_4BY3) {
		Gx3211Vpu_RawRatio(clip_rect,view_rect);
	}
	else {
		GxAvRect raw_view = {0};
		if(clip_rect->width <= view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.height  = clip_rect->height;
			raw_view.width  = (raw_view.height<<2) / 3;
			if(raw_view.width > clip_rect->width) {
				raw_view.width = clip_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
			}
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
			}
		}
		else if(clip_rect->width <= view_rect->width && clip_rect->height > view_rect->height) {
			raw_view.height = view_rect->height;
			raw_view.width  = (raw_view.height<<2) / 3;
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
			}
		}
		else if(clip_rect->width > view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.width  = view_rect->width;
			raw_view.height = (raw_view.width * 3)>>2;
			if(raw_view.height > view_rect->height) {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<2) / 3;
			}
		}
		else {
			unsigned int w_times = 0,h_times = 0;
			w_times = (clip_rect->width * 1000) / (view_rect->width);
			h_times = (clip_rect->height * 1000) / (view_rect->height);
			if(w_times >= h_times) {
				raw_view.width  = view_rect->width;
				raw_view.height = (raw_view.width * 3)>>2;
				if(raw_view.height > view_rect->height) {
					raw_view.height = view_rect->height;
					raw_view.width  = (raw_view.height<<2) / 3;
				}
			}
			else {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<2) / 3;
				if(raw_view.width > view_rect->width) {
					raw_view.width = view_rect->width;
					raw_view.height = (raw_view.width * 3)>>2;
				}
			}
		}
		//全屏，原始比例处理
		Gx3211Vpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int Gx3211Vpu_16X9PullDown(GxLayerID layer, int aspect,GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_16BY9) {
		Gx3211Vpu_RawRatio(clip_rect,view_rect);
	}
	else {
		GxAvRect raw_view = {0};
		if(clip_rect->width <= view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.height  = clip_rect->height;
			raw_view.width  = (raw_view.height<<4) / 9;
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 9)>>4;
			}
		}
		else if(clip_rect->width <= view_rect->width && clip_rect->height > view_rect->height) {
			raw_view.height = view_rect->height;
			raw_view.width  = (raw_view.height<<4) / 9;
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 9)>>4;
			}
		}
		else if(clip_rect->width > view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.width  = view_rect->width;
			raw_view.height = (raw_view.width * 9)>>4;
			if(raw_view.height > view_rect->height) {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<4) / 9;
			}
		}
		else {
			unsigned int w_times = 0, h_times = 0;
			w_times = (clip_rect->width * 1000) / (view_rect->width);
			h_times = (clip_rect->height * 1000) / (view_rect->height);
			if(w_times >= h_times) {
				raw_view.width  = view_rect->width;
				raw_view.height = (raw_view.width * 9)>>4;
				if(raw_view.height > view_rect->height) {
					raw_view.height = view_rect->height;
					raw_view.width  = (raw_view.height<<4) / 9;
				}
			}
			else {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<4) / 9;
				if(raw_view.width > view_rect->width) {
					raw_view.width = view_rect->width;
					raw_view.height = (raw_view.width * 9)>>4;
				}
			}
		}
		//全屏，原始比例处理
		Gx3211Vpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int Gx3211Vpu_16X9CutOut(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_16BY9) {
		Gx3211Vpu_RawRatio(clip_rect,view_rect);
	}
	else {
		GxAvRect raw_view = {0};
		if(clip_rect->width <= view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.width  = clip_rect->width;
			raw_view.height = (raw_view.width * 9)>>4;
			if(raw_view.height > view_rect->height) {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<4) / 9;
			}
		}
		else if(clip_rect->width <= view_rect->width && clip_rect->height > view_rect->height) {
			raw_view.height = view_rect->height;
			raw_view.width  = (raw_view.height<<4) / 9;
			if(raw_view.width > view_rect->width) {
				raw_view.width = view_rect->width;
				raw_view.height = (raw_view.width * 9)>>4;
			}
		}
		else if(clip_rect->width > view_rect->width && clip_rect->height <= view_rect->height) {
			raw_view.width  = view_rect->width;
			raw_view.height = (raw_view.width * 9)>>4;
			if(raw_view.height > view_rect->height) {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<4) / 9;
			}
		}
		else {
			unsigned int w_times = 0,h_times = 0;
			w_times = (clip_rect->width * 1000) / (view_rect->width);
			h_times = (clip_rect->height * 1000) / (view_rect->height);
			if(w_times >= h_times) {
				raw_view.width  = view_rect->width;
				raw_view.height = (raw_view.width * 9)>>4;
				if(raw_view.height > view_rect->height) {
					raw_view.height = view_rect->height;
					raw_view.width  = (raw_view.height<<4) / 9;
				}
			}
			else {
				raw_view.height = view_rect->height;
				raw_view.width  = (raw_view.height<<4) / 9;
				if(raw_view.width > view_rect->width) {
					raw_view.width = view_rect->width;
					raw_view.height = (raw_view.width * 9)>>4;
				}
			}
		}
		//全屏，原始比例处理
		Gx3211Vpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int Gx3211Vpu_4X3(GxLayerID layer, int aspect,GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int x, y, width, height;

	x = view_rect->x;
	y = view_rect->y;
	width = view_rect->width;
	height= view_rect->height;

	/* 宽高比大于等于4:3才能4:3显示 */
	if(width*3 >= height*4) {
		view_rect->width = (height/3)<<2;
		view_rect->x = x + ((width-view_rect->width)>>1);
	}

	return 0;
}

static int Gx3211Vpu_16X9(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return 0;
}

void Gx3211Vpu_GetVppPort(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	GxVpuProperty_Resolution referance;
	GxVpuProperty_Resolution virtual_resolution;

	gx3211_vpu_GetVirtualResolution(&virtual_resolution);
	if(VPU_CLOSED())
	{
		clip_rect->x = 0;
		clip_rect->y = 0;
		clip_rect->width = 720;
		clip_rect->height = 576;
		return;
	}

	gx3211_vpu_GetActualResolution(&referance);
	view_rect->x      = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.x, referance.xres, 1);
	view_rect->y      = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.y, referance.yres, 0);
	view_rect->width  = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.width,  referance.xres, 1);
	view_rect->height = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.height, referance.yres, 0);

	referance.xres    = ((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->frame_width;
	referance.yres    = ((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->frame_height;
	clip_rect->x      = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].clip_rect.x, referance.xres, 1);
	clip_rect->y      = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].clip_rect.y, referance.yres, 0);
	clip_rect->width  = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].clip_rect.width,  referance.xres, 1);
	clip_rect->height = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[GX_LAYER_VPP].clip_rect.height, referance.yres, 0);
}

int Gx3211Vpu_AspectRatio_Adapt(GxLayerID layer, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int aspect = (gx3211vpu_info&&gx3211vpu_info->layer[GX_LAYER_VPP].priv ? ((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->stream_ratio : -1);

	if (aspect != -1 && (layer == GX_LAYER_VPP || layer == GX_LAYER_SPP) && \
			(gx3211vpu_info->layer[layer].surface) && \
			gx3211vpu_info->layer[layer].surface->surface_mode == GX_SURFACE_MODE_VIDEO) {
		switch(gx3211vpu_info->layer[layer].spec)
		{
		case VP_LETTER_BOX:
			Gx3211Vpu_LetterBox(layer, aspect, view_rect);
			break;
		case VP_PAN_SCAN:
			Gx3211Vpu_PanScan(layer, aspect, clip_rect);
			break;
		case VP_COMBINED:
			if(aspect == ASPECTRATIO_4BY3) {
				Gx3211Vpu_LetterBox(layer, aspect, view_rect);
				Gx3211Vpu_PanScan(layer, aspect, clip_rect);
			} else if (aspect == ASPECTRATIO_16BY9) {
				Gx3211Vpu_PanScan(layer, aspect, clip_rect);
				Gx3211Vpu_LetterBox(layer, aspect, view_rect);
			}
			break;
		case VP_RAW_SIZE:   //原始大小
			Gx3211Vpu_RawSize(clip_rect, view_rect);
			break;
		case VP_RAW_RATIO:  //原始比例
			Gx3211Vpu_RawRatio(clip_rect, view_rect);
			break;
		case VP_4X3_PULL:   //4:3拉伸
			Gx3211Vpu_4X3PullDown(layer, aspect, clip_rect, view_rect);
			break;
		case VP_4X3_CUT:    //4:3裁剪
			Gx3211Vpu_4X3CutOut(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9_PULL:  //16:9拉伸
			Gx3211Vpu_16X9PullDown(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9_CUT:   //16:9裁剪
			Gx3211Vpu_16X9CutOut(layer, aspect, clip_rect, view_rect);
			break;
		case VP_4X3:        //4:3
			Gx3211Vpu_4X3(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9:       //16:9
			Gx3211Vpu_16X9(layer, aspect, clip_rect, view_rect);
			break;
		case VP_AUTO:
			if(aspect == ASPECTRATIO_4BY3) {
				gx3211vpu_info->layer[layer].screen = SCREEN_16X9;
				Gx3211Vpu_PanScan(layer,aspect, clip_rect);
			}
			else if(aspect == ASPECTRATIO_16BY9) {
				gx3211vpu_info->layer[layer].screen = SCREEN_4X3;
				Gx3211Vpu_LetterBox(layer, aspect, clip_rect);
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

static int Gx3211Layer_Get_ActualViewrect(GxLayerID layer, GxAvRect *view_rect)
{
	GxVpuProperty_Resolution referance;
	GxVpuProperty_Resolution virtual_resolution;

	if(VPU_CLOSED())
		return -1;

	gx3211_vpu_GetActualResolution(&referance);
	gx3211_vpu_GetVirtualResolution(&virtual_resolution);
	view_rect->x     = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].view_rect.x,	referance.xres, 1);
	view_rect->y     = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].view_rect.y, referance.yres, 0);
	view_rect->width = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].view_rect.width,  referance.xres, 1);
	view_rect->height= gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].view_rect.height, referance.yres, 0);

	if (referance.yres == 576 && view_rect->height == 576)
		return 0;

	//patch vpu bug
	if (view_rect->y && view_rect->y + view_rect->height >= referance.yres) {
		view_rect->height -= 1;
	}

	return 0;
}

static int Gx3211Layer_Get_ActualCliprect(GxLayerID layer, GxAvRect *clip_rect)
{
	Gx3211VpuVppPriv *info;
	GxVpuProperty_Resolution referance;

	if(layer==GX_LAYER_VPP) {
		if(gx3211vpu_info && gx3211vpu_info->layer[GX_LAYER_VPP].priv) {
			info = gx3211vpu_info->layer[GX_LAYER_VPP].priv;
			referance.xres	= info->frame_width;
			referance.yres  = info->frame_height;
			clip_rect->x	= gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].clip_rect.x, referance.xres, 1);
			clip_rect->y	= gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].clip_rect.y, referance.yres, 0);
			clip_rect->width = gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].clip_rect.width,  referance.xres, 1);
			clip_rect->height= gx3211_vpu_VirtualToActual(gx3211vpu_info->layer[layer].clip_rect.height, referance.yres, 0);
		}
		else {
			gx_printf("%s:%d, get actual cliprect failed\n", __FILE__, __LINE__);
			return -1;
		}
	}
	else {
		clip_rect->x     = gx3211vpu_info->layer[layer].clip_rect.x;
		clip_rect->y     = gx3211vpu_info->layer[layer].clip_rect.y;
		clip_rect->width = gx3211vpu_info->layer[layer].clip_rect.width;
		clip_rect->height= gx3211vpu_info->layer[layer].clip_rect.height;
	}

	return 0;
}

static int Gx3211Layer_Set_ActualCliprect(GxLayerID layer, GxAvRect *clip_rect)
{
	Gx3211VpuLayer *Layer;
	GxVpuProperty_Resolution virtual_resolution;
	gx3211_vpu_GetVirtualResolution(&virtual_resolution);

	if(layer==GX_LAYER_VPP) {
		Layer = &gx3211vpu_info->layer[GX_LAYER_VPP];
		Layer->clip_rect.x      = gx3211_vpu_ActualToVirtual(clip_rect->x, virtual_resolution.xres, 1);
		Layer->clip_rect.y      = gx3211_vpu_ActualToVirtual(clip_rect->y, virtual_resolution.yres, 0);
		Layer->clip_rect.width  = gx3211_vpu_ActualToVirtual(clip_rect->width, virtual_resolution.xres, 1);
		Layer->clip_rect.height = gx3211_vpu_ActualToVirtual(clip_rect->height, virtual_resolution.yres, 0);
	}

	return 0;
}

#define ALIGN_BY_X(val, x) (val)&(~(x-1))
#define CLIPRECT_ALIGN(rect)\
	do{\
		(rect).x     = ALIGN_BY_X((rect).x, 8);\
		(rect).y     = ALIGN_BY_X((rect).y, 4);\
		(rect).width = ALIGN_BY_X((rect).width, 8);\
		(rect).height= ALIGN_BY_X((rect).height,2);\
	}while(0)
#define VIEWRECT_ALIGN(rect)\
	do{\
		(rect).width  = ALIGN_BY_X((rect).width,  2);\
		(rect).height = ALIGN_BY_X((rect).height, 2);\
	}while(0)

int Gx3211Vpu_Zoom(GxLayerID layer)
{
	int ret = -1;
	GxAvRect clip_rect, view_rect;

	if(VPU_CLOSED())
		return 0;

	if(gx3211vpu_info && gx3211vpu_info->layer[layer].surface) {
		clip_rect = gx3211vpu_info->layer[layer].clip_rect;
		view_rect = gx3211vpu_info->layer[layer].view_rect;
		if(gx3211vpu_info->layer[layer].auto_zoom) {
			Gx3211Layer_Get_ActualViewrect(layer, &view_rect);
			Gx3211Layer_Get_ActualCliprect(layer, &clip_rect);
			Gx3211Vpu_AspectRatio_Adapt(layer, &clip_rect, &view_rect);
			VIEWRECT_ALIGN(view_rect);
			CLIPRECT_ALIGN(clip_rect);
		} else {
			clip_rect = view_rect;
		}

		gx_interrupt_disable();
		ret = gx3211vpu_info->layer[layer].ops->set_view_port(&clip_rect, &view_rect);
		gx_interrupt_enable();
	}

	return ret;
}

int gx3211_osd_anti_flicker(int enable)
{
	if(enable) {
		VPU_OSD_PP_ANTI_FLICKER_ENABLE(gx3211vpu_reg->rOSD_CTRL);
		VPU_OSD_SET_PHASE_0(gx3211vpu_reg->rOSD_PHASE_0,VPU_OSD_PHASE_0_ANTI_FLICKER_ENABLE);
		VPU_OSD_CLR_ZOOM_MODE(gx3211vpu_reg->rOSD_CTRL);
	}
	else {
		VPU_OSD_PP_ANTI_FLICKER_DISABLE(gx3211vpu_reg->rOSD_CTRL);
		VPU_OSD_SET_PHASE_0(gx3211vpu_reg->rOSD_PHASE_0,VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE);
		VPU_OSD_SET_ZOOM_MODE(gx3211vpu_reg->rOSD_CTRL);
	}

	return 0;
}

int gx3211_osd_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int vzoom, hzoom;
	unsigned int data_addr = 0, data_offset = 0;
	unsigned int view_width=0, view_height = 0;
	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution    actual_resolution;
	volatile OsdRegion *gx3211_osd_head_ptr = *(OSD_HEADPTR);

	view_width      = view_rect->width;
	view_height     = view_rect->height;

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	gx3211_vpu_GetActualResolution(&actual_resolution);

	/* Set Zoom mod */
	switch(dvemode.mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
	case GXAV_VOUT_480P:
	case GXAV_VOUT_576I:
	case GXAV_VOUT_576P:
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ: {
		hzoom = ((clip_rect->width - 1) << 12) / (view_rect->width - 1);
		vzoom = ((clip_rect->height - 1) << 12) / (view_rect->height - 1);

		if(((clip_rect->width - 1) << 12) % (view_rect->width - 1))
			hzoom += 1;
		if(((clip_rect->height - 1) << 12) % (view_rect->height - 1))
			vzoom += 1;

		if((hzoom > 8192) || (vzoom > 14336))
			return -1;
		if(view_rect->x + view_rect->width > actual_resolution.xres)
			return -1;
		break;
	}
	default:
		gx_printf("%s(),unsupport tv out mode\n",__func__);
		return -1;
	}

	if(hzoom > 4096) {
		VPU_OSD_H_DOWNSCALE_ENABLE(gx3211vpu_reg->rOSD_CTRL);
		hzoom = (((clip_rect->width >> 1) - 1) << 12) / (view_rect->width - 1);
		if((((clip_rect->width >> 1) - 1) << 12) % (view_rect->width - 1))
			hzoom += 1;
	}
	else
		VPU_OSD_H_DOWNSCALE_DISABLE(gx3211vpu_reg->rOSD_CTRL);

#define OSD_MAINSURFACE_BUF_ADDR \
	(unsigned int)((Gx3211VpuSurface*)gx3211vpu_info->layer[GX_LAYER_OSD].surface)->buffer
#define OSD_MAINSURFACE_DATA_FORMAT \
	((Gx3211VpuSurface*)gx3211vpu_info->layer[GX_LAYER_OSD].surface)->color_format
#define OSD_MAINSURFACE_WIDTH\
	(((Gx3211VpuSurface*)gx3211vpu_info->layer[GX_LAYER_OSD].surface)->width)
	data_addr  = gx_virt_to_phys(OSD_MAINSURFACE_BUF_ADDR);
	data_offset= gx_color_get_bpp(OSD_MAINSURFACE_DATA_FORMAT)*(clip_rect->y*OSD_MAINSURFACE_WIDTH+clip_rect->x)>>3;
	data_addr += data_offset;
	VPU_OSD_SET_DATA_ADDR(gx3211_osd_head_ptr->word5, data_addr);

	VPU_OSD_SET_VTOP_PHASE(gx3211vpu_reg->rOSD_CTRL, 0x00);
	VPU_OSD_SET_WIDTH(gx3211_osd_head_ptr->word3, clip_rect->x, clip_rect->width + clip_rect->x - 1);
	VPU_OSD_SET_HEIGHT(gx3211_osd_head_ptr->word4, clip_rect->y, clip_rect->height + clip_rect->y - 1);
	VPU_OSD_SET_POSITION(gx3211vpu_reg->rOSD_POSITION, view_rect->x, view_rect->y);
	VPU_OSD_SET_VIEW_SIZE(gx3211vpu_reg->rOSD_VIEW_SIZE, view_width, view_height);
	VPU_OSD_SET_VZOOM(gx3211vpu_reg->rOSD_ZOOM, vzoom);
	VPU_OSD_SET_HZOOM(gx3211vpu_reg->rOSD_ZOOM, hzoom);

	if((clip_rect->width) <= 720 && view_width >= 1280)
	{
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[0], 2 << 24 | 3 << 16 | 4 << 8 | 0);
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[1], 35 << 24 | 3 << 16 | 14 << 8 | 12);
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[2], 84 << 8 | 70);
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[3], 0x38);
	} else {
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[0], 0);
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[1], 0);
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[2], (256 << 8));
		 REG_SET_VAL(&gx3211vpu_reg->rOSD_FILTER_PARA[3], 0);
	}
	gx3211_osd_anti_flicker(gx3211vpu_info->layer[GX_LAYER_OSD].anti_flicker_en);
	return 0;
}

int gx3211_osd_enable(int enable)
{
	if(enable == 0) {
		VPU_OSD_DISABLE(gx3211vpu_reg->rOSD_CTRL);
	}
	else {
		VPU_OSD_SET_FIRST_HEAD(gx3211vpu_reg->rOSD_FIRST_HEAD_PTR, OSD_HEADPTR_PHYS);
		VPU_OSD_ENABLE(gx3211vpu_reg->rOSD_CTRL);
	}

	return 0;
}

int gx3211_osd_palette(GxPalette *palette)
{
	int clut_len;
	volatile OsdRegion *gx3211_osd_head_ptr = *(OSD_HEADPTR);

	switch (palette->num_entries) {
	case 2:
		clut_len = VPU_CLUT1_LEN;
		break;
	case 4:
		clut_len = VPU_CLUT2_LEN;
		break;
	case 16:
		clut_len = VPU_CLUT4_LEN;
		break;
	case 256:
		clut_len = VPU_CLUT8_LEN;
		break;
	default:
		return -1;
	}
	VPU_OSD_SET_CLUT_PTR(gx3211_osd_head_ptr->word2, gx_virt_to_phys((unsigned int)palette->entries));
	VPU_OSD_SET_CLUT_LENGTH(gx3211_osd_head_ptr->word1, clut_len);
	VPU_OSD_CLUT_UPDATA_ENABLE(gx3211_osd_head_ptr->word1);

	return 0;
}

int gx3211_osd_alpha(GxAlpha alpha)
{
	GxColorFormat format;
	volatile OsdRegion *gx3211_osd_head_ptr = *(OSD_HEADPTR);

	if(alpha.type == GX_ALPHA_GLOBAL) {
		VPU_OSD_GLOBAL_ALHPA_ENABLE(gx3211_osd_head_ptr->word1);
		VPU_OSD_SET_MIX_WEIGHT(gx3211_osd_head_ptr->word1, alpha.value);
		VPU_OSD_SET_ALPHA_RATIO_DISABLE(gx3211_osd_head_ptr->word7);
	}
	else {
		VPU_OSD_GLOBAL_ALHPA_DISABLE(gx3211_osd_head_ptr->word1);
		//patch, 0xFF should equal with ratio_disable, but actually not
		if(alpha.value == 0xFF) {
			VPU_OSD_SET_ALPHA_RATIO_DISABLE(gx3211_osd_head_ptr->word7);
		}
		else {
			VPU_OSD_SET_ALPHA_RATIO_VALUE(gx3211_osd_head_ptr->word7, alpha.value);
			VPU_OSD_SET_ALPHA_RATIO_ENABLE(gx3211_osd_head_ptr->word7);
		}
		VPU_OSD_GET_COLOR_TYPE(gx3211_osd_head_ptr->word1, format);
		if(GX_COLOR_FMT_RGBA5551 == format) {
			/* reserved */
		}
	}

	return 0;
}

int gx3211_osd_color_format(GxColorFormat format)
{
	int true_color_mode, bpp = 0;
	ByteSequence byteseq = ABCD_EFGH;
	volatile OsdRegion *gx3211_osd_head_ptr = *(OSD_HEADPTR);

	if(format <= GX_COLOR_FMT_CLUT8)
		VPU_OSD_CLR_ZOOM_MODE_EN_IPS(gx3211vpu_reg->rOSD_CTRL);
	else
		VPU_OSD_SET_ZOOM_MODE_EN_IPS(gx3211vpu_reg->rOSD_CTRL);

	if((format >= GX_COLOR_FMT_RGBA8888)&&(format <= GX_COLOR_FMT_BGR888)) {
		true_color_mode = format - GX_COLOR_FMT_RGBA8888;
		VPU_OSD_SET_COLOR_TYPE(gx3211_osd_head_ptr->word1, 0x7);
		VPU_OSD_SET_TRUE_COLOR_MODE(gx3211_osd_head_ptr->word1, true_color_mode);
	}
	else {
		if( (format==GX_COLOR_FMT_ARGB4444)||
				(format==GX_COLOR_FMT_ARGB1555)||
				(format==GX_COLOR_FMT_ARGB8888))
			VPU_OSD_SET_ARGB_CONVERT(gx3211_osd_head_ptr->word1,1);
		else
			VPU_OSD_SET_ARGB_CONVERT(gx3211_osd_head_ptr->word1,0);

		if(format == GX_COLOR_FMT_ABGR8888)
			VPU_OSD_SET_ABGR_CONVERT(gx3211_osd_head_ptr->word1, 1);
		else
			VPU_OSD_SET_ABGR_CONVERT(gx3211_osd_head_ptr->word1, 0);

		switch(format) {
		case GX_COLOR_FMT_ARGB4444:
			format = GX_COLOR_FMT_RGBA4444;
			break;
		case GX_COLOR_FMT_ARGB1555:
			format = GX_COLOR_FMT_RGBA5551;
			break;
		case GX_COLOR_FMT_ABGR8888:
		case GX_COLOR_FMT_ARGB8888:
			format = GX_COLOR_FMT_RGBA8888;
			break;
		default:
			break;
		}
		VPU_OSD_SET_COLOR_TYPE(gx3211_osd_head_ptr->word1, format);
	}

	bpp = gx_color_get_bpp(format);

	if(bpp == 32)
		byteseq = ABCD_EFGH;
	else if(bpp == 16)
		byteseq = CDAB_GHEF;
	else if(bpp <= 8)
		byteseq = DCBA_HGFE;

	((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->data_byte_seq = byteseq;
	VPU_OSD_SET_DATA_RW_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);

	byteseq = ABCD_EFGH;
	((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq = byteseq;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);

	bpp = gx_color_get_bpp(format);

	if(bpp == 32)
		byteseq = ABCD_EFGH;
	else if(bpp == 16)
		byteseq = CDAB_GHEF;
	else if(bpp <= 8)
		byteseq = DCBA_HGFE;

	((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->data_byte_seq = byteseq;
	VPU_OSD_SET_DATA_RW_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);

	byteseq = ABCD_EFGH;
	((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq = byteseq;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);

	return 0;
}

int gx3211_osd_color_key(GxColor *color, int enable)
{
	unsigned char r, g, b;
	volatile OsdRegion *region_head = *(OSD_HEADPTR);

	if(enable == 0 && region_head) {
		VPU_OSD_COLOR_KEY_DISABLE(region_head->word1);
	} else {
		if (region_head) {
			VPU_OSD_COLOR_KEY_ENABLE(region_head->word1);
		}
		switch (gx3211vpu_info->layer[GX_LAYER_OSD].surface->color_format) {
		case GX_COLOR_FMT_RGB565:
			r = (color->r & 0xF8) | (color->r >> 5);
			g = (color->g & 0xFC) | (color->g >> 6);
			b = (color->b & 0xF8) | (color->b >> 5);
			break;
		case GX_COLOR_FMT_RGBA4444:
		case GX_COLOR_FMT_ARGB4444:
			r = (color->r & 0xF0) | (color->r >> 4);
			g = (color->g & 0xF0) | (color->g >> 4);
			b = (color->b & 0xF0) | (color->b >> 4);
			break;
		case GX_COLOR_FMT_RGBA5551:
		case GX_COLOR_FMT_ARGB1555:
			r = (color->r & 0xF8) | (color->r >> 5);
			g = (color->g & 0xF8) | (color->g >> 5);
			b = (color->b & 0xF8) | (color->b >> 5);
			break;
		default:
			r = color->r;
			g = color->g;
			b = color->b;
			break;
		}
		VPU_OSD_SET_COLOR_KEY(gx3211vpu_reg->rOSD_COLOR_KEY, r, g, b, color->a);
	}

	return 0;
}

#define VPU_CAL_REQUEST_BLOCK(width, bpp, align, max, min, ret)\
	do{\
		unsigned line_word = ((width*bpp)>>3)>>2;\
		unsigned request = (line_word)/align*align;\
		if(request < min)\
		ret = min;\
		else {\
			while(request<max && (line_word%request && line_word%request<min)) {\
				request += align;\
			}\
			if(request>=max || (line_word%request && line_word%request<min))\
			ret = min;\
			else\
			ret = request;\
		}\
	}while(0)
static void osd_common_config(Gx3211VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	GxAvRect rect={0};
	ByteSequence byteseq = ABCD_EFGH;
	unsigned int request_block=0, bpp=1;

	rect.x     = pos_x;
	rect.y     = pos_y;
	rect.width = surface->width;
	rect.height= surface->height;

	bpp = gx_color_get_bpp(surface->color_format);
	request_block = rect.width*(bpp>>3)/4/128*128;
	if(request_block < 128) {
		request_block = 128;
	}
	else if(request_block > 896) {
		request_block = 896;
	}
	/* set vpu byte-sequence(data) */
	if(bpp == 32)
		byteseq = ABCD_EFGH;
	else if(bpp == 16)
		byteseq = CDAB_GHEF;
	else if(bpp <= 8)
		byteseq = DCBA_HGFE;
	((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->data_byte_seq = byteseq;
	VPU_OSD_SET_DATA_RW_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);
	/* set vpu byte-sequence(region head) */
	byteseq = ABCD_EFGH;
	((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq = byteseq;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);
	REG_SET_FIELD(&(gx3211vpu_reg->rBUFF_CTRL2),0x7FF<<0,request_block,0);
	VPU_OSD_SET_POSITION(gx3211vpu_reg->rOSD_POSITION, rect.x, rect.y);
	if(VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE == VPU_OSD_GET_PHASE_0(gx3211vpu_reg->rOSD_PHASE_0))
		VPU_OSD_SET_ZOOM_MODE(gx3211vpu_reg->rOSD_CTRL);
	else
		VPU_OSD_CLR_ZOOM_MODE(gx3211vpu_reg->rOSD_CTRL);
	gx3211_osd_color_key(&surface->color_key, surface->color_key_en);
}

static int gx3211_osd_add_region(Gx3211VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	int ret = -1;

	if (surface) {
		osd_common_config(surface, pos_x, pos_y);
		gx3211vpu_info->layer[GX_LAYER_OSD].surface = surface;
		ret = gx_region_add(surface, pos_x, pos_y);
	}

	return ret;
}

int gx3211_osd_remove_region(Gx3211VpuSurface *surface)
{
	return gx_region_remove(surface);
}

int gx3211_osd_update_region(Gx3211VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return gx_region_update(surface, pos_x, pos_y);
}

int gx3211_osd_pan_display(void *buffer)
{
	OsdRegion **p_region_header = OSD_HEADPTR;

	if (buffer && *p_region_header) {
		OsdRegion *region = *p_region_header;
		VPU_OSD_SET_DATA_ADDR(region->word5, gx_virt_to_phys((unsigned int)buffer));
	}

	return 0;
}

int gx3211_osd_main_surface(Gx3211VpuSurface *surface)
{
	unsigned ret = -1;
	unsigned pos_x = 0, pos_y = 0;
	OsdRegion *region_header = *(OSD_HEADPTR);

	if (region_header == NULL) {
		ret = gx3211_osd_add_region(surface, pos_x, pos_y);
	} else {
		ret = gx3211_osd_add_region(surface, pos_x, pos_y);
		gx3211_osd_remove_region(region_header->surface);
	}

	if((surface->color_format == GX_COLOR_FMT_ARGB1555) ||
	   (surface->color_format == GX_COLOR_FMT_RGBA5551)){
		REG_SET_VAL(&(gx3211vpu_reg->rOSD_ALPHA_5551), 0x00FF00);
	}

	return ret;
}

int gx3211_osd_on_top(int enable)
{
	/*
	   if(enable == 0)
	   VPU_SOSD_SET_ON_TOP_ENABLE(gx3211vpu_reg->rSOSD_CTRL);
	   else
	   VPU_SOSD_SET_ON_TOP_DISABLE(gx3211vpu_reg->rSOSD_CTRL);
	   */
	return 0;
}

int gx3211sosd_anti_flicker(int enable)
{
	if(enable) {
		VPU_OSD_PP_ANTI_FLICKER_ENABLE(gx3211vpu_reg->rSOSD_CTRL);
		VPU_OSD_SET_PHASE_0(gx3211vpu_reg->rSOSD_PHASE_0, VPU_OSD_PHASE_0_ANTI_FLICKER_ENABLE);
		VPU_OSD_CLR_ZOOM_MODE(gx3211vpu_reg->rSOSD_CTRL);
	}
	else {
		VPU_OSD_PP_ANTI_FLICKER_DISABLE(gx3211vpu_reg->rSOSD_CTRL);
		VPU_OSD_SET_PHASE_0(gx3211vpu_reg->rSOSD_PHASE_0,VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE);
		VPU_OSD_SET_ZOOM_MODE(gx3211vpu_reg->rSOSD_CTRL);
	}

	return 0;
}

int gx3211sosd_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int vzoom, hzoom;
	unsigned int data_addr = 0, data_offset = 0;
	unsigned int view_width=0, view_height = 0;
	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution    actual_resolution;
	volatile OsdRegion *gx3211sosd_head_ptr = *(SOSD_HEADPTR);

	view_width      = view_rect->width;
	view_height     = view_rect->height;
	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	gx3211_vpu_GetActualResolution(&actual_resolution);

	/* Set Zoom mod */
	switch(dvemode.mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
	case GXAV_VOUT_480P:
	case GXAV_VOUT_576I:
	case GXAV_VOUT_576P:
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ: {
		hzoom = ((clip_rect->width - 1) << 12) / (view_rect->width - 1);
		vzoom = ((clip_rect->height - 1) << 12) / (view_rect->height - 1);

		if(((clip_rect->width - 1) << 12) % (view_rect->width - 1))
			hzoom += 1;
		if(((clip_rect->height - 1) << 12) % (view_rect->height - 1))
			vzoom += 1;

		if((hzoom > 8192) || (vzoom > 14336))
			return -1;
		if(view_rect->x + view_rect->width > actual_resolution.xres)
			return -1;
		break;
	}
	default:
		gx_printf("%s(),unsupport tv out mode\n",__func__);
		return -1;
	}

	if(hzoom > 4096) {
		VPU_OSD_H_DOWNSCALE_ENABLE(gx3211vpu_reg->rSOSD_CTRL);
		hzoom = (((clip_rect->width >> 1) - 1) << 12) / (view_rect->width - 1);
		if((((clip_rect->width >> 1) - 1) << 12) % (view_rect->width - 1))
			hzoom += 1;
	}
	else
		VPU_OSD_H_DOWNSCALE_DISABLE(gx3211vpu_reg->rSOSD_CTRL);

#define SOSD_MAINSURFACE_BUF_ADDR \
	(unsigned int)((Gx3211VpuSurface*)gx3211vpu_info->layer[GX_LAYER_SOSD].surface)->buffer
#define SOSD_MAINSURFACE_DATA_FORMAT \
	((Gx3211VpuSurface*)gx3211vpu_info->layer[GX_LAYER_SOSD].surface)->color_format
#define SOSD_MAINSURFACE_WIDTH\
	(((Gx3211VpuSurface*)gx3211vpu_info->layer[GX_LAYER_SOSD].surface)->width)
	data_addr  = gx_virt_to_phys(SOSD_MAINSURFACE_BUF_ADDR);
	data_offset= gx_color_get_bpp(SOSD_MAINSURFACE_DATA_FORMAT)*(clip_rect->y*SOSD_MAINSURFACE_WIDTH+clip_rect->x)>>3;
	data_addr += data_offset;
	VPU_OSD_SET_DATA_ADDR(gx3211sosd_head_ptr->word5, data_addr);

	VPU_OSD_SET_VTOP_PHASE(gx3211vpu_reg->rSOSD_CTRL, 0x00);
	VPU_OSD_SET_WIDTH(gx3211sosd_head_ptr->word3, clip_rect->x, clip_rect->width + clip_rect->x - 1);
	VPU_OSD_SET_HEIGHT(gx3211sosd_head_ptr->word4, clip_rect->y, clip_rect->height + clip_rect->y - 1);
	VPU_OSD_SET_POSITION(gx3211vpu_reg->rSOSD_POSITION, view_rect->x, view_rect->y);
	VPU_OSD_SET_VIEW_SIZE(gx3211vpu_reg->rSOSD_VIEW_SIZE, view_width, view_height);
	VPU_OSD_SET_VZOOM(gx3211vpu_reg->rSOSD_ZOOM, vzoom);
	VPU_OSD_SET_HZOOM(gx3211vpu_reg->rSOSD_ZOOM, hzoom);

	gx3211sosd_anti_flicker(gx3211vpu_info->layer[GX_LAYER_SOSD].anti_flicker_en);
	return 0;
}

int gx3211sosd_enable(int enable)
{
	if(enable == 0) {
		VPU_OSD_DISABLE(gx3211vpu_reg->rSOSD_CTRL);
	}
	else {
		VPU_OSD_SET_FIRST_HEAD(gx3211vpu_reg->rSOSD_FIRST_HEAD_PTR, SOSD_HEADPTR_PHYS);
		VPU_OSD_ENABLE(gx3211vpu_reg->rSOSD_CTRL);
	}

	return 0;
}

int gx3211sosd_alpha(GxAlpha alpha)
{
	GxColorFormat format;
	volatile OsdRegion *gx3211sosd_head_ptr = *(SOSD_HEADPTR);

	if(alpha.type == GX_ALPHA_GLOBAL) {
		VPU_OSD_GLOBAL_ALHPA_ENABLE(gx3211sosd_head_ptr->word1);
		VPU_OSD_SET_MIX_WEIGHT(gx3211sosd_head_ptr->word1, alpha.value);
		VPU_OSD_SET_ALPHA_RATIO_DISABLE(gx3211sosd_head_ptr->word7);
	}
	else {
		VPU_OSD_GLOBAL_ALHPA_DISABLE(gx3211sosd_head_ptr->word1);
		//patch, 0xFF should equal with ratio_disable, but actually not
		if(alpha.value == 0xFF) {
			VPU_OSD_SET_ALPHA_RATIO_DISABLE(gx3211sosd_head_ptr->word7);
		}
		else {
			VPU_OSD_SET_ALPHA_RATIO_VALUE(gx3211sosd_head_ptr->word7, alpha.value);
			VPU_OSD_SET_ALPHA_RATIO_ENABLE(gx3211sosd_head_ptr->word7);
		}
		VPU_OSD_GET_COLOR_TYPE(gx3211sosd_head_ptr->word1, format);
		if(GX_COLOR_FMT_RGBA5551 == format) {
			/* reserved */
		}
	}

	return 0;
}

int gx3211sosd_color_format(GxColorFormat format)
{
	int true_color_mode;
	volatile OsdRegion *gx3211sosd_head_ptr = *(SOSD_HEADPTR);

	if(format <= GX_COLOR_FMT_CLUT8)
		VPU_OSD_CLR_ZOOM_MODE_EN_IPS(gx3211vpu_reg->rSOSD_CTRL);
	else
		VPU_OSD_SET_ZOOM_MODE_EN_IPS(gx3211vpu_reg->rSOSD_CTRL);

	if((format >= GX_COLOR_FMT_RGBA8888)&&(format <= GX_COLOR_FMT_BGR888)) {
		true_color_mode = format - GX_COLOR_FMT_RGBA8888;
		VPU_OSD_SET_COLOR_TYPE(gx3211sosd_head_ptr->word1, 0x7);
		VPU_OSD_SET_TRUE_COLOR_MODE(gx3211sosd_head_ptr->word1, true_color_mode);
	}
	else {
		if( (format==GX_COLOR_FMT_ARGB4444)||
				(format==GX_COLOR_FMT_ARGB1555)||
				(format==GX_COLOR_FMT_ARGB8888))
			VPU_OSD_SET_ARGB_CONVERT(gx3211sosd_head_ptr->word1,1);
		else
			VPU_OSD_SET_ARGB_CONVERT(gx3211sosd_head_ptr->word1,0);

		if(format == GX_COLOR_FMT_ABGR8888)
			VPU_OSD_SET_ABGR_CONVERT(gx3211sosd_head_ptr->word1, 1);
		else
			VPU_OSD_SET_ABGR_CONVERT(gx3211sosd_head_ptr->word1, 0);

		switch(format) {
		case GX_COLOR_FMT_ARGB4444:
			format = GX_COLOR_FMT_RGBA4444;
			break;
		case GX_COLOR_FMT_ARGB1555:
			format = GX_COLOR_FMT_RGBA5551;
			break;
		case GX_COLOR_FMT_ABGR8888:
		case GX_COLOR_FMT_ARGB8888:
			format = GX_COLOR_FMT_RGBA8888;
			break;
		default:
			break;
		}
		VPU_OSD_SET_COLOR_TYPE(gx3211sosd_head_ptr->word1, format);
	}

	return 0;
}

int gx3211sosd_color_key(GxColor *color, int enable)
{
	unsigned char r, g, b;
	volatile OsdRegion *gx3211sosd_head_ptr = *(SOSD_HEADPTR);

	if(enable == 0) {
		VPU_OSD_COLOR_KEY_DISABLE(gx3211sosd_head_ptr->word1);
	}
	else {
		VPU_OSD_COLOR_KEY_ENABLE(gx3211sosd_head_ptr->word1);
		switch (gx3211vpu_info->layer[GX_LAYER_SOSD].surface->color_format) {
		case GX_COLOR_FMT_RGB565:
			r = (color->r & 0xF8) | (color->r >> 5);
			g = (color->g & 0xFC) | (color->g >> 6);
			b = (color->b & 0xF8) | (color->b >> 5);
			break;
		case GX_COLOR_FMT_RGBA4444:
		case GX_COLOR_FMT_ARGB4444:
			r = (color->r & 0xF0) | (color->r >> 4);
			g = (color->g & 0xF0) | (color->g >> 4);
			b = (color->b & 0xF0) | (color->b >> 4);
			break;
		case GX_COLOR_FMT_RGBA5551:
		case GX_COLOR_FMT_ARGB1555:
			r = (color->r & 0xF8) | (color->r >> 5);
			g = (color->g & 0xF8) | (color->g >> 5);
			b = (color->b & 0xF8) | (color->b >> 5);
			break;
		default:
			r = color->r;
			g = color->g;
			b = color->b;
			break;
		}
		VPU_OSD_SET_COLOR_KEY(gx3211vpu_reg->rSOSD_COLOR_KEY, r, g, b, color->a);
	}

	return 0;
}

#define VPU_CAL_REQUEST_BLOCK(width, bpp, align, max, min, ret)\
	do{\
		unsigned line_word = ((width*bpp)>>3)>>2;\
		unsigned request = (line_word)/align*align;\
		if(request < min)\
		ret = min;\
		else {\
			while(request<max && (line_word%request && line_word%request<min)) {\
				request += align;\
			}\
			if(request>=max || (line_word%request && line_word%request<min))\
			ret = min;\
			else\
			ret = request;\
		}\
	}while(0)
int gx3211sosd_main_surface(Gx3211VpuSurface *surface)
{
	GxAvRect rect={0};
	ByteSequence byteseq = ABCD_EFGH;
	unsigned int request_block=0, bpp=1;
	volatile OsdRegion *gx3211sosd_head_ptr = *(SOSD_HEADPTR);

	rect.width = surface->width;
	rect.height= surface->height;
	bpp = gx_color_get_bpp(surface->color_format);
	request_block = rect.width*(bpp>>3)/4/128*128;
	if(request_block < 128) {
		request_block = 128;
	}
	else if(request_block > 896) {
		request_block = 896;
	}

	/* set vpu byte-sequence(data) */
	if(bpp == 32)
		byteseq = ABCD_EFGH;
	else if(bpp == 16)
		byteseq = CDAB_GHEF;
	else if(bpp <= 8)
		byteseq = DCBA_HGFE;
	((Gx3211VpuSosdPriv*)gx3211vpu_info->layer[GX_LAYER_SOSD].priv)->data_byte_seq = byteseq;
	VPU_SOSD_SET_DATA_RW_BYTESEQ(gx3211vpu_reg->rSOSD_PARA, byteseq);
	/* set vpu byte-sequence(region head) */
	byteseq = ABCD_EFGH;
	((Gx3211VpuSosdPriv*)gx3211vpu_info->layer[GX_LAYER_SOSD].priv)->regionhead_byte_seq = byteseq;
	VPU_SOSD_SET_REGIONHEAD_BYTESEQ(gx3211vpu_reg->rSOSD_PARA, byteseq);

	REG_SET_FIELD(&(gx3211vpu_reg->rSOSD_PARA), 0x7FF<<0, request_block, 0);
	VPU_OSD_SET_WIDTH(gx3211sosd_head_ptr->word3, rect.x, rect.width + rect.x - 1);
	VPU_OSD_SET_HEIGHT(gx3211sosd_head_ptr->word4, rect.y, rect.height + rect.y - 1);
	VPU_OSD_SET_POSITION(gx3211vpu_reg->rSOSD_POSITION, rect.x, rect.y);

	gx3211sosd_alpha(surface->alpha);
	gx3211sosd_color_format(surface->color_format);
	gx3211sosd_color_key(&surface->color_key, surface->color_key_en);

	VPU_OSD_SET_DATA_ADDR(gx3211sosd_head_ptr->word5, gx_virt_to_phys((unsigned int)surface->buffer));
	VPU_OSD_SET_FIRST_HEAD(gx3211vpu_reg->rSOSD_FIRST_HEAD_PTR, SOSD_HEADPTR_PHYS);
	VPU_OSD_LIST_END_ENABLE(gx3211sosd_head_ptr->word7);
	VPU_OSD_SET_BASE_LINE(gx3211sosd_head_ptr->word7,rect.width);

	if(VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE == VPU_OSD_GET_PHASE_0(gx3211vpu_reg->rSOSD_PHASE_0))
		VPU_OSD_SET_ZOOM_MODE(gx3211vpu_reg->rSOSD_CTRL);
	else
		VPU_OSD_CLR_ZOOM_MODE(gx3211vpu_reg->rSOSD_CTRL);

	return 0;
}

int gx3211sosd_on_top(int enable)
{
	/*
	   if(enable == 0)
	   VPU_SOSD_SET_ON_TOP_ENABLE(gx3211vpu_reg->rSOSD_CTRL);
	   else
	   VPU_SOSD_SET_ON_TOP_DISABLE(gx3211vpu_reg->rSOSD_CTRL);
	   */
	return 0;
}

unsigned int gx3211_vpp_calc_requestblock(unsigned int old_width, unsigned int new_width)
{
	unsigned int min, max, align, req;

	//init
	align = 8;
	min = 32;
	max = 512;
	req = 256;

	//search
#define IS_BAD_REQ(req, width, min) ((width)%(req)<=(min))
	while(req<=max && (IS_BAD_REQ(req, old_width, min)||IS_BAD_REQ(req, new_width, min))) {
		req += align;
	}
	if(req>=max && (IS_BAD_REQ(req, old_width, min)||IS_BAD_REQ(req, new_width, min))) {
		req = 512;
		gx_printf("old_width = %d, new_width = %d, can not get avaliable request block\n", old_width, new_width);
	}

	return req;
}

int gx3211_vpp_view_port(GxAvRect *clip_rect, GxAvRect *view_rect)
{
#define VPP_GET_ZOOM_PARA(para,clip_size,view_size,down_scale) \
	do { \
		(para) = ((((clip_size) >> (down_scale))) << VPU_VPP_ZOOM_WIDTH) / (view_size); \
	}while (0)

	int i = 0, play_mode;
	int vzoom = 0, hzoom = 0;
	int vtbias = 0, vbbias = 0;
	int vdown_scale_en = 0, hdown_scale_en = 0;

	unsigned int baseline = 0;
	unsigned int request_block = 0, uv_mode;
	unsigned int src_width = 0, src_height = 0, old_src_width = 0;
	unsigned int view_width = 0, view_height = 0;
	struct vout_dvemode dvemode;
	DispControler *controler = &((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->controler;

	if(VPU_CLOSED()) {
		return 0;
	}

	view_width    = view_rect->width;
	view_height   = view_rect->height;
	src_width     = clip_rect->width;
	src_height    = clip_rect->height;
	baseline      = controler->last_frame.baseline;
	play_mode     = ((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->play_mode;
	old_src_width = VPU_VPP_GET_PP_SOURCE_WIDTH(gx3211vpu_reg->rPP_SOURCE_SIZE);
	request_block = gx3211_vpp_calc_requestblock(old_src_width, src_width);

	VPP_GET_ZOOM_PARA(hzoom, clip_rect->width, view_width, hdown_scale_en);
	while(VPU_VPP_ZOOM_OUT_MAX < hzoom) {
		hdown_scale_en++;
		VPP_GET_ZOOM_PARA(hzoom, clip_rect->width, view_width, hdown_scale_en);
		if(hdown_scale_en > 5)
			return -1;
	}

	VPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_height, vdown_scale_en);
	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if (IS_INTERLACE_MODE(dvemode.mode)) {
		if (PLAY_MODE_FRAME == play_mode) {
			while ((VPU_VPP_ZOOM_OUT_MAX >> 1) < vzoom) {
				vdown_scale_en++;
				VPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_height, vdown_scale_en);
				if (vdown_scale_en > 5)
					return -1;
			}
			vtbias = 0;
			vbbias =vzoom;
		} else {
			while (VPU_VPP_ZOOM_OUT_MAX < vzoom) {
				vdown_scale_en++;
				VPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_height, vdown_scale_en);
				if (vdown_scale_en > 5)
					return -1;
			}

			if (VPU_VPP_ZOOM_NONE < vzoom) {
				vtbias = 0;
				vbbias = (vzoom >> 1) - 2048;
			} else {
				vtbias = 2048 - (vzoom >> 1);
				vbbias = 0;
			}
		}
	} else if(IS_PROGRESSIVE_MODE(dvemode.mode)) {
		while (VPU_VPP_ZOOM_OUT_MAX < vzoom) {
			vdown_scale_en++;
			VPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_height, vdown_scale_en);
			if(vdown_scale_en > 5)
				return -1;
		}

		if (PLAY_MODE_FRAME == play_mode) {
			vtbias = 0;
			vbbias = 0;
		} else {
			vtbias = 2048;
			vbbias = 0;
		}
	}

	if (hdown_scale_en)
		src_width = (src_width >> 1) << 1;
	else
		src_width = (src_width >> 3) << 3;

	if (vzoom == VPU_VPP_ZOOM_NONE && controler->last_frame.playmode == PLAY_MODE_Y_FRAME_UV_FIELD)
		uv_mode = 0x1;
	else
		uv_mode = 0x0;
	VPU_VPP_SET_FIELD_MODE_UV_FIELD(gx3211vpu_reg->rPP_CTRL, uv_mode);

	VPU_VPP_SET_PP_SOURCE_SIZE(gx3211vpu_reg->rPP_SOURCE_SIZE, (src_width), (src_height));
	VPU_VPP_SET_REQUEST_BLOCK(gx3211vpu_reg->rBUFF_CTRL1, request_block);

	VPU_VPP_SET_PP_POSITION(gx3211vpu_reg->rPP_POSITION, view_rect->x, view_rect->y);
	VPU_VPP_SET_PP_SIZE(gx3211vpu_reg->rPP_VIEW_SIZE, view_rect->width, view_rect->height);
	VPU_VPP_SET_ZOOM(gx3211vpu_reg->rPP_ZOOM, vzoom ,hzoom);
	VPU_VPP_SET_BASE_LINE(gx3211vpu_reg->rPP_FRAME_STRIDE, baseline);

	VPU_VPP_SET_VT_PHASE(gx3211vpu_reg->rPP_V_PHASE, vtbias & 0x7FFF);
	VPU_VPP_SET_VB_PHASE(gx3211vpu_reg->rPP_V_PHASE, vbbias & 0x7FFF);

	if((clip_rect->width) <= 720 && view_width >= 1280) {
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[0], 2 << 24 | 3 << 16 | 4 << 8 | 0);
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[1], 35 << 24 | 3 << 16 | 14 << 8 | 12);
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[2], 84 << 8 | 70);
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[3], 0x38);
	} else {
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[0], 0);
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[1], 0);
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[2], (256 << 8));
		 REG_SET_VAL(&gx3211vpu_reg->rPP_FILTER_PARA[3], 0);
	}

	switch(vdown_scale_en) {
	case 0:
		VPU_VPP_V_DOWNSCALE_DISABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 0);
		break;
	case 1:
		VPU_VPP_V_DOWNSCALE_ENABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 0);
		//VPU_VPP_SET_BASE_LINE(gx3211vpu_reg->rPP_FRAME_STRIDE, baseline*2);
		break;
	case 2:
		VPU_VPP_V_DOWNSCALE_DISABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 1);
		break;
	case 3:
		VPU_VPP_V_DOWNSCALE_ENABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 1);
		break;
	case 4:
		VPU_VPP_V_DOWNSCALE_DISABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 2);
		break;
	case 5:
		VPU_VPP_V_DOWNSCALE_ENABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 2);
		break;
	default:
		gx_printf("unsupport down_scale\n");
		break;
	}

	switch(hdown_scale_en) {
	case 0:
		VPU_VPP_H_DOWNSCALE_DISABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 0);
		break;
	case 1:
		VPU_VPP_H_DOWNSCALE_ENABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 0);
		break;
	case 2:
		VPU_VPP_H_DOWNSCALE_DISABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 1);
		break;
	case 3:
		VPU_VPP_H_DOWNSCALE_ENABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 1);
		break;
	case 4:
		VPU_VPP_H_DOWNSCALE_DISABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 2);
		break;
	case 5:
		VPU_VPP_H_DOWNSCALE_ENABLE(gx3211vpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(gx3211vpu_reg->rPP_POSITION, 2);
		break;
	default:
		gx_printf("unsupport down_scale\n");
		break;
	}

	gx3211vpu_reg->rPP_FILTER_SIGN = 0x998080;
	if (hzoom == 4096)
		VPU_VPP_SET_PP_PHASE_0_H(gx3211vpu_reg->rPP_PHASE_0_H, 0x00ff0000);
	else
		VPU_VPP_SET_PP_PHASE_0_H(gx3211vpu_reg->rPP_PHASE_0_H, 0x00ff0100);
	if (vzoom == 4096)
		VPU_VPP_SET_PP_PHASE_0_V(gx3211vpu_reg->rPP_PHASE_0_V, 0x00ff0000);
	else
		VPU_VPP_SET_PP_PHASE_0_V(gx3211vpu_reg->rPP_PHASE_0_V, 0x00ff0100);

	for (i = 0; i < 64; i++)
		VPU_VPP_SET_PHASE(gx3211vpu_reg->rPP_PHASE_PARA_V, i, sPhaseNomal[i]);
	for (i = 0; i < 64; i++)
		VPU_VPP_SET_PHASE(gx3211vpu_reg->rPP_PHASE_PARA_H, i, sPhaseNomal[i]);

	return 0;
}

int gx3211_vpp_enable(int enable)
{
	if (gx3211vpu_info == NULL)
		return 0;

	if(enable == 0) {
		VPU_VPP_DISABLE(gx3211vpu_reg->rPP_CTRL);
	}
	else {
		VPU_VPP_ENABLE(gx3211vpu_reg->rPP_CTRL);
	}

	return 0;
}

int gx3211_vpp_blur(int enable)
{
	return 0;
}

int gx3211_vpp_video_mode(GxLayerVideoMode mode)
{
	return 0;
}

int gx3211_vpp_bg_color(GxColor color)
{
	VPU_VPP_SET_BG_COLOR(gx3211vpu_reg->rPP_BACK_COLOR, color.y, color.cb, color.cr);
	return 0;
}

static int gx3211_vpp_get_rdptr(void)
{
	int rd_ptr;
	DispControler *controler = NULL;

	if (VPU_CLOSED())
		return -1;
	controler = &((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->controler;

	rd_ptr = VPU_DISP_GET_BUFF_ID(gx3211vpu_reg->rPP_DISP_R_PTR);
	//gx_printf("\nrd = %d\n", rd_ptr);
	if (gx3211vpu_info->reset_flag && rd_ptr == 1) {
		rd_ptr = 0;
		controler->buf_to_wr = 0;
	}

	return rd_ptr;
}

	//gx_printf("cfg to %d\n", id);
#define CONFIG_DISP_UNIT(top, bot, offset_y, offset_cbcr, playmode, topfirst)\
do {\
	int id = controler->buf_to_wr;\
	controler->unit[id]->word0 = top.addry    + offset_y;\
	controler->unit[id]->word2 = top.addrcbcr + offset_cbcr;\
	controler->unit[id]->word1 = bot.addry    + offset_y;\
	controler->unit[id]->word3 = bot.addrcbcr + offset_cbcr;\
	controler->unit[id]->word7 = (topfirst)|(playmode);\
	controler->buf_to_wr += 1;\
	controler->buf_to_wr %= MAX_DISPBUF_NUM;\
} while(0);

int gx3211_vpp_zoom_require(void)
{
	DispControler *controler = &((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->controler;

	controler->zoom_require++;
	return 0;
}

#define ITU_R_BT_709_6          (1)
#define ITU_R_BT_470_6_M        (4)
#define ITU_R_BT_601_7_625      (5)
#define ITU_R_BT_601_7_525      (6)
#define SMPTE_ST_240            (7)
#define GENERIC_FILM            (8)
#define ITU_R_BT_2020_2         (9)
#define SMPTE_ST_428_1          (10)
#define SMPTE_RP_431_2          (11)
#define SMPTE_EG_432_1          (12)
#define EBU_TECH_3213_E         (22)

typedef enum {
	TV_SYSTEM_525,
	TV_SYSTEM_625,
	TV_SYSTEM_709_6
} TVColorType_t;

static TVColorType_t _get_tv_color_type(GxVideoOutProperty_Mode mode)
{
	switch(mode) {
	case GXAV_VOUT_480I:
	case GXAV_VOUT_480P:
		return (TV_SYSTEM_525);
	case GXAV_VOUT_576I:
	case GXAV_VOUT_576P:
		return (TV_SYSTEM_625);
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080I_60HZ:
	case GXAV_VOUT_1080P_60HZ:
		return (TV_SYSTEM_709_6);
	default:
		return (TV_SYSTEM_709_6);
	}
}

static int _get_colorimetry_index(int colorimetry)
{
	int ret = 0;

	switch(colorimetry) {
		case ITU_R_BT_709_6:     return (0);
		case ITU_R_BT_470_6_M:   return (1);
		case ITU_R_BT_601_7_625: return (2);
		case ITU_R_BT_601_7_525: return (3);
		case SMPTE_ST_240:       return (4);
		case GENERIC_FILM:       return (5);
		case ITU_R_BT_2020_2:    return (6);
		case SMPTE_ST_428_1:     return (7);
		case SMPTE_RP_431_2:     return (8);
		case SMPTE_EG_432_1:     return (9);
		case EBU_TECH_3213_E:    return (10);
		default:                 return (11);
	}
}

typedef struct {
	int cscCtrl0;
	int cscCtrl1;
	int cscCtrl2;
	int cscCtrl3;
} CSC_Param_t;

CSC_Param_t sCSCCtrlConfig[3][12] = {
	//tv format 480i/480p
	{
		/* colour_primaries = 1 */
		/* Video Colour Primaries is ITU_R_BT_709_6    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040800,
			.cscCtrl2 = 0x000ffd15,
			.cscCtrl3 = 0x000ffff6,
		},
		/* colour_primaries = 4 */
		/* Video Colour Primaries is ITU_R_BT_470_6_M  */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00e48bff,
			.cscCtrl2 = 0x009feda2,
			.cscCtrl3 = 0x000fbfb0,
		},
		/* colour_primaries = 5 */
		/* Video Colour Primaries is ITU_R_BT_601_7_625*/
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003ffff,
			.cscCtrl2 = 0x00000121,
			.cscCtrl3 = 0x000007ef,
		},
		/* colour_primaries = 6 */
		/* Video Colour Primaries is ITU_R_BT_601_7_525*/
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
		/* colour_primaries = 7 */
		/* Video Colour Primaries is SMPTE_ST_240      */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
		/* colour_primaries = 8 */
		/* Video Colour Primaries is GenericFilm       */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00e4d000,
			.cscCtrl2 = 0x00901584,
			.cscCtrl3 = 0x000f97bb,
		},
		/* colour_primaries = 9 */
		/* Video Colour Primaries is ITU_R_BT_2020_2   */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00049c04,
			.cscCtrl2 = 0x000fddea,
			.cscCtrl3 = 0x000fab8f,
		},
		/* colour_primaries = 10 */
		/* Video Colour Primaries is SMPTE_ST_428_1    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x80000000,
			.cscCtrl1 = 0x80000000,
			.cscCtrl2 = 0x80000000,
			.cscCtrl3 = 0x80000000,
		},
		/* colour_primaries = 11 */
		/* Video Colour Primaries is SMPTE_RP_431_2    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x3ec3e801,
			.cscCtrl2 = 0x3ec0194a,
			.cscCtrl3 = 0x000013d9,
		},
		/* colour_primaries = 12 */
		/* Video Colour Primaries is SMPTE_EG_432_1    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00047001,
			.cscCtrl2 = 0x00001d60,
			.cscCtrl3 = 0x000fc7cc,
		},
		/* colour_primaries = 22 */
		/* Video Colour Primaries is EBU_TECH_3213_E   */
		/* PP output Colour Primaries is ITU_R_BT_601_7_525*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003f400,
			.cscCtrl2 = 0x00000117,
			.cscCtrl3 = 0x00000bf4,
		},
		/*default parametre*/
		{
			.cscCtrl0 = 0x10000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
	},
	//tv format 576i/576p
	{
		/* colour_primaries = 1 */
		/* Video Colour Primaries is ITU_R_BT_709_6    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040c01,
			.cscCtrl2 = 0x000ffcf5,
			.cscCtrl3 = 0x000ff806,
		},
		/* colour_primaries = 4 */
		/* Video Colour Primaries is ITU_R_BT_470_6_M  */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00e48c00,
			.cscCtrl2 = 0x008ff172,
			.cscCtrl3 = 0x000fbbc8,
		},
		/* colour_primaries = 5 */
		/* Video Colour Primaries is ITU_R_BT_601_7_625*/
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
		/* colour_primaries = 6 */
		/* Video Colour Primaries is ITU_R_BT_601_7_525*/
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040401,
			.cscCtrl2 = 0x000000e2,
			.cscCtrl3 = 0x000ffc0f,
		},
		/* colour_primaries = 7 */
		/* Video Colour Primaries is SMPTE_ST_240      */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040401,
			.cscCtrl2 = 0x000000e2,
			.cscCtrl3 = 0x000ffc0f,
		},
		/* colour_primaries = 8 */
		/* Video Colour Primaries is GenericFilm       */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00e4d401,
			.cscCtrl2 = 0x00801157,
			.cscCtrl3 = 0x000f93d2,
		},
		/* colour_primaries = 9 */
		/* Video Colour Primaries is ITU_R_BT_2020_2   */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0004a006,
			.cscCtrl2 = 0x000fe1b1,
			.cscCtrl3 = 0x000fa7ab,
		},
		/* colour_primaries = 10 */
		/* Video Colour Primaries is SMPTE_ST_428_1    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x80000000,
			.cscCtrl1 = 0x80000000,
			.cscCtrl2 = 0x80000000,
			.cscCtrl3 = 0x80000000,
		},
		/* colour_primaries = 11 */
		/* Video Colour Primaries is SMPTE_RP_431_2    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x3ec3e802,
			.cscCtrl2 = 0x3ee01524,
			.cscCtrl3 = 0x00000fec,
		},
		/* colour_primaries = 12 */
		/* Video Colour Primaries is SMPTE_EG_432_1    */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00047402,
			.cscCtrl2 = 0x00001938,
			.cscCtrl3 = 0x000fc3e1,
		},
		/* colour_primaries = 22 */
		/* Video Colour Primaries is EBU_TECH_3213_E   */
		/* PP output Colour Primaries is ITU_R_BT_601_7_625*/
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003f401,
			.cscCtrl2 = 0x000000f7,
			.cscCtrl3 = 0x00000404,
		},
		/*default parametre*/
		{
			.cscCtrl0 = 0x10000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
	},
	//tv format 720p50/720p60/1080i50/1080i60/1080p50/1080p60
	{
		/* colour_primaries = 1 */
		/* Video Colour Primaries is ITU_R_BT_709_6    */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
		/* colour_primaries = 4 */
		/* Video Colour Primaries is ITU_R_BT_470_6_M  */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00e47fff,
			.cscCtrl2 = 0x008ff583,
			.cscCtrl3 = 0x000fc3bf,
		},
		/* colour_primaries = 5 */
		/* Video Colour Primaries is ITU_R_BT_601_7_625*/
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003f7ff,
			.cscCtrl2 = 0x0000050c,
			.cscCtrl3 = 0x00000bfa,
		},
		/* colour_primaries = 6 */
		/* Video Colour Primaries is ITU_R_BT_601_7_525*/
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003f800,
			.cscCtrl2 = 0x000004ed,
			.cscCtrl3 = 0x00000409,
		},
		/* colour_primaries = 7 */
		/* Video Colour Primaries is SMPTE_ST_240      */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003f800,
			.cscCtrl2 = 0x000004ed,
			.cscCtrl3 = 0x00000409,
		},
		/* colour_primaries = 8 */
		/* Video Colour Primaries is GenericFilm       */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00e4c400,
			.cscCtrl2 = 0x00801967,
			.cscCtrl3 = 0x000f9bc9,
		},
		/* colour_primaries = 9 */
		/* Video Colour Primaries is ITU_R_BT_2020_2   */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00049404,
			.cscCtrl2 = 0x000fe5c5,
			.cscCtrl3 = 0x000fafa1,
		},
		/* colour_primaries = 10 */
		/* Video Colour Primaries is SMPTE_ST_428_1    */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x80000000,
			.cscCtrl1 = 0x80000000,
			.cscCtrl2 = 0x80000000,
			.cscCtrl3 = 0x80000000,
		},
		/* colour_primaries = 11 */
		/* Video Colour Primaries is SMPTE_RP_431_2    */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x3ec3dc01,
			.cscCtrl2 = 0x3ed01d32,
			.cscCtrl3 = 0x000017e5,
		},
		/* colour_primaries = 12 */
		/* Video Colour Primaries is SMPTE_EG_432_1    */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x00046401,
			.cscCtrl2 = 0x00002146,
			.cscCtrl3 = 0x000fcfd9,
		},
		/* colour_primaries = 22 */
		/* Video Colour Primaries is EBU_TECH_3213_E   */
		/* PP output Colour Primaries is ITU_R_BT_709_6    */
		{
			.cscCtrl0 = 0x90000000,
			.cscCtrl1 = 0x0003e800,
			.cscCtrl2 = 0x00000502,
			.cscCtrl3 = 0x00000ffe,
		},
		/*default parametre*/
		{
			.cscCtrl0 = 0x10000000,
			.cscCtrl1 = 0x00040000,
			.cscCtrl2 = 0x00000100,
			.cscCtrl3 = 0x00000000,
		},
	}
};

static void gx3211_vpp_set_csc(int colorimetry)
{
	struct vout_dvemode dvemode;
	TVColorType_t colortype;
	int index = 0;
	CSC_Param_t csc_para = {0};

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	colortype = _get_tv_color_type(dvemode.mode);
	index = _get_colorimetry_index(colorimetry);

	csc_para = sCSCCtrlConfig[colortype][index];
	REG_SET_VAL(&(gx3211vpu_reg->rPPCSC_CTRL[0]), csc_para.cscCtrl0);
	REG_SET_VAL(&(gx3211vpu_reg->rPPCSC_CTRL[1]), csc_para.cscCtrl1);
	REG_SET_VAL(&(gx3211vpu_reg->rPPCSC_CTRL[2]), csc_para.cscCtrl2);
	REG_SET_VAL(&(gx3211vpu_reg->rPPCSC_CTRL[3]), csc_para.cscCtrl3);
}

int gx3211_vpp_play_frame(struct frame *frame)
{
	int ret = -1;
	int rdptr= gx3211_vpp_get_rdptr();
	int playmode_changed = 0;
	static int last_topfirst = 1;
	unsigned offset_y = 0, offset_cbcr = 0;
	DispControler *controler = NULL;

	if (VPU_CLOSED())
		return -1;
	controler = &((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->controler;

	if (frame) {
		//updata frame size
		if (frame->w && frame->h)
			gx3211_vpp_set_framesize(frame->w, frame->h);

		if (gx3211vpu_info->layer[GX_LAYER_VPP].surface)
			gx3211vpu_info->layer[GX_LAYER_VPP].surface->surface_mode = (frame->is_image ? GX_SURFACE_MODE_IMAGE : GX_SURFACE_MODE_VIDEO);

		//update window
		if (controler->zoom_require) {
			gx3211_vpp_get_actual_cliprect(&controler->clip_rect);
			gx3211_vpp_get_actual_viewrect(&controler->view_rect);
		}

		//check same frame
		if (gx_memcmp(frame, &controler->last_frame, sizeof(struct frame)) == 0)
			if (controler->zoom_require == 0)
				goto out;

		if (frame->topfirst != last_topfirst)
			gx_printf("last = %d, cur = %d\n", last_topfirst, frame->topfirst);

		//get disp unit id
		if (controler->buf_to_wr != rdptr) {
			gx_printf("\nrd = %d, wr = %d\n", rdptr, controler->buf_to_wr);
			if (frame->is_freeze == 0)
				goto out;
		}

		if (controler->view_rect.height < controler->clip_rect.height && frame->playmode == PLAY_MODE_Y_FRAME_UV_FIELD)
			frame->playmode = PLAY_MODE_FRAME;

		//config disp unit
		offset_y    = controler->clip_rect.x +   frame->baseline * controler->clip_rect.y;
		offset_cbcr = controler->clip_rect.x + ((frame->baseline * controler->clip_rect.y)>>1);
		if (frame->playmode == PLAY_MODE_FIELD) {
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame->playmode,  frame->topfirst);
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame->playmode, !frame->topfirst);
			((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->play_mode = PLAY_MODE_FIELD;
		} else {
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame->playmode,  frame->topfirst);
			((Gx3211VpuVppPriv*)gx3211vpu_info->layer[GX_LAYER_VPP].priv)->play_mode = PLAY_MODE_FRAME;
		}

		ret = 0;
		gx3211vpu_info->reset_flag = 0;
		playmode_changed = (controler->last_frame.playmode != frame->playmode);
		last_topfirst = frame->topfirst;
		controler->last_frame = *frame;
		if (frame->is_freeze == 1)
			controler->zoom_require += 3;

		//rezoom
		if (controler->zoom_require || playmode_changed) {
			VPU_VPP_SET_BASE_LINE(gx3211vpu_reg->rPP_FRAME_STRIDE, frame->baseline);
			gx3211_vpp_view_port(&controler->clip_rect, &controler->view_rect);
			controler->zoom_require--;
		}
		if((gxcore_chip_probe() == GXAV_ID_GX6605S) &&
		   (gxcore_chip_sub_probe() == 1)) {
			gx3211_vpp_set_csc(frame->colorimetry);
		}
	}

out:
	return ret;
}

int gx3211_vpp_main_surface(Gx3211VpuSurface *surface)
{
	unsigned int base_line = 0;
	struct frame frame = {0};

	surface->bg_color.y  = 0x10;
	surface->bg_color.cb = 0x80;
	surface->bg_color.cr = 0x80;

	gx_printf("\nvpp main surface start\n");
	if (GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
		if(GX_COLOR_FMT_YCBCR420_Y_UV== surface->color_format) {

			gx_dcache_clean_range(0, 0);
			gx3211_vpp_set_framesize(surface->width, surface->height);
			VPU_VPP_DISP_CTRL_RESET_CLR(gx3211vpu_reg->rPP_DISP_CTRL);
			VPU_VPP_SET_PP_SOURCE_SIZE(gx3211vpu_reg->rPP_SOURCE_SIZE, surface->width, surface->height);
			base_line = surface->width;
			VPU_VPP_SET_BASE_LINE(gx3211vpu_reg->rPP_FRAME_STRIDE, base_line);

			frame.w = surface->width;
			frame.h = surface->height;
			frame.baseline  = surface->width;
			frame.topfirst  = 1;
			frame.is_image  = 1;
			frame.is_freeze = 1;
			frame.playmode  = PLAY_MODE_FRAME;
			frame.top.addry    = gx_virt_to_phys((unsigned int)surface->buffer);
			frame.top.addrcbcr = frame.top.addry + base_line * surface->height;
			frame.bot.addry    = frame.top.addry + base_line;
			frame.bot.addrcbcr = frame.bot.addry + base_line * surface->height;
			gx3211_vpp_play_frame(&frame);
		}
	}
	gx_printf("\nvpp main surface end\n");

	return 0;
}

int gx3211_vpp_set_stream_ratio(unsigned int ratio)
{
	Gx3211VpuVppPriv *info;

	if(gx3211vpu_info && gx3211vpu_info->layer[GX_LAYER_VPP].priv) {
		info = gx3211vpu_info->layer[GX_LAYER_VPP].priv;
		if(ratio != info->stream_ratio) {
			info->stream_ratio = ratio;
			return gx3211_vpp_zoom_require();
		}
		return 0;
	}
	else
		return -1;
}

int gx3211_vpp_set_framesize(unsigned int width, unsigned int height)
{
	int ret = -1;
	Gx3211VpuVppPriv *info;

	if (gx3211vpu_info && gx3211vpu_info->layer[GX_LAYER_VPP].priv) {
		info = gx3211vpu_info->layer[GX_LAYER_VPP].priv;
		if(info->frame_width!=width || info->frame_height!=height) {
			info->frame_width = width;
			info->frame_height= height;
			ret = gx3211_vpp_zoom_require();
		}
	}

	return ret;
}

int gx3211_vpp_set_playmode(GxVpuLayerPlayMode mode)
{
	int ret = -1;
	Gx3211VpuVppPriv *info;

	if (gx3211vpu_info && gx3211vpu_info->layer[GX_LAYER_VPP].priv) {
		info = gx3211vpu_info->layer[GX_LAYER_VPP].priv;
		info->play_mode = mode;

		if (gx3211vpu_info->layer[GX_LAYER_VPP].surface)
			gx3211vpu_info->layer[GX_LAYER_VPP].surface->surface_mode = GX_SURFACE_MODE_VIDEO;
		ret = 0;
	}

	return ret;
}

int gx3211_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	GxAvRect clip_rect;

	if(VPU_CLOSED())
		return 0;

	Gx3211Layer_Get_ActualViewrect(GX_LAYER_VPP,  view_rect);
	Gx3211Layer_Get_ActualCliprect(GX_LAYER_VPP, &clip_rect);
	Gx3211Vpu_AspectRatio_Adapt(GX_LAYER_VPP, &clip_rect, view_rect);
	VIEWRECT_ALIGN(*view_rect);
	return 0;
}

int gx3211_vpp_get_actual_cliprect(GxAvRect *clip_rect)
{
	GxAvRect view_rect;

	if(VPU_CLOSED())
		return 0;

	Gx3211Layer_Get_ActualViewrect(GX_LAYER_VPP,  &view_rect);
	Gx3211Layer_Get_ActualCliprect(GX_LAYER_VPP, clip_rect);
	Gx3211Vpu_AspectRatio_Adapt(GX_LAYER_VPP, clip_rect, &view_rect);
	CLIPRECT_ALIGN(*clip_rect);
	return 0;
}

int gx3211_vpp_set_actual_cliprect(GxAvRect *clip_rect)
{

	if(VPU_CLOSED())
		return 0;

	Gx3211Layer_Set_ActualCliprect(GX_LAYER_VPP, clip_rect);
	return 0;
}

int gx3211_spp_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
#define SPP_GET_ZOOM_PARA(para,clip_size,view_size,down_scale) \
	do { \
		(para) = ((((clip_size) >> (down_scale)) - 1) << VPU_SPP_ZOOM_WIDTH) / (view_size - 1); \
	}while (0)

	int vtbias = 0, vbbias = 0;
	int i = 0, vzoom = 0, hzoom = 0;
	int vdown_scale_en = 0, hdown_scale_en = 0;
	unsigned int req_width = 0, req_height = 0;
	unsigned int view_width = 0, view_height =0;
	struct vout_dvemode dvemode;

	int play_mode = VPU_SPP_PLAY_MODE_FRAME;
	view_width  = view_rect->width;
	view_height = view_rect->height;
	req_width   = clip_rect->width;
	req_height  = clip_rect->height;

	SPP_GET_ZOOM_PARA(hzoom, clip_rect->width, view_rect->width, hdown_scale_en);
	if(VPU_SPP_ZOOM_OUT_MAX < hzoom) {
		req_width >>= 1;
		hdown_scale_en = 1;
		SPP_GET_ZOOM_PARA(hzoom, clip_rect->width, view_rect->width, hdown_scale_en);
		if(VPU_SPP_ZOOM_OUT_MAX < hzoom)
			return -1;
	}
	SPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_rect->height, vdown_scale_en);

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if(IS_INTERLACE_MODE(dvemode.mode)) {
		if(VPU_SPP_PLAY_MODE_FRAME == play_mode) {
			if((VPU_SPP_ZOOM_OUT_MAX >> 1) < vzoom) {
				vdown_scale_en = 1;
				SPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_rect->height, vdown_scale_en);
				if((VPU_SPP_ZOOM_OUT_MAX >> 1) < vzoom)
					return -1;
			}
			vtbias = 0;
			vbbias =vzoom;
		}
		else {
			if(VPU_SPP_ZOOM_OUT_MAX < vzoom) {
				vdown_scale_en = 1;
				SPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_rect->height, vdown_scale_en);
				if(VPU_SPP_ZOOM_OUT_MAX < vzoom)
					return -1;
			}

			if(VPU_SPP_ZOOM_NONE < vzoom) {
				vtbias = 0;
				vbbias = (vzoom >> 1) - 2048;
			}
			else {
				vtbias = 2048 - (vzoom >> 1);
				vbbias = 0;
			}
		}
	}
	else if(IS_PROGRESSIVE_MODE(dvemode.mode)) {
		if(VPU_SPP_ZOOM_OUT_MAX < vzoom) {
			vdown_scale_en = 1;
			SPP_GET_ZOOM_PARA(vzoom, clip_rect->height, view_rect->height, vdown_scale_en);
			if(VPU_SPP_ZOOM_OUT_MAX < vzoom)
				return -1;
		}

		if(VPU_SPP_PLAY_MODE_FRAME == play_mode) {
			vtbias = 0;
			vbbias = 0;
		}
		else {
			vtbias = 2048;
			vbbias = 0;
		}
	}
	else
		return -1;

	if(hdown_scale_en)
		req_width = (req_width >> 1) << 1;
	else
		req_width = (req_width >> 2) << 2;

	req_width = req_width < 240 ? 240 : req_width;

	VPU_SPP_SET_PLAY_MODE(gx3211vpu_reg->rPIC_PARA, play_mode);
	VPU_SPP_SET_PIC_POSITION(gx3211vpu_reg->rPIC_POSITION, view_rect->x, view_rect->y);
	VPU_SPP_SET_PIC_SIZE(gx3211vpu_reg->rPIC_VIEW_SIZE, view_width, view_height);
	VPU_SPP_SET_ZOOM(gx3211vpu_reg->rPIC_ZOOM, vzoom ,hzoom );
	VPU_SPP_SET_PIC_SOURCE_SIZE(gx3211vpu_reg->rPIC_SOURCE_SIZE, req_width, req_height);
	VPU_SPP_SET_VT_PHASE(gx3211vpu_reg->rPIC_V_PHASE, vtbias & 0xFF);
	VPU_SPP_SET_VB_PHASE(gx3211vpu_reg->rPIC_V_PHASE, vbbias);

	if(vdown_scale_en)
		VPU_SPP_V_DOWNSCALE_ENABLE(gx3211vpu_reg->rPIC_CTRL);
	else
		VPU_SPP_V_DOWNSCALE_DISABLE(gx3211vpu_reg->rPIC_CTRL);

	if(hdown_scale_en)
		VPU_SPP_H_DOWNSCALE_ENABLE(gx3211vpu_reg->rPIC_CTRL);
	else
		VPU_SPP_H_DOWNSCALE_DISABLE(gx3211vpu_reg->rPIC_CTRL);

	gx3211vpu_reg->rPIC_FILTER_SIGN = 0x998080;
	if(hzoom == 4096) {
		VPU_SPP_SET_SPP_PHASE_0_H(gx3211vpu_reg->rPIC_PHASE_0_H, 0x00ff0000);
	}
	else {
		VPU_SPP_SET_SPP_PHASE_0_H(gx3211vpu_reg->rPIC_PHASE_0_H, 0x00ff0100);
	}
	if(vzoom == 4096) {
		VPU_SPP_SET_SPP_PHASE_0_V(gx3211vpu_reg->rPIC_PHASE_0_V, 0x00ff0000);
	}
	else {
		VPU_SPP_SET_SPP_PHASE_0_V(gx3211vpu_reg->rPIC_PHASE_0_V, 0x00ff0100);
	}

	if(VPU_SPP_ZOOM_NONE < vzoom) {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( gx3211vpu_reg->rPIC_PHASE_PARA_V,i,sPhaseNomal[i]);
	}
	else {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( gx3211vpu_reg->rPIC_PHASE_PARA_V,i,sPhaseNomal[i]);
	}

	if(VPU_SPP_ZOOM_NONE < hzoom) {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( gx3211vpu_reg->rPIC_PHASE_PARA_H,i,sPhaseNomal[i]);
	}
	else {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( gx3211vpu_reg->rPIC_PHASE_PARA_H,i,sPhaseNomal[i]);
	}

	return 0;
}

int gx3211_spp_enable(int enable)
{
	if (gx3211vpu_info == NULL)
		return 0;

	if(enable == 0) {
		VPU_SPP_DISABLE(gx3211vpu_reg->rPIC_CTRL);
	}
	else {
		VPU_SPP_ENABLE(gx3211vpu_reg->rPIC_CTRL);
	}

	return 0;
}

int gx3211_spp_alpha(GxAlpha alpha)
{
	VPU_SPP_SET_MIXWEIGHT(gx3211vpu_reg->rMIX_CTRL2, alpha.value);
	return 0;
}

int gx3211_spp_on_top(int enable)
{
	if(enable == 0)
		VPU_SPP_ON_BOTTOM(gx3211vpu_reg->rMIX_CTRL2);
	else
		VPU_SPP_ON_TOP(gx3211vpu_reg->rMIX_CTRL2);

	return 0;
}

int gx3211_spp_video_mode(GxLayerVideoMode mode)
{
	return 0;
}

int gx3211_spp_color_format(GxColorFormat format)
{
	ByteSequence byteseq = ABCD_EFGH;

	switch (format) {
	case GX_COLOR_FMT_YCBCRA6442:
		VPU_SPP_SET_COLOR_TYPE(gx3211vpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_6442_YCBCRA);
		byteseq = DCBA_HGFE;
		VPU_SPP_SET_DATA_RW_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);
		break;
	case GX_COLOR_FMT_YCBCR422:
		VPU_SPP_SET_COLOR_TYPE(gx3211vpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_422_CBYCRY);
		byteseq = DCBA_HGFE;
		VPU_SPP_SET_DATA_RW_BYTESEQ(gx3211vpu_reg->rSYS_PARA, byteseq);
		break;
	case GX_COLOR_FMT_YCBCR422_Y_UV:
		VPU_SPP_SET_COLOR_TYPE(gx3211vpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_422_Y_CBCR);
		break;
	case GX_COLOR_FMT_YCBCR420:
		VPU_SPP_SET_COLOR_TYPE(gx3211vpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_420);
		break;
	default:
		return -1;
	}

	return 0;
}

int gx3211_spp_bg_color(GxColor color)
{
	VPU_SPP_SET_BG_COLOR(gx3211vpu_reg->rPIC_BACK_COLOR, color.y, color.cb, color.cr);
	return 0;
}

int gx3211_spp_main_surface(Gx3211VpuSurface *surface)
{
	if(GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
		int base_line = 0;
		unsigned int data = 0;
		GxAvRect rect = {0, 0, 0, 0};

		unsigned int request_block = 0;
		request_block = rect.width/4/128*128;
		if(request_block < 128) {
			request_block = 128;
		}
		else if(request_block > 512) {
			request_block = 512;
		}

		REG_SET_FIELD(&(gx3211vpu_reg->rPIC_PARA),0x7FF<<16,request_block,16);
		gx3211_spp_color_format(surface->color_format);
		rect.width  = surface->width;
		rect.height = surface->height;

		if( GX_COLOR_FMT_YCBCR422_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420      == surface->color_format )
			base_line = rect.width;
		else
			base_line = rect.width *gx_color_get_bpp(surface->color_format) >> 3;

		VPU_SPP_SET_BASE_LINE(gx3211vpu_reg->rPIC_PARA, base_line);
		data = (unsigned int)surface->buffer;
		VPU_SPP_SET_Y_TOP_ADDR(gx3211vpu_reg->rPIC_Y_TOP_ADDR, data);
		VPU_SPP_SET_Y_BOTTOM_ADDR(gx3211vpu_reg->rPIC_Y_BOTTOM_ADDR, data + base_line);

		if( GX_COLOR_FMT_YCBCR422_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420      == surface->color_format ) {
			data = data + base_line *rect.height;
			VPU_SPP_SET_UV_TOP_ADDR(gx3211vpu_reg->rPIC_UV_TOP_ADDR, data);
			VPU_SPP_SET_UV_BOTTOM_ADDR(gx3211vpu_reg->rPIC_UV_BOTTOM_ADDR, data + base_line);
		}
		return 0;
	}

	return -1;
}

int gx3211_bkg_bg_color(GxColor color)
{
	VPU_BKG_SET_COLOR(gx3211vpu_reg->rMIX_CTRL2, color.y, color.cb, color.cr);
	return 0;
}

int gx3211_bkg_main_surface(Gx3211VpuSurface *surface)
{
	gx3211_bkg_bg_color(surface->bg_color);
	return 0;
}

unsigned int gx3211_vpu_get_scan_line(void)
{
	return VPU_DISP_GET_VIEW_ACTIVE_CNT(gx3211vpu_reg->rSCAN_LINE);
}

unsigned int gx3211_vpu_check_layers_close(void)
{
	return REG_GET_VAL(&(gx3211vpu_reg->rSYS_PARA))&0x01;
}

Gx3211VpuLayerOps gx3211_osd_ops = {
	.set_view_port    = gx3211_osd_zoom,
	.set_main_surface = gx3211_osd_main_surface,
	.set_pan_display  = gx3211_osd_pan_display,
	.set_enable       = gx3211_osd_enable,
	.set_anti_flicker = gx3211_osd_anti_flicker,
	.set_palette      = gx3211_osd_palette,
	.set_alpha        = gx3211_osd_alpha,
	.set_color_format = gx3211_osd_color_format,
	.set_color_key    = gx3211_osd_color_key,
	.set_on_top       = gx3211_osd_on_top,

	.add_region       = gx3211_osd_add_region,
	.remove_region    = gx3211_osd_remove_region,
	.update_region    = gx3211_osd_update_region,
};

Gx3211VpuLayerOps gx3211_sosd_ops = {
	.set_view_port    = gx3211sosd_zoom,
	.set_main_surface = gx3211sosd_main_surface,
	.set_enable       = gx3211sosd_enable,
	.set_anti_flicker = gx3211sosd_anti_flicker,
	.set_alpha        = gx3211sosd_alpha,
	.set_color_format = gx3211sosd_color_format,
	.set_color_key    = gx3211sosd_color_key,
	.set_on_top       = gx3211sosd_on_top,
};

Gx3211VpuLayerOps gx3211_vpp_ops = {
	.set_view_port    = gx3211_vpp_view_port,
	.set_main_surface = gx3211_vpp_main_surface,
	.set_enable       = gx3211_vpp_enable,
	.set_anti_flicker = gx3211_vpp_blur,
	.set_video_mode   = gx3211_vpp_video_mode,
	.set_bg_color     = gx3211_vpp_bg_color,
};

Gx3211VpuLayerOps gx3211_spp_ops = {
	.set_view_port    = gx3211_spp_zoom,
	.set_main_surface = gx3211_spp_main_surface,
	.set_enable       = gx3211_spp_enable,
	.set_alpha        = gx3211_spp_alpha,
	.set_on_top       = gx3211_spp_on_top,
	.set_video_mode   = gx3211_spp_video_mode,
	.set_color_format = gx3211_spp_color_format,
	.set_bg_color     = gx3211_spp_bg_color,
};

Gx3211VpuLayerOps gx3211_bkg_ops = {
	.set_main_surface = gx3211_bkg_main_surface,
	.set_bg_color     = gx3211_bkg_bg_color,
};

