#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "cygnus_vpu.h"
#include "vpu_color.h"
#include "vout_hal.h"
#include "porting.h"
#include "video.h"
#include "hdmi_hal.h"
#include "cygnus_vpu_internel.h"
#include "hdr.h"

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

#define VPU_CLOSED() (cygnusvpu_info == NULL)

struct osd_region** cygnus_get_region_head_by_layer_id(GxLayerID layer_id)
{
	struct osd_region **region_head = NULL;

	switch (layer_id) {
		case GX_LAYER_OSD : {
			region_head = (struct osd_region **)&(((CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_OSD].priv)->region_head);
			break;
		}
		case GX_LAYER_SOSD : {
			region_head = (struct osd_region **)&(((CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_SOSD].priv)->region_head);
			break;
		}
		case GX_LAYER_SOSD2 : {
			region_head = (struct osd_region **)&(((CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_SOSD2].priv)->region_head);
			break;
		}
		default : {
			break;
		}
	}

	return region_head;
}

static int CygnusVpu_LetterBox(GxLayerID layer, int aspect, GxAvRect *view_rect)
{
	int full_screen;
	GxVpuProperty_Resolution actual_resolution;

	cygnus_vpu_GetActualResolution(&actual_resolution);
	if(actual_resolution.xres==view_rect->width && actual_resolution.yres==view_rect->height)
		full_screen = 1;
	else
		full_screen = 0;

	if(full_screen) {
		if(cygnusvpu_info->layer[layer].screen == SCREEN_16X9) {
			if(aspect == ASPECTRATIO_4BY3) {
				int fix_width = view_rect->width*3/4;
				view_rect->x       = view_rect->x + ((view_rect->width - fix_width) >> 1);
				view_rect->width   = fix_width;
			}
		}
		else if(cygnusvpu_info->layer[layer].screen == SCREEN_4X3) {
			if(aspect == ASPECTRATIO_16BY9) {
				int fix_height = (view_rect->height*3)/4;
				view_rect->y = view_rect->y + ((view_rect->height - fix_height) >> 1);
				view_rect->height  = fix_height;
			}
		}
	}
	else {
		int ah = 0, aw = 0;
		if(cygnusvpu_info->layer[layer].screen == SCREEN_16X9) {
			if(aspect == ASPECTRATIO_4BY3) {
				aw = 4 * actual_resolution.xres /16;
				ah = 3 * actual_resolution.yres /9;
			}
		}
		else if(cygnusvpu_info->layer[layer].screen == SCREEN_4X3) {
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

static int CygnusVpu_PanScan(GxLayerID layer, int aspect, GxAvRect *clip_rect)
{
	if(cygnusvpu_info->layer[layer].screen == SCREEN_16X9) {
		if(aspect == ASPECTRATIO_4BY3) {
			int i = (clip_rect->height*3)>>2;
			clip_rect->y      += ((clip_rect->height - i) >> 1);
			clip_rect->height  = i;
		}
	}
	else if(cygnusvpu_info->layer[layer].screen == SCREEN_4X3) {
		if(aspect == ASPECTRATIO_16BY9) {
			clip_rect->x = (clip_rect->width >> 3);
			clip_rect->width -= (clip_rect->x << 1);
		}
	}

	return 0;
}

static int CygnusVpu_RawSize(GxAvRect *clip_rect, GxAvRect *view_rect)
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

static int CygnusVpu_RawRatio(GxAvRect *clip_rect, GxAvRect *view_rect)
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

static int CygnusVpu_4X3PullDown(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_4BY3) {
		CygnusVpu_RawRatio(clip_rect,view_rect);
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
		CygnusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int CygnusVpu_4X3CutOut(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_4BY3) {
		CygnusVpu_RawRatio(clip_rect,view_rect);
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
		CygnusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int CygnusVpu_16X9PullDown(GxLayerID layer, int aspect,GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_16BY9) {
		CygnusVpu_RawRatio(clip_rect,view_rect);
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
		CygnusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int CygnusVpu_16X9CutOut(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_16BY9) {
		CygnusVpu_RawRatio(clip_rect,view_rect);
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
		CygnusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int CygnusVpu_4X3(GxLayerID layer, int aspect,GxAvRect *clip_rect, GxAvRect *view_rect)
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

static int CygnusVpu_16X9(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return 0;
}

void CygnusVpu_GetVppPort(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	GxVpuProperty_Resolution referance;
	GxVpuProperty_Resolution virtual_resolution;

	cygnus_vpu_GetVirtualResolution(&virtual_resolution);
	if(VPU_CLOSED())
	{
		clip_rect->x = 0;
		clip_rect->y = 0;
		clip_rect->width = 720;
		clip_rect->height = 576;
		return;
	}

	cygnus_vpu_GetActualResolution(&referance);
	view_rect->x      = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.x, referance.xres, 1);
	view_rect->y      = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.y, referance.yres, 0);
	view_rect->width  = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.width,  referance.xres, 1);
	view_rect->height = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.height, referance.yres, 0);

	referance.xres    = ((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->frame_width;
	referance.yres    = ((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->frame_height;
	clip_rect->x      = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect.x, referance.xres, 1);
	clip_rect->y      = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect.y, referance.yres, 0);
	clip_rect->width  = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect.width,  referance.xres, 1);
	clip_rect->height = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect.height, referance.yres, 0);
}

int CygnusVpu_AspectRatio_Adapt(GxLayerID layer, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int aspect = (cygnusvpu_info&&cygnusvpu_info->layer[GX_LAYER_VPP].priv ? ((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->stream_ratio : -1);

	if (aspect != -1 && (layer == GX_LAYER_VPP || layer == GX_LAYER_SPP) && \
			(cygnusvpu_info->layer[layer].surface) && \
			cygnusvpu_info->layer[layer].surface->surface_mode == GX_SURFACE_MODE_VIDEO) {
		switch(cygnusvpu_info->layer[layer].spec)
		{
		case VP_LETTER_BOX:
			CygnusVpu_LetterBox(layer, aspect, view_rect);
			break;
		case VP_PAN_SCAN:
			CygnusVpu_PanScan(layer, aspect, clip_rect);
			break;
		case VP_COMBINED:
			if(aspect == ASPECTRATIO_4BY3) {
				CygnusVpu_LetterBox(layer, aspect, view_rect);
				CygnusVpu_PanScan(layer, aspect, clip_rect);
			} else if(aspect == ASPECTRATIO_16BY9) {
				CygnusVpu_PanScan(layer, aspect, clip_rect);
				CygnusVpu_LetterBox(layer, aspect, view_rect);
			}
			break;
		case VP_RAW_SIZE:   //原始大小
			CygnusVpu_RawSize(clip_rect, view_rect);
			break;
		case VP_RAW_RATIO:  //原始比例
			CygnusVpu_RawRatio(clip_rect, view_rect);
			break;
		case VP_4X3_PULL:   //4:3拉伸
			CygnusVpu_4X3PullDown(layer, aspect, clip_rect, view_rect);
			break;
		case VP_4X3_CUT:    //4:3裁剪
			CygnusVpu_4X3CutOut(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9_PULL:  //16:9拉伸
			CygnusVpu_16X9PullDown(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9_CUT:   //16:9裁剪
			CygnusVpu_16X9CutOut(layer, aspect, clip_rect, view_rect);
			break;
		case VP_4X3:        //4:3
			CygnusVpu_4X3(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9:       //16:9
			CygnusVpu_16X9(layer, aspect, clip_rect, view_rect);
			break;
		case VP_AUTO:
			if(aspect == ASPECTRATIO_4BY3) {
				cygnusvpu_info->layer[layer].screen = SCREEN_16X9;
				CygnusVpu_PanScan(layer,aspect, clip_rect);
			}
			else if(aspect == ASPECTRATIO_16BY9) {
				cygnusvpu_info->layer[layer].screen = SCREEN_4X3;
				CygnusVpu_LetterBox(layer, aspect, clip_rect);
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

static int CygnusLayer_Get_ActualViewrect(GxLayerID layer, GxAvRect *view_rect)
{
	GxVpuProperty_Resolution referance;
	GxVpuProperty_Resolution virtual_resolution;

	if(VPU_CLOSED())
		return -1;

	cygnus_vpu_GetActualResolution(&referance);
	cygnus_vpu_GetVirtualResolution(&virtual_resolution);
	view_rect->x     = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].view_rect.x,	referance.xres, 1);
	view_rect->y     = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].view_rect.y, referance.yres, 0);
	view_rect->width = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].view_rect.width,  referance.xres, 1);
	view_rect->height= cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].view_rect.height, referance.yres, 0);

	return 0;
}

static int CygnusLayer_Get_ActualCliprect(GxLayerID layer, GxAvRect *clip_rect)
{
	CygnusVpuVppPriv *info;
	GxVpuProperty_Resolution referance;

	if(layer==GX_LAYER_VPP) {
		if(cygnusvpu_info && cygnusvpu_info->layer[GX_LAYER_VPP].priv) {
			info = cygnusvpu_info->layer[GX_LAYER_VPP].priv;
			referance.xres	= info->frame_width;
			referance.yres  = info->frame_height;
			clip_rect->x	= cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].clip_rect.x, referance.xres, 1);
			clip_rect->y	= cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].clip_rect.y, referance.yres, 0);
			clip_rect->width = cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].clip_rect.width,  referance.xres, 1);
			clip_rect->height= cygnus_vpu_VirtualToActual(cygnusvpu_info->layer[layer].clip_rect.height, referance.yres, 0);
		}
		else {
			gx_printf("%s:%d, get actual cliprect failed\n", __FILE__, __LINE__);
			return -1;
		}
	}
	else {
		clip_rect->x     = cygnusvpu_info->layer[layer].clip_rect.x;
		clip_rect->y     = cygnusvpu_info->layer[layer].clip_rect.y;
		clip_rect->width = cygnusvpu_info->layer[layer].clip_rect.width;
		clip_rect->height= cygnusvpu_info->layer[layer].clip_rect.height;
	}

	return 0;
}

