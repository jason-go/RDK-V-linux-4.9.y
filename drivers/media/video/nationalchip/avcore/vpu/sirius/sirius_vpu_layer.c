#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "sirius_vpu.h"
#include "vpu_color.h"
#include "vout_hal.h"
#include "porting.h"
#include "video.h"
#include "hdmi_hal.h"
#include "sirius_vpu_internel.h"

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

#define VPU_CLOSED() (siriusvpu_info == NULL)

struct osd_reg* get_reg_by_layer_id(GxLayerID layer_id)
{
	struct osd_reg *reg = NULL;

	switch (layer_id) {
		case GX_LAYER_OSD : {
			reg = (struct osd_reg*)&siriusvpu_reg->osd_0;
			break;
		}
		case GX_LAYER_SOSD : {
			reg = (struct osd_reg*)&siriusvpu_reg->osd_1;
			break;
		}
		case GX_LAYER_SOSD2 : {
			reg = (struct osd_reg*)&siriusvpu_reg->osd_2;
			break;
		}
		default : {
			break;
		}
	}

	return reg;
}

struct osd_region** get_region_head_by_layer_id(GxLayerID layer_id)
{
	struct osd_region **region_head = NULL;

	switch (layer_id) {
		case GX_LAYER_OSD : {
			region_head = (struct osd_region **)&(((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_OSD].priv)->region_head);
			break;
		}
		case GX_LAYER_SOSD : {
			region_head = (struct osd_region **)&(((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_SOSD].priv)->region_head);
			break;
		}
		case GX_LAYER_SOSD2 : {
			region_head = (struct osd_region **)&(((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_SOSD2].priv)->region_head);
			break;
		}
		default : {
			break;
		}
	}

	return region_head;
}

static int SiriusVpu_LetterBox(GxLayerID layer, int aspect, GxAvRect *view_rect)
{
	int full_screen;
	GxVpuProperty_Resolution actual_resolution;

	sirius_vpu_GetActualResolution(&actual_resolution);
	if(actual_resolution.xres==view_rect->width && actual_resolution.yres==view_rect->height)
		full_screen = 1;
	else
		full_screen = 0;

	if(full_screen) {
		if(siriusvpu_info->layer[layer].screen == SCREEN_16X9) {
			if(aspect == ASPECTRATIO_4BY3) {
				int fix_width = view_rect->width*3/4;
				view_rect->x       = view_rect->x + ((view_rect->width - fix_width) >> 1);
				view_rect->width   = fix_width;
			}
		}
		else if(siriusvpu_info->layer[layer].screen == SCREEN_4X3) {
			if(aspect == ASPECTRATIO_16BY9) {
				int fix_height = (view_rect->height*3)/4;
				view_rect->y = view_rect->y + ((view_rect->height - fix_height) >> 1);
				view_rect->height  = fix_height;
			}
		}
	}
	else {
		int ah = 0, aw = 0;
		if(siriusvpu_info->layer[layer].screen == SCREEN_16X9) {
			if(aspect == ASPECTRATIO_4BY3) {
				aw = 4 * actual_resolution.xres /16;
				ah = 3 * actual_resolution.yres /9;
			}
		}
		else if(siriusvpu_info->layer[layer].screen == SCREEN_4X3) {
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

static int SiriusVpu_PanScan(GxLayerID layer, int aspect, GxAvRect *clip_rect)
{
	if(siriusvpu_info->layer[layer].screen == SCREEN_16X9) {
		if(aspect == ASPECTRATIO_4BY3) {
			int i = (clip_rect->height*3)>>2;
			clip_rect->y      += ((clip_rect->height - i) >> 1);
			clip_rect->height  = i;
		}
	}
	else if(siriusvpu_info->layer[layer].screen == SCREEN_4X3) {
		if(aspect == ASPECTRATIO_16BY9) {
			clip_rect->x = (clip_rect->width >> 3);
			clip_rect->width -= (clip_rect->x << 1);
		}
	}

	return 0;
}

static int SiriusVpu_RawSize(GxAvRect *clip_rect, GxAvRect *view_rect)
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

static int SiriusVpu_RawRatio(GxAvRect *clip_rect, GxAvRect *view_rect)
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

static int SiriusVpu_4X3PullDown(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_4BY3) {
		SiriusVpu_RawRatio(clip_rect,view_rect);
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
		SiriusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int SiriusVpu_4X3CutOut(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_4BY3) {
		SiriusVpu_RawRatio(clip_rect,view_rect);
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
		SiriusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int SiriusVpu_16X9PullDown(GxLayerID layer, int aspect,GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_16BY9) {
		SiriusVpu_RawRatio(clip_rect,view_rect);
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
		SiriusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int SiriusVpu_16X9CutOut(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	if(aspect == ASPECTRATIO_16BY9) {
		SiriusVpu_RawRatio(clip_rect,view_rect);
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
		SiriusVpu_RawRatio(&raw_view,view_rect);
	}

	return 0;
}

static int SiriusVpu_4X3(GxLayerID layer, int aspect,GxAvRect *clip_rect, GxAvRect *view_rect)
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

static int SiriusVpu_16X9(GxLayerID layer, int aspect, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return 0;
}

void SiriusVpu_GetVppPort(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	GxVpuProperty_Resolution referance;
	GxVpuProperty_Resolution virtual_resolution;

	sirius_vpu_GetVirtualResolution(&virtual_resolution);
	if(VPU_CLOSED())
	{
		clip_rect->x = 0;
		clip_rect->y = 0;
		clip_rect->width = 720;
		clip_rect->height = 576;
		return;
	}

	sirius_vpu_GetActualResolution(&referance);
	view_rect->x      = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].view_rect.x, referance.xres, 1);
	view_rect->y      = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].view_rect.y, referance.yres, 0);
	view_rect->width  = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].view_rect.width,  referance.xres, 1);
	view_rect->height = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].view_rect.height, referance.yres, 0);

	referance.xres    = ((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->frame_width;
	referance.yres    = ((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->frame_height;
	clip_rect->x      = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].clip_rect.x, referance.xres, 1);
	clip_rect->y      = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].clip_rect.y, referance.yres, 0);
	clip_rect->width  = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].clip_rect.width,  referance.xres, 1);
	clip_rect->height = sirius_vpu_VirtualToActual(siriusvpu_info->layer[GX_LAYER_VPP].clip_rect.height, referance.yres, 0);
}

int SiriusVpu_AspectRatio_Adapt(GxLayerID layer, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int aspect = (siriusvpu_info&&siriusvpu_info->layer[GX_LAYER_VPP].priv ? ((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->stream_ratio : -1);

	if (aspect != -1 && (layer == GX_LAYER_VPP || layer == GX_LAYER_SPP) && \
			(siriusvpu_info->layer[layer].surface) && \
			siriusvpu_info->layer[layer].surface->surface_mode == GX_SURFACE_MODE_VIDEO) {
		switch(siriusvpu_info->layer[layer].spec)
		{
		case VP_LETTER_BOX:
			SiriusVpu_LetterBox(layer, aspect, view_rect);
			break;
		case VP_PAN_SCAN:
			SiriusVpu_PanScan(layer, aspect, clip_rect);
			break;
		case VP_COMBINED:
			if(aspect == ASPECTRATIO_4BY3) {
				SiriusVpu_LetterBox(layer, aspect, view_rect);
				SiriusVpu_PanScan(layer, aspect, clip_rect);
			} else if(aspect == ASPECTRATIO_16BY9) {
				SiriusVpu_PanScan(layer, aspect, clip_rect);
				SiriusVpu_LetterBox(layer, aspect, view_rect);
			}
			break;
		case VP_RAW_SIZE:   //原始大小
			SiriusVpu_RawSize(clip_rect, view_rect);
			break;
		case VP_RAW_RATIO:  //原始比例
			SiriusVpu_RawRatio(clip_rect, view_rect);
			break;
		case VP_4X3_PULL:   //4:3拉伸
			SiriusVpu_4X3PullDown(layer, aspect, clip_rect, view_rect);
			break;
		case VP_4X3_CUT:    //4:3裁剪
			SiriusVpu_4X3CutOut(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9_PULL:  //16:9拉伸
			SiriusVpu_16X9PullDown(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9_CUT:   //16:9裁剪
			SiriusVpu_16X9CutOut(layer, aspect, clip_rect, view_rect);
			break;
		case VP_4X3:        //4:3
			SiriusVpu_4X3(layer, aspect, clip_rect, view_rect);
			break;
		case VP_16X9:       //16:9
			SiriusVpu_16X9(layer, aspect, clip_rect, view_rect);
			break;
		case VP_AUTO:
			if(aspect == ASPECTRATIO_4BY3) {
				siriusvpu_info->layer[layer].screen = SCREEN_16X9;
				SiriusVpu_PanScan(layer,aspect, clip_rect);
			}
			else if(aspect == ASPECTRATIO_16BY9) {
				siriusvpu_info->layer[layer].screen = SCREEN_4X3;
				SiriusVpu_LetterBox(layer, aspect, clip_rect);
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

static int SiriusLayer_Get_ActualViewrect(GxLayerID layer, GxAvRect *view_rect)
{
	GxVpuProperty_Resolution referance;
	GxVpuProperty_Resolution virtual_resolution;

	if(VPU_CLOSED())
		return -1;

	sirius_vpu_GetActualResolution(&referance);
	sirius_vpu_GetVirtualResolution(&virtual_resolution);
	view_rect->x     = sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].view_rect.x,	referance.xres, 1);
	view_rect->y     = sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].view_rect.y, referance.yres, 0);
	view_rect->width = sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].view_rect.width,  referance.xres, 1);
	view_rect->height= sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].view_rect.height, referance.yres, 0);

	return 0;
}