#define CLIPRECT_ALIGN(rect) \
	do{\
		(rect).x = ((rect).x>>3)<<3;\
		(rect).y = ((rect).y>>2)<<2;\
		(rect).width = ((rect).width>>3)<<3;\
		(rect).height= ((rect).height>>2)<<2;\
	}while(0)
#define VIEWRECT_ALIGN(rect) \
	do{\
		(rect).width  += ((rect).width&0x1);\
		(rect).height += ((rect).height&0x1);\
	}while(0)

int CygnusVpu_Zoom(GxLayerID layer)
{
	int ret = -1;
	unsigned long flags;
	GxAvRect clip_rect, view_rect;

	if(VPU_CLOSED())
		return 0;

	if (cygnusvpu_info && cygnusvpu_info->layer[layer].surface) {
		clip_rect = cygnusvpu_info->layer[layer].clip_rect;
		view_rect = cygnusvpu_info->layer[layer].view_rect;

		if(cygnusvpu_info->layer[layer].auto_zoom) {
			CygnusLayer_Get_ActualViewrect(layer, &view_rect);
			CygnusLayer_Get_ActualCliprect(layer, &clip_rect);
			CygnusVpu_AspectRatio_Adapt(layer, &clip_rect, &view_rect);
			VIEWRECT_ALIGN(view_rect);
			CLIPRECT_ALIGN(clip_rect);
		} else {
			clip_rect = view_rect;
		}

		VPU_SPIN_LOCK();
		if(cygnusvpu_info->layer[layer].ops->set_view_port) {
			ret = cygnusvpu_info->layer[layer].ops->set_view_port(&clip_rect, &view_rect);
		}
		VPU_SPIN_UNLOCK();
	}

	return ret;
}

#define OSD_MAINSURFACE_BUF_ADDR(layer_id) \
	(unsigned int)((CygnusVpuSurface*)cygnusvpu_info->layer[layer_id].surface)->buffer
#define OSD_MAINSURFACE_DATA_FORMAT(layer_id) \
	((CygnusVpuSurface*)cygnusvpu_info->layer[layer_id].surface)->color_format
#define OSD_MAINSURFACE_WIDTH(layer_id)\
	(((CygnusVpuSurface*)cygnusvpu_info->layer[layer_id].surface)->width)
static int osdx_zoom(GxLayerID layer_id, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int vzoom, hzoom;
	unsigned int data_addr = 0, data_offset = 0;
	unsigned int view_width=0, view_height = 0;
	unsigned int hzoom_gate = 0, i = 0;
	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution    actual_resolution;

	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);

	view_width      = view_rect->width;
	view_height     = view_rect->height;

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	cygnus_vpu_GetActualResolution(&actual_resolution);

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

		if((hzoom > 4096*4) || (vzoom > 4096*4))
			return -1;
		if(view_rect->x + view_rect->width > actual_resolution.xres)
			return -1;
		break;
	}
	default:
		gx_printf("%s(),unsupport tv out mode\n",__func__);
		return -1;
	}

	if (gx_color_get_bpp(OSD_MAINSURFACE_DATA_FORMAT(layer_id)) <= 8)
		hzoom_gate = 4096;
	else
		hzoom_gate = 8191;

	if (hzoom > hzoom_gate) {
		REG_SET_BIT(&(reg->rOSD_CTRL), b_OSD_H_DOWNSCALE);
		hzoom = (((clip_rect->width >> 1) - 1) << 12) / (view_rect->width - 1);
		if((((clip_rect->width >> 1) - 1) << 12) % (view_rect->width - 1))
			hzoom += 1;
	}
	else
		REG_CLR_BIT(&(reg->rOSD_CTRL), b_OSD_H_DOWNSCALE);

	data_addr  = gx_virt_to_phys(OSD_MAINSURFACE_BUF_ADDR(layer_id));
	data_offset= gx_color_get_bpp(OSD_MAINSURFACE_DATA_FORMAT(layer_id))*(clip_rect->y*OSD_MAINSURFACE_WIDTH(layer_id)+clip_rect->x)>>3;
	data_addr += data_offset;
//	VPU_OSD_SET_DATA_ADDR(region_head->word5, data_addr);

	REG_SET_FIELD(&(reg->rOSD_CTRL), m_OSD_V_TOP_PHASE, 0x00, b_OSD_V_TOP_PHASE);
//	VPU_OSD_SET_WIDTH(region_head->word3, clip_rect->x, clip_rect->width + clip_rect->x - 1);
//	VPU_OSD_SET_HEIGHT(region_head->word4, clip_rect->y, clip_rect->height + clip_rect->y - 1);
	/*view info*/
	REG_SET_FIELD(&(reg->rOSD_POSITION), m_START_X_OSD, view_rect->x, b_START_X_OSD);
	REG_SET_FIELD(&(reg->rOSD_POSITION), m_START_Y_OSD, view_rect->y, b_START_Y_OSD);
	REG_SET_FIELD(&(reg->rOSD_VIEW_SIZE), m_OSD_LENGTH, view_width,  b_OSD_LENGTH);
	REG_SET_FIELD(&(reg->rOSD_VIEW_SIZE), m_OSD_HEIGHT, view_height, b_OSD_HEIGHT);
	/*zoom info*/
	REG_SET_FIELD(&(reg->rOSD_ZOOM), m_OSD_V_ZOOM, vzoom, b_OSD_V_ZOOM);
	REG_SET_FIELD(&(reg->rOSD_ZOOM), m_OSD_H_ZOOM, hzoom, b_OSD_H_ZOOM);

	/*zoom para*/
	for (i = 0; i < 64; i++)
		REG_SET_VAL(&(reg->rOSD_ZOOM_PARA[i]), sPhaseNomal[i]);

	return 0;
}

static int osdx_enable(GxLayerID layer_id, int enable)
{
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);

	if (enable == 0) {
		REG_CLR_BIT(&(reg->rOSD_ENABLE), b_OSD_VISIBLE);
	} else {
		REG_SET_FIELD(&(reg->rOSD_FIRST_HEAD_PTR), m_OSD_FIRST_HEAD_PTR, (unsigned int)gx_dma_to_phys((unsigned int)region_head), b_OSD_FIRST_HEAD_PTR);
		REG_SET_BIT(&(reg->rOSD_ENABLE), b_OSD_VISIBLE);
	}

	return 0;
}

static int osdx_palette(GxLayerID layer_id, GxPalette *palette)
{
	int clut_len;
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);

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
	VPU_OSD_SET_CLUT_PTR      (region_head->word2, gx_virt_to_phys((unsigned int)palette->entries));
	VPU_OSD_SET_CLUT_LENGTH   (region_head->word1, clut_len);
	VPU_OSD_CLUT_UPDATA_ENABLE(region_head->word1);

	return 0;
}

static int osdx_alpha(GxLayerID layer_id, GxAlpha alpha)
{
	GxColorFormat format;
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);

	if (alpha.type == GX_ALPHA_GLOBAL) {
		VPU_OSD_GLOBAL_ALHPA_ENABLE(region_head->word1);
		VPU_OSD_SET_MIX_WEIGHT(region_head->word1, alpha.value);
		VPU_OSD_SET_ALPHA_RATIO_DISABLE(region_head->word7);
	} else {
		VPU_OSD_GLOBAL_ALHPA_DISABLE(region_head->word1);
		//patch, 0xFF should equal with ratio_disable, but actually not
		if(alpha.value == 0xFF) {
			VPU_OSD_SET_ALPHA_RATIO_DISABLE(region_head->word7);
		} else {
			VPU_OSD_SET_ALPHA_RATIO_VALUE(region_head->word7, alpha.value);
			VPU_OSD_SET_ALPHA_RATIO_ENABLE(region_head->word7);
		}
		VPU_OSD_GET_COLOR_TYPE(region_head->word1, format);
		if(GX_COLOR_FMT_RGBA5551 == format) {
			/* reserved */
		}
	}

	return 0;
}

static int osdx_color_format(GxLayerID layer_id, GxColorFormat format)
{
	int ret = 0;
	int true_color_mode, bpp = 0;
	ByteSequence byteseq = ABCD_EFGH;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);

	if (format <= GX_COLOR_FMT_CLUT8)
		REG_CLR_BIT(&(reg->rOSD_CTRL), b_OSD_ZOOM_MODE_EN_IPS);
	else
		REG_SET_BIT(&(reg->rOSD_CTRL), b_OSD_ZOOM_MODE_EN_IPS);

	if (format == GX_COLOR_FMT_YCBCRA6442 || format == GX_COLOR_FMT_YUVA8888) {
		if (layer_id != GX_LAYER_SOSD && layer_id != GX_LAYER_SOSD2) {
			ret = -1;
			goto exit;
		}
		VPU_OSD_SET_YUV_MODE(region_head->word1, 1);
		if (format == GX_COLOR_FMT_YUVA8888) {
			VPU_OSD_SET_COLOR_TYPE(region_head->word1, 0x7);
		} else {
			VPU_OSD_SET_COLOR_TYPE(region_head->word1, 0x6);
		}
	} else {
		VPU_OSD_SET_YUV_MODE(region_head->word1, 0);
		if((format >= GX_COLOR_FMT_RGBA8888)&&(format <= GX_COLOR_FMT_BGR888)) {
			true_color_mode = format - GX_COLOR_FMT_RGBA8888;
			VPU_OSD_SET_COLOR_TYPE(region_head->word1, 0x7);
			VPU_OSD_SET_TRUE_COLOR_MODE(region_head->word1, true_color_mode);
		} else {
			if( (format==GX_COLOR_FMT_ARGB4444)||
					(format==GX_COLOR_FMT_ARGB1555)||
					(format==GX_COLOR_FMT_ARGB8888))
				VPU_OSD_SET_ARGB_CONVERT(region_head->word1,1);
			else
				VPU_OSD_SET_ARGB_CONVERT(region_head->word1,0);

			if(format == GX_COLOR_FMT_ABGR8888)
				VPU_OSD_SET_ABGR_CONVERT(region_head->word1, 1);
			else
				VPU_OSD_SET_ABGR_CONVERT(region_head->word1, 0);

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
			VPU_OSD_SET_COLOR_TYPE(region_head->word1, format);
		}
	}

	bpp = gx_color_get_bpp(format);

	/* set vpu byte-sequence(data) */
	if(bpp == 32)
		byteseq = ABCD_EFGH;
	else if(bpp == 16) {
		if (format != GX_COLOR_FMT_YCBCRA6442)
			byteseq = CDAB_GHEF;
		else
			byteseq = DCBA_HGFE;
	} else if(bpp <= 8)
		byteseq = DCBA_HGFE;
	((CygnusVpuOsdPriv*)cygnusvpu_info->layer[layer_id].priv)->data_byte_seq = byteseq;
	REG_SET_FIELD(&(reg->rOSD_FRAME_PARA), m_OSD_ENDIAN_MODE, byteseq, b_OSD_ENDIAN_MODE);

	byteseq = ABCD_EFGH;
	((CygnusVpuOsdPriv*)cygnusvpu_info->layer[layer_id].priv)->regionhead_byte_seq = byteseq;
	REG_SET_FIELD(&(reg->rOSD_FRAME_PARA), m_OSD_ENDIAN_MODE_HEAD, byteseq , b_OSD_ENDIAN_MODE_HEAD);

exit:
	return ret;
}

int cygnus_osdx_color_key_mode(GxLayerID layer_id, GxColorkeyMode mode, unsigned char ext_alpha)
{
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);

	REG_SET_FIELD(&(reg->rOSD_COLORKEY_CTROL), m_OSD_COLORKEY_MODE, mode, b_OSD_COLORKEY_MODE);
	if (mode == RGBA_MODE)
		REG_SET_FIELD(&(reg->rOSD_COLORKEY_CTROL), m_OSD_COLORKEY_ALPHA, ext_alpha, b_OSD_COLORKEY_ALPHA);

	return 0;
}

int cygnus_osdx_color_key(GxLayerID layer_id, GxColor *color, int enable)
{
	unsigned int r, g, b, a, color_val;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);
	GxColorFormat format = cygnusvpu_info->layer[layer_id].surface->color_format;

	if(enable == 0 && region_head) {
		VPU_OSD_COLOR_KEY_DISABLE(region_head->word1);
	} else {
		if (region_head) {
			VPU_OSD_COLOR_KEY_ENABLE(region_head->word1);
		}
		switch (format) {
		case GX_COLOR_FMT_RGB565:
			r = (color->r & 0xF8) | (color->r >> 5);
			g = (color->g & 0xFC) | (color->g >> 6);
			b = (color->b & 0xF8) | (color->b >> 5);
			a = color->a;
			break;
		case GX_COLOR_FMT_RGBA4444:
		case GX_COLOR_FMT_ARGB4444:
			r = (color->r & 0xF0) | (color->r >> 4);
			g = (color->g & 0xF0) | (color->g >> 4);
			b = (color->b & 0xF0) | (color->b >> 4);
			a = color->a;
			break;
		case GX_COLOR_FMT_RGBA5551:
		case GX_COLOR_FMT_ARGB1555:
			r = (color->r & 0xF8) | (color->r >> 5);
			g = (color->g & 0xF8) | (color->g >> 5);
			b = (color->b & 0xF8) | (color->b >> 5);
			a = color->a;
			break;
		case GX_COLOR_FMT_YCBCRA6442:
			r = (color->y  & 0xFC) | (color->y  >> 6);
			g = (color->cb & 0xF0);// | (color->cb >> 4);
			b = (color->cr & 0xF0);// | (color->cr >> 4);
			a = color->alpha;
			break;
		default:
			r = color->r;
			g = color->g;
			b = color->b;
			a = color->a;
			break;
		}
		color_val = (((unsigned char)r)<<24)|(((unsigned char)g)<<16)|(((unsigned char)b)<<8)|((unsigned char)a);\
		REG_SET_FIELD(&(reg->rOSD_COLOR_KEY), m_OSD_COLORKEY, color_val, b_OSD_COLORKEY);
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
static void osdx_common_config(GxLayerID layer_id, CygnusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	GxAvRect rect={0};
	ByteSequence byteseq = ABCD_EFGH;
	unsigned int request_block=0, bpp=1;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);

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
	else if(bpp == 16) {
		if (surface->color_format != GX_COLOR_FMT_YCBCRA6442)
			byteseq = CDAB_GHEF;
		else
			byteseq = DCBA_HGFE;
	} else if(bpp <= 8)
		byteseq = DCBA_HGFE;
	((CygnusVpuOsdPriv*)cygnusvpu_info->layer[layer_id].priv)->data_byte_seq = byteseq;
	REG_SET_FIELD(&(reg->rOSD_FRAME_PARA), m_OSD_ENDIAN_MODE, byteseq, b_OSD_ENDIAN_MODE);
	/* set vpu byte-sequence(region head) */
	byteseq = ABCD_EFGH;
	((CygnusVpuOsdPriv*)cygnusvpu_info->layer[layer_id].priv)->regionhead_byte_seq = byteseq;
	REG_SET_FIELD(&(reg->rOSD_FRAME_PARA), m_OSD_ENDIAN_MODE_HEAD, byteseq, b_OSD_ENDIAN_MODE_HEAD);
	REG_SET_FIELD(&(reg->rOSD_FRAME_PARA), m_OSD_BUFF_LEN, request_block, b_OSD_BUFF_LEN);
	REG_SET_FIELD(&(reg->rOSD_POSITION), m_START_X_OSD, rect.x, b_START_X_OSD);
	REG_SET_FIELD(&(reg->rOSD_POSITION), m_START_Y_OSD, rect.y, b_START_Y_OSD);
	cygnus_osdx_color_key(layer_id, &surface->color_key, surface->color_key_en);
}

static int osdx_add_region(GxLayerID layer_id, CygnusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	int ret = -1;

	if (surface) {
		//struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);
		//if (region_head == NULL) {
		if (1) {
			cygnusvpu_info->layer[layer_id].surface = surface;
			osdx_common_config(layer_id, surface, pos_x, pos_y);
		}
		ret = cygnus_region_add(layer_id, surface, pos_x, pos_y);
	}

	return ret;
}

static OsdRegion* osdx_revoke_region(GxLayerID layer_id, CygnusVpuSurface *surface)
{
	return cygnus_region_revoke(layer_id, surface);
}

static int osdx_remove_region(GxLayerID layer_id, CygnusVpuSurface *surface)
{
	return cygnus_region_remove(layer_id, surface);
}

static int osdx_update_region(GxLayerID layer_id, CygnusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return cygnus_region_update(layer_id, surface, pos_x, pos_y);
}

static int osdx_reconfig_region(GxLayerID layer_id, OsdRegion *region, VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	osdx_common_config(layer_id, surface, pos_x, pos_y);
	cygnusvpu_info->layer[layer_id].surface = surface;
	return cygnus_region_reconfig(layer_id, region, surface, pos_x, pos_y);
}

static int osdx_main_surface(GxLayerID layer_id, CygnusVpuSurface *surface)
{
	unsigned ret = -1;
	unsigned pos_x = 0, pos_y = 0;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region *region_head = *cygnus_get_region_head_by_layer_id(layer_id);
	OsdRegion *region = NULL;

	if (region_head == NULL) {
		ret = osdx_add_region(layer_id, surface, pos_x, pos_y);
	} else {
		region = osdx_revoke_region(layer_id, surface);
		if(region) {
			memset(region, 0, sizeof(OsdRegion));
			ret = osdx_reconfig_region(layer_id, region, surface, pos_x, pos_y);
		} else {
			ret = osdx_add_region(layer_id, surface, pos_x, pos_y);
		}
	}

	if ((surface->color_format == GX_COLOR_FMT_ARGB1555) ||
	   (surface->color_format == GX_COLOR_FMT_RGBA5551)){
		REG_SET_FIELD(&(reg->rOSD_ALPHA_5551), m_OSD_ALPHA_REG0, 0x00, b_OSD_ALPHA_REG0);
		REG_SET_FIELD(&(reg->rOSD_ALPHA_5551), m_OSD_ALPHA_REG1, 0xff, b_OSD_ALPHA_REG1);
	}

	return ret;
}