static int SiriusLayer_Get_ActualCliprect(GxLayerID layer, GxAvRect *clip_rect)
{
	SiriusVpuVppPriv *info;
	GxVpuProperty_Resolution referance;

	if(layer==GX_LAYER_VPP) {
		if(siriusvpu_info && siriusvpu_info->layer[GX_LAYER_VPP].priv) {
			info = siriusvpu_info->layer[GX_LAYER_VPP].priv;
			referance.xres	= info->frame_width;
			referance.yres  = info->frame_height;
			clip_rect->x	= sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].clip_rect.x, referance.xres, 1);
			clip_rect->y	= sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].clip_rect.y, referance.yres, 0);
			clip_rect->width = sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].clip_rect.width,  referance.xres, 1);
			clip_rect->height= sirius_vpu_VirtualToActual(siriusvpu_info->layer[layer].clip_rect.height, referance.yres, 0);
		}
		else {
			gx_printf("%s:%d, get actual cliprect failed\n", __FILE__, __LINE__);
			return -1;
		}
	}
	else {
		clip_rect->x     = siriusvpu_info->layer[layer].clip_rect.x;
		clip_rect->y     = siriusvpu_info->layer[layer].clip_rect.y;
		clip_rect->width = siriusvpu_info->layer[layer].clip_rect.width;
		clip_rect->height= siriusvpu_info->layer[layer].clip_rect.height;
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

int SiriusVpu_Zoom(GxLayerID layer)
{
	int ret = -1;
	unsigned long flags;
	GxAvRect clip_rect, view_rect;

	if(VPU_CLOSED())
		return 0;

	if (siriusvpu_info && siriusvpu_info->layer[layer].surface) {
		clip_rect = siriusvpu_info->layer[layer].clip_rect;
		view_rect = siriusvpu_info->layer[layer].view_rect;

		if(siriusvpu_info->layer[layer].auto_zoom) {
			SiriusLayer_Get_ActualViewrect(layer, &view_rect);
			SiriusLayer_Get_ActualCliprect(layer, &clip_rect);
			SiriusVpu_AspectRatio_Adapt(layer, &clip_rect, &view_rect);
			VIEWRECT_ALIGN(view_rect);
			CLIPRECT_ALIGN(clip_rect);
		} else {
			clip_rect = view_rect;
		}

		VPU_SPIN_LOCK();
		ret = siriusvpu_info->layer[layer].ops->set_view_port(&clip_rect, &view_rect);
		VPU_SPIN_UNLOCK();
	}

	return ret;
}

static int osdx_anti_flicker(GxLayerID layer_id, int enable)
{
	struct osd_reg *reg = get_reg_by_layer_id(layer_id);

	if(enable) {
		VPU_OSD_PP_ANTI_FLICKER_ENABLE(reg->rOSD_CTRL);
		VPU_OSD_SET_PHASE_0(reg->rOSD_PHASE_0_V, VPU_OSD_PHASE_0_ZOOM_COEF);
		VPU_OSD_SET_PHASE_0(reg->rOSD_PHASE_0_H, VPU_OSD_PHASE_0_ZOOM_COEF);
		VPU_OSD_CLR_ZOOM_MODE(reg->rOSD_CTRL);
	} else {
		VPU_OSD_PP_ANTI_FLICKER_DISABLE(reg->rOSD_CTRL);
		VPU_OSD_SET_PHASE_0(reg->rOSD_PHASE_0_V, VPU_OSD_PHASE_0_ZOOM_COEF);
		VPU_OSD_SET_PHASE_0(reg->rOSD_PHASE_0_H, VPU_OSD_PHASE_0_ZOOM_COEF);
		VPU_OSD_SET_ZOOM_MODE(reg->rOSD_CTRL);
	}

	return 0;
}

#define OSD_MAINSURFACE_BUF_ADDR(layer_id) \
	(unsigned int)((SiriusVpuSurface*)siriusvpu_info->layer[layer_id].surface)->buffer
#define OSD_MAINSURFACE_DATA_FORMAT(layer_id) \
	((SiriusVpuSurface*)siriusvpu_info->layer[layer_id].surface)->color_format
#define OSD_MAINSURFACE_WIDTH(layer_id)\
	(((SiriusVpuSurface*)siriusvpu_info->layer[layer_id].surface)->width)
static int osdx_zoom(GxLayerID layer_id, GxAvRect *clip_rect, GxAvRect *view_rect)
{
	int vzoom, hzoom;
	unsigned int data_addr = 0, data_offset = 0;
	unsigned int view_width=0, view_height = 0;
	unsigned int hzoom_gate = 0;
	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution    actual_resolution;

	struct osd_reg *reg = get_reg_by_layer_id(layer_id);
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);

	view_width      = view_rect->width;
	view_height     = view_rect->height;

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	sirius_vpu_GetActualResolution(&actual_resolution);

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

	if (gx_color_get_bpp(OSD_MAINSURFACE_DATA_FORMAT(layer_id)) <= 8)
		hzoom_gate = 4096;
	else
		hzoom_gate = 8191;

	if(hzoom > hzoom_gate) {
		VPU_OSD_H_DOWNSCALE_ENABLE(reg->rOSD_CTRL);
		hzoom = (((clip_rect->width >> 1) - 1) << 12) / (view_rect->width - 1);
		if((((clip_rect->width >> 1) - 1) << 12) % (view_rect->width - 1))
			hzoom += 1;
	}
	else
		VPU_OSD_H_DOWNSCALE_DISABLE(reg->rOSD_CTRL);

	data_addr  = gx_virt_to_phys(OSD_MAINSURFACE_BUF_ADDR(layer_id));
	data_offset= gx_color_get_bpp(OSD_MAINSURFACE_DATA_FORMAT(layer_id))*(clip_rect->y*OSD_MAINSURFACE_WIDTH(layer_id)+clip_rect->x)>>3;
	data_addr += data_offset;
	VPU_OSD_SET_DATA_ADDR(region_head->word5, data_addr);

	VPU_OSD_SET_VTOP_PHASE(reg->rOSD_CTRL, 0x00);
	VPU_OSD_SET_WIDTH(region_head->word3, clip_rect->x, clip_rect->width + clip_rect->x - 1);
	VPU_OSD_SET_HEIGHT(region_head->word4, clip_rect->y, clip_rect->height + clip_rect->y - 1);
	VPU_OSD_SET_POSITION(reg->rOSD_POSITION, view_rect->x, view_rect->y);
	VPU_OSD_SET_VIEW_SIZE(reg->rOSD_VIEW_SIZE, view_width, view_height);
	VPU_OSD_SET_VZOOM(reg->rOSD_ZOOM, vzoom);
	VPU_OSD_SET_HZOOM(reg->rOSD_ZOOM, hzoom);
	osdx_anti_flicker(layer_id, siriusvpu_info->layer[layer_id].anti_flicker_en);

	return 0;
}

static int osdx_enable(GxLayerID layer_id, int enable)
{
	struct osd_reg *reg = get_reg_by_layer_id(layer_id);
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);

	if(enable == 0) {
		VPU_OSD_DISABLE(reg->rOSD_CTRL);
	} else {
		VPU_OSD_SET_FIRST_HEAD(reg->rOSD_FIRST_HEAD_PTR, (unsigned int)gx_dma_to_phys((unsigned int)region_head));
		VPU_OSD_ENABLE(reg->rOSD_CTRL);
	}

	return 0;
}

static int osdx_palette(GxLayerID layer_id, GxPalette *palette)
{
	int clut_len;
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);

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
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);

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
	struct osd_reg *reg = get_reg_by_layer_id(layer_id);
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);

	if (format <= GX_COLOR_FMT_CLUT8) {
		VPU_OSD_CLR_ZOOM_MODE_EN_IPS(reg->rOSD_CTRL);
	} else {
		VPU_OSD_SET_ZOOM_MODE_EN_IPS(reg->rOSD_CTRL);
	}

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
	((SiriusVpuOsdPriv*)siriusvpu_info->layer[layer_id].priv)->data_byte_seq = byteseq;
	VPU_OSD_SET_DATA_RW_BYTESEQ(reg->rOSD_PARA, byteseq);

	byteseq = ABCD_EFGH;
	((SiriusVpuOsdPriv*)siriusvpu_info->layer[layer_id].priv)->regionhead_byte_seq = byteseq;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(reg->rOSD_PARA, byteseq);

exit:
	return ret;
}