static int cygnus_osd_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return osdx_zoom(GX_LAYER_OSD, clip_rect, view_rect);
}

static int cygnus_osd_enable(int enable)
{
	return osdx_enable(GX_LAYER_OSD, enable);
}

static int cygnus_osd_palette(GxPalette *palette)
{
	return osdx_palette(GX_LAYER_OSD, palette);
}

static int cygnus_osd_alpha(GxAlpha alpha)
{
	return osdx_alpha(GX_LAYER_OSD, alpha);
}

static int cygnus_osd_color_format(GxColorFormat format)
{
	return osdx_color_format(GX_LAYER_OSD, format);
}

static int cygnus_osd_color_key_mode(GxColorkeyMode mode, unsigned char ext_alpha)
{
	return cygnus_osdx_color_key_mode(GX_LAYER_OSD, mode, ext_alpha);
}

static int cygnus_osd_color_key(GxColor *color, int enable)
{
	return cygnus_osdx_color_key(GX_LAYER_OSD, color, enable);
}

static int cygnus_osd_pan_display(void *buffer)
{
	OsdRegion **p_region_header = OSD_HEADPTR;

	if (buffer && *p_region_header) {
		OsdRegion *region = *p_region_header;
		VPU_OSD_SET_DATA_ADDR(region->word5, gx_virt_to_phys((unsigned int)buffer));
	}

	return 0;
}

static int cygnus_osd_add_region(CygnusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return osdx_add_region(GX_LAYER_OSD, surface, pos_x, pos_y);
}

static int cygnus_osd_remove_region(CygnusVpuSurface *surface)
{
	return osdx_remove_region(GX_LAYER_OSD, surface);
}

static int cygnus_osd_update_region(CygnusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return osdx_update_region(GX_LAYER_OSD, surface, pos_x, pos_y);
}

static int cygnus_osd_main_surface(CygnusVpuSurface *surface)
{
	return osdx_main_surface(GX_LAYER_OSD, surface);
}

unsigned int cygnus_vpp_calc_requestblock(unsigned int old_width, unsigned int new_width)
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

int cygnus_vpp_view_port(GxAvRect *clip, GxAvRect *view)
{
#define VPP_GET_ZOOM_PARA(para,clip_size,view_size,down_scale) \
	do { \
		(para) = ((((clip_size) >> (down_scale))) << VPU_VPP_ZOOM_WIDTH) / (view_size); \
	}while (0)

	int val = 0;

	int i = 0, play_mode;
	int vzoom = 0, hzoom = 0;
	int vtbias = 0, vbbias = 0;
	int vdown_scale_en = 0, hdown_scale_en = 0;

	unsigned int stride = 0;
	unsigned int request_block = 0, uv_mode;
	unsigned int src_width = 0, src_height = 0, old_src_width = 0;
	unsigned int view_width = 0, view_height = 0;
	struct vout_dvemode dvemode;
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);
	DispControler *controler = &((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	view_width    = view->width;
	view_height   = view->height;
	src_width     = clip->width;
	src_height    = clip->height;
	stride         = controler->last_frame.baseline;
	play_mode     = ((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->play_mode;
	old_src_width = REG_GET_FIELD(&(reg->rPP_SOURCE_SIZE), m_PP_REQ_LENGTH, b_PP_REQ_LENGTH);
	request_block = cygnus_vpp_calc_requestblock(old_src_width, src_width);

	VPP_GET_ZOOM_PARA(hzoom, clip->width, view->width, hdown_scale_en);
	while(VPU_VPP_ZOOM_OUT_MAX < hzoom) {
		//src_width >>= 1;
		//src_width = (src_width >> 3) << 3;
		hdown_scale_en++;
		VPP_GET_ZOOM_PARA(hzoom, clip->width, view_width, hdown_scale_en);
		if(hdown_scale_en > 5)
			return -1;
	}

	VPP_GET_ZOOM_PARA(vzoom, clip->height, view_height, vdown_scale_en);
	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if (IS_INTERLACE_MODE(dvemode.mode)) {
		if (PLAY_MODE_FRAME == play_mode) {
			while ((VPU_VPP_ZOOM_OUT_MAX >> 1) < vzoom) {
				vdown_scale_en++;
				VPP_GET_ZOOM_PARA(vzoom, clip->height, view_height, vdown_scale_en);
				if (vdown_scale_en > 5)
					return -1;
			}
			vtbias = 0;
			vbbias =vzoom;
		} else {
			while (VPU_VPP_ZOOM_OUT_MAX < vzoom) {
				vdown_scale_en++;
				VPP_GET_ZOOM_PARA(vzoom, clip->height, view_height, vdown_scale_en);
				if (vdown_scale_en > 3)
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
			VPP_GET_ZOOM_PARA(vzoom, clip->height, view_height, vdown_scale_en);
			if(vdown_scale_en > 3)
				return -1;
		}

		if(PLAY_MODE_FRAME == play_mode) {
			vtbias = 0;
			vbbias = 0;
		} else {
			vtbias = 2048;
			vbbias = 0;
		}
	}

	if(hdown_scale_en)
		src_width = (src_width >> 1) << 1;
	else
		src_width = (src_width >> 3) << 3;

	if (vzoom == VPU_VPP_ZOOM_NONE && controler->last_frame.playmode == PLAY_MODE_Y_FRAME_UV_FIELD)
		uv_mode = 0x1;
	else
		uv_mode = 0x0;
	REG_SET_FIELD(&(reg->rPP_CTRL), m_PP_UV_MODE, uv_mode, b_PP_UV_MODE);

	/*src*/
	REG_SET_FIELD(&(reg->rPP_SOURCE_SIZE), m_PP_REQ_LENGTH, src_width,  b_PP_REQ_LENGTH);
	REG_SET_FIELD(&(reg->rPP_SOURCE_SIZE), m_PP_REQ_HEIGHT, src_height, b_PP_REQ_HEIGHT);
	/*dst*/
	REG_SET_FIELD(&(reg->rPP_POSITION), m_PP_START_X, view->x, b_PP_START_X);
	REG_SET_FIELD(&(reg->rPP_POSITION), m_PP_START_Y, view->y, b_PP_START_Y);
	REG_SET_FIELD(&(reg->rPP_VIEW_SIZE), m_PP_VIEW_LENGTH, view->width, b_PP_VIEW_LENGTH);
	REG_SET_FIELD(&(reg->rPP_VIEW_SIZE), m_PP_VIEW_HEIGHT, view->height, b_PP_VIEW_HEIGHT);

	REG_SET_FIELD(&(reg->rPP_ZOOM), m_PP_V_ZOOM, vzoom, b_PP_V_ZOOM);
	REG_SET_FIELD(&(reg->rPP_ZOOM), m_PP_H_ZOOM, hzoom, b_PP_H_ZOOM);

	REG_SET_FIELD(&(reg->rPP_V_PHASE), m_PP_VT_PHASE_BIASE, vtbias&0x7FFF, b_PP_VT_PHASE_BIASE);
	REG_SET_FIELD(&(reg->rPP_V_PHASE), m_PP_VB_PHASE_BIASE, vbbias&0x7FFF, b_PP_VB_PHASE_BIASE);

	REG_SET_FIELD(&(reg->rPP_POSITION), m_PP_V_ZOOM_DOWN_SCALE, vdown_scale_en, b_PP_V_ZOOM_DOWN_SCALE);
	REG_SET_FIELD(&(reg->rPP_POSITION), m_PP_H_ZOOM_DOWN_SCALE, hdown_scale_en, b_PP_H_ZOOM_DOWN_SCALE);

	/*### 确认如此配置是否正确*/
	reg->rPP_ZOOM_FILTER_SIGN = 0x998080;

	if (hzoom == 4096) {
		REG_SET_BIT(&(reg->rPP_CTRL), b_PP_Y_ZOOM_MODE_H);
		REG_SET_FIELD(&(reg->rPP_PHASE0_H), m_PP_PHASE_0_H, 0x00ff0000, b_PP_PHASE_0_H);
	} else {
		REG_CLR_BIT(&(reg->rPP_CTRL), b_PP_Y_ZOOM_MODE_H);
		REG_SET_FIELD(&(reg->rPP_PHASE0_H), m_PP_PHASE_0_H, 0x00ff0100, b_PP_PHASE_0_H);
	}

	if (vzoom == 4096) {
		REG_SET_BIT(&(reg->rPP_CTRL), b_PP_Y_ZOOM_MODE_V);
		REG_SET_FIELD(&(reg->rPP_PHASE0_V), m_PP_PHASE_0_V, 0x00ff0000, b_PP_PHASE_0_V);
	} else {
		REG_CLR_BIT(&(reg->rPP_CTRL), b_PP_Y_ZOOM_MODE_V);
		REG_SET_FIELD(&(reg->rPP_PHASE0_V), m_PP_PHASE_0_V, 0x00ff0100, b_PP_PHASE_0_V);
	}

	for (i = 0; i < 64; i++)
		REG_SET_FIELD(&(reg->rPP_ZOOM_PARA_H[i]), m_PP_ZOOM_PARA_HXX, sPhaseNomal[i], b_PP_ZOOM_PARA_HXX);

	for (i = 0; i < 64; i++)
		REG_SET_FIELD(&(reg->rPP_ZOOM_PARA_V[i]), m_PP_ZOOM_PARA_VXX, sPhaseNomal[i], b_PP_ZOOM_PARA_VXX);

	/*### 待定硬件提供系数*/
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA1), m_PP_FIR3, 0, b_PP_FIR3);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA1), m_PP_FIR2, 0, b_PP_FIR2);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA1), m_PP_FIR1, 0, b_PP_FIR1);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA1), m_PP_FIR0, 0, b_PP_FIR0);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA2), m_PP_FIR7, 0, b_PP_FIR7);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA2), m_PP_FIR6, 0, b_PP_FIR6);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA2), m_PP_FIR5, 0, b_PP_FIR5);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA2), m_PP_FIR4, 0, b_PP_FIR4);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA3), m_PP_FIR9, 256, b_PP_FIR9);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA3), m_PP_FIR8, 0, b_PP_FIR8);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA4), m_PP_FIR_BY_PASS, 1, b_PP_FIR_BY_PASS);
	REG_SET_FIELD(&(reg->rPP_HFILTER_PARA4), m_PP_FIR_SIGN, 0, b_PP_FIR_SIGN);

	/*### 待定如何配置*/
	REG_SET_FIELD(&(reg->rPP_ZOOM_FILTER_SIGN), m_PP_PHASE0_V_PN, 9, b_PP_PHASE0_V_PN);
	REG_SET_FIELD(&(reg->rPP_ZOOM_FILTER_SIGN), m_PP_PHASE0_H_PN, 9, b_PP_PHASE0_H_PN);
	REG_SET_FIELD(&(reg->rPP_ZOOM_FILTER_SIGN), m_PP_V_PN_CHG_POS, 128, b_PP_V_PN_CHG_POS);
	REG_SET_FIELD(&(reg->rPP_ZOOM_FILTER_SIGN), m_PP_H_PN_CHG_POS, 128, b_PP_H_PN_CHG_POS);

	return 0;
}

int cygnus_vpp_enable(int enable)
{
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	if (cygnusvpu_info == NULL)
		return 0;

	if (enable == 0)
		REG_CLR_BIT(&(reg->rPP_ENABLE), b_PP_EN);
	else
		REG_SET_BIT(&(reg->rPP_ENABLE), b_PP_EN);

	return 0;
}

int cygnus_vpp_bg_color(GxColor color)
{
	unsigned val = (color.y << 16 | color.cb << 8 | color.cr);
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	REG_SET_FIELD(&(reg->rPP_BACKCOLOR), m_PP_BACK_COLOR, val, b_PP_BACK_COLOR);
	return 0;
}

static int cygnus_vpp_get_rdptr(void)
{
	int rd_ptr;
	DispControler *controler = NULL;
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	if (VPU_CLOSED())
		return -1;
	controler = &((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	rd_ptr = REG_GET_FIELD(&(reg->rPP_DISP_R_PTR), m_DISP_R_PTR, b_DISP_R_PTR);
	rd_ptr = rd_ptr >> 4;
	//gx_printf("\nrd = %d\n", rd_ptr);
	if (cygnusvpu_info->reset_flag && rd_ptr == 1) {
		rd_ptr = 0;
		controler->buf_to_wr = 0;
	}

	return rd_ptr;
}

	//gx_printf("cfg to %d\n", id);
#define CONFIG_DISP_UNIT(top, bot, offset_y, offset_cbcr, playmode, topfirst, bpp, base_width, base_height)\
do {\
	int id = controler->buf_to_wr;\
	controler->unit[id]->word0 = top.addry    + offset_y;\
	controler->unit[id]->word2 = top.addrcbcr + offset_cbcr;\
	controler->unit[id]->word1 = bot.addry    + offset_y;\
	controler->unit[id]->word3 = bot.addrcbcr + offset_cbcr;\
	controler->unit[id]->word4 = top.addry_2bit;\
	controler->unit[id]->word5 = bot.addry_2bit;\
	controler->unit[id]->word6 = top.addrcbcr_2bit;\
	controler->unit[id]->word7 = bot.addrcbcr_2bit;\
	controler->unit[id]->word9 = (base_width<<b_DISP_SOURCE_STRIDE);\
	controler->unit[id]->word11 = (topfirst)|(playmode) | (bpp<<b_DISP_BIT_MODE);\
	controler->buf_to_wr += 1;\
	controler->buf_to_wr %= MAX_DISPBUF_NUM;\
} while(0);

int cygnus_vpp_zoom_require(void)
{
	DispControler *controler = &((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	controler->zoom_require++;
	return 0;
}

static int frame_color_param_changed(struct frame *frame)
{
	int ret = 0;
	DispControler *controler = &((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	ret |= (frame->colorimetry != controler->last_frame.colorimetry);
	ret |= (frame->transfer_characteristic != controler->last_frame.transfer_characteristic);
	ret |= gx_memcmp(&(frame->mdmd), &(controler->last_frame.mdmd), sizeof(frame->mdmd));
	ret |= gx_memcmp(&(frame->clmd), &(controler->last_frame.clmd), sizeof(frame->clmd));

	return ret;
}

static int vpp_play_frame(struct frame *frame)
{
	int ret = -1;
	int rdptr;
	int playmode_changed = 0, frame_playmode = 0;
	static int last_topfirst = 1;
	unsigned offset_y = 0, offset_cbcr = 0;
	unsigned bpp, base_width, base_height;
	GxColorimetry c;
	DispControler *controler = NULL;
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	if (VPU_CLOSED())
		return -1;
	controler = &((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	if (frame) {
		//gx_printf("c = %d\n", frame->colorimetry);
		rdptr = cygnus_vpp_get_rdptr();
		//updata frame size
		if (frame->w && frame->h)
			cygnus_vpp_set_framesize(frame->w, frame->h);

		if (cygnusvpu_info->layer[GX_LAYER_VPP].surface)
			cygnusvpu_info->layer[GX_LAYER_VPP].surface->surface_mode = (frame->is_image ? GX_SURFACE_MODE_IMAGE : GX_SURFACE_MODE_VIDEO);

		//update window
		if (controler->zoom_require) {
			cygnus_vpp_get_actual_cliprect(&controler->clip_rect);
			cygnus_vpp_get_actual_viewrect(&controler->view_rect);
		}

		if (frame_color_param_changed(frame)) {
			//HDR10
			if (frame->transfer_characteristic == GXCOL_TRC_SMPTE2084) {
				if (gxav_hdmi_support_hdr10()) {
					//hdmi接口: 配置曲线和元数据
					gxav_hdmi_set_hdr10(frame);
					hdr_set_work_mode(HDR_TO_SDR);
				} else {
					hdr_set_work_mode(BYPASS);
				}
			}
			//HLG
			if (frame->transfer_characteristic == GXCOL_TRC_ARIB_STD_B67) {
				if (gxav_hdmi_support_hlg()) {
					//hdmi接口: 配置曲线
					gxav_hdmi_set_hlg(frame);
					hdr_set_work_mode(HLG_TO_SDR);
				} else {
					hdr_set_work_mode(BYPASS);
				}
			}
			gxav_hdmi_set_colorimetry(frame->colorimetry);
		}

		//check same frame
		if (gx_memcmp(frame, &controler->last_frame, sizeof(struct frame)) == 0)
			if (controler->zoom_require == 0)
				goto out;

		if (frame->topfirst != last_topfirst)
			gx_printf("last = %d, cur = %d\n", last_topfirst, frame->topfirst);

		//get disp unit id
		if (controler->buf_to_wr != rdptr) {
			//gx_printf("\nrd = %d, wr = %d\n", rdptr, controler->buf_to_wr);
			if (frame->is_freeze == 0)
				goto out;
		}

		if (controler->view_rect.height < controler->clip_rect.height && frame->playmode == PLAY_MODE_Y_FRAME_UV_FIELD)
			frame->playmode = PLAY_MODE_FRAME;

		//config disp unit
		bpp         = frame->bpp == 8 ? 0 : frame->bpp == 9 ? 1 : 2;
		base_width  = frame->baseline;
		base_height = frame->baseheight;
		offset_y    = controler->clip_rect.x +   frame->baseline * controler->clip_rect.y;
		offset_cbcr = controler->clip_rect.x + ((frame->baseline * controler->clip_rect.y)>>1);
		if (frame->playmode == PLAY_MODE_FIELD)
			frame_playmode = (0<<b_DISP_FRAME_MODE)|(1<<b_DISP_FRAME_MODE1);
		if (frame->playmode == PLAY_MODE_FRAME)
			frame_playmode = (1<<b_DISP_FRAME_MODE)|(0<<b_DISP_FRAME_MODE1);
		if (frame->playmode == PLAY_MODE_Y_FRAME_UV_FIELD)
			frame_playmode = (1<<b_DISP_FRAME_MODE)|(1<<b_DISP_FRAME_MODE1);

		if (frame->playmode == PLAY_MODE_FIELD) {
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame_playmode,  frame->topfirst, bpp, base_width, base_height);
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame_playmode, !frame->topfirst, bpp, base_width, base_height);
			((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->play_mode = PLAY_MODE_FIELD;
		} else {
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame_playmode,  frame->topfirst, bpp, base_width, base_height);
			((CygnusVpuVppPriv*)cygnusvpu_info->layer[GX_LAYER_VPP].priv)->play_mode = PLAY_MODE_FRAME;
		}

		ret = 0;
		playmode_changed = (controler->last_frame.playmode != frame->playmode);
		cygnusvpu_info->reset_flag = 0;
		last_topfirst = frame->topfirst;
		controler->last_frame = *frame;
		if (frame->is_freeze == 1)
			controler->zoom_require += 3;

		//rezoom
		if (controler->zoom_require || playmode_changed) {
			cygnus_vpp_view_port(&controler->clip_rect, &controler->view_rect);
			controler->zoom_require--;
		}
	}

out:
	return ret;
}

int cygnus_vpp_play_frame(struct frame *frame)
{
	int ret = -1;
	unsigned long flags;

	VPU_SPIN_LOCK();
	ret = vpp_play_frame(frame);
	VPU_SPIN_UNLOCK();

	return ret;
}

int cygnus_vpp_main_surface(CygnusVpuSurface *surface)
{
	unsigned int base_line = 0;
	struct frame frame = {0};
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	surface->bg_color.y  = 0x10;
	surface->bg_color.cb = 0x80;
	surface->bg_color.cr = 0x80;

	gx_printf("\nvpp main surface start\n");
	if (GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
		if(GX_COLOR_FMT_YCBCR420_Y_UV== surface->color_format) {

			gx_dcache_clean_range(0, 0);
			cygnus_vpp_set_framesize(surface->width, surface->height);
			REG_SET_FIELD(&(reg->rPP_SOURCE_SIZE), m_PP_REQ_LENGTH, surface->width,  b_PP_REQ_LENGTH);
			REG_SET_FIELD(&(reg->rPP_SOURCE_SIZE), m_PP_REQ_HEIGHT, surface->height, b_PP_REQ_HEIGHT);
			base_line = surface->width;

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
			vpp_play_frame(&frame);
		}
	}
	gx_printf("\nvpp main surface end\n");

	return 0;
}

int cygnus_vpp_set_stream_ratio(unsigned int ratio)
{
	CygnusVpuVppPriv *info;

	if(cygnusvpu_info && cygnusvpu_info->layer[GX_LAYER_VPP].priv) {
		info = cygnusvpu_info->layer[GX_LAYER_VPP].priv;
		if(ratio != info->stream_ratio) {
			info->stream_ratio = ratio;
			return cygnus_vpp_zoom_require();
		}
		return 0;
	}
	else
		return -1;
}

int cygnus_vpp_set_framesize(unsigned int width, unsigned int height)
{
	int ret = -1;
	CygnusVpuVppPriv *info;

	if (cygnusvpu_info && cygnusvpu_info->layer[GX_LAYER_VPP].priv) {
		info = cygnusvpu_info->layer[GX_LAYER_VPP].priv;
		if (info->frame_width!=width || info->frame_height!=height) {
			info->frame_width = width;
			info->frame_height= height;
			ret = cygnus_vpp_zoom_require();
		}
	}

	return ret;
}

int cygnus_vpp_set_playmode(GxVpuLayerPlayMode mode)
{
	int ret = -1;
	CygnusVpuVppPriv *info;

	if(cygnusvpu_info && cygnusvpu_info->layer[GX_LAYER_VPP].priv) {
		info = cygnusvpu_info->layer[GX_LAYER_VPP].priv;
		info->play_mode = mode;

		if (cygnusvpu_info->layer[GX_LAYER_VPP].surface)
			cygnusvpu_info->layer[GX_LAYER_VPP].surface->surface_mode = GX_SURFACE_MODE_VIDEO;

		ret = 0;
	}

	return ret;
}

int cygnus_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	GxAvRect clip_rect;

	if(VPU_CLOSED())
		return 0;

	CygnusLayer_Get_ActualViewrect(GX_LAYER_VPP,  view_rect);
	CygnusLayer_Get_ActualCliprect(GX_LAYER_VPP, &clip_rect);
	CygnusVpu_AspectRatio_Adapt(GX_LAYER_VPP, &clip_rect, view_rect);
	VIEWRECT_ALIGN(*view_rect);
	return 0;
}

int cygnus_vpp_get_actual_cliprect(GxAvRect *clip_rect)
{
	GxAvRect view_rect;

	if(VPU_CLOSED())
		return 0;

	CygnusLayer_Get_ActualViewrect(GX_LAYER_VPP,  &view_rect);
	CygnusLayer_Get_ActualCliprect(GX_LAYER_VPP, clip_rect);
	CygnusVpu_AspectRatio_Adapt(GX_LAYER_VPP, clip_rect, &view_rect);
	CLIPRECT_ALIGN(*clip_rect);
	return 0;
}

int cygnus_spp_zoom(GxAvRect *clip, GxAvRect *view)
{
#define SPP_GET_ZOOM_PARA(para,clip_size,view_size,down_scale) \
	do { \
		(para) = ((((clip_size) >> (down_scale)) - 1) << VPU_SPP_ZOOM_WIDTH) / (view_size - 1); \
	}while (0)

	int val = 0;

	int vtbias = 0, vbbias = 0;
	int i = 0, vzoom = 0, hzoom = 0;
	int vdown_scale_en = 0, hdown_scale_en = 0;
	unsigned int req_width = 0, req_height = 0;
	unsigned int view_width = 0, view_height =0;
	struct vout_dvemode dvemode;

	struct cygnus_vpu_pic_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PIC);

	int play_mode = VPU_SPP_PLAY_MODE_FRAME;
	view_width  = view->width;
	view_height = view->height;
	req_width   = clip->width;
	req_height  = clip->height;

	SPP_GET_ZOOM_PARA(hzoom, clip->width, view->width, hdown_scale_en);
	if (VPU_SPP_ZOOM_OUT_MAX < hzoom) {
		hdown_scale_en = 1;
		SPP_GET_ZOOM_PARA(hzoom, clip->width, view->width, hdown_scale_en);
		if (VPU_SPP_ZOOM_OUT_MAX < hzoom)
			return -1;
	}
	SPP_GET_ZOOM_PARA(vzoom, clip->height, view->height, vdown_scale_en);

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if (IS_INTERLACE_MODE(dvemode.mode)) {
		if (VPU_SPP_PLAY_MODE_FRAME == play_mode) {
			if ((VPU_SPP_ZOOM_OUT_MAX >> 1) < vzoom) {
				vdown_scale_en = 1;
				SPP_GET_ZOOM_PARA(vzoom, clip->height, view->height, vdown_scale_en);
				if ((VPU_SPP_ZOOM_OUT_MAX >> 1) < vzoom)
					return -1;
			}
			vtbias = 0;
			vbbias =vzoom;
		} else {
			if (VPU_SPP_ZOOM_OUT_MAX < vzoom) {
				vdown_scale_en = 1;
				SPP_GET_ZOOM_PARA(vzoom, clip->height, view->height, vdown_scale_en);
				if(VPU_SPP_ZOOM_OUT_MAX < vzoom)
					return -1;
			}

			if(VPU_SPP_ZOOM_NONE < vzoom) {
				vtbias = 0;
				vbbias = (vzoom >> 1) - 2048;
			} else {
				vtbias = 2048 - (vzoom >> 1);
				vbbias = 0;
			}
		}
	} else if(IS_PROGRESSIVE_MODE(dvemode.mode)) {
		if (VPU_SPP_ZOOM_OUT_MAX < vzoom) {
			vdown_scale_en = 1;
			SPP_GET_ZOOM_PARA(vzoom, clip->height, view->height, vdown_scale_en);
			if (VPU_SPP_ZOOM_OUT_MAX < vzoom)
				return -1;
		}
		if (VPU_SPP_PLAY_MODE_FRAME == play_mode) {
			vtbias = 0;
			vbbias = 0;
		} else {
			vtbias = 2048;
			vbbias = 0;
		}
	}

	if(hdown_scale_en)
		req_width = (req_width >> 1) << 1;
	else
		req_width = (req_width >> 2) << 2;


	REG_SET_FIELD(&(reg->rPIC_POSITION), m_PIC_START_X, view->x, b_PIC_START_X);
	REG_SET_FIELD(&(reg->rPIC_POSITION), m_PIC_START_Y, view->y, b_PIC_START_Y);

	REG_SET_FIELD(&(reg->rPIC_SOURCE_SIZE), m_PIC_REQ_LENGTH, req_width,  b_PIC_REQ_LENGTH);
	REG_SET_FIELD(&(reg->rPIC_SOURCE_SIZE), m_PIC_REQ_HEIGHT, req_height, b_PIC_REQ_HEIGHT);

	REG_SET_FIELD(&(reg->rPIC_VIEW_SIZE), m_PIC_VIEW_LENGTH, view_width,  b_PIC_VIEW_LENGTH);
	REG_SET_FIELD(&(reg->rPIC_VIEW_SIZE), m_PIC_VIEW_HEIGHT, view_height, b_PIC_VIEW_HEIGHT);

	REG_SET_FIELD(&(reg->rPIC_ZOOM), m_PIC_V_ZOOM, vzoom, b_PIC_V_ZOOM);
	REG_SET_FIELD(&(reg->rPIC_ZOOM), m_PIC_H_ZOOM, hzoom, b_PIC_H_ZOOM);

	REG_SET_FIELD(&(reg->rPIC_V_PHASE), m_PIC_VT_PHASE_BIASE, (vtbias&0xFF), b_PIC_VT_PHASE_BIASE);
	REG_SET_FIELD(&(reg->rPIC_V_PHASE), m_PIC_VB_PHASE_BIASE, (vbbias), b_PIC_VB_PHASE_BIASE);

	if (vdown_scale_en)
		REG_SET_BIT(&(reg->rPIC_CTRL), b_PIC_V_DOWN_SCALE);
	else
		REG_CLR_BIT(&(reg->rPIC_CTRL), b_PIC_V_DOWN_SCALE);

	if (hdown_scale_en)
		REG_SET_BIT(&(reg->rPIC_CTRL), b_PIC_H_DOWN_SCALE);
	else
		REG_CLR_BIT(&(reg->rPIC_CTRL), b_PIC_H_DOWN_SCALE);

	/*### 有待确认正确定*/
	reg->rPIC_ZOOM_FILTER_SIGN = 0x00998080;

	if (hzoom == 4096) {
		REG_SET_BIT(&(reg->rPIC_CTRL), b_PIC_Y_ZOOM_MODE_H);
		REG_SET_FIELD(&(reg->rPIC_PHASE0_H), m_PIC_PHASE_0_H, 0x00ff0000, b_PIC_PHASE_0_H);
	} else {
		REG_CLR_BIT(&(reg->rPIC_CTRL), b_PIC_Y_ZOOM_MODE_H);
		REG_SET_FIELD(&(reg->rPIC_PHASE0_H), m_PIC_PHASE_0_H, 0x00ff0100, b_PIC_PHASE_0_H);
	}

	if (vzoom == 4096) {
		REG_SET_BIT(&(reg->rPIC_CTRL), b_PIC_Y_ZOOM_MODE_V);
		REG_SET_FIELD(&(reg->rPIC_PHASE0_V), m_PIC_PHASE_0_V, 0x00ff0000, b_PIC_PHASE_0_V);
	} else {
		REG_CLR_BIT(&(reg->rPIC_CTRL), b_PIC_Y_ZOOM_MODE_V);
		REG_SET_FIELD(&(reg->rPIC_PHASE0_V), m_PIC_PHASE_0_V, 0x00ff0100, b_PIC_PHASE_0_V);
	}

	/*#### 缩放系数仍采用原来的 */
	for (i = 0; i < 64; i++)
		REG_SET_FIELD(&(reg->rPIC_ZOOM_PARA_V[i]), m_PIC_ZOOM_PARA_V, sPhaseNomal[i], b_PIC_ZOOM_PARA_V);

	for (i = 0; i < 64; i++)
		REG_SET_FIELD(&(reg->rPIC_ZOOM_PARA_H[i]), m_PIC_ZOOM_PARA_H, sPhaseNomal[i], b_PIC_ZOOM_PARA_H);

	/*### 硬件提供系数*/
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA1), m_PIC_FIR3, val, b_PIC_FIR3);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA1), m_PIC_FIR2, val, b_PIC_FIR2);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA1), m_PIC_FIR1, val, b_PIC_FIR1);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA1), m_PIC_FIR0, val, b_PIC_FIR0);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA2), m_PIC_FIR7, val, b_PIC_FIR7);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA2), m_PIC_FIR6, val, b_PIC_FIR6);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA2), m_PIC_FIR5, val, b_PIC_FIR5);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA2), m_PIC_FIR4, val, b_PIC_FIR4);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA3), m_PIC_FIR9, val, b_PIC_FIR9);
	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA3), m_PIC_FIR8, val, b_PIC_FIR8);