int osdx_color_key(GxLayerID layer_id, GxColor *color, int enable)
{
	unsigned char r, g, b, a;
	struct osd_reg *reg = get_reg_by_layer_id(layer_id);
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);
	GxColorFormat format = siriusvpu_info->layer[layer_id].surface->color_format;

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
		VPU_OSD_SET_COLOR_KEY(reg->rOSD_COLOR_KEY, r, g, b, a);
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
static void osdx_common_config(GxLayerID layer_id, SiriusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	GxAvRect rect={0};
	ByteSequence byteseq = ABCD_EFGH;
	unsigned int request_block=0, bpp=1;
	struct osd_reg *reg = get_reg_by_layer_id(layer_id);

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
	((SiriusVpuOsdPriv*)siriusvpu_info->layer[layer_id].priv)->data_byte_seq = byteseq;
	VPU_OSD_SET_DATA_RW_BYTESEQ(reg->rOSD_PARA, byteseq);
	/* set vpu byte-sequence(region head) */
	byteseq = ABCD_EFGH;
	((SiriusVpuOsdPriv*)siriusvpu_info->layer[layer_id].priv)->regionhead_byte_seq = byteseq;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(reg->rOSD_PARA, byteseq);
	REG_SET_FIELD(&(reg->rOSD_PARA),0x7FF<<0, request_block, 0);
	VPU_OSD_SET_POSITION(reg->rOSD_POSITION, rect.x, rect.y);
	osdx_color_key(layer_id, &surface->color_key, surface->color_key_en);
}

static int osdx_add_region(GxLayerID layer_id, SiriusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	int ret = -1;

	if (surface) {
		//struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);
		//if (region_head == NULL) {
		if (1) {
			osdx_common_config(layer_id, surface, pos_x, pos_y);
			siriusvpu_info->layer[layer_id].surface = surface;
		}
		ret = sirius_region_add(layer_id, surface, pos_x, pos_y);
	}

	return ret;
}

static OsdRegion* osdx_revoke_region(GxLayerID layer_id, SiriusVpuSurface *surface)
{
	return sirius_region_revoke(layer_id, surface);
}

static int osdx_remove_region(GxLayerID layer_id, SiriusVpuSurface *surface)
{
	return sirius_region_remove(layer_id, surface);
}

static int osdx_update_region(GxLayerID layer_id, SiriusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return sirius_region_update(layer_id, surface, pos_x, pos_y);
}

static int osdx_reconfig_region(GxLayerID layer_id, OsdRegion *region, VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	osdx_common_config(layer_id, surface, pos_x, pos_y);
	siriusvpu_info->layer[layer_id].surface = surface;
	return sirius_region_reconfig(layer_id, region, surface, pos_x, pos_y);
}

static int osdx_main_surface(GxLayerID layer_id, SiriusVpuSurface *surface)
{
	unsigned ret = -1;
	unsigned pos_x = 0, pos_y = 0;
	struct osd_region *region_head = *get_region_head_by_layer_id(layer_id);
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
	
	if((surface->color_format == GX_COLOR_FMT_ARGB1555) ||
	   (surface->color_format == GX_COLOR_FMT_RGBA5551)){
		REG_SET_VAL(&(siriusvpu_reg->osd_0.rOSD_ALPHA_5551), 0x00FF00);
	}

	return ret;
}

static int sirius_osd_anti_flicker(int enable)
{
	return osdx_anti_flicker(GX_LAYER_OSD, enable);
}

static int sirius_osd_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return osdx_zoom(GX_LAYER_OSD, clip_rect, view_rect);
}

static int sirius_osd_enable(int enable)
{
	return osdx_enable(GX_LAYER_OSD, enable);
}

static int sirius_osd_palette(GxPalette *palette)
{
	return osdx_palette(GX_LAYER_OSD, palette);
}

static int sirius_osd_alpha(GxAlpha alpha)
{
	return osdx_alpha(GX_LAYER_OSD, alpha);
}

static int sirius_osd_color_format(GxColorFormat format)
{
	return osdx_color_format(GX_LAYER_OSD, format);
}

static int sirius_osd_color_key(GxColor *color, int enable)
{
	return osdx_color_key(GX_LAYER_OSD, color, enable);
}

static int sirius_osd_pan_display(void *buffer)
{
	OsdRegion **p_region_header = OSD_HEADPTR;

	if (buffer && *p_region_header) {
		OsdRegion *region = *p_region_header;
		VPU_OSD_SET_DATA_ADDR(region->word5, gx_virt_to_phys((unsigned int)buffer));
	}

	return 0;
}

static int sirius_osd_add_region(SiriusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return osdx_add_region(GX_LAYER_OSD, surface, pos_x, pos_y);
}

static int sirius_osd_remove_region(SiriusVpuSurface *surface)
{
	return osdx_remove_region(GX_LAYER_OSD, surface);
}

static int sirius_osd_update_region(SiriusVpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	return osdx_update_region(GX_LAYER_OSD, surface, pos_x, pos_y);
}

static int sirius_osd_main_surface(SiriusVpuSurface *surface)
{
	return osdx_main_surface(GX_LAYER_OSD, surface);
}
static int sirius_osd_on_top(int enable)
{
	/*
	   if(enable == 0)
	   VPU_SOSD_SET_ON_TOP_ENABLE(siriusvpu_reg->rSOSD_CTRL);
	   else
	   VPU_SOSD_SET_ON_TOP_DISABLE(siriusvpu_reg->rSOSD_CTRL);
	   */
	return 0;
}

int siriussosd_anti_flicker(int enable)
{
	return osdx_anti_flicker(GX_LAYER_SOSD, enable);
}

int siriussosd_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return osdx_zoom(GX_LAYER_SOSD, clip_rect, view_rect);
}

int siriussosd_enable(int enable)
{
	return osdx_enable(GX_LAYER_SOSD, enable);
}

int siriussosd_alpha(GxAlpha alpha)
{
	return osdx_alpha(GX_LAYER_SOSD, alpha);
}

int siriussosd_color_format(GxColorFormat format)
{
	return osdx_color_format(GX_LAYER_SOSD, format);
}

int siriussosd_color_key(GxColor *color, int enable)
{
	return osdx_color_key(GX_LAYER_SOSD, color, enable);
}

int siriussosd_main_surface(SiriusVpuSurface *surface)
{
	return osdx_main_surface(GX_LAYER_SOSD, surface);
}

int siriussosd_on_top(int enable)
{
	/*
	   if(enable == 0)
	   VPU_SOSD_SET_ON_TOP_ENABLE(siriusvpu_reg->rSOSD_CTRL);
	   else
	   VPU_SOSD_SET_ON_TOP_DISABLE(siriusvpu_reg->rSOSD_CTRL);
	   */
	return 0;
}