//	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA4), m_PIC_FIR_BY_PASS, val, b_PIC_FIR_BY_PASS);
//	REG_SET_FIELD(&(reg->rPIC_HFILTER_PARA4), m_PIC_FIR_SIGN, val, b_PIC_FIR_SIGN);

	/*### 待确认如何配置*/
	//REG_SET_FIELD(&(reg->rPIC_ZOOM_FILTER_SIGN), m_PIC_PHASE0_V_PN, val, b_PIC_PHASE0_V_PN);
	//REG_SET_FIELD(&(reg->rPIC_ZOOM_FILTER_SIGN), m_PIC_PHASE0_H_PN, val, b_PIC_PHASE0_H_PN);
	//REG_SET_FIELD(&(reg->rPIC_ZOOM_FILTER_SIGN), m_PIC_V_PN_CHG_POS, val, b_PIC_V_PN_CHG_POS);
	//REG_SET_FIELD(&(reg->rPIC_ZOOM_FILTER_SIGN), m_PIC_H_PN_CHG_POS, val, b_PIC_H_PN_CHG_POS);

	return 0;
}

int cygnus_spp_enable(int enable)
{
	struct cygnus_vpu_pic_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PIC);

	if (enable)
		REG_SET_BIT(&(reg->rPIC_ENABLE), b_PIC_EN);
	else
		REG_CLR_BIT(&(reg->rPIC_ENABLE), b_PIC_EN);

	return 0;
}