int siriussosd2_anti_flicker(int enable)
{
	return osdx_anti_flicker(GX_LAYER_SOSD2, enable);
}

int siriussosd2_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
{
	return osdx_zoom(GX_LAYER_SOSD2, clip_rect, view_rect);
}

int siriussosd2_enable(int enable)
{
	return osdx_enable(GX_LAYER_SOSD2, enable);
}

int siriussosd2_alpha(GxAlpha alpha)
{
	return osdx_alpha(GX_LAYER_SOSD2, alpha);
}

int siriussosd2_color_format(GxColorFormat format)
{
	return osdx_color_format(GX_LAYER_SOSD2, format);
}

int siriussosd2_color_key(GxColor *color, int enable)
{
	return osdx_color_key(GX_LAYER_SOSD2, color, enable);
}

int siriussosd2_main_surface(SiriusVpuSurface *surface)
{
	return osdx_main_surface(GX_LAYER_SOSD2, surface);
}

int siriussosd2_on_top(int enable)
{
	/*
	   if(enable == 0)
	   VPU_SOSD_SET_ON_TOP_ENABLE(siriusvpu_reg->rSOSD_CTRL);
	   else
	   VPU_SOSD_SET_ON_TOP_DISABLE(siriusvpu_reg->rSOSD_CTRL);
	   */
	return 0;
}
unsigned int sirius_vpp_calc_requestblock(unsigned int old_width, unsigned int new_width)
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

int sirius_vpp_view_port(GxAvRect *clip_rect, GxAvRect *view_rect)
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
	DispControler *controler = &((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	view_width    = view_rect->width;
	view_height   = view_rect->height;
	src_width     = clip_rect->width;
	src_height    = clip_rect->height;
	baseline      = controler->last_frame.baseline;
	play_mode     = ((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->play_mode;
	old_src_width = VPU_VPP_GET_PP_SOURCE_WIDTH(siriusvpu_reg->rPP_SOURCE_SIZE);
	request_block = sirius_vpp_calc_requestblock(old_src_width, src_width);
	//gx_printf("clip[%d, %d], view[%d, %d]\n", src_width, src_height, view_width, view_height);

	VPP_GET_ZOOM_PARA(hzoom, clip_rect->width, view_rect->width, hdown_scale_en);
	while(VPU_VPP_ZOOM_OUT_MAX < hzoom) {
		//src_width >>= 1;
		//src_width = (src_width >> 3) << 3;
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
	VPU_VPP_SET_FIELD_MODE_UV_FIELD(siriusvpu_reg->rPP_CTRL, uv_mode);

	VPU_VPP_SET_PP_SOURCE_SIZE(siriusvpu_reg->rPP_SOURCE_SIZE, (src_width), (src_height));
	VPU_VPP_SET_REQUEST_BLOCK(siriusvpu_reg->rPP_FRAME_STRIDE, request_block);

	VPU_VPP_SET_PP_POSITION(siriusvpu_reg->rPP_POSITION, view_rect->x, view_rect->y);
	VPU_VPP_SET_PP_SIZE(siriusvpu_reg->rPP_VIEW_SIZE, view_rect->width, view_rect->height);
	VPU_VPP_SET_ZOOM(siriusvpu_reg->rPP_ZOOM, vzoom ,hzoom);
	VPU_VPP_SET_BASE_LINE(siriusvpu_reg->rPP_FRAME_STRIDE, baseline);

	VPU_VPP_SET_VT_PHASE(siriusvpu_reg->rPP_V_PHASE, vtbias & 0x7FFF);
	VPU_VPP_SET_VB_PHASE(siriusvpu_reg->rPP_V_PHASE, vbbias & 0x7FFF);

	switch(vdown_scale_en) {
	case 0:
		VPU_VPP_V_DOWNSCALE_DISABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 0);
		break;
	case 1:
		VPU_VPP_V_DOWNSCALE_ENABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 0);
		//VPU_VPP_SET_BASE_LINE(siriusvpu_reg->rPP_FRAME_STRIDE, baseline*2);
		break;
	case 2:
		VPU_VPP_V_DOWNSCALE_DISABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 1);
		break;
	case 3:
		VPU_VPP_V_DOWNSCALE_ENABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 1);
		break;
	case 4:
		VPU_VPP_V_DOWNSCALE_DISABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 2);
		break;
	case 5:
		VPU_VPP_V_DOWNSCALE_ENABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_V_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 2);
		break;
	default:
		gx_printf("unsupport down_scale\n");
		break;
	}

	switch(hdown_scale_en) {
	case 0:
		VPU_VPP_H_DOWNSCALE_DISABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 0);
		break;
	case 1:
		VPU_VPP_H_DOWNSCALE_ENABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 0);
		break;
	case 2:
		VPU_VPP_H_DOWNSCALE_DISABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 1);
		break;
	case 3:
		VPU_VPP_H_DOWNSCALE_ENABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 1);
		break;
	case 4:
		VPU_VPP_H_DOWNSCALE_DISABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 2);
		break;
	case 5:
		VPU_VPP_H_DOWNSCALE_ENABLE(siriusvpu_reg->rPP_CTRL);
		VPU_VPP_H_DOWNSCALE(siriusvpu_reg->rPP_POSITION, 2);
		break;
	default:
		gx_printf("unsupport down_scale\n");
		break;
	}

	siriusvpu_reg->rPP_FILTER_SIGN = 0x998080;
	if (hzoom == 4096)
		VPU_VPP_SET_PP_PHASE_0_H(siriusvpu_reg->rPP_PHASE_0_H, 0x00ff0000);
	else
		VPU_VPP_SET_PP_PHASE_0_H(siriusvpu_reg->rPP_PHASE_0_H, 0x00ff0100);

	if (vzoom == 4096)
		VPU_VPP_SET_PP_PHASE_0_V(siriusvpu_reg->rPP_PHASE_0_V, 0x00ff0000);
	else
		VPU_VPP_SET_PP_PHASE_0_V(siriusvpu_reg->rPP_PHASE_0_V, 0x00ff0100);

	for (i = 0; i < 64; i++)
		VPU_VPP_SET_PHASE( siriusvpu_reg->rPP_PHASE_PARA_V,i,sPhaseNomal[i]);

	for (i = 0; i < 64; i++)
		VPU_VPP_SET_PHASE( siriusvpu_reg->rPP_PHASE_PARA_H,i,sPhaseNomal[i]);

	return 0;
}