int cygnus_spp_alpha(GxAlpha alpha)
{
	struct cygnus_vpu_pic_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PIC);
	return 0;
}

int cygnus_spp_bg_color(GxColor color)
{
	unsigned val = (color.y << 16 | color.cb << 8 | color.cr);
	struct cygnus_vpu_pic_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PIC);

	REG_SET_FIELD(&(reg->rPIC_BACKCOLOR), m_PIC_BACK_COLOR, val, b_PIC_BACK_COLOR);
	return 0;
}

static int cygnus_spp_main_surface(CygnusVpuSurface *surface)
{
	int stride;
	int fmt, play_mode = PLAY_MODE_FRAME;
	int addr_y, addr_uv, byte_per_line;
	struct cygnus_vpu_pic_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PIC);

	/*data format*/
	if (surface->color_format == GX_COLOR_FMT_YCBCR420_Y_UV)
		fmt = 0;
	if (surface->color_format == GX_COLOR_FMT_YCBCR422_Y_UV)
		fmt = 1;
	if (surface->color_format == GX_COLOR_FMT_YCBCR422)
		fmt = 2;
	if (surface->color_format == GX_COLOR_FMT_YCBCRA6442)
		fmt = 3;
	REG_SET_FIELD(&(reg->rPIC_FRAME_PARA), m_PIC_MODE, fmt, b_PIC_MODE);

	/*frame or field mode*/
	if (play_mode==PLAY_MODE_FRAME)
		REG_SET_BIT(&(reg->rPIC_FRAME_PARA), b_FRAME_MODE_PIC);
	else
		REG_CLR_BIT(&(reg->rPIC_FRAME_PARA), b_FRAME_MODE_PIC);

	/*src w&h*/
	REG_SET_FIELD(&(reg->rPIC_SOURCE_SIZE), m_PIC_REQ_LENGTH, surface->width,  b_PIC_REQ_LENGTH);
	REG_SET_FIELD(&(reg->rPIC_SOURCE_SIZE), m_PIC_REQ_HEIGHT, surface->height, b_PIC_REQ_HEIGHT);
	/*src stride*/
	stride = surface->width;
	REG_SET_FIELD(&(reg->rPIC_FRAME_PARA), m_PIC_FRAME_STRIDE, surface->width, b_PIC_FRAME_STRIDE);

	/*### endian*/
	REG_SET_FIELD(&(reg->rPIC_ZOOM_FILTER_SIGN), m_PIC_ENDIAN_MODE, 0, b_PIC_ENDIAN_MODE);
	reg->rPIC_ZOOM_FILTER_SIGN = 0x00998080;

	/*addr*/
	if( GX_COLOR_FMT_YCBCR422_Y_UV == surface->color_format ||\
		GX_COLOR_FMT_YCBCR420_Y_UV == surface->color_format ||\
		GX_COLOR_FMT_YCBCR420      == surface->color_format )
		byte_per_line = surface->width;
	else
		byte_per_line = surface->width *gx_color_get_bpp(surface->color_format) >> 3;
	addr_y = gx_virt_to_phys((unsigned)surface->buffer);
	/*addr_y*/
	REG_SET_FIELD(&(reg->rPIC_Y_TOP_ADDR), m_PIC_Y_TOP_FIELD_START_ADDR, addr_y, b_PIC_Y_TOP_FIELD_START_ADDR);
	REG_SET_FIELD(&(reg->rPIC_Y_BOTTOM_ADDR), m_PIC_Y_BOTTOM_FIELD_START_ADDR, addr_y + byte_per_line, b_PIC_Y_BOTTOM_FIELD_START_ADDR);
	/*addr_y*/
	addr_uv = addr_y + surface->width * surface->height;
	REG_SET_FIELD(&(reg->rPIC_UV_TOP_ADDR), m_PIC_UV_TOP_FIELD_START_ADDR, addr_uv, b_PIC_UV_TOP_FIELD_START_ADDR);
	REG_SET_FIELD(&(reg->rPIC_UV_BOTTOM_ADDR), m_PIC_UV_BOTTOM_FIELD_START_ADDR, addr_uv+byte_per_line, b_PIC_UV_BOTTOM_FIELD_START_ADDR);

	REG_SET_VAL(&(reg->rPIC_HFILTER_PARA4), 0x80000000);

	return 0;
}

unsigned int cygnus_vpu_get_scan_line(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return REG_GET_FIELD(&(reg->rSYS_STATUS), m_ACTIVE_LINE_COUNTER, b_ACTIVE_LINE_COUNTER);
}

unsigned int cygnus_vpu_check_layers_close(void)
{
	/*### 待硬件增加寄存器*/
	return 1;
}


int vpp2_enable(int en)
{
	struct cygnus_vpu_pp2_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP2);

	REG_SET_FIELD(&(reg->rPP2_CTRL), m_PP2_EN, (en&0x1), b_PP2_EN);
	REG_SET_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);

	return 0;
}