int sirius_vpp_enable(int enable)
{
	if (siriusvpu_info == NULL)
		return 0;

	if(enable == 0) {
		VPU_VPP_DISABLE(siriusvpu_reg->rPP_CTRL);
	}
	else {
		VPU_VPP_ENABLE(siriusvpu_reg->rPP_CTRL);
	}

	return 0;
}

int sirius_vpp_blur(int enable)
{
	return 0;
}

int sirius_vpp_video_mode(GxLayerVideoMode mode)
{
	return 0;
}

int sirius_vpp_bg_color(GxColor color)
{
	VPU_VPP_SET_BG_COLOR(siriusvpu_reg->rPP_BACK_COLOR, color.y, color.cb, color.cr);
	return 0;
}

static int sirius_vpp_get_rdptr(void)
{
	int rd_ptr;
	DispControler *controler = NULL;

	if (VPU_CLOSED())
		return -1;
	controler = &((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	rd_ptr = VPU_DISP_GET_BUFF_ID(siriusvpu_reg->rPP_DISP_R_PTR);
	//gx_printf("\nrd = %d\n", rd_ptr);
	if (siriusvpu_info->reset_flag && rd_ptr == 1) {
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

int sirius_vpp_zoom_require(void)
{
	DispControler *controler = &((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	controler->zoom_require++;
	return 0;
}


static int vpp_play_frame(struct frame *frame)
{
	int ret = -1;
	int rdptr;
	int playmode_changed = 0;
	static int last_topfirst = 1;
	unsigned offset_y = 0, offset_cbcr = 0;
	GxColorimetry c;
	DispControler *controler = NULL;

	if (VPU_CLOSED())
		return -1;
	controler = &((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->controler;

	if (frame) {
		//gx_printf("c = %d\n", frame->colorimetry);
		rdptr = sirius_vpp_get_rdptr();
		//updata frame size
		if (frame->w && frame->h)
			sirius_vpp_set_framesize(frame->w, frame->h);

		if (siriusvpu_info->layer[GX_LAYER_VPP].surface)
			siriusvpu_info->layer[GX_LAYER_VPP].surface->surface_mode = (frame->is_image ? GX_SURFACE_MODE_IMAGE : GX_SURFACE_MODE_VIDEO);

		//update window
		if (controler->zoom_require) {
			sirius_vpp_get_actual_cliprect(&controler->clip_rect);
			sirius_vpp_get_actual_viewrect(&controler->view_rect);
		}

		if (frame->colorimetry && frame->colorimetry != controler->last_frame.colorimetry) {
#if 0
			if (frame->colorimetry == 5 || frame->colorimetry == 6 || frame->colorimetry == 7)
				c = GX_ITU601;
			else if (frame->colorimetry == 1)
				c = GX_ITU709;
			else if (frame->colorimetry == 9)
				c = GX_ITU2020;
			else
				c = GX_EXTENDED_COLORIMETRY;
#endif
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
		offset_y    = controler->clip_rect.x +   frame->baseline * controler->clip_rect.y;
		offset_cbcr = controler->clip_rect.x + ((frame->baseline * controler->clip_rect.y)>>1);
		if (frame->playmode == PLAY_MODE_FIELD) {
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame->playmode,  frame->topfirst);
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame->playmode, !frame->topfirst);
			((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->play_mode = PLAY_MODE_FIELD;
		} else {
			CONFIG_DISP_UNIT(frame->top, frame->bot, offset_y, offset_cbcr, frame->playmode,  frame->topfirst);
			((SiriusVpuVppPriv*)siriusvpu_info->layer[GX_LAYER_VPP].priv)->play_mode = PLAY_MODE_FRAME;
		}

		ret = 0;
		playmode_changed = (controler->last_frame.playmode != frame->playmode);
		siriusvpu_info->reset_flag = 0;
		last_topfirst = frame->topfirst;
		controler->last_frame = *frame;
		if (frame->is_freeze == 1)
			controler->zoom_require += 3;

		//rezoom
		if (controler->zoom_require || playmode_changed) {
			VPU_VPP_SET_BASE_LINE(siriusvpu_reg->rPP_FRAME_STRIDE, frame->baseline);
			sirius_vpp_view_port(&controler->clip_rect, &controler->view_rect);
			controler->zoom_require--;
		}
	}

out:
	return ret;
}

int sirius_vpp_play_frame(struct frame *frame)
{
	int ret = -1;
	unsigned long flags;

	VPU_SPIN_LOCK();
	ret = vpp_play_frame(frame);
	VPU_SPIN_UNLOCK();

	return ret;
}

int sirius_vpp_main_surface(SiriusVpuSurface *surface)
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
			sirius_vpp_set_framesize(surface->width, surface->height);
			VPU_VPP_DISP_CTRL_RESET_CLR(siriusvpu_reg->rPP_DISP_CTRL);
			VPU_VPP_SET_PP_SOURCE_SIZE(siriusvpu_reg->rPP_SOURCE_SIZE, surface->width, surface->height);
			base_line = surface->width;
			VPU_VPP_SET_BASE_LINE(siriusvpu_reg->rPP_FRAME_STRIDE, base_line);

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

int sirius_vpp_set_stream_ratio(unsigned int ratio)
{
	SiriusVpuVppPriv *info;

	if(siriusvpu_info && siriusvpu_info->layer[GX_LAYER_VPP].priv) {
		info = siriusvpu_info->layer[GX_LAYER_VPP].priv;
		if(ratio != info->stream_ratio) {
			info->stream_ratio = ratio;
			return sirius_vpp_zoom_require();
		}
		return 0;
	}
	else
		return -1;
}

int sirius_vpp_set_framesize(unsigned int width, unsigned int height)
{
	int ret = -1;
	SiriusVpuVppPriv *info;

	if (siriusvpu_info && siriusvpu_info->layer[GX_LAYER_VPP].priv) {
		info = siriusvpu_info->layer[GX_LAYER_VPP].priv;
		if (info->frame_width!=width || info->frame_height!=height) {
			info->frame_width = width;
			info->frame_height= height;
			ret = sirius_vpp_zoom_require();
		}
	}

	return ret;
}

int sirius_vpp_set_playmode(GxVpuLayerPlayMode mode)
{
	int ret = -1;
	SiriusVpuVppPriv *info;

	if(siriusvpu_info && siriusvpu_info->layer[GX_LAYER_VPP].priv) {
		info = siriusvpu_info->layer[GX_LAYER_VPP].priv;
		info->play_mode = mode;

		if (siriusvpu_info->layer[GX_LAYER_VPP].surface)
			siriusvpu_info->layer[GX_LAYER_VPP].surface->surface_mode = GX_SURFACE_MODE_VIDEO;

		ret = 0;
	}

	return ret;
}

int sirius_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	GxAvRect clip_rect;

	if(VPU_CLOSED())
		return 0;

	SiriusLayer_Get_ActualViewrect(GX_LAYER_VPP,  view_rect);
	SiriusLayer_Get_ActualCliprect(GX_LAYER_VPP, &clip_rect);
	SiriusVpu_AspectRatio_Adapt(GX_LAYER_VPP, &clip_rect, view_rect);
	VIEWRECT_ALIGN(*view_rect);
	return 0;
}

int sirius_vpp_get_actual_cliprect(GxAvRect *clip_rect)
{
	GxAvRect view_rect;

	if(VPU_CLOSED())
		return 0;

	SiriusLayer_Get_ActualViewrect(GX_LAYER_VPP,  &view_rect);
	SiriusLayer_Get_ActualCliprect(GX_LAYER_VPP, clip_rect);
	SiriusVpu_AspectRatio_Adapt(GX_LAYER_VPP, clip_rect, &view_rect);
	CLIPRECT_ALIGN(*clip_rect);
	return 0;
}

int sirius_spp_zoom(GxAvRect *clip_rect, GxAvRect *view_rect)
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

	VPU_SPP_SET_PLAY_MODE(siriusvpu_reg->rPIC_PARA, play_mode);
	VPU_SPP_SET_PIC_POSITION(siriusvpu_reg->rPIC_POSITION, view_rect->x, view_rect->y);
	VPU_SPP_SET_PIC_SIZE(siriusvpu_reg->rPIC_VIEW_SIZE, view_width, view_height);
	VPU_SPP_SET_ZOOM(siriusvpu_reg->rPIC_ZOOM, vzoom ,hzoom );
	VPU_SPP_SET_PIC_SOURCE_SIZE(siriusvpu_reg->rPIC_SOURCE_SIZE, req_width, req_height);
	VPU_SPP_SET_VT_PHASE(siriusvpu_reg->rPIC_V_PHASE, vtbias & 0xFF);
	VPU_SPP_SET_VB_PHASE(siriusvpu_reg->rPIC_V_PHASE, vbbias);

	if(vdown_scale_en)
		VPU_SPP_V_DOWNSCALE_ENABLE(siriusvpu_reg->rPIC_CTRL);
	else
		VPU_SPP_V_DOWNSCALE_DISABLE(siriusvpu_reg->rPIC_CTRL);

	if(hdown_scale_en)
		VPU_SPP_H_DOWNSCALE_ENABLE(siriusvpu_reg->rPIC_CTRL);
	else
		VPU_SPP_H_DOWNSCALE_DISABLE(siriusvpu_reg->rPIC_CTRL);

	siriusvpu_reg->rPIC_FILTER_SIGN = 0x998080;
	if(hzoom == 4096) {
		VPU_SPP_SET_SPP_PHASE_0_H(siriusvpu_reg->rPIC_PHASE_0_H, 0x00ff0000);
	}
	else {
		VPU_SPP_SET_SPP_PHASE_0_H(siriusvpu_reg->rPIC_PHASE_0_H, 0x00ff0100);
	}
	if(vzoom == 4096) {
		VPU_SPP_SET_SPP_PHASE_0_V(siriusvpu_reg->rPIC_PHASE_0_V, 0x00ff0000);
	}
	else {
		VPU_SPP_SET_SPP_PHASE_0_V(siriusvpu_reg->rPIC_PHASE_0_V, 0x00ff0100);
	}

	if(VPU_SPP_ZOOM_NONE < vzoom) {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( siriusvpu_reg->rPIC_PHASE_PARA_V,i,sPhaseNomal[i]);
	}
	else {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( siriusvpu_reg->rPIC_PHASE_PARA_V,i,sPhaseNomal[i]);
	}

	if(VPU_SPP_ZOOM_NONE < hzoom) {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( siriusvpu_reg->rPIC_PHASE_PARA_H,i,sPhaseNomal[i]);
	}
	else {
		for(i = 0; i < 64; i++)
			VPU_SPP_SET_PHASE( siriusvpu_reg->rPIC_PHASE_PARA_H,i,sPhaseNomal[i]);
	}

	return 0;
}

int sirius_spp_enable(int enable)
{
	if (siriusvpu_info == NULL)
		return 0;

	if(enable == 0) {
		VPU_SPP_DISABLE(siriusvpu_reg->rPIC_CTRL);
	}
	else {
		VPU_SPP_ENABLE(siriusvpu_reg->rPIC_CTRL);
	}

	return 0;
}

int sirius_spp_alpha(GxAlpha alpha)
{
	VPU_SPP_SET_MIXWEIGHT(siriusvpu_reg->rMIX_CTRL2, alpha.value);
	return 0;
}

int sirius_spp_on_top(int enable)
{
	if(CHIP_IS_SIRIUS) {
		if(enable == 0)
			SIRIUS_VPU_SPP_ON_BOTTOM(siriusvpu_reg->rMIX_CTRL2);
		else
			SIRIUS_VPU_SPP_ON_TOP(siriusvpu_reg->rMIX_CTRL2);
	}else {
		if(enable == 0)
			TAURUS_VPU_SPP_ON_BOTTOM(siriusvpu_reg->rLAYER_CTRL);
		else
			TAURUS_VPU_SPP_ON_TOP(siriusvpu_reg->rLAYER_CTRL);
	}

	return 0;
}

int sirius_spp_video_mode(GxLayerVideoMode mode)
{
	return 0;
}

int sirius_spp_color_format(GxColorFormat format)
{
	switch (format) {
	case GX_COLOR_FMT_YCBCRA6442:
		VPU_SPP_SET_COLOR_TYPE(siriusvpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_6442_YCBCRA);
		break;
	case GX_COLOR_FMT_YCBCR422:
		VPU_SPP_SET_COLOR_TYPE(siriusvpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_422_CBYCRY);
		break;
	case GX_COLOR_FMT_YCBCR422_Y_UV:
		VPU_SPP_SET_COLOR_TYPE(siriusvpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_422_Y_CBCR);
		break;
	case GX_COLOR_FMT_YCBCR420:
		VPU_SPP_SET_COLOR_TYPE(siriusvpu_reg->rPIC_PARA, VPU_SPP_COLOR_TYPE_420);
		break;
	default:
		return -1;
	}

	return 0;
}

int sirius_spp_bg_color(GxColor color)
{
	VPU_SPP_SET_BG_COLOR(siriusvpu_reg->rPIC_BACK_COLOR, color.y, color.cb, color.cr);
	return 0;
}

static int sirius_spp_main_surface(SiriusVpuSurface *surface)
{
	if(GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
		int base_line = 0;
		unsigned int data = 0;
		GxAvRect rect = {0, 0, 0, 0};

		unsigned int request_block = 0;
		request_block = rect.width/4/128*128;
		if (request_block < 128) {
			request_block = 256;
		} else if(request_block > 512) {
			request_block = 512;
		}

		REG_SET_FIELD(&(siriusvpu_reg->rPIC_PARA),0x7FF<<16,request_block,16);
		sirius_spp_color_format(surface->color_format);
		rect.width  = surface->width;
		rect.height = surface->height;

		if( GX_COLOR_FMT_YCBCR422_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420      == surface->color_format )
			base_line = rect.width;
		else
			base_line = rect.width *gx_color_get_bpp(surface->color_format) >> 3;

		VPU_SPP_SET_BASE_LINE(siriusvpu_reg->rPIC_PARA, base_line);
		data = (unsigned int)surface->buffer;
		VPU_SPP_SET_Y_TOP_ADDR(siriusvpu_reg->rPIC_Y_TOP_ADDR, data);
		VPU_SPP_SET_Y_BOTTOM_ADDR(siriusvpu_reg->rPIC_Y_BOTTOM_ADDR, data + base_line);

		if( GX_COLOR_FMT_YCBCR422_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420_Y_UV == surface->color_format ||\
				GX_COLOR_FMT_YCBCR420      == surface->color_format ) {
			data = data + base_line *rect.height;
			VPU_SPP_SET_UV_TOP_ADDR(siriusvpu_reg->rPIC_UV_TOP_ADDR, data);
			VPU_SPP_SET_UV_BOTTOM_ADDR(siriusvpu_reg->rPIC_UV_BOTTOM_ADDR, data + base_line);
		}
		return 0;
	}

	return -1;
}

int sirius_bkg_bg_color(GxColor color)
{
	VPU_BKG_SET_COLOR(siriusvpu_reg->rMIX_CTRL2, color.y, color.cb, color.cr);
	return 0;
}

int sirius_bkg_main_surface(SiriusVpuSurface *surface)
{
	sirius_bkg_bg_color(surface->bg_color);
	return 0;
}

unsigned int sirius_vpu_get_scan_line(void)
{
	return VPU_DISP_GET_VIEW_ACTIVE_CNT(siriusvpu_reg->rSCAN_LINE);
}

unsigned int sirius_vpu_check_layers_close(void)
{
	return REG_GET_VAL(&(siriusvpu_reg->rSYS_PARA))&0x01;
}

SiriusVpuLayerOps sirius_osd_ops = {
	.set_view_port    = sirius_osd_zoom,
	.set_main_surface = sirius_osd_main_surface,
	.set_pan_display  = sirius_osd_pan_display,
	.set_enable       = sirius_osd_enable,
	.set_anti_flicker = sirius_osd_anti_flicker,
	.set_palette      = sirius_osd_palette,
	.set_alpha        = sirius_osd_alpha,
	.set_color_format = sirius_osd_color_format,
	.set_color_key    = sirius_osd_color_key,
	.set_on_top       = sirius_osd_on_top,

	.add_region       = sirius_osd_add_region,
	.remove_region    = sirius_osd_remove_region,
	.update_region    = sirius_osd_update_region,
};

SiriusVpuLayerOps sirius_sosd_ops = {
	.set_view_port    = siriussosd_zoom,
	.set_main_surface = siriussosd_main_surface,
	.set_enable       = siriussosd_enable,
	.set_anti_flicker = siriussosd_anti_flicker,
	.set_alpha        = siriussosd_alpha,
	.set_color_format = siriussosd_color_format,
	.set_color_key    = siriussosd_color_key,
	.set_on_top       = siriussosd_on_top,
};

SiriusVpuLayerOps sirius_sosd2_ops = {
	.set_view_port    = siriussosd2_zoom,
	.set_main_surface = siriussosd2_main_surface,
	.set_enable       = siriussosd2_enable,
	.set_anti_flicker = siriussosd2_anti_flicker,
	.set_alpha        = siriussosd2_alpha,
	.set_color_format = siriussosd2_color_format,
	.set_color_key    = siriussosd2_color_key,
	.set_on_top       = siriussosd2_on_top,
};

SiriusVpuLayerOps sirius_vpp_ops = {
	.set_view_port    = sirius_vpp_view_port,
	.set_main_surface = sirius_vpp_main_surface,
	.set_enable       = sirius_vpp_enable,
	.set_anti_flicker = sirius_vpp_blur,
	.set_video_mode   = sirius_vpp_video_mode,
	.set_bg_color     = sirius_vpp_bg_color,
};

SiriusVpuLayerOps sirius_spp_ops = {
	.set_view_port    = sirius_spp_zoom,
	.set_main_surface = sirius_spp_main_surface,
	.set_enable       = sirius_spp_enable,
	.set_alpha        = sirius_spp_alpha,
	.set_on_top       = sirius_spp_on_top,
	.set_video_mode   = sirius_spp_video_mode,
	.set_color_format = sirius_spp_color_format,
	.set_bg_color     = sirius_spp_bg_color,
};

SiriusVpuLayerOps sirius_bkg_ops = {
	.set_main_surface = sirius_bkg_main_surface,
	.set_bg_color     = sirius_bkg_bg_color,
};