int vpp2_zoom(GxAvRect *clip, GxAvRect *view)
{
	struct cygnus_vpu_pp2_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP2);

	REG_SET_FIELD(&(reg->rPP2_VIEW_V), m_PP2_VIEW_TOP, view->y, b_PP2_VIEW_TOP);
	REG_SET_FIELD(&(reg->rPP2_VIEW_V), m_PP2_VIEW_BOTTOM, view->y+view->height-1, b_PP2_VIEW_BOTTOM);

	REG_SET_FIELD(&(reg->rPP2_VIEW_H), m_PP2_VIEW_LEFT, view->x, b_PP2_VIEW_LEFT);
	REG_SET_FIELD(&(reg->rPP2_VIEW_H), m_PP2_VIEW_RIGHT, view->x+view->width-1, b_PP2_VIEW_RIGHT);

	REG_SET_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);

	return 0;
}

static int vpp2_main_surface(CygnusVpuSurface *surface)
{
	unsigned fmt;
	unsigned addr_y = 0, addr_u = 0, addr_v = 0;
	unsigned byte_per_line = surface->width * gx_color_get_bpp(surface->color_format) >> 3;
	int val = 0;
	struct cygnus_vpu_pp2_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP2);

	GxAvRect clip = {0}, view = {0};

	clip.x = clip.y = 0;
	clip.width  = surface->width;
	clip.height = surface->height;
	view = clip;

	/*addr*/
	addr_y = gx_virt_to_phys((unsigned)surface->buffer);
	if (surface->color_format == GX_COLOR_FMT_YCBCR422)
		byte_per_line = surface->width * gx_color_get_bpp(surface->color_format) >> 3;
	if ((surface->color_format == GX_COLOR_FMT_YCBCR444_Y_U_V) ||
	    (surface->color_format == GX_COLOR_FMT_YCBCR422_Y_UV))
		byte_per_line = surface->width;
	REG_SET_FIELD(&(reg->rPP2_Y_TOP_START_ADDR), m_PP2_Y_TOP_FIELD_START_ADDR, addr_y, b_PP2_Y_TOP_FIELD_START_ADDR);
	REG_SET_FIELD(&(reg->rPP2_Y_BOTTOM_START_ADDR), m_PP2_Y_BOTTOM_FIELD_START_ADDR, addr_y+byte_per_line, b_PP2_Y_BOTTOM_FIELD_START_ADDR);

	if (surface->color_format == GX_COLOR_FMT_YCBCR444_Y_U_V) {
		addr_u = addr_y + surface->width * surface->height;
		REG_SET_FIELD(&(reg->rPP2_U_TOP_START_ADDR), m_PP2_U_TOP_FIELD_START_ADDR, addr_u, b_PP2_U_TOP_FIELD_START_ADDR);
		REG_SET_FIELD(&(reg->rPP2_U_BOTTOM_START_ADDR), m_PP2_U_BOTTOM_FIELD_START_ADDR, addr_u+byte_per_line, b_PP2_U_BOTTOM_FIELD_START_ADDR);
		addr_v = addr_u + surface->width * surface->height;
		REG_SET_FIELD(&(reg->rPP2_V_TOP_START_ADDR), m_PP2_V_TOP_FIELD_START_ADDR, addr_v, b_PP2_V_TOP_FIELD_START_ADDR);
		REG_SET_FIELD(&(reg->rPP2_V_BOTTOM_START_ADDR), m_PP2_V_BOTTOM_FIELD_START_ADDR, addr_v+byte_per_line, b_PP2_V_BOTTOM_FIELD_START_ADDR);
	} else if (surface->color_format == GX_COLOR_FMT_YCBCR422_Y_UV) {
		addr_u = addr_y + surface->width * surface->height;
		REG_SET_FIELD(&(reg->rPP2_U_TOP_START_ADDR), m_PP2_U_TOP_FIELD_START_ADDR, addr_u, b_PP2_U_TOP_FIELD_START_ADDR);
		REG_SET_FIELD(&(reg->rPP2_U_BOTTOM_START_ADDR), m_PP2_U_BOTTOM_FIELD_START_ADDR, addr_u+byte_per_line, b_PP2_U_BOTTOM_FIELD_START_ADDR);
	}

	/* desc: 数据存储格式格式控制，0：存储格式为UYVY；1：存储格式为YUV422，半交织；2：存储格式为YUV420，半交织;3:存储格式为YUV444，非交织 */
	if (surface->color_format == GX_COLOR_FMT_YCBCR422)
		fmt = 0;
	if (surface->color_format == GX_COLOR_FMT_YCBCR422_Y_UV)
		fmt = 1;
	if (surface->color_format == GX_COLOR_FMT_YCBCR420_Y_UV)
		fmt = 2;
	if (surface->color_format == GX_COLOR_FMT_YCBCR444_Y_U_V)
		fmt = 3;
	REG_SET_FIELD(&(reg->rPP2_MODE_CTRL), m_PP2_DATA_TYPE, fmt, b_PP2_DATA_TYPE);

	/*### w&h, 缺少配置h的寄存器*/
	REG_SET_FIELD(&(reg->rPP2_FRAME_WIDTH),  m_PP2_FRAME_STRIDE,  surface->width,  b_PP2_FRAME_STRIDE);
	//REG_SET_FIELD(&(reg->rPP2_FRAME_HEIGHT), m_PP2_FRAME_HEIGHT, surface->height, b_PP2_FRAME_HEIGHT);

	/*### 待确认如何配置*/
	REG_SET_FIELD(&(reg->rPP2_STEP), m_PP2_ENDIAN_MODE, val, b_PP2_ENDIAN_MODE);
	vpp2_zoom(&clip, &view);

	REG_SET_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);

	return 0;
}

int vpp2_set_backcolor(GxColor color)
{
	unsigned val = (color.y << 16 | color.cb << 8 | color.cr);
	struct cygnus_vpu_pp2_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP2);

	REG_SET_FIELD(&(reg->rPP2_CTRL), m_PP2_BACK_COLOR, val, b_PP2_BACK_COLOR);
	REG_SET_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);

	return 0;
}

static int vpp2_set_alpha(GxAlpha alpha)
{
	struct cygnus_vpu_pp2_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP2);

	REG_SET_FIELD(&reg->rPP2_ALPHA, m_PP2_ALPHA, (alpha.value&0xff), b_PP2_ALPHA);
	REG_SET_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rPP2_PARA_UPDATE), b_PP2_PARA_UPDATE);

	return 0;
}

static int check_spp_on_top(GxLayerID *layers, int num)
{
	int i = 0;
	if(NULL == layers) {
		return (false);
	}

	for(i = num - 1; i >= 0; i--) {
		if(layers[i] == GX_LAYER_SPP) {
			return (true);
		} else if(layers[i] == GX_LAYER_VPP) {
			return (false);
		}
	}

	return (false);
}

static int adjust_layer_sequence(GxLayerID *layers, int num, int enable)
{
	int i = 0, spp_sel = 0, vpp_sel = 0, tmp = 0;

	if(NULL == layers) {
		return (-1);
	}

	if(check_spp_on_top(layers, num) == enable) {
		return (0);
	} else {
		for(i = 0; i < num; i++) {
			if(layers[i] == GX_LAYER_SPP) {
				spp_sel = i;
			}

			if(layers[i] == GX_LAYER_VPP) {
				vpp_sel = i;
			}
		}
		tmp = layers[spp_sel];
		layers[spp_sel] = layers[vpp_sel];
		layers[vpp_sel] = tmp;
	}
	return (0);
}

static int cygnus_spp_on_top(int enable)
{
	GxVpuMixerLayers mixer = {0};
	int ret = 0;

	mixer.id = HD_MIXER;
//	ret = vpu_mixer_get_layers(&mixer);

	ret |= adjust_layer_sequence(mixer.layers, 3, enable);
	ret |= vpu_mixer_set_layers(&mixer);

	return (ret);
}

CygnusVpuLayerOps cygnus_osd_ops = {
	.set_view_port      = cygnus_osd_zoom,
	.set_main_surface   = cygnus_osd_main_surface,
	.set_pan_display    = cygnus_osd_pan_display,
	.set_enable         = cygnus_osd_enable,
	.set_palette        = cygnus_osd_palette,
	.set_alpha          = cygnus_osd_alpha,
	.set_color_format   = cygnus_osd_color_format,
	.set_color_key      = cygnus_osd_color_key,
	.set_color_key_mode = cygnus_osd_color_key_mode,

	.add_region       = cygnus_osd_add_region,
	.remove_region    = cygnus_osd_remove_region,
	.update_region    = cygnus_osd_update_region,
};

CygnusVpuLayerOps cygnus_vpp_ops = {
	.set_view_port    = cygnus_vpp_view_port,
	.set_main_surface = cygnus_vpp_main_surface,
	.set_enable       = cygnus_vpp_enable,
	.set_bg_color     = cygnus_vpp_bg_color,
};

CygnusVpuLayerOps cygnus_spp_ops = {
	.set_view_port    = cygnus_spp_zoom,
	.set_main_surface = cygnus_spp_main_surface,
	.set_enable       = cygnus_spp_enable,
	.set_alpha        = cygnus_spp_alpha,
	.set_bg_color     = cygnus_spp_bg_color,
	.set_on_top       = cygnus_spp_on_top,
};

CygnusVpuLayerOps cygnus_pp2_ops = {
	.set_view_port    = vpp2_zoom,
	.set_main_surface = vpp2_main_surface,
	.set_alpha        = vpp2_set_alpha,
	.set_enable       = vpp2_enable,
	.set_bg_color     = vpp2_set_backcolor,
};

