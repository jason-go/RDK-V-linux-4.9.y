#include "kernelcalls.h"
#include "porting.h"
#include "firewall.h"
#include "gxav_bitops.h"
#include "sirius_vpu.h"
#include "vout_hal.h"
#include "vpu_color.h"
#include "sirius_ga.h"
#include "sirius_svpu.h"
#include "sirius_vpu_internel.h"
#include "kernelcalls.h"
#include "clock_hal.h"
#include "video.h"
#include "vpu_module.h"
#include "video_display.h"
#include "sirius_svpu.h"
#include "mempool.h"
#include "log_printf.h"

// Base Address
static unsigned int VPU_REG_BASE_ADDR = 0x8A800000;
unsigned int SVPU_REG_BASE_ADDR = 0x8a900000;
unsigned int GA_REG_BASE_ADDR = 0x8a700000;

// Interrup Number
unsigned int GA_INT_SRC;
unsigned int VPU_INT_SRC;

// VPU info
SiriusVpu *siriusvpu_info = NULL;
gx_spin_lock_t siriusvpu_spin_lock;

// VPU register structure
volatile SiriusVpuReg *siriusvpu_reg = NULL;

// Enhance register structure
static enhance_reg *vpu_enhance_reg = NULL;

unsigned int sirius_osd_filter_table[OSD_FLICKER_TABLE_LEN] = {
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

extern SiriusVpuLayerOps sirius_osd_ops;
extern SiriusVpuLayerOps sirius_sosd_ops;
extern SiriusVpuLayerOps sirius_sosd2_ops;
extern SiriusVpuLayerOps sirius_vpp_ops;
extern SiriusVpuLayerOps sirius_spp_ops;
extern SiriusVpuLayerOps sirius_bkg_ops;

#define M_DIV_N(m, n) \
	((m)%(n)==0 ? (m)/(n) : (m)/(n)+1)

#define MAX_TMP_SURFACE_NUM (10)
#define VPU_CLOSED() (siriusvpu_info == NULL)

#define SURFACE_MEM_POOL_ALLOCZ(p, size) \
	{                                \
		p = gxav_mem_pool_allocz(siriusvpu_info->memory_pool);  \
		if(NULL == p) {                    \
			p = gx_malloc(size);       \
		}             \
	}

#define SURFACE_MEM_POOL_FREE(p)  \
	{   \
		if(gxav_mem_pool_free(siriusvpu_info->memory_pool, p)) {   \
			gx_free(p);   \
			p = NULL;    \
		}    \
	}

static void vpu_triming_init(void)
{
	int i;
	unsigned val = REG_GET_VAL(&siriusvpu_reg->rGIAN_DAC);

	for (i = 1; i <= 4; i++)
		siriusvpu_info->triming.val[i-1] = (val>>((i-1)*8))&0xff;
}

static void vpu_set_update_triming_val(void)
{
	unsigned i, val = 0;

	for (i = 1; i <= 4; i++)
		val |= ((siriusvpu_info->triming.val[i-1] + siriusvpu_info->triming.offset[i-1])&0xff)<<((i-1)*8);
	REG_SET_VAL(&siriusvpu_reg->rGIAN_DAC, val);
}

static void vpu_set_triming_offset(GxVpuDacID dac_id, char offset)
{
	if (dac_id&VPU_DAC_0)
		siriusvpu_info->triming.offset[0] = offset;
	if (dac_id&VPU_DAC_1)
		siriusvpu_info->triming.offset[1] = offset;
	if (dac_id&VPU_DAC_2)
		siriusvpu_info->triming.offset[2] = offset;
	if (dac_id&VPU_DAC_3)
		siriusvpu_info->triming.offset[3] = offset;

	vpu_set_update_triming_val();
}



static int remove_surface_from_list(SiriusVpuSurface *surface)
{
	SiriusVpuSurface *now   = siriusvpu_info->surface;
	SiriusVpuSurface *prev  = siriusvpu_info->surface;

	if(surface == NULL)
		return -1;

	while ((now) && (now != surface)) {
		prev = now;
		now = now->next;
	}
	if(now == NULL)
		return -1;
	prev->next = now->next;

	if(!now->prealloced && now->buffer) {
		gx_page_free(now->buffer, now->width * now->height * gx_color_get_bpp(now->color_format) >> 3);
	}
	if(now == siriusvpu_info->surface)
		siriusvpu_info->surface = prev->next ;
//	gx_free(now);
	SURFACE_MEM_POOL_FREE(now);

	return 0;
}

static int is_in_surface_list(SiriusVpuSurface *surface)
{
	SiriusVpuSurface *now   = siriusvpu_info->surface;

	if(NULL == surface) {
		return (false);
	}

	while ((now) && (now != surface)) {
		now = now->next;
	}

	if(now == surface) {
		return (true);
	} else {
		return (false);
	}
}

int sirius_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
int sirius_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
static int modify_surface_from_list(GxVpuProperty_ModifySurface *property)
{
	GxVpuProperty_LayerMainSurface MainSurface = {0};
	SiriusVpuSurface *surface = NULL;
	int i = 0;

	surface = (SiriusVpuSurface *)(property->surface);
	if(NULL == surface) {
		return (false);
	}

	if(is_in_surface_list(surface)) {
		if(property->width) {
			surface->width = property->width;
		}
		if(property->height) {
			surface->height = property->height;
		}
		if(property->buffer) {
			surface->buffer = property->buffer;
			surface->prealloced = 1;
		}

		if(-1 != property->color_format) {
			surface->color_format = property->color_format;
		}
		if(-1 != property->mode) {
			surface->surface_mode = property->mode;
		}
		for(i = 0; i < GX_LAYER_MAX; i++) {
			MainSurface.layer = i;
			sirius_vpu_GetLayerMainSurface(&MainSurface);
			if(MainSurface.surface == surface) {
				sirius_vpu_SetLayerMainSurface(&MainSurface);
				break;
			}
		}
		return (true);
	} else {
		return (false);
	}
}

int sirius_vpu_GetVirtualResolution(GxVpuProperty_Resolution *property)
{
	if(siriusvpu_info != NULL){
		property->xres = siriusvpu_info->virtual_resolution.xres;
		property->yres = siriusvpu_info->virtual_resolution.yres;
	}
	else{
		property->xres = 1280;
		property->yres = 720;
	}

	return 0;
}

int sirius_vpu_GetActualResolution(GxVpuProperty_Resolution *property)
{
	GxVpuProperty_Resolution    actual_resolution;
	struct vout_dvemode dvemode;

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);

	switch (dvemode.mode){
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
	case GXAV_VOUT_480P:
		actual_resolution.xres = 720;
		actual_resolution.yres = 480;
		break;
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_576I:
	case GXAV_VOUT_576P:
		actual_resolution.xres = 720;
		actual_resolution.yres = 576;
		break;
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		actual_resolution.xres = 1280;
		actual_resolution.yres = 720;
		break;
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ:
		actual_resolution.xres = 1920;
		actual_resolution.yres = 1080;
		break;
	default:
		actual_resolution.xres = 1920;
		actual_resolution.yres = 1080;
		break;
	}
	property->xres = actual_resolution.xres;
	property->yres = actual_resolution.yres;

	return 0;
}

int sirius_vpu_VirtualToActual(int virtual, int referance, int horizontal)
{
	GxVpuProperty_Resolution virtual_resolution;
	sirius_vpu_GetVirtualResolution(&virtual_resolution);

	if(horizontal)
		return virtual*referance/virtual_resolution.xres;
	else
		return virtual*referance/virtual_resolution.yres;
}

int sirius_vpu_GetCreateSurface(GxVpuProperty_CreateSurface *property)
{
	unsigned int bpp;
	SiriusVpuSurface *surface = NULL;
	unsigned int size = 0;

	GXAV_ASSERT(property != NULL);

	if(property->width == 0 || property->height == 0)
		return -1;
	if((property->mode != GX_SURFACE_MODE_IMAGE) &&
			(property->mode != GX_SURFACE_MODE_VIDEO))
		return -1;

	bpp = gx_color_get_bpp(property->format);
	if(bpp < 0)
		return -1;

//	surface = gx_malloc(sizeof(SiriusVpuSurface));
	SURFACE_MEM_POOL_ALLOCZ((surface), (sizeof(SiriusVpuSurface)));
	if(surface == NULL) {
		VPU_PRINTF("[vpu] GetCreateSurface error:[gx_malloc surface return 0] !\n");
		return -1;
	}
	gx_memset(surface, 0, sizeof(SiriusVpuSurface));

	if(property->mode == GX_SURFACE_MODE_IMAGE) {
		if(property->buffer != NULL){
			surface->prealloced = 1;
		}
		else {
			if(property->format == GX_COLOR_FMT_YCBCR420 \
					|| property->format == GX_COLOR_FMT_YCBCR420_Y_UV
					|| property->format == GX_COLOR_FMT_YCBCR420_Y_U_V) {
				size = property->width * (property->height + (property->height >> 1));
			}
			else
				size = property->width *property->height * bpp >> 3;

			if(!size) {
				SURFACE_MEM_POOL_FREE(surface);
				return -1;
			}

			property->buffer = gx_page_malloc(size);
			if(property->buffer == NULL) {
				SURFACE_MEM_POOL_FREE(surface);
				gx_printf("[vpu] GetCreateSurface error:[gx_malloc surface buffer NULL] !\n");
				return -1;
			}
		}
	}
	else
		property->buffer = NULL;

	surface->width          = property->width;
	surface->height         = property->height;
	surface->buffer         = property->buffer;
	surface->alpha.value    = 0xff;
	surface->alpha.type     = GX_ALPHA_GLOBAL;
	surface->surface_mode   = property->mode;
	surface->color_format   = property->format;

	property->surface = (void *)surface;
	surface->next = siriusvpu_info->surface;
	siriusvpu_info->surface = surface;

	return 0;
}

int sirius_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);

static void sirius_check_layer_surface(SiriusVpuSurface *surface)
{
	SiriusSurfaceManager *sm = GET_SURFACE_MANAGER(GX_LAYER_OSD);
	GxVpuProperty_LayerMainSurface main_surface = {0};
	int i = 0;

	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		if(surface == sm->surfaces[i]) {
			sm->surfaces[i] = NULL;
		}
	}

	for(i = GX_LAYER_OSD; i < GX_LAYER_MAX; i++) {
		main_surface.layer = i;
		sirius_vpu_GetLayerMainSurface(&main_surface);
		if(main_surface.surface == surface) {
			sirius_vpu_UnsetLayerMainSurface(&main_surface);
		}
	}
}

int sirius_vpu_SetDestroySurface(GxVpuProperty_DestroySurface *property)
{
	unsigned long flags;

	VPU_SPIN_LOCK();
	sirius_check_layer_surface(property->surface);
	VPU_SPIN_UNLOCK();
	return remove_surface_from_list(property->surface);
}

int sirius_vpu_ModifySurface(GxVpuProperty_ModifySurface *property)
{
	return modify_surface_from_list(property);
}

void SiriusVpu_SetPoint(GxVpuProperty_Point *point)
{
	unsigned int addr;
	SiriusVpuSurface *surface = (SiriusVpuSurface*)point->surface;

	addr = (unsigned int)surface->buffer +
		((gx_color_get_bpp(surface->color_format) * (point->point.y * surface->width + point->point.x)) >> 3);

	if(IS_INVAILD_POINT(&point->point, surface->width, surface->height))
		return;

	gx_color_pixelcolor_write((void*)addr, surface->color_format, &(point->color));
}

int SiriusVpu_GetPoint(GxVpuProperty_Point *point)
{
	void *addr;
	unsigned int value;
	unsigned char byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
	SiriusVpuSurface *surface = (SiriusVpuSurface*)point->surface;

	addr = (void *)((unsigned char *)surface->buffer +
			((gx_color_get_bpp(surface->color_format) *
			  (point->point.y * surface->width + point->point.x)) >> 3));

	switch (gx_color_get_bpp(surface->color_format)) {
	case 8:
		byte3 = (*(unsigned char *)addr);
		break;
	case 16:
		value = GET_ENDIAN_16(*(unsigned short *)addr);
		byte2 = REG_GET_BYTE0(value);
		byte3 = REG_GET_BYTE1(value);
		break;
	case 24:
	case 32:
		value = GET_ENDIAN_32(*(unsigned int *)addr);
		byte0 = REG_GET_BYTE0(value);
		byte1 = REG_GET_BYTE1(value);
		byte2 = REG_GET_BYTE2(value);
		byte3 = REG_GET_BYTE3(value);
		break;
	default:
		return -1;
	}

	switch (surface->color_format) {
	case GX_COLOR_FMT_CLUT8:
		point->color.entry = byte3;
		break;
	case GX_COLOR_FMT_RGBA4444:
		point->color.r = (byte3 & 0xF0) >> 4;
		point->color.g = (byte3 & 0x0F);
		point->color.b = (byte2 & 0xF0) >> 4;
		point->color.a = (byte2 & 0x0F);
		break;
	case GX_COLOR_FMT_ARGB4444:
		point->color.a = (byte3 & 0xF0) >> 4;
		point->color.r = (byte3 & 0x0F);
		point->color.g = (byte2 & 0xF0) >> 4;
		point->color.b = (byte2 & 0x0F);
		break;
	case GX_COLOR_FMT_RGBA5551:
		point->color.r = (byte3 & 0xF8) >> 3;
		point->color.g = (byte3 & 0x07) << 2 | (byte2 & 0xC0) >> 6;
		point->color.b = (byte2 & 0x3F) >> 1;
		point->color.a = (byte2 & 0x01);
		break;
	case GX_COLOR_FMT_ARGB1555:
		point->color.a = (byte3 & 0x80) >> 7;
		point->color.r = (byte3 & 0x7C) >> 2;
		point->color.g = (byte3 & 0x03) << 3 | (byte2 & 0xE0) >> 5;
		point->color.b = (byte2 & 0x1F);
		break;
	case GX_COLOR_FMT_RGB565:
		point->color.r = (byte3 & 0xF8) >> 3;
		point->color.g = (byte3 & 0x07) << 2 | (byte2 & 0xE0) >> 5;
		point->color.b = (byte2 & 0x1F);
		break;
	case GX_COLOR_FMT_RGBA8888:
		point->color.r = byte3;
		point->color.g = byte2;
		point->color.b = byte1;
		point->color.a = byte0;
		break;
	case GX_COLOR_FMT_ARGB8888:
		point->color.a = byte3;
		point->color.r = byte2;
		point->color.g = byte1;
		point->color.b = byte0;
		break;
	case GX_COLOR_FMT_RGB888:
		point->color.r = byte3;
		point->color.g = byte2;
		point->color.b = byte1;
		break;
	case GX_COLOR_FMT_BGR888:
		point->color.b = byte3;
		point->color.g = byte2;
		point->color.r = byte1;
		break;
	case GX_COLOR_FMT_YCBCR422:
		point->color.y  = byte2;
		point->color.cb = byte3;
		point->color.cr = byte1;
		break;
	case GX_COLOR_FMT_YCBCRA6442:
		point->color.y     = (byte3 & 0xFC) >> 2;
		point->color.cb    = (byte3 & 0x03) << 2 | (byte2 & 0xC0) >> 6;
		point->color.cr    = (byte2 & 0x3F) >> 2;
		point->color.alpha = (byte2 & 0x03);
		break;
	default:
		return -1;
	}
	return 0;
}

int SiriusVpu_DrawLine(GxVpuProperty_DrawLine *line)
{
	int is_xline, is_yline, i;
	unsigned int len, width;
	GxVpuProperty_Point point;

	width       = line->width;
	is_xline    = (line->start.y==line->end.y);
	is_yline    = (line->start.x==line->end.x);
	if((!is_xline && !is_yline) || width==0)
		return -1;

	point.color   = line->color;
	point.surface = line->surface;
	if(is_xline) {
		len = GX_ABS(line->end.x-line->start.x) + 1;
		while (width != 0) {
			point.point.x = GX_MIN(line->start.x, line->end.x);
			point.point.y = line->start.y + width - 1;
			for (i = 0; i < len; i++, point.point.x++) {
				SiriusVpu_SetPoint(&point);
			}
			width--;
		}
	}
	if(is_yline) {
		len = GX_ABS(line->end.y-line->start.y) + 1;
		while (width != 0) {
			point.point.y = GX_MIN(line->start.y, line->end.y);
			point.point.x = line->start.x + width - 1;
			for (i = 0; i < len; i++, point.point.y++) {
				SiriusVpu_SetPoint(&point);
			}
			width--;
		}
	}

	return 0;
}

enum {
	CE_LAYER_MIX   = 0,
	CE_LAYER_VPP   = 1,
	CE_LAYER_OSD   = 2,
	CE_LAYER_SPP   = 3,
	CE_LAYER_SOSD  = 4,
	CE_LAYER_SOSD2 = 5,
};
static int siriusce_layerid_get(GxLayerID gx_layer_id)
{
	switch (gx_layer_id) {
	case GX_LAYER_OSD:
		return CE_LAYER_OSD;
	case GX_LAYER_SPP:
		return CE_LAYER_SPP;
	case GX_LAYER_VPP:
		return CE_LAYER_VPP;
	case GX_LAYER_MIX_ALL:
		return CE_LAYER_MIX;
	case GX_LAYER_SOSD:
		return CE_LAYER_SOSD;
	case GX_LAYER_SOSD2:
		return CE_LAYER_SOSD2;
	default:
		return -1;
	}
}

#define CHANGE_VALUE(a,b)      (tmp) = (a); (a) = (b); (b) = (tmp);

static void _adjust_byte_sequence(char *src, int size)
{
	int i = 0;
	char tmp = 0;

	for(i = 0; i < size; i += 8) {
		CHANGE_VALUE(src[i], src[i + 7]);
		CHANGE_VALUE(src[i + 1], src[i + 6]);
		CHANGE_VALUE(src[i + 2], src[i + 5]);
		CHANGE_VALUE(src[i + 3], src[i + 4]);
	}
}

static void _save_y_uv_to_uyvy(void *buffer, SiriusVpuSurface *surface)
{
	char *src = (char *)buffer, *y_addr = NULL, *uv_addr = NULL;
	char *dst = surface->buffer;
	int width = 0, height = 0, i = 0, j = 0;

	width = sirius_svpu_get_sce_length();
	height = sirius_svpu_get_sce_height();

	y_addr = src;
	uv_addr = src + VOUT_BUF_SIZE / 3;

	_adjust_byte_sequence(y_addr, width * height);
	for(i = 0; i < height; i++) {
		_adjust_byte_sequence(uv_addr + i * width, width);
	}

	for(i = 0; i < height; i++) {
		for(j = 0; j < width; j++) {
			dst[(i * surface->width + j) * 2] = uv_addr[i * width + j];
			dst[(i * surface->width + j) * 2 + 1] = y_addr[i * width + j];
		}
	}
}

int SiriusVpu_Capture(GxVpuProperty_LayerCapture *capture)
{
	int i, ce_layer_id;
	SiriusVpuCe   ce;
	unsigned char *src ,*dst;
	unsigned int  width ,line ,request_block = 0;

	if(capture->svpu_cap) {
		SvpuSurfaceInfo buf_info;

		memset(&buf_info, 0, sizeof(SvpuSurfaceInfo));
		siriusvpu_info->svpu_tmp_buffer[0] = gx_page_malloc(VOUT_BUF_SIZE);
		siriusvpu_info->svpu_tmp_buffer[1] = siriusvpu_info->svpu_tmp_buffer[0];
		siriusvpu_info->svpu_tmp_buffer[2] = siriusvpu_info->svpu_tmp_buffer[1];
		siriusvpu_info->svpu_tmp_buffer[3] = siriusvpu_info->svpu_tmp_buffer[2];
		if(NULL == siriusvpu_info->svpu_tmp_buffer[0]) {
			return (-1);
		}

		sirius_svpu_get_buf(&buf_info);
		for(i = 0; i < 4; i++) {
			siriusvpu_info->svpu_buffer[i] = (void *)(buf_info.buf_addrs[i]);
		}
		if(siriusvpu_info->svpu_tmp_buffer[0] && siriusvpu_info->svpu_buffer[0]) {
#if 0
			memcpy(siriusvpu_info->svpu_tmp_buffer[0],
				siriusvpu_info->svpu_buffer[0],
				VOUT_BUF_SIZE);
#else
			GxVpuProperty_BeginUpdate Begin = {0};
			GxVpuProperty_EndUpdate End = {0};
			GxVpuProperty_CreateSurface src = {0}, dst = {0};
			GxVpuProperty_Blit Blit;
			GxVpuProperty_DestroySurface DesSurface = {0};

			src.width = 720 * 3;
			src.height = VOUT_BUF_SIZE / src.width;
			src.format = GX_COLOR_FMT_CLUT8;
			src.mode = GX_SURFACE_MODE_IMAGE;
			src.buffer = siriusvpu_info->svpu_buffer[0];
			gxav_vpu_GetCreateSurface(&src);

			dst.width = 720 * 3;
			dst.height = VOUT_BUF_SIZE / dst.width;
			dst.format = GX_COLOR_FMT_CLUT8;
			dst.mode = GX_SURFACE_MODE_IMAGE;
			dst.buffer = siriusvpu_info->svpu_tmp_buffer[0];
			gxav_vpu_GetCreateSurface(&dst);

			memset(&Blit, 0, sizeof(GxVpuProperty_Blit));
			Begin.max_job_num = 2;
			gxav_vpu_SetBeginUpdate(&Begin);

			Blit.srca.surface = src.surface;
			Blit.srca.is_ksurface = 1;
			Blit.srca.dst_format = GX_COLOR_FMT_CLUT8;
			Blit.srca.rect.x = Blit.srca.rect.y = 0;
			Blit.srca.rect.width = src.width;
			Blit.srca.rect.height = src.height;

			Blit.srcb.surface = dst.surface;
			Blit.srcb.is_ksurface = 1;
			Blit.srcb.dst_format = GX_COLOR_FMT_CLUT8;
			Blit.srcb.rect.x = Blit.srca.rect.y = 0;
			Blit.srcb.rect.width = dst.width;
			Blit.srcb.rect.height = dst.height;

			memcpy(&(Blit.dst), &(Blit.srcb), sizeof(GxBlitObject));
			Blit.mode = GX_ALU_ROP_COPY;
			Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
			Blit.dst.alpha = 0xFF;
			Blit.colorkey_info.src_colorkey_en = 0;
			gxav_vpu_SetBlit(&Blit);

			gxav_vpu_SetEndUpdate(&End);
			DesSurface.surface = src.surface;
			gxav_vpu_SetDestroySurface(&DesSurface);
			DesSurface.surface = dst.surface;
			gxav_vpu_SetDestroySurface(&DesSurface);
#endif
		}
		if(siriusvpu_info->svpu_tmp_buffer[0]) {
			_save_y_uv_to_uyvy(siriusvpu_info->svpu_tmp_buffer[0],
					   (SiriusVpuSurface *) capture->surface);
			gx_page_free(siriusvpu_info->svpu_tmp_buffer[0], VOUT_BUF_SIZE);
			siriusvpu_info->svpu_tmp_buffer[0] = NULL;
		}

		return (0);
	}

	ce.layer    = capture->layer;
	ce.left     = capture->rect.x;
	ce.right    = capture->rect.x + capture->rect.width - 1;
	ce.top      = capture->rect.y;
	ce.bottom   = capture->rect.y + capture->rect.height - 1;
	ce.buffer   = ((SiriusVpuSurface *) capture->surface)->buffer;
	if(capture->layer == GX_LAYER_MIX_VPP_BKG) {
		ce_layer_id = CE_LAYER_VPP;
		VPU_CAP_SET_MIX_BKG_ENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
	}
	else if(capture->layer == GX_LAYER_MIX_SPP_BKG){
		ce_layer_id = CE_LAYER_SPP;
		VPU_CAP_SET_MIX_BKG_ENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
	}
	else {
		ce_layer_id = siriusce_layerid_get(capture->layer);
		VPU_CAP_SET_MIX_BKG_DISENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
	}

	request_block = (ce.right - ce.left)/2/128*128;
	if(request_block < 128) {
		request_block = 128;
	}
	else if(request_block > 896) {
		request_block = 896;
	}

	{
		unsigned int addr, size, align = 1024;
		addr = (unsigned int)gx_virt_to_phys((unsigned int)ce.buffer);
		if(addr % 1024) {
			gxlog_e(LOG_VPU, "addr is not 1k align\n");
			return (-1);
		}
		size = capture->rect.width * capture->rect.height * 2;
		addr = addr / align * align;
		size = size / align * align + align;
		gxav_firewall_register_buffer(GXAV_BUFFER_VPU_CAP, addr, size);
	}

	VPU_VPP_SET_REQUEST_BLOCK(siriusvpu_reg->rPP_FRAME_STRIDE, request_block);
	VPU_CAP_SET_LEVEL(siriusvpu_reg->rCAP_CTRL, ce_layer_id);
	VPU_CAP_SET_PIC_ADDR(siriusvpu_reg->rCAP_ADDR, (unsigned int)gx_virt_to_phys((unsigned int)ce.buffer));
	VPU_CAP_SET_PIC_HORIZONTAL(siriusvpu_reg->rCAP_WIDTH, ce.left, ce.right);
	VPU_CAP_SET_PIC_VERTICAL(siriusvpu_reg->rCAP_HEIGHT, ce.top, ce.bottom);
	VPU_CAP_START(siriusvpu_reg->rCAP_CTRL);
	VPU_CAP_STOP(siriusvpu_reg->rCAP_CTRL);
	gx_dcache_inv_range(0, 0);

	dst = (unsigned char*)ce.buffer;
	src = dst + (capture->rect.width <<1);
	line = (capture->rect.height >> 1);
	width = (capture->rect.width << 2);
	for(i = 0; i < line; i++){
		memcpy(dst,src,(capture->rect.width <<1));
		src += width;
		dst += width;
	}
	gx_dcache_clean_range(0, 0);

	return 0;
}

extern int SiriusVpu_Zoom(GxLayerID layer);
int sirius_vpu_SetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	SiriusVpuLayer *layer = NULL;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	sirius_vpu_GetVirtualResolution(&virtual_resolution);
	if((IS_INVAILD_LAYER(property->layer))
			|| ((siriusvpu_info->layer[property->layer].auto_zoom) && (IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres)))
			|| (IS_NULL(siriusvpu_info->layer[property->layer].surface)))
		return -1;

	layer = &siriusvpu_info->layer[property->layer];

	layer->view_rect = property->rect;
	if (property->layer == GX_LAYER_VPP
			&& (siriusvpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		sirius_vpp_zoom_require();
	} else if(property->layer == GX_LAYER_SPP
			&& (siriusvpu_info->layer[GX_LAYER_SPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		;
	} else {
		gx_mutex_lock(&siriusvpu_info->mutex);
		if(siriusvpu_info->vout_is_ready)
			SiriusVpu_Zoom(property->layer);
		gx_mutex_unlock(&siriusvpu_info->mutex);
	}

	return 0;
}

int sirius_vpu_SetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	SiriusVpuLayer *layer;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	sirius_vpu_GetVirtualResolution(&virtual_resolution);
	if( (IS_INVAILD_LAYER(property->layer))||
			(IS_NULL(siriusvpu_info))||
			(IS_NULL(siriusvpu_info->layer[property->layer].surface)))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
		return -1;

	layer = &siriusvpu_info->layer[property->layer];
	layer->clip_rect.x      = property->rect.x;
	layer->clip_rect.y      = property->rect.y;
	layer->clip_rect.width  = property->rect.width;
	layer->clip_rect.height = property->rect.height;

	if (property->layer == GX_LAYER_VPP &&
			(siriusvpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO){
		return sirius_vpp_zoom_require();
	}

	return 0;
}

static int sirius_vpu_disp_field_error_int_en(void);
static int sirius_vpu_disp_field_start_int_en(void);
int sirius_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	SiriusVpuLayer *layer;
	SiriusVpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	if(false == is_in_surface_list(property->surface)) {
		return (-1);
	}

	surface = (SiriusVpuSurface*)property->surface;
	layer = &siriusvpu_info->layer[property->layer];
	if(layer->ops->set_main_surface != NULL) {
		if((surface->width * gx_color_get_bpp(surface->color_format) >> 3) % 8 != 0)
			return -1;

		layer->surface = (SiriusVpuSurface *) property->surface;
		layer->surface->layer = layer;

		if (property->layer == GX_LAYER_VPP && GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
			sirius_vpu_disp_field_start_int_en();
			sirius_vpu_disp_field_error_int_en();
			siriusvpu_info->layer[GX_LAYER_VPP].flip_require = 1;
			while(siriusvpu_info->layer[GX_LAYER_VPP].flip_require);
			return 0;
		} else {
			layer->clip_rect.x = 0 ;
			layer->clip_rect.y = 0 ;
			layer->clip_rect.width  = layer->surface->width;
			layer->clip_rect.height = layer->surface->height;
			return layer->ops->set_main_surface(layer->surface);
		}
	}

	return -1;
}

int sirius_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	SiriusVpuLayer *layer;
	SiriusVpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	surface = (SiriusVpuSurface*)property->surface;
	layer   = &siriusvpu_info->layer[property->layer];
	if(property->surface == layer->surface) {
		layer->surface = NULL;
		if(layer->enable)
			gx_printf("\n%s:%d, layer is still enabled!\n", __func__, __LINE__);
		//if(layer->ops->unset_main_surface != NULL) {
		//	return layer->ops->unset_main_surface(layer->surface);
		//}
		return 0;
	}

	return -1;
}

#define RUN_LAYER_FUNC(layer_id, func, attr, value) {\
	SiriusVpuLayerOps *layer_ops;\
	GXAV_ASSERT(property != NULL);\
	if (IS_INVAILD_LAYER(layer_id) || siriusvpu_info == NULL)\
	return -1;\
	layer_ops = siriusvpu_info->layer[layer_id].ops;\
	if (layer_ops->func != NULL) {\
		siriusvpu_info->layer[layer_id].attr = value;\
		return layer_ops->func(value);\
	}\
	return -1;\
}\

static int siriusVpu_LayerEnable(GxLayerID layer, int enable)
{
	if (IS_INVAILD_LAYER(layer))
		return -1;
	if (siriusvpu_info->layer[layer].ops->set_enable)
		siriusvpu_info->layer[layer].ops->set_enable(enable);
	return 0;
}

int sirius_vpu_SetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GXAV_ASSERT(property != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	if (property->enable == 0) {
		if(property->layer == GX_LAYER_VPP) {
			gx_video_ppclose_require(0);
		}
		siriusVpu_LayerEnable(property->layer, 0);
		siriusvpu_info->layer[property->layer].enable = 0;
	}
	else {
		if (siriusvpu_info->layer[property->layer].surface == NULL)
			return -1;

		if (siriusvpu_info->vout_is_ready) {
			if(property->layer == GX_LAYER_VPP) {
				gx_video_ppopen_require(0);
			}
			siriusVpu_LayerEnable(property->layer, 1);
		}
		siriusvpu_info->layer[property->layer].enable = 1;
	}


	return 0;
}

int sirius_vpu_SetLayerEnablePatch(GxVpuProperty_LayerEnable *property)
{
	RUN_LAYER_FUNC(property->layer, set_enable, enable, property->enable);
}

int sirius_vpu_SetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	RUN_LAYER_FUNC(property->layer, set_anti_flicker, anti_flicker_en, property->enable);
}

int sirius_vpu_SetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	RUN_LAYER_FUNC(property->layer, set_on_top, on_top, property->enable);
}

int sirius_vpu_SetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	RUN_LAYER_FUNC(property->layer, set_video_mode, video_mode, property->mode);
}

int sirius_vpu_SetLayerMixConfig(GxVpuProperty_LayerMixConfig *property)
{
	if(property->covermode_en) {
		/* covermode en */
		VPU_MIX_LAYER_COVERMODE_ENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
		/* spp alpha config */
		if(property->spp_alpha.type == GX_ALPHA_GLOBAL) {
			VPU_MIX_SPP_GLOBAL_ALPHA_ENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
			VPU_MIX_SET_SPP_GLOBAL_ALPHA(siriusvpu_reg->rPP_JPG_MIX_CTRL, property->spp_alpha.value);
		}
		else {
			VPU_MIX_SPP_GLOBAL_ALPHA_DISENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
		}
		/* vpp alpha config */
		VPU_MIX_SET_VPP_GLOBAL_ALPHA(siriusvpu_reg->rPP_JPG_MIX_CTRL, property->vpp_alpha_value);
	}
	else {
		VPU_MIX_LAYER_COVERMODE_DISENABLE(siriusvpu_reg->rPP_JPG_MIX_CTRL);
	}

	return 0;
}

#define ACTIVE_SURFACE_CHANGE(layer_id, func,value)\
{\
	SiriusVpuLayerOps *layer_ops;\
	if(IS_INVAILD_LAYER(layer_id) || siriusvpu_info == NULL)\
	return -1;\
	layer_ops = siriusvpu_info->layer[layer_id].ops;\
	if(layer_ops->func != NULL)\
	return layer_ops->func(value);\
	return -1;\
}

#define RUN_SURFACE_FUNC(surface_handle, func, attr,value)\
{\
	SiriusVpuSurface *surface;\
	if(IS_NULL(surface_handle) || siriusvpu_info == NULL)\
	return -1;\
	surface = (SiriusVpuSurface *) (surface_handle);\
	surface->attr = value;\
	if(IS_MAIN_SURFACE(surface))\
	ACTIVE_SURFACE_CHANGE(surface->layer->id,func,value);\
	return 0;\
}

int sirius_vpu_SetPalette(GxVpuProperty_Palette *property)
{
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	if(surface->palette == NULL) {
		surface->palette = (GxPalette *) gx_malloc(sizeof(GxPalette));
	}
	if(surface->palette == NULL)
		return -1;

	gx_memcpy(surface->palette, &(property->palette), sizeof(GxPalette));
	if(IS_MAIN_SURFACE(surface)) {
		ACTIVE_SURFACE_CHANGE(surface->layer->id, set_palette, surface->palette);
	}

	return 0;
}

int sirius_vpu_SetAlpha(GxVpuProperty_Alpha *property)
{
	if(property->alpha.type == GX_ALPHA_GLOBAL && property->alpha.value >=256)
		return -1;
	if(property->alpha.type != GX_ALPHA_GLOBAL && property->alpha.type != GX_ALPHA_PIXEL)
		return -1;

	RUN_SURFACE_FUNC(property->surface, set_alpha, alpha, property->alpha);
}

int sirius_vpu_SetColorFormat(GxVpuProperty_ColorFormat *property)
{
	if(property->format<GX_COLOR_FMT_CLUT1 || property->format>GX_COLOR_FMT_YCBCR420_UV)
		return -1;
	RUN_SURFACE_FUNC(property->surface, set_color_format, color_format, property->format);
}

int sirius_vpu_SetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	SiriusVpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &siriusvpu_info->layer[property->layer];
		if(layer->ops->set_bg_color != NULL) {
			layer->bg_color = property->color;
			ret = layer->ops->set_bg_color(property->color);
		}
	}

	return ret;
}

int sirius_vpu_SetColorKey(GxVpuProperty_ColorKey *property)
{
	SiriusVpuSurface *surface;
	SiriusVpuLayerOps *layer_ops;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	surface->color_key = property->color;
	surface->color_key_en = property->enable;
	if(IS_MAIN_SURFACE(surface)) {
		if(IS_INVAILD_LAYER(surface->layer->id))
			return -1;
		layer_ops = siriusvpu_info->layer[surface->layer->id].ops;
		if(layer_ops->set_color_key != NULL) {
			return layer_ops->set_color_key(&surface->color_key, surface->color_key_en);
		}
	}

	return 0;
}

int sirius_vpu_SetPoint(GxVpuProperty_Point *property)
{
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;
	SiriusVpu_SetPoint(property);

	return 0;
}

int sirius_vpu_SetDrawLine(GxVpuProperty_DrawLine *property)
{
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->start, surface->width, surface->height)
			|| IS_INVAILD_POINT(&property->end, surface->width, surface->height))
		return -1;

	return SiriusVpu_DrawLine(property);
}

#define GET_SURFACE(_src_is_ksurface, _src, _dst)\
	do{\
		typeof(_src) src = (_src);\
		typeof(_dst) dst = (_dst);\
		typeof(_src_is_ksurface) src_is_ksurface = (_src_is_ksurface);\
		gx_memset(dst, 0, sizeof(SiriusVpuSurface));\
		if(src_is_ksurface){\
			gx_memcpy(dst, src, sizeof(SiriusVpuSurface));\
		}\
		else{\
			GxSurface ts;\
			gx_copy_from_user(&ts, src, sizeof(GxSurface));\
			dst->buffer = ts.buffer;\
			dst->width  = ts.width;\
			dst->height = ts.height;\
			dst->color_format = ts.color_format;\
			dst->surface_mode = GX_SURFACE_MODE_IMAGE;\
		}\
	}while(0)

int sirius_vpu_SetBlit(GxVpuProperty_Blit *property)
{
	int i, ret=-1;
	SiriusVpuSurface *sura, *surb, *surd;
	GxVpuProperty_Blit kproperty;
	SiriusVpuSurface isrca, isrcb,  idst;

	GXAV_ASSERT(property != NULL);
	if( IS_NULL(property->srca.surface)|| IS_NULL(property->srcb.surface)|| IS_NULL(property->dst.surface) )
		return -1;

	/* copy surface */
	gx_memcpy(&kproperty, property, sizeof(GxVpuProperty_Blit));
	GET_SURFACE(property->srca.is_ksurface, property->srca.surface, &isrca);
	kproperty.srca.surface = &isrca;
	GET_SURFACE(property->srcb.is_ksurface, property->srcb.surface, &isrcb);
	kproperty.srcb.surface = &isrcb;
	GET_SURFACE(property->dst.is_ksurface, property->dst.surface, &idst);
	kproperty.dst.surface = &idst;

	sura = kproperty.srca.surface;
	surb = kproperty.srcb.surface;
	surd = kproperty.dst.surface;
	if( IS_NULL(sura->buffer) ||
			IS_NULL(surb->buffer) || IS_INVAILD_GA_COLOR(surb->color_format)||
			IS_NULL(surd->buffer) || IS_INVAILD_GA_COLOR(surd->color_format) ) {
		gx_printf("BLIT ERROR:surface or clolor type\n");
		return -1;
	}
	if( IS_INVAILD_RECT(&property->srca.rect, sura->width, sura->height)||
			IS_INVAILD_RECT(&property->srcb.rect, surb->width, surb->height)||
			IS_INVAILD_RECT(&property->dst.rect, surd->width, surd->height)) {
		gx_printf("BLIT ERROR:invaild rect\n");
		return -1;
	}

	if(IS_INVALID_ALU_MODE(property->mode)) {
		gx_printf("BLIT ERROR:invaild alu mode \n");
		return -1;
	}

	if( (kproperty.srca.rect.width == kproperty.srcb.rect.width &&
			kproperty.srca.rect.height== kproperty.srcb.rect.height) ||
			(GX_ALU_PAVE == property->mode)) {
		if((GX_COLOR_FMT_YCBCR420_Y_UV == sura->color_format) && (GX_COLOR_FMT_YCBCR420_Y_UV == surd->color_format)) {
			SiriusVpuSurface *sur_tmp;
			GxVpuProperty_Blit tmp_property;

			sur_tmp = (SiriusVpuSurface*)gx_malloc(sizeof(SiriusVpuSurface)* 3);
			gx_memcpy(&sur_tmp[0], sura, sizeof(SiriusVpuSurface));
			gx_memcpy(&sur_tmp[1], surb, sizeof(SiriusVpuSurface));
			gx_memcpy(&sur_tmp[2], surd, sizeof(SiriusVpuSurface));
			gx_memcpy(&tmp_property, &kproperty, sizeof(GxVpuProperty_Blit));
			for (i = 0; i < 2; i++) {
				if(0 == i) {// Y component
					sur_tmp[0].color_format = sur_tmp[1].color_format = sur_tmp[2].color_format = GX_COLOR_FMT_YCBCR420_Y;
				}
				else {// UV component
					sur_tmp[0].color_format = sur_tmp[1].color_format = sur_tmp[2].color_format = GX_COLOR_FMT_YCBCR420_UV;

					tmp_property.srca.rect.height = sur_tmp[0].height = sur_tmp[0].height << 1;
					tmp_property.srcb.rect.height = sur_tmp[1].height = sur_tmp[1].height << 1;
					tmp_property.dst.rect.height = sur_tmp[2].height = sur_tmp[2].height << 1;
				}

				tmp_property.srca.surface = sur_tmp + 0;
				tmp_property.srcb.surface = sur_tmp + 1;
				tmp_property.dst.surface  = sur_tmp + 2;

				ret = siriusga_blit(&tmp_property);
			}
			gx_free(sur_tmp);
		}
		else {
			ret = siriusga_blit(&kproperty);
		}
	}

	return ret;
}

int sirius_vpu_SetDfbBlit(GxVpuProperty_DfbBlit *property)
{
	int i, ret=-1;
	SiriusVpuSurface *sura, *surb, *surd;
	GxVpuProperty_DfbBlit kproperty;
	SiriusVpuSurface isrca, isrcb,  idst;

	GXAV_ASSERT(property != NULL);
	if( IS_NULL(property->basic.srca.surface)|| IS_NULL(property->basic.srcb.surface)|| IS_NULL(property->basic.dst.surface) )
		return -1;

	/* copy surface */
	gx_memcpy(&kproperty, property, sizeof(GxVpuProperty_DfbBlit));
	GET_SURFACE(property->basic.srca.is_ksurface, property->basic.srca.surface, &isrca);
	kproperty.basic.srca.surface = &isrca;
	GET_SURFACE(property->basic.srcb.is_ksurface, property->basic.srcb.surface, &isrcb);
	kproperty.basic.srcb.surface = &isrcb;
	GET_SURFACE(property->basic.dst.is_ksurface, property->basic.dst.surface, &idst);
	kproperty.basic.dst.surface = &idst;

	sura = kproperty.basic.srca.surface;
	surb = kproperty.basic.srcb.surface;
	surd = kproperty.basic.dst.surface;
	if( IS_NULL(sura->buffer) || IS_INVAILD_GA_COLOR(sura->color_format)||
			IS_NULL(surb->buffer) || IS_INVAILD_GA_COLOR(surb->color_format)||
			IS_NULL(surd->buffer) || IS_INVAILD_GA_COLOR(surd->color_format) ) {
		VPU_PRINTF("BLIT ERROR:surface or clolor type\n");
		return -1;
	}
	if( IS_INVAILD_RECT(&property->basic.srca.rect, sura->width, sura->height)||
			IS_INVAILD_RECT(&property->basic.srcb.rect, surb->width, surb->height)||
			IS_INVAILD_RECT(&property->basic.dst.rect, surd->width, surd->height)) {
		VPU_PRINTF("BLIT ERROR:invaild rect\n");
		return -1;
	}

	if(IS_INVALID_ALU_MODE(property->basic.mode)) {
		VPU_PRINTF("BLIT ERROR:invaild alu mode \n");
		return -1;
	}

	if(GX_COLOR_FMT_YCBCR420_Y_UV == sura->color_format) {
		SiriusVpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (SiriusVpuSurface*)gx_malloc(sizeof(SiriusVpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(SiriusVpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(SiriusVpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(SiriusVpuSurface));
		gx_memcpy(&tmp_property, &kproperty.basic, sizeof(GxVpuProperty_Blit));
		for (i = 0; i < 2; i++) {
			if(0 == i) {// Y component
				sur_tmp[0].color_format = sur_tmp[1].color_format = sur_tmp[2].color_format = GX_COLOR_FMT_YCBCR420_Y;
			}
			else {// UV component
				sur_tmp[0].color_format = sur_tmp[1].color_format = sur_tmp[2].color_format = GX_COLOR_FMT_YCBCR420_UV;

				tmp_property.srca.rect.height = sur_tmp[0].height = sur_tmp[0].height << 1;
				tmp_property.srcb.rect.height = sur_tmp[1].height = sur_tmp[1].height << 1;
				tmp_property.dst.rect.height = sur_tmp[2].height = sur_tmp[2].height << 1;
			}

			tmp_property.srca.surface = sur_tmp + 0;
			tmp_property.srcb.surface = sur_tmp + 1;
			tmp_property.dst.surface  = sur_tmp + 2;

			ret = siriusga_blit(&tmp_property);
		}
		gx_free(sur_tmp);
	} else {
		GxAvRect src_rect, dst_rect;
		GxVpuProperty_CreateSurface  tmp_dst;
		GxBlitObject tmp_obj = {0};
		GxBlitObject tmp_bak;
		GxBlitObject tmp_bakd;
		if (property->ifscale) {
			GxVpuProperty_DestroySurface destroy_surface[MAX_TMP_SURFACE_NUM];
			int tmp_cnt= 0;
			tmp_bakd = kproperty.basic.dst;
			tmp_bak  = kproperty.basic.srcb;
			src_rect = kproperty.basic.srca.rect;
			dst_rect = kproperty.basic.srcb.rect;
			while(dst_rect.width*4 < src_rect.width || dst_rect.height*4 < src_rect.height) {
				/* create surface for tmp result */
				tmp_dst.buffer = NULL;
				tmp_dst.format = surd->color_format;
				tmp_dst.mode   = GX_SURFACE_MODE_IMAGE;
				if(dst_rect.width*4 < src_rect.width)
					tmp_dst.width = M_DIV_N(src_rect.width, 4);
				else
					tmp_dst.width = src_rect.width;
				if(dst_rect.height*4 < src_rect.height)
					tmp_dst.height = M_DIV_N(src_rect.height, 4);
				else
					tmp_dst.height = src_rect.height;
				if(sirius_vpu_GetCreateSurface(&tmp_dst)) {
					VPU_PRINTF("%s : create surface failed!\n", __func__);
					goto out;
				}

				/* zoom */
				tmp_obj.alpha_en = 0;
				tmp_obj.cct = NULL;
				tmp_obj.is_ksurface = 1;
				tmp_obj.surface = tmp_dst.surface;
				tmp_obj.rect.x = tmp_obj.rect.y = 0;
				tmp_obj.rect.width = tmp_dst.width;
				tmp_obj.rect.height = tmp_dst.height;
				kproperty.basic.srcb = tmp_obj;
				kproperty.basic.dst = kproperty.basic.srcb;

				//				printk("%s : %d, %d -> %d, %d D: %d,%d\n", __func__, src_rect.width, src_rect.height,
				//						tmp_dst.width, tmp_dst.height,dst_rect.width,dst_rect.height);
				siriusga_dfb_blit(&kproperty);

				/* last zoom result will be the src of next zoom */
				kproperty.basic.srca = tmp_obj;
				src_rect = kproperty.basic.srca.rect;

				/* tmp surface will be destryed */
				destroy_surface[tmp_cnt++].surface = tmp_dst.surface;
				if(tmp_cnt >= MAX_TMP_SURFACE_NUM) {
					VPU_PRINTF("destroy_surface array overflow!(%s():%d)\n", __func__, __LINE__);
					goto out;
				}
			}
			kproperty.basic.srcb = tmp_bak;
			kproperty.basic.dst   = tmp_bakd;
			ret = siriusga_dfb_blit(&kproperty);
out:
			/* destroy tmp surfaces */
			for(i = 0; i < tmp_cnt; i++) {
				if(destroy_surface[i].surface != NULL)
					sirius_vpu_SetDestroySurface(&destroy_surface[i]);
			}
		} else {
			ret = siriusga_dfb_blit(&kproperty);
		}
		return ret;
	}
	return ret;
}

int sirius_vpu_SetBatchDfbBlit(GxVpuProperty_BatchDfbBlit *property)
{
	int i, ret=-1;
	SiriusVpuSurface *sura, *surb, *surd;
	GxVpuProperty_BatchDfbBlit *kproperty;
	SiriusVpuSurface isrca, isrcb,  idst;

	GXAV_ASSERT(property != NULL);
	if( IS_NULL(property->basic.srca.surface)|| IS_NULL(property->basic.srcb.surface)|| IS_NULL(property->basic.dst.surface) )
		return -1;

	if ((kproperty=gx_malloc(sizeof(GxVpuProperty_BatchDfbBlit))) == NULL)
		return -1;

	/* copy surface */
	gx_memcpy(kproperty, property, sizeof(GxVpuProperty_BatchDfbBlit));
	GET_SURFACE(property->basic.srca.is_ksurface, property->basic.srca.surface, &isrca);
	kproperty->basic.srca.surface = &isrca;
	GET_SURFACE(property->basic.srcb.is_ksurface, property->basic.srcb.surface, &isrcb);
	kproperty->basic.srcb.surface = &isrcb;
	GET_SURFACE(property->basic.dst.is_ksurface, property->basic.dst.surface, &idst);
	kproperty->basic.dst.surface = &idst;

	sura = kproperty->basic.srca.surface;
	surb = kproperty->basic.srcb.surface;
	surd = kproperty->basic.dst.surface;
	if( IS_NULL(sura->buffer) || IS_INVAILD_GA_COLOR(sura->color_format)||
			IS_NULL(surb->buffer) || IS_INVAILD_GA_COLOR(surb->color_format)||
			IS_NULL(surd->buffer) || IS_INVAILD_GA_COLOR(surd->color_format) ) {
		VPU_PRINTF("BLIT ERROR:surface or clolor type\n");
		gx_free(kproperty);
		return -1;
	}

	if(IS_INVALID_ALU_MODE(property->basic.mode)) {
		VPU_PRINTF("BLIT ERROR:invaild alu mode \n");
		gx_free(kproperty);
		return -1;
	}

	if(GX_COLOR_FMT_YCBCR420_Y_UV == sura->color_format) {
		SiriusVpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (SiriusVpuSurface*)gx_malloc(sizeof(SiriusVpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(SiriusVpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(SiriusVpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(SiriusVpuSurface));
		gx_memcpy(&tmp_property, &kproperty->basic, sizeof(GxVpuProperty_Blit));
		for (i = 0; i < 2; i++) {
			if(0 == i) {// Y component
				sur_tmp[0].color_format = sur_tmp[1].color_format = sur_tmp[2].color_format = GX_COLOR_FMT_YCBCR420_Y;
			}
			else {// UV component
				sur_tmp[0].color_format = sur_tmp[1].color_format = sur_tmp[2].color_format = GX_COLOR_FMT_YCBCR420_UV;

				tmp_property.srca.rect.height = sur_tmp[0].height = sur_tmp[0].height << 1;
				tmp_property.srcb.rect.height = sur_tmp[1].height = sur_tmp[1].height << 1;
				tmp_property.dst.rect.height = sur_tmp[2].height = sur_tmp[2].height << 1;
			}

			tmp_property.srca.surface = sur_tmp + 0;
			tmp_property.srcb.surface = sur_tmp + 1;
			tmp_property.dst.surface  = sur_tmp + 2;

			ret = siriusga_blit(&tmp_property);
		}
		gx_free(sur_tmp);
	} else {
		ret = siriusga_batch_dfb_blit(kproperty);
		gx_free(kproperty);
		return ret;
	}
	gx_free(kproperty);
	return ret;
}

int sirius_vpu_SetFillRect(GxVpuProperty_FillRect *property)
{
	SiriusVpuSurface surface;
	GxVpuProperty_FillRect kproperty;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	gx_memcpy(&kproperty, property, sizeof(GxVpuProperty_FillRect));
	GET_SURFACE(property->is_ksurface, property->surface, &surface);
	kproperty.surface = &surface;

	if(IS_VIDEO_SURFACE(&surface)||IS_NULL(surface.buffer)||IS_INVAILD_GA_COLOR(surface.color_format))
		return -1;
	if(IS_INVAILD_RECT(&kproperty.rect, surface.width, surface.height))
		return -1;

	return siriusga_fillrect(&kproperty);
}

int sirius_vpu_SetFillPolygon(GxVpuProperty_FillPolygon *property)
{
#if 0
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_GA_COLOR(surface->color_format))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, surface->width, surface->height))
		return -1;

	return siriusga_fillpolygon(property);
#else
	return 0;
#endif
}

int sirius_vpu_SetSetMode(GxVpuProperty_SetGAMode *property)
{
	GXAV_ASSERT(property != NULL);
	return siriusga_setmode(property);
}

int sirius_vpu_SetWaitUpdate(GxVpuProperty_WaitUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return siriusga_wait(property);
}

int sirius_vpu_SetBeginUpdate(GxVpuProperty_BeginUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return siriusga_begin(property);
}

int sirius_vpu_SetEndUpdate(GxVpuProperty_EndUpdate *property)
{
	return siriusga_end(property);
}

int sirius_vpu_SetConvertColor(GxVpuProperty_ConvertColor *property)
{
	GXAV_ASSERT(property != NULL);
	return 0;
}

int sirius_vpu_SetVirtualResolution(GxVpuProperty_Resolution *property)
{
	GXAV_ASSERT(property != NULL);
	siriusvpu_info->virtual_resolution.xres = property->xres;
	siriusvpu_info->virtual_resolution.yres = property->yres;

	siriusvpu_info->layer[GX_LAYER_VPP].clip_rect.width  = property->xres;
	siriusvpu_info->layer[GX_LAYER_VPP].clip_rect.height = property->yres;

	return 0;
}

int sirius_vpu_SetAspectRatio(GxVpuProperty_AspectRatio *property)
{
	GXAV_ASSERT(property != NULL);
	if((property->AspectRatio > VP_AUTO)
			|| (property->AspectRatio < VP_UDEF))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	siriusvpu_info->layer[GX_LAYER_SPP].spec=property->AspectRatio;
	siriusvpu_info->layer[GX_LAYER_VPP].spec=property->AspectRatio;
	sirius_vpp_zoom_require();

	return 0;
}

int sirius_vpu_SetTvScreen(GxVpuProperty_TvScreen *property)
{
	GxVpuProperty_LayerViewport  vpp_viewport, spp_viewport;

	GXAV_ASSERT(property != NULL);
	if((property->Screen > SCREEN_16X9) || (property->Screen < SCREEN_4X3))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	spp_viewport.layer= GX_LAYER_SPP;
	spp_viewport.rect = siriusvpu_info->layer[GX_LAYER_SPP].view_rect;
	siriusvpu_info->layer[GX_LAYER_SPP].screen = property->Screen;
	sirius_vpu_SetLayerViewport(&spp_viewport);

	vpp_viewport.layer= GX_LAYER_VPP;
	vpp_viewport.rect = siriusvpu_info->layer[GX_LAYER_VPP].view_rect;
	siriusvpu_info->layer[GX_LAYER_VPP].screen = property->Screen;
	sirius_vpu_SetLayerViewport(&vpp_viewport);

	return 0;
}

int sirius_vpu_SetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	GXAV_ASSERT(property != NULL);
	siriusvpu_info->vbi.enable = property->enable;
	sirius_vpu_VbiEnable(&siriusvpu_info->vbi);

	return 0;
}

#define GET_LAYER_PROPERTY(attr,layer_attr)\
{\
	GXAV_ASSERT(property != NULL);\
	if(IS_INVAILD_LAYER(property->layer))\
	return -1;\
	if( siriusvpu_info!=NULL ){\
		property->attr  = siriusvpu_info->layer[property->layer].layer_attr ;\
		return 0;\
	}\
	else\
	return -1; \
}

#define GET_SURFACE_PROPERTY(attr,surface_attr)\
{\
	SiriusVpuSurface *surface;\
	GXAV_ASSERT(property != NULL);\
	if(IS_NULL(property->surface))\
	return -1;\
	surface = (SiriusVpuSurface *)(property->surface);\
	property->attr  = surface->surface_attr;\
	return 0;\
}

int sirius_vpu_GetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	GET_LAYER_PROPERTY(rect, view_rect);
}

int sirius_vpu_GetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	GET_LAYER_PROPERTY(rect, clip_rect);
}

int sirius_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	GET_LAYER_PROPERTY(surface, surface);
}

int sirius_vpu_GetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GET_LAYER_PROPERTY(enable, enable);
}

int sirius_vpu_GetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	GET_LAYER_PROPERTY(enable, anti_flicker_en);
}

int sirius_vpu_GetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	GET_LAYER_PROPERTY(enable, on_top);
}

int sirius_vpu_GetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	GET_LAYER_PROPERTY(mode, video_mode);
}

int sirius_vpu_GetLayerCapture(GxVpuProperty_LayerCapture *property)
{
#define IS_POINT_IN_RECT(point,rect)\
	((point.x>=rect.x)&&(point.y>=rect.y)&&(point.x<=rect.x+rect.width)&&(point.y<=rect.y+rect.height))

	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution	referance, virtual_resolution;
	SiriusVpuSurface *surface = (SiriusVpuSurface*)property->surface;
	GxVpuProperty_LayerCapture  cap_property;
	SiriusVpuLayer *layer;
	GxLayerID       layer_id;
	GxAvRect        capture_rect = {0};
	GxAvPoint       end_point    = {0};
	GxAvPoint       point_topleft= {0}, point_topright={0}, point_bottomleft={0}, point_bottomright={0};

	GXAV_ASSERT(property != NULL);
	GXAV_ASSERT(property->surface != NULL);

	sirius_vpu_GetVirtualResolution(&virtual_resolution);
	if(IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
		return -1;

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if((dvemode.mode>=GXAV_VOUT_NULL_MAX)&&(dvemode.mode<GXAV_VOUT_PAL))
		return -1;

	layer_id = property->layer;
	if(layer_id == GX_LAYER_VPP && property->soft_cap) {
		struct cap_frame frame;
		if(surface->color_format!=GX_COLOR_FMT_YCBCR420 && surface->color_format!=GX_COLOR_FMT_YCBCR420_Y_UV) {
			gx_printf("invalid color format for capture surface!\n");
			return -1;
		}
		frame.buf_addr = (unsigned)gx_virt_to_phys((unsigned int)surface->buffer);
		frame.buf_size = (surface->width*surface->height*gx_color_get_bpp(GX_COLOR_FMT_YCBCR420))>>3;
		if(gx_video_cap_frame(0, &frame) != 0) {
			gx_printf("invalid width or height for capture surface!\n");
			return -1;
		}
		return 0;
	} else {
		if(surface->color_format!=GX_COLOR_FMT_YCBCR422) {
			gx_printf("invalid color format for capture surface!\n");
			return -1;
		}
		if((layer_id>=GX_LAYER_OSD)&&(layer_id<=GX_LAYER_BKG) ) {
			point_topleft.x     = property->rect.x;
			point_topleft.y     = property->rect.y;
			point_topright.x    = property->rect.x+property->rect.width;
			point_topright.y    = property->rect.y;
			point_bottomleft.x  = property->rect.x;
			point_bottomleft.y  = property->rect.y+property->rect.height;
			point_bottomright.x = property->rect.x+property->rect.width;
			point_bottomright.y = property->rect.y+property->rect.height;

			layer = &(siriusvpu_info->layer[layer_id]);
			///< ÅÐ¶Ï4µã×ø±êÊÇ·ñÂäÔÚview_rectÄÚ²¿
			if(IS_POINT_IN_RECT(point_topleft,layer->view_rect)
					||IS_POINT_IN_RECT(point_topright, layer->view_rect)
					||IS_POINT_IN_RECT(point_bottomleft, layer->view_rect)
					||IS_POINT_IN_RECT(point_bottomright, layer->view_rect) ) {
				capture_rect.x = GX_MAX(property->rect.x, layer->view_rect.x);
				capture_rect.y = GX_MAX(property->rect.y, layer->view_rect.y);

				end_point.x = GX_MIN(property->rect.x+property->rect.width,
						layer->view_rect.x+layer->view_rect.width);
				end_point.y = GX_MIN(property->rect.y+property->rect.height,
						layer->view_rect.y+layer->view_rect.height);

				capture_rect.width = end_point.x-capture_rect.x;
				capture_rect.height= end_point.y-capture_rect.y;
			}
			else {
				return -1;
			}
		}
		else if(layer_id >= GX_LAYER_MIX_VPP_BKG){
			capture_rect.x = property->rect.x;
			capture_rect.y = property->rect.y;
			capture_rect.width = property->rect.width;
			capture_rect.height = property->rect.height;
		}
		else {
			return -1;
		}

		sirius_vpu_GetActualResolution(&referance);
		cap_property.layer       =  property->layer;
		cap_property.rect.x      =  sirius_vpu_VirtualToActual(capture_rect.x, referance.xres, 1);
		cap_property.rect.y      =  sirius_vpu_VirtualToActual(capture_rect.y, referance.yres, 0);
		cap_property.rect.width  =  sirius_vpu_VirtualToActual(capture_rect.width,  referance.xres, 1);
		cap_property.rect.height =  sirius_vpu_VirtualToActual(capture_rect.height, referance.yres, 0);
		cap_property.surface = (void*)property->surface;
		cap_property.svpu_cap = property->svpu_cap;

		return SiriusVpu_Capture(&cap_property);
	}
}

int sirius_vpu_SetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	GXAV_ASSERT(property != NULL);

	siriusvpu_info->layer[property->layer].auto_zoom = property->auto_zoom;

	return (0);
}

int sirius_vpu_GetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	GXAV_ASSERT(property != NULL);

	property->auto_zoom = siriusvpu_info->layer[property->layer].auto_zoom;

	return (0);
}

int sirius_vpu_GetCreatePalette(GxVpuProperty_CreatePalette *property)
{
	unsigned int  i;
	unsigned int  num_entries = property->num_entries;
	GxPalette * gx_palette;

	if(( property->num_entries!=1) && (property->num_entries!=2) &&
			(property->num_entries!=4) && (property->num_entries!=8) &&
			(property->num_entries!=16) && (property->num_entries!=32) &&
			(property->num_entries!=64) && (property->num_entries!=128)  &&
			(property->num_entries!=256)) {
		gx_printf("[vpu] CreatePalette num_entries error !\n");
		return -1;
	}
	if(property->palette_num==0) {
		gx_printf("[vpu] CreatePalette palette_num error !\n");
		return -1;
	}
	property->palette = (GxPalette *)gx_malloc(property->palette_num * (sizeof(GxPalette) + num_entries * sizeof(GxColor)));
	if(NULL == property->palette) {
		gx_printf("[vpu] CreatePalette malloc NULL ,error! \n");
		return -1;
	}

	gx_palette = property->palette;
	for (i=0; i<property->palette_num; i++) {
		gx_palette->num_entries = num_entries;
		gx_palette->entries= (GxColor*)((unsigned int)(gx_palette)+sizeof(GxPalette));
		gx_palette = (GxPalette*)((unsigned int)gx_palette + sizeof(GxPalette) + num_entries * sizeof(GxColor));
	}

	return 0;
}

int sirius_vpu_DestroyPalette(GxVpuProperty_DestroyPalette *property)
{
	if(property->palette!=NULL)
		gx_free(property->palette);
	property->palette = NULL;

	return 0;
}

int sirius_vpu_SurfaceBindPalette(GxVpuProperty_SurfaceBindPalette *property)
{
	SiriusVpuSurface* surface = (SiriusVpuSurface*)(property->surface);;
	if(NULL == property->surface) {
		gx_printf("[vpu] SurfaceBindPalette surface NULL ,error! \n");
		return -1;
	}
	if(NULL == property->palette) {
		gx_printf("[vpu] SurfaceBindPalette palette NULL ,error! \n");
		return -1;
	}
	if(NULL == surface->palette) {
		surface->palette = (GxPalette *) gx_malloc(sizeof(GxPalette));
		if(NULL == surface->palette) {
			gx_printf("[vpu] SurfaceBindPallete malloc  NULL ,error! \n");
			return -1;
		}
	}

	surface->palette->num_entries = property->palette->num_entries;
	surface->palette->entries     = property->palette->entries;
	if(IS_MAIN_SURFACE(surface)) {
		ACTIVE_SURFACE_CHANGE(surface->layer->id, set_palette, surface->palette);
	}

	return 0;
}

/*from user to kernel*/
int sirius_vpu_WPalette(GxVpuProperty_RWPalette *property)
{
	GxPalette *k_gx_palette;
	GxPalette u_gx_palette;
	int palette_id;
	int num_entries;

	if(property == NULL || property->k_palette == NULL || property->u_palette == NULL)
		return -1;

	k_gx_palette= property->k_palette;
	palette_id  = property->palette_id;
	num_entries = property->k_palette->num_entries;
	k_gx_palette->entries = (GxColor *)((unsigned int)(k_gx_palette)+
			(palette_id + 1) * sizeof(GxPalette) +
			palette_id * num_entries * sizeof(GxColor));

	gx_copy_from_user(&u_gx_palette, property->u_palette, sizeof(GxPalette));
	if(u_gx_palette.num_entries <= num_entries) {
		gx_copy_from_user(k_gx_palette->entries, u_gx_palette.entries, u_gx_palette.num_entries * sizeof(GxColor));
	}

	return 0;
}

/*from kernel to user*/
int sirius_vpu_RPalette(GxVpuProperty_RWPalette *property)
{
	GxPalette * k_gx_palette;
	GxPalette * u_gx_palette;
	int palette_id;
	int num_entries;

	if(property == NULL || property->k_palette == NULL || property->u_palette == NULL)
		return -1;

	k_gx_palette= property->k_palette;
	palette_id  = property->palette_id;
	num_entries = property->k_palette->num_entries;
	k_gx_palette->entries = (GxColor *)((unsigned int)(k_gx_palette)+
			(palette_id + 1) * sizeof(GxPalette) +
			palette_id * num_entries * sizeof(GxColor));

	u_gx_palette = property->u_palette;
	gx_copy_to_user(u_gx_palette->entries,k_gx_palette->entries,num_entries*(sizeof(GxColor)));

	return 0;
}

int sirius_vpu_GetEntries(GxVpuProperty_GetEntries *property)
{
	GxPalette * k_gx_palette;

	if(NULL==property)
		return -1;
	if(NULL==property->k_palette)
		return -1;

	k_gx_palette = property->k_palette;
	property->entries= k_gx_palette->entries;

	return 0;
}

int sirius_vpu_GetReadSurface(GxVpuProperty_ReadSurface *property)
{
	SiriusVpuSurface *surface = NULL;
	SiriusVpuSurface *now = NULL;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(siriusvpu_info))
		return -1;
	surface = (SiriusVpuSurface *) property->surface;
	if(surface == NULL)
		return -1;

	now = siriusvpu_info->surface;
	while ((now) && (now != surface)) {
		now = now->next;
	}
	if(now == NULL)
		return -1;

	property->width  = surface->width;
	property->height = surface->height;
	property->format = surface->color_format;
	property->mode   = surface->surface_mode;
	property->buffer = surface->buffer;

	return 0;
}

int sirius_vpu_GetPalette(GxVpuProperty_Palette *property)
{
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;
	surface = (SiriusVpuSurface *) (property->surface);
	if(surface->palette == NULL)
		return -1;

	gx_memcpy(&property->palette, surface->palette, sizeof(GxPalette));
	return 0;
}

int sirius_vpu_GetAlpha(GxVpuProperty_Alpha *property)
{
	GET_SURFACE_PROPERTY(alpha, alpha);
}

int sirius_vpu_GetColorFormat(GxVpuProperty_ColorFormat *property)
{
	GET_SURFACE_PROPERTY(format, color_format);
}

int sirius_vpu_GetColorKey(GxVpuProperty_ColorKey *property)
{
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	property->color = surface->color_key;
	property->enable= surface->color_key_en;

	return 0;
}

int sirius_vpu_GetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	SiriusVpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &siriusvpu_info->layer[property->layer];
		property->color = layer->bg_color;
		ret = 0;
	}

	return ret;
}

int sirius_vpu_GetPoint(GxVpuProperty_Point *property)
{
	SiriusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (SiriusVpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;

	SiriusVpu_GetPoint(property);
	return 0;
}

int sirius_vpu_GetConvertColor(GxVpuProperty_ConvertColor *property)
{
	return 0;
}

int sirius_vpu_GetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	property->enable = siriusvpu_info->vbi.enable;
	return 0;
}

int sirius_vpu_GetVbiCreateBuffer(GxVpuProperty_VbiCreateBuffer *property)
{
	if(property->unit_num == 0)
		return -1;
	if(property->buffer == NULL) {
		property->buffer = gx_malloc((property->unit_data_len + 1) * 4 *property->unit_num);
		if(property->buffer == NULL)
			return -1;
	}

	sirius_vpu_VbiCreateBuffer(&siriusvpu_info->vbi);
	return 0;
}

int sirius_vpu_GetVbiReadAddress(GxVpuProperty_VbiReadAddress *property)
{
	sirius_vpu_VbiGetReadPtr(&siriusvpu_info->vbi);
	property->read_address = siriusvpu_info->vbi.read_address;
	return 0;
}

int sirius_vpu_ZoomSurface(GxVpuProperty_ZoomSurface *property)
{
	unsigned int ret=0;
	SiriusVpuSurface src_surface, dst_surface;
	GxAvRect src_rect, dst_rect;
	GxVpuProperty_ZoomSurface kproperty;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->src_surface)||IS_NULL(property->dst_surface)) {
		return -1;
	}

	GET_SURFACE(property->src_is_ksurface, property->src_surface, &src_surface);
	GET_SURFACE(property->dst_is_ksurface, property->dst_surface, &dst_surface);
	src_rect = property->src_rect;
	dst_rect = property->dst_rect;
	if( IS_NULL(src_surface.buffer) ||
			IS_VIDEO_SURFACE(&src_surface)||
			IS_VIDEO_SURFACE(&dst_surface)||
			IS_INVAILD_ZOOM_SRC_COLOR(src_surface.color_format)||
			IS_INVAILD_ZOOM_DST_COLOR(dst_surface.color_format)||
			IS_INVAILD_RECT(&src_rect, src_surface.width, src_surface.height)||
			IS_INVAILD_RECT(&dst_rect, dst_surface.width, dst_surface.height)) {
		VPU_PRINTF("please check the mode, buffer pointer and color format of src and dst surface!\n");
		return -1;
	}

	if(dst_rect.width*4 < src_rect.width || dst_rect.height*4 < src_rect.height) {
		VPU_PRINTF("Only dst/src > 1/4 is supported every single zoom!\n");
		return -1;
	}

	/* zoom src to dst */
	VPU_DBG("%s : %d, %d -> %d, %d\n", __func__, src_rect.width, src_rect.height, dst_rect.width, dst_rect.height);
	gx_memset(&kproperty, 0, sizeof(GxVpuProperty_ZoomSurface));
	kproperty.src_surface = &src_surface;
	kproperty.dst_surface = &dst_surface;
	kproperty.src_rect = src_rect;
	kproperty.dst_rect = dst_rect;
	ret = siriusga_zoom(&kproperty);

	return ret ;
}

int sirius_vpu_TurnSurface(GxVpuProperty_TurnSurface *property)
{
	SiriusVpuSurface src_surface, dst_surface;
	GxVpuProperty_TurnSurface kproperty;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->src_surface) || IS_NULL(property->dst_surface)) {
		VPU_PRINTF("the src surface and dst surface cannot be NULL!\n");
		return -1;
	}
	if((property->screw > SCREW_180) || (property->reverse > REVERSE_VERTICAL)) {
		VPU_PRINTF("check params about screw and reverse mode!\n");
		return -1;
	}

	GET_SURFACE(property->src_is_ksurface, property->src_surface, &src_surface);
	if( IS_VIDEO_SURFACE(&src_surface) || IS_NULL(src_surface.buffer)||
			IS_INVAILD_TURN_COLOR(src_surface.color_format)||
			IS_INVAILD_RECT(&property->src_rect, src_surface.width, src_surface.height)) {
		VPU_PRINTF("please check the mode, buffer pointer and color format of src surface!\n");
		return -1;
	}
	GET_SURFACE(property->dst_is_ksurface, property->dst_surface, &dst_surface);
	if( IS_VIDEO_SURFACE(&dst_surface) || IS_NULL(dst_surface.buffer) ||
			IS_INVAILD_TURN_COLOR(dst_surface.color_format) ||
			IS_INVAILD_RECT(&property->dst_rect, dst_surface.width, dst_surface.height)) {
		VPU_PRINTF("please check the mode, buffer pointer and color format of dst surface!\n");
		return -1;
	}

	gx_memset(&kproperty, 0, sizeof(GxVpuProperty_TurnSurface));
	kproperty.screw			= property->screw;
	kproperty.reverse		= property->reverse;
	kproperty.src_rect		= property->src_rect;
	kproperty.dst_rect		= property->dst_rect;
	kproperty.src_surface	= &src_surface;
	kproperty.dst_surface	= &dst_surface;

	return siriusga_turn(&kproperty);
}

int sirius_vpu_Complet(GxVpuProperty_Complet *property)
{
	GXAV_ASSERT(property != NULL);
	return siriusga_complet(property);
}

int sirius_vpu_Ga_Interrupt(int irq)
{
	if (irq != GA_INT_SRC) {
		VPU_PRINTF("vpu ga interrup inode error! irq_num=%x\n",irq);
		return 1;
	}

	return siriusga_interrupt(irq);
}

static void vpp_disp_controler_init(DispControler *controler)
{
	int i;

	if (controler) {
		controler->buf_to_wr = 0;
		for(i = 0; i < MAX_DISPBUF_NUM; i++)
			controler->unit[i] = (struct disp_unit*)siriusvpu_reg->rDISP_CTRL[i];;
	}
}

static void vpu_init_auto_zoom(SiriusVpuLayer *layer)
{
	int i = 0;

	for(i = 0; i < GX_LAYER_MAX; i++) {
		layer[i].auto_zoom = true;
	}
}

int sirius_vpu_open(void)
{
	unsigned int value;
	SiriusVpuOsdPriv  *osd_priv  = NULL;
	SiriusVpuSosdPriv *sosd_priv = NULL;
	SiriusVpuSosdPriv *sosd2_priv = NULL;
	SiriusVpuVppPriv  *vpp_priv  = NULL;

	VPU_DBG("[SIRIUS]:vpu Open start!\n");
	siriusvpu_info = (SiriusVpu *) gx_malloc(sizeof(SiriusVpu));
	if(siriusvpu_info == NULL) {
		VPU_PRINTF("Open Error: siriusvpu_info MALLOC\n");
		return -1;
	}

	gx_memset(siriusvpu_info, 0, sizeof(SiriusVpu));
	siriusvpu_info->layer[GX_LAYER_OSD].ops  = &sirius_osd_ops;
	siriusvpu_info->layer[GX_LAYER_SOSD].ops = &sirius_sosd_ops;
	siriusvpu_info->layer[GX_LAYER_SOSD2].ops = &sirius_sosd2_ops;
	siriusvpu_info->layer[GX_LAYER_VPP].ops  = &sirius_vpp_ops;
	siriusvpu_info->layer[GX_LAYER_SPP].ops  = &sirius_spp_ops;
	siriusvpu_info->layer[GX_LAYER_BKG].ops  = &sirius_bkg_ops;

	siriusvpu_info->layer[GX_LAYER_OSD].id   = GX_LAYER_OSD;
	siriusvpu_info->layer[GX_LAYER_SOSD].id  = GX_LAYER_SOSD;
	siriusvpu_info->layer[GX_LAYER_SOSD2].id  = GX_LAYER_SOSD2;
	siriusvpu_info->layer[GX_LAYER_VPP].id   = GX_LAYER_VPP;
	siriusvpu_info->layer[GX_LAYER_SPP].id   = GX_LAYER_SPP;
	siriusvpu_info->layer[GX_LAYER_BKG].id   = GX_LAYER_BKG;

	siriusvpu_info->layer[GX_LAYER_VPP].view_rect.x = 0;
	siriusvpu_info->layer[GX_LAYER_VPP].view_rect.y = 0;
	siriusvpu_info->layer[GX_LAYER_VPP].view_rect.width = SIRIUS_MAX_XRES;
	siriusvpu_info->layer[GX_LAYER_VPP].view_rect.height= SIRIUS_MAX_YRES;
	siriusvpu_info->layer[GX_LAYER_VPP].clip_rect = siriusvpu_info->layer[GX_LAYER_VPP].view_rect;

	vpu_init_auto_zoom(siriusvpu_info->layer);

	//alloc osd priv
	osd_priv = (SiriusVpuOsdPriv*)gx_dma_malloc(sizeof(SiriusVpuOsdPriv));
	if(osd_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)osd_priv, 0, sizeof(SiriusVpuOsdPriv));
	siriusvpu_info->layer[GX_LAYER_OSD].priv = osd_priv;
#if REGION_POOL_EN
	osd_priv->region_pool = mpool_create(sizeof(OsdRegion), 8);
	if (osd_priv->region_pool == NULL) {
		VPU_PRINTF("Open Error: RegionPool ALLOC\n");
	}
#endif

	//alloc sosd priv
	sosd_priv = (SiriusVpuSosdPriv*)gx_dma_malloc(sizeof(SiriusVpuSosdPriv));
	if(sosd_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)sosd_priv, 0, sizeof(SiriusVpuSosdPriv));
	siriusvpu_info->layer[GX_LAYER_SOSD].priv = sosd_priv;

	sosd2_priv = (SiriusVpuSosdPriv*)gx_dma_malloc(sizeof(SiriusVpuSosdPriv));
	if(sosd2_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)sosd2_priv, 0, sizeof(SiriusVpuSosdPriv));
	siriusvpu_info->layer[GX_LAYER_SOSD2].priv = sosd2_priv;

	//alloc vpp priv
	vpp_priv = (SiriusVpuVppPriv*)gx_malloc(sizeof(SiriusVpuVppPriv));
	if(vpp_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)vpp_priv, 0, sizeof(SiriusVpuVppPriv));
	siriusvpu_info->layer[GX_LAYER_VPP].priv = vpp_priv;
	vpp_disp_controler_init(&vpp_priv->controler);

	value = REG_GET_VAL(&(siriusvpu_reg->rPP_DISP_CTRL))|(1<<30);
	REG_SET_VAL(&(siriusvpu_reg->rPP_DISP_CTRL),value);

	{
		clock_params clock;
		clock.vpu.div = 1;
		clock.vpu.clock = 7;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
		clock.vpu.clock = 30;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
	}
	siriusvpu_info->memory_pool = gxav_mem_pool_init(sizeof(SiriusVpuSurface), SIRIUS_MEM_POOL_SIZE);
	siriusga_open();

	siriusvpu_info->reset_flag = 1;
	gx_mutex_init(&siriusvpu_info->mutex);

	vpu_triming_init();
	return 0;
}

int sirius_vpu_close(void)
{
	SiriusVpuOsdPriv **osd_priv  = NULL;
	SiriusVpuOsdPriv **sosd_priv = NULL;
	SiriusVpuVppPriv **vpp_priv  = NULL;

	if(siriusvpu_info) {
		SiriusVpuSurface *next;
		SiriusVpuSurface *now = siriusvpu_info->surface;

		while (now != NULL) {
			next = now->next;
			if(remove_surface_from_list(now) != 0)
				break;
			now = next;
		}

		//free osd priv
		osd_priv = (SiriusVpuOsdPriv**)(&(siriusvpu_info->layer[GX_LAYER_OSD].priv));
		if(*osd_priv) {
#if REGION_POOL_EN
			if ((*osd_priv)->region_pool) {
				mpool_destroy((*osd_priv)->region_pool);
			}
#endif
			gx_dma_free((unsigned char *)*osd_priv, sizeof(SiriusVpuOsdPriv));
			*osd_priv = NULL;
		}

		//free sosd priv
		sosd_priv = (SiriusVpuSosdPriv**)(&(siriusvpu_info->layer[GX_LAYER_SOSD].priv));
		if(*sosd_priv) {
			gx_dma_free((unsigned char *)*sosd_priv, sizeof(SiriusVpuSosdPriv));
			*sosd_priv = NULL;
		}

		//free vpp priv
		vpp_priv = (SiriusVpuVppPriv**)(&(siriusvpu_info->layer[GX_LAYER_VPP].priv));
		if(*vpp_priv) {
			gx_free(*vpp_priv);
			*vpp_priv = NULL;
		}
		if(siriusvpu_info->memory_pool) {
			gxav_mem_pool_destroy(siriusvpu_info->memory_pool);
			siriusvpu_info->memory_pool = 0;
		}

		gx_mutex_destroy(&siriusvpu_info->mutex);
		gx_free(siriusvpu_info);
		siriusvpu_info = NULL;
	}

	VPU_VPP_DISABLE(siriusvpu_reg->rPP_CTRL);
	VPU_SPP_DISABLE(siriusvpu_reg->rPIC_CTRL);
	VPU_OSD_DISABLE(siriusvpu_reg->osd_0.rOSD_CTRL);
	VPU_VBI_STOP(siriusvpu_reg->rVBI_CTRL);
	VPU_DBG("[SIRIUS]:Close OK!\n");

	return 0;
}

static void sirius_VpuIOUnmap(void)
{
	if(siriusvpu_reg) {
		gx_iounmap(siriusvpu_reg);
		gx_release_mem_region(VPU_REG_BASE_ADDR, sizeof(SiriusVpuReg));
		siriusvpu_reg = NULL;
	}

	return ;
}

static int sirius_VpuIORemap(void)
{
	if(!gx_request_mem_region(VPU_REG_BASE_ADDR, sizeof(SiriusVpuReg))) {
		VPU_PRINTF("request_mem_region failed");
		goto VPU_IOMAP_ERROR;
	}
	siriusvpu_reg = gx_ioremap(VPU_REG_BASE_ADDR, sizeof(SiriusVpuReg));
	if(!siriusvpu_reg) {
		goto VPU_IOMAP_ERROR;
	}
	vpu_enhance_reg = (enhance_reg *)&(siriusvpu_reg->enhance);

	return 0;

VPU_IOMAP_ERROR:
	sirius_VpuIOUnmap();
	return -1;
}

static int sirius_vpu_init(void)
{
	unsigned int i;

	if(0 != sirius_VpuIORemap()) {
		return -1;
	}

	siriusga_init();
	sirius_svpu_init();
	//VPU_VPP_DISABLE(siriusvpu_reg->rPP_CTRL);
	//VPU_SPP_DISABLE(siriusvpu_reg->rPIC_CTRL);
	//VPU_OSD_DISABLE(siriusvpu_reg->rOSD_CTRL);
	VPU_VPP_SET_REQUEST_BLOCK(siriusvpu_reg->rPP_FRAME_STRIDE, 0x100);
	VPU_SPP_SET_BUFFER_REQ_DATA_LEN(siriusvpu_reg->rPIC_PARA, 0x100);
	VPU_VPP_SET_ZOOM(siriusvpu_reg->rPP_ZOOM, 0x1000, 0x1000);
	VPU_VPP_SET_PP_SIZE(siriusvpu_reg->rPP_VIEW_SIZE, SIRIUS_MAX_XRES, SIRIUS_MAX_YRES);
	VPU_VPP_SET_FIELD_MODE_UV_FIELD(siriusvpu_reg->rPP_CTRL,0x1);
	VPU_SPP_SET_BG_COLOR(siriusvpu_reg->rPIC_BACK_COLOR, 0x10, 0x80, 0x80);
	for (i = 0; i < OSD_FLICKER_TABLE_LEN; i++) {
		VPU_OSD_SET_FLIKER_FLITER(siriusvpu_reg->rOSD_PHASE_PARA, i, sirius_osd_filter_table[i]);
		VPU_OSD_SET_FLIKER_FLITER(siriusvpu_reg->rSOSD_PHASE_PARA, i, sirius_osd_filter_table[i]);
		VPU_OSD_SET_FLIKER_FLITER(siriusvpu_reg->rSOSD2_PHASE_PARA, i, sirius_osd_filter_table[i]);
	}

	VPU_SPIN_LOCK_INIT();
	return 0;
}

static int sirius_vpu_cleanup(void)
{
	sirius_svpu_cleanup();
	sirius_vpu_close();
	siriusga_cleanup();

	sirius_VpuIOUnmap();
	return 0;
}

static int taurus_vpu_set_gamma(GxVpuProperty_GAMMAConfig *param) {

	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support set gamma.\n");
	}

	GXAV_ASSERT(param != NULL);

	switch(param->type){
		case GAMMA_R:
			EHN_SET_GAMMA_R_COEF0(vpu_enhance_reg->rGAMMA_COEF00,
					      param->gamma_bypass_enable,
					      param->crc709_enable,
					      param->crc_bypass_enable);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF01,
					   param->data_point[0],
					   param->data_point[1],
					   param->data_point[2],
					   param->data_point[3]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF02,
					   param->data_point[4],
					   param->data_point[5],
					   param->data_point[6],
					   param->data_point[7]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF03,
					   param->data_point[8],
					   param->data_point[9],
					   param->data_point[10],
					   param->data_point[11]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF04,
					   param->data_point[12],
					   param->data_point[13],
					   param->data_point[14],
					   0);
			break;
		case GAMMA_G:
			EHN_SET_GAMMA_COEF0(vpu_enhance_reg->rGAMMA_COEF10, param->gamma_bypass_enable);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF11,
					   param->data_point[0],
					   param->data_point[1],
					   param->data_point[2],
					   param->data_point[3]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF12,
					   param->data_point[4],
					   param->data_point[5],
					   param->data_point[6],
					   param->data_point[7]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF13,
					   param->data_point[8],
					   param->data_point[9],
					   param->data_point[10],
					   param->data_point[11]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF14,
					   param->data_point[12],
					   param->data_point[13],
					   param->data_point[14],
					   0);
			break;
		case GAMMA_B:
			EHN_SET_GAMMA_COEF0(vpu_enhance_reg->rGAMMA_COEF20, param->gamma_bypass_enable);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF21,
					   param->data_point[0],
					   param->data_point[1],
					   param->data_point[2],
					   param->data_point[3]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF22,
					   param->data_point[4],
					   param->data_point[5],
					   param->data_point[6],
					   param->data_point[7]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF23,
					   param->data_point[8],
					   param->data_point[9],
					   param->data_point[10],
					   param->data_point[11]);
			EHN_SET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF24,
					   param->data_point[12],
					   param->data_point[13],
					   param->data_point[14],
					   0);
			break;
		default:
			break;
	}

	return (0);
}

static int taurus_vpu_get_gamma(GxVpuProperty_GAMMAConfig *param) {
	char tmp = 0;

	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support get gamma.\n");
	}

	GXAV_ASSERT(param != NULL);

	switch(param->type){
		case GAMMA_R:
			EHN_GET_GAMMA_R_COEF0(vpu_enhance_reg->rGAMMA_COEF00,
					      param->gamma_bypass_enable,
					      param->crc709_enable,
					      param->crc_bypass_enable);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF01,
					   param->data_point[0],
					   param->data_point[1],
					   param->data_point[2],
					   param->data_point[3]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF02,
					   param->data_point[4],
					   param->data_point[5],
					   param->data_point[6],
					   param->data_point[7]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF03,
					   param->data_point[8],
					   param->data_point[9],
					   param->data_point[10],
					   param->data_point[11]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF04,
					   param->data_point[12],
					   param->data_point[13],
					   param->data_point[14],
					   tmp);
			break;
		case GAMMA_G:
			EHN_GET_GAMMA_COEF0(vpu_enhance_reg->rGAMMA_COEF10, param->gamma_bypass_enable);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF11,
					   param->data_point[0],
					   param->data_point[1],
					   param->data_point[2],
					   param->data_point[3]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF12,
					   param->data_point[4],
					   param->data_point[5],
					   param->data_point[6],
					   param->data_point[7]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF13,
					   param->data_point[8],
					   param->data_point[9],
					   param->data_point[10],
					   param->data_point[11]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF14,
					   param->data_point[12],
					   param->data_point[13],
					   param->data_point[14],
					   tmp);
			break;
		case GAMMA_B:
			EHN_GET_GAMMA_COEF0(vpu_enhance_reg->rGAMMA_COEF20, param->gamma_bypass_enable);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF21,
					   param->data_point[0],
					   param->data_point[1],
					   param->data_point[2],
					   param->data_point[3]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF22,
					   param->data_point[4],
					   param->data_point[5],
					   param->data_point[6],
					   param->data_point[7]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF23,
					   param->data_point[8],
					   param->data_point[9],
					   param->data_point[10],
					   param->data_point[11]);
			EHN_GET_GAMMA_COEF(vpu_enhance_reg->rGAMMA_COEF24,
					   param->data_point[12],
					   param->data_point[13],
					   param->data_point[14],
					   tmp);
			break;
		default:
			break;
	}
	return (0);
}

static int taurus_vpu_set_green_blue(GxVpuProperty_GreenBlueConfig *param) {
	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support set green blue.\n");
	}

	GXAV_ASSERT(param != NULL);

	EHN_GREEN_SET(vpu_enhance_reg->rG_ENHANCEMENT_CTL, param);
	EHN_BLUE_SET(vpu_enhance_reg->rB_STRETCH_CTL, param);
	return (0);
}

static int taurus_vpu_get_green_blue(GxVpuProperty_GreenBlueConfig *param) {
	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support get green blue.\n");
	}

	GXAV_ASSERT(param != NULL);

	EHN_GREEN_GET(vpu_enhance_reg->rG_ENHANCEMENT_CTL, param);
	EHN_BLUE_GET(vpu_enhance_reg->rB_STRETCH_CTL, param);
	return (0);
}

static int _get_src_layer(GxLayerID layer){
	switch(layer){
	case GX_LAYER_OSD:
		return (EHN_SRC_LAYER_OSD);
	case GX_LAYER_SPP:
		return (EHN_SRC_LAYER_SPP);
	case GX_LAYER_VPP:
		return (EHN_SRC_LAYER_VPP);
	case GX_LAYER_SOSD:
		return (EHN_SRC_LAYER_SOSD);
	case GX_LAYER_SOSD2:
		return (EHN_SRC_LAYER_SOSD2);
	default:
		return (EHN_SRC_LAYER_VPP);
	}
}

static int _vpu_ehance_set(int channel, GxEnhanceChannel *param){
	int val = _get_src_layer(param->layer);

	EHN_SRC_SEL(vpu_enhance_reg->rENH_SRC_SEL, val, channel);
	if(0 == channel){
		EHN_LTI_CFG0_SET(vpu_enhance_reg->rPRM_LTI_CFG_0,
				 param->lti_data.lti_enable,
				 param->lti_data.tot_limit,
				 param->lti_data.tot_core,
				 param->lti_data.dif_core);
		EHN_LTI_CFG1_SET(vpu_enhance_reg->rPRM_LTI_CFG_1,
				 param->lti_data.gain_delay,
				 param->lti_data.tot_gain,
				 param->lti_data.lvl_gain,
				 param->lti_data.pos_gain);
		EHN_PEAKING_CFG0_SET(vpu_enhance_reg->rPRM_PEAKING_CFG_0,
				     param->peaking_data.peaking_enable,
				     param->peaking_data.frequency_core,
				     param->peaking_data.filter_bk,
				     param->peaking_data.filter_hk);
		EHN_PEAKING_CFG1_SET(vpu_enhance_reg->rPRM_PEAKING_CFG_1,
				     param->peaking_data.frequency_th,
				     param->peaking_data.window_th);
		EHN_CTI_CFG0_SET(vpu_enhance_reg->rPRM_CTI_CFG_0,
				 param->cti_data.cti_enable,
				 param->cti_data.step,
				 param->cti_data.filter_select,
				 param->cti_data.window_delta,
				 param->cti_data.core);
		EHN_CTI_CFG1_SET(vpu_enhance_reg->rPRM_CTI_CFG_1,
				 param->cti_data.gain_delay,
				 param->cti_data.lvl_gain,
				 param->cti_data.dif_gain);
		EHN_BLEC_BRT_CFG_SET(vpu_enhance_reg->rPRM_BLEC_BRT_CFG,
				     param->brt_data.step,
				     param->blec_data.gain,
				     param->blec_data.tilt);
		EHN_HUE_SAT_CON_CFG_SET(vpu_enhance_reg->rPRM_HUE_STA_CON_CFG,
					param->con_data.step,
					param->sat_data.step,
					param->hue_data.angle);
		EHN_HEC_CFG0_SET(vpu_enhance_reg->rPRM_HEC_CFG_0,
				 param->hec_data.hec_enable,
				 param->hec_data.hec_diff);
		EHN_HEC_CFG1_SET(vpu_enhance_reg->rPRM_HEC_CFG_1,
				 param->hec_data.grn_sat_step,
				 param->hec_data.cyn_sat_step,
				 param->hec_data.ylw_sat_step);
		EHN_HEC_CFG2_SET(vpu_enhance_reg->rPRM_HEC_CFG_2,
				 param->hec_data.blu_sat_step,
				 param->hec_data.red_sat_step,
				 param->hec_data.mgn_sat_step);
		EHN_HEC_CFG3_SET(vpu_enhance_reg->rPRM_HEC_CFG_3,
				 param->hec_data.grn_hue_angle,
				 param->hec_data.cyn_hue_angle,
				 param->hec_data.ylw_hue_angle);
		EHN_HEC_CFG4_SET(vpu_enhance_reg->rPRM_HEC_CFG_4,
				 param->hec_data.blu_hue_angle,
				 param->hec_data.red_hue_angle,
				 param->hec_data.mgn_hue_angle);
	}else{
		EHN_SCN_BLEC_BRT_HUE_SET(vpu_enhance_reg->rSCN_BLEC_BRT_HUE_CFG,
					 param->hue_data.angle,
					 param->brt_data.step,
					 param->blec_data.gain,
					 param->blec_data.tilt);
		EHN_SCN_SAT_CON_SET(vpu_enhance_reg->rSCN_STA_CON_CFG,
				    param->con_data.step,
				    param->sat_data.step);
	}

	return (0);
}

static int _vpu_ehance_get(int channel, GxEnhanceChannel *param){

	if(0 == channel){
		EHN_LTI_CFG0_GET(vpu_enhance_reg->rPRM_LTI_CFG_0,
				 param->lti_data.lti_enable,
				 param->lti_data.tot_limit,
				 param->lti_data.tot_core,
				 param->lti_data.dif_core);
		EHN_LTI_CFG1_GET(vpu_enhance_reg->rPRM_LTI_CFG_1,
				 param->lti_data.gain_delay,
				 param->lti_data.tot_gain,
				 param->lti_data.lvl_gain,
				 param->lti_data.pos_gain);
		EHN_PEAKING_CFG0_GET(vpu_enhance_reg->rPRM_PEAKING_CFG_0,
				     param->peaking_data.peaking_enable,
				     param->peaking_data.frequency_core,
				     param->peaking_data.filter_bk,
				     param->peaking_data.filter_hk);
		EHN_PEAKING_CFG1_GET(vpu_enhance_reg->rPRM_PEAKING_CFG_1,
				     param->peaking_data.frequency_th,
				     param->peaking_data.window_th);
		EHN_CTI_CFG0_GET(vpu_enhance_reg->rPRM_CTI_CFG_0,
				 param->cti_data.cti_enable,
				 param->cti_data.step,
				 param->cti_data.filter_select,
				 param->cti_data.window_delta,
				 param->cti_data.core);
		EHN_CTI_CFG1_GET(vpu_enhance_reg->rPRM_CTI_CFG_1,
				 param->cti_data.gain_delay,
				 param->cti_data.lvl_gain,
				 param->cti_data.dif_gain);
		EHN_BLEC_BRT_CFG_GET(vpu_enhance_reg->rPRM_BLEC_BRT_CFG,
				     param->brt_data.step,
				     param->blec_data.gain,
				     param->blec_data.tilt);
		EHN_HUE_SAT_CON_CFG_GET(vpu_enhance_reg->rPRM_HUE_STA_CON_CFG,
					param->con_data.step,
					param->sat_data.step,
					param->hue_data.angle);
		EHN_HEC_CFG0_GET(vpu_enhance_reg->rPRM_HEC_CFG_0,
				 param->hec_data.hec_enable,
				 param->hec_data.hec_diff);
		EHN_HEC_CFG1_GET(vpu_enhance_reg->rPRM_HEC_CFG_1,
				 param->hec_data.grn_sat_step,
				 param->hec_data.cyn_sat_step,
				 param->hec_data.ylw_sat_step);
		EHN_HEC_CFG2_GET(vpu_enhance_reg->rPRM_HEC_CFG_2,
				 param->hec_data.blu_sat_step,
				 param->hec_data.red_sat_step,
				 param->hec_data.mgn_sat_step);
		EHN_HEC_CFG3_GET(vpu_enhance_reg->rPRM_HEC_CFG_3,
				 param->hec_data.grn_hue_angle,
				 param->hec_data.cyn_hue_angle,
				 param->hec_data.ylw_hue_angle);
		EHN_HEC_CFG4_GET(vpu_enhance_reg->rPRM_HEC_CFG_4,
				 param->hec_data.blu_hue_angle,
				 param->hec_data.red_hue_angle,
				 param->hec_data.mgn_hue_angle);
	}else{
		EHN_SCN_BLEC_BRT_HUE_GET(vpu_enhance_reg->rSCN_BLEC_BRT_HUE_CFG,
					 param->hue_data.angle,
					 param->brt_data.step,
					 param->blec_data.gain,
					 param->blec_data.tilt);
		EHN_SCN_SAT_CON_GET(vpu_enhance_reg->rSCN_STA_CON_CFG,
				    param->con_data.step,
				    param->sat_data.step);
	}

	return (0);
}

static int taurus_vpu_set_enhance(GxVpuProperty_EnhanceConfig *param) {
	GxEnhanceChannel channel = {0};
	int i = 0;

	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support set enhance.\n");
	}

	GXAV_ASSERT(param != NULL);

	while(param->channel[i]){
		gx_copy_from_user(&channel, param->channel[i], sizeof(GxEnhanceChannel));
		_vpu_ehance_set(i, &channel);
		i++;
	}

	return (0);
}

static int taurus_vpu_get_enhance(GxVpuProperty_EnhanceConfig *param) {
	GxEnhanceChannel channel = {0};
	int i = 0;

	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support get enhance.\n");
	}

	GXAV_ASSERT(param != NULL);

	while(param->channel[i]){
		_vpu_ehance_get(i, &channel);
		gx_copy_to_user(param->channel[i], &channel, sizeof(GxEnhanceChannel));
		i++;
	}
	return (0);
}

static int taurus_vpu_set_mix_filter(GxVpuProperty_MixFilter *param) {
	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support set mix filter.\n");
	}

	GXAV_ASSERT(param != NULL);

	EHN_MIX_0_FILTER_SET(vpu_enhance_reg->rMIX_Y_FIR_PARA0,
			     param->y_coe[0],
			     param->y_coe[1],
			     param->y_coe[2]);
	EHN_MIX_1_FILTER_SET(vpu_enhance_reg->rMIX_Y_FIR_PARA1,
			     param->y_mix_filter_bypass_enable,
			     param->y_coe[3],
			     param->y_coe[4]);

	EHN_MIX_0_FILTER_SET(vpu_enhance_reg->rMIX_C_FIR_PARA0,
			     param->c_coe[0],
			     param->c_coe[1],
			     param->c_coe[2]);
	EHN_MIX_1_FILTER_SET(vpu_enhance_reg->rMIX_C_FIR_PARA1,
			     param->c_mix_filter_bypass_enable,
			     param->c_coe[3],
			     param->c_coe[4]);
	return (0);
}

static int taurus_vpu_get_mix_filter(GxVpuProperty_MixFilter *param) {
	if((!CHIP_IS_TAURUS) &&
	   (!CHIP_IS_GEMINI)) {
		gx_printf("[VPU] The Chipset cannot support get mix filter.\n");
	}

	GXAV_ASSERT(param != NULL);

	EHN_MIX_0_FILTER_GET(vpu_enhance_reg->rMIX_Y_FIR_PARA0,
			     param->y_coe[0],
			     param->y_coe[1],
			     param->y_coe[2]);
	EHN_MIX_1_FILTER_GET(vpu_enhance_reg->rMIX_Y_FIR_PARA1,
			     param->y_mix_filter_bypass_enable,
			     param->y_coe[3],
			     param->y_coe[4]);

	EHN_MIX_0_FILTER_GET(vpu_enhance_reg->rMIX_C_FIR_PARA0,
			     param->c_coe[0],
			     param->c_coe[1],
			     param->c_coe[2]);
	EHN_MIX_1_FILTER_GET(vpu_enhance_reg->rMIX_C_FIR_PARA1,
			     param->c_mix_filter_bypass_enable,
			     param->c_coe[3],
			     param->c_coe[4]);
	return (0);
}

static int sirius_vpu_set_svpu(GxVpuProperty_SVPUConfig *param) {
	sirius_svpu_set_para(param);
	return (0);
}

void sirius_vpu_SetBufferStateDelay(int v)
{
	VPU_SET_BUFF_STATE_DELAY(siriusvpu_reg->rLAYER_CTRL, v);
}

void taurus_vpu_SetBufferStateDelay(int v)
{
	VPU_SET_BUFF_STATE_DELAY(siriusvpu_reg->rLAYER_CTRL, v);
}

static int sirius_vpu_SetVoutIsready(int ready)
{
	if(siriusvpu_info != NULL) {
		gx_mutex_lock(&siriusvpu_info->mutex);
		siriusvpu_info->vout_is_ready = ready;
		if(ready) {
			gx_video_disp_patch(0);
			//re-zoom all layer
			if(siriusvpu_info->layer[GX_LAYER_OSD].auto_zoom) {
				SiriusVpu_Zoom(GX_LAYER_OSD);
			}

			if(siriusvpu_info->layer[GX_LAYER_SOSD].auto_zoom) {
				SiriusVpu_Zoom(GX_LAYER_SOSD);
			}

			if(siriusvpu_info->layer[GX_LAYER_SPP].auto_zoom) {
				SiriusVpu_Zoom(GX_LAYER_SPP);
			}

			if(siriusvpu_info->layer[GX_LAYER_VPP].auto_zoom) {
				sirius_vpp_zoom_require();
			}

			//enable on needs
			if (siriusvpu_info->layer[GX_LAYER_OSD].enable) {
				siriusVpu_LayerEnable(GX_LAYER_OSD, 1);
			}
			if (siriusvpu_info->layer[GX_LAYER_SOSD].enable) {
				siriusVpu_LayerEnable(GX_LAYER_SOSD, 1);
			}
			if (siriusvpu_info->layer[GX_LAYER_SPP].enable) {
				siriusVpu_LayerEnable(GX_LAYER_SPP, 1);
			}
			if(siriusvpu_info->layer[GX_LAYER_VPP].enable) {
				gx_video_ppopen_require(0);
			}
			gx_mutex_unlock(&siriusvpu_info->mutex);
		}
		else {
			unsigned long flags;
			gx_mutex_unlock(&siriusvpu_info->mutex);
			VPU_SPIN_LOCK();
			siriusVpu_LayerEnable(GX_LAYER_OSD, 0);
			siriusVpu_LayerEnable(GX_LAYER_SOSD, 0);
			siriusVpu_LayerEnable(GX_LAYER_SPP, 0);
			siriusVpu_LayerEnable(GX_LAYER_VPP, 0);
			gx_video_ppclose_require(0);
			VPU_SPIN_UNLOCK();
			while(!sirius_vpu_check_layers_close()) gx_msleep(10);
		}
		return 0;
	}

	return -1;
}

int sirius_vpu_DACEnable(GxVpuDacID dac_id, int enable)
{
	if(siriusvpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if(enable) {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				VPU_DAC_ENABLE(siriusvpu_reg->rPOWER_DOWN, 0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				VPU_DAC_ENABLE(siriusvpu_reg->rPOWER_DOWN, 1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				VPU_DAC_ENABLE(siriusvpu_reg->rPOWER_DOWN, 2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				VPU_DAC_ENABLE(siriusvpu_reg->rPOWER_DOWN, 3);
			if((dac_id&VPU_DAC_ALL) == VPU_DAC_ALL)
				VPU_DAC_ENABLE(siriusvpu_reg->rPOWER_DOWN, 4);
		}
		else {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				VPU_DAC_DISABLE(siriusvpu_reg->rPOWER_DOWN, 0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				VPU_DAC_DISABLE(siriusvpu_reg->rPOWER_DOWN, 1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				VPU_DAC_DISABLE(siriusvpu_reg->rPOWER_DOWN, 2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				VPU_DAC_DISABLE(siriusvpu_reg->rPOWER_DOWN, 3);
			if((dac_id&VPU_DAC_ALL) == VPU_DAC_ALL)
				VPU_DAC_DISABLE(siriusvpu_reg->rPOWER_DOWN, 4);
		}
		return 0;
	}

	return -1;
}

int sirius_vpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source)
{
	if(siriusvpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if(source == VPU_DAC_SRC_VPU || source == VPU_DAC_SRC_SVPU) {
			if(source == VPU_DAC_SRC_VPU) {
				if(dac_id&VPU_DAC_0) {
					REG_CLR_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 0);
					gxav_clock_set_video_dac_clock_source(0, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_1) {
					REG_CLR_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 1);
					gxav_clock_set_video_dac_clock_source(1, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_2) {
					REG_CLR_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 2);
					gxav_clock_set_video_dac_clock_source(2, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_3) {
					REG_CLR_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 3);
					gxav_clock_set_video_dac_clock_source(3, VIDEO_DAC_SRC_VPU);
				}
			}
			else {
				if(dac_id&VPU_DAC_0) {
					REG_SET_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 0);
					gxav_clock_set_video_dac_clock_source(0, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_1) {
					REG_SET_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 1);
					gxav_clock_set_video_dac_clock_source(1, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_2) {
					REG_SET_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 2);
					gxav_clock_set_video_dac_clock_source(2, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_3) {
					REG_SET_BIT(&siriusvpu_reg->rPOWER_DOWN_BYSELF, 3);
					gxav_clock_set_video_dac_clock_source(3, VIDEO_DAC_SRC_SVPU);
				}
			}
			return 0;
		}
	}

	return -1;
}

int sirius_vpu_DACPower(GxVpuDacID dac_id)
{
	int ret = 0;
#if 0
	if (siriusvpu_reg)
		gx_printf("\n%s:%d, reg = %p, dac_id = %d, dac-reg = 0x%x\n", __func__, __LINE__, siriusvpu_reg, dac_id, siriusvpu_reg->rPOWER_DOWN);
	else
		gx_printf("\n%s:%d, reg = %p, dac_id = %d\n", __func__, __LINE__, siriusvpu_reg, dac_id);
#endif

	if (siriusvpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if (REG_GET_BIT(&siriusvpu_reg->rPOWER_DOWN, 4) == 1) {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				ret |= (REG_GET_BIT(&siriusvpu_reg->rPOWER_DOWN, 0) << VPU_DAC_0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				ret |= (REG_GET_BIT(&siriusvpu_reg->rPOWER_DOWN, 1) << VPU_DAC_1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				ret |= (REG_GET_BIT(&siriusvpu_reg->rPOWER_DOWN, 2) << VPU_DAC_2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				ret |= (REG_GET_BIT(&siriusvpu_reg->rPOWER_DOWN, 3) << VPU_DAC_3);
		}
	}

	return ret;
}

static int sirius_vpu_vpp_get_base_line(void)
{
	return VPU_VPP_GET_BASE_LINE(siriusvpu_reg->rPP_FRAME_STRIDE);
}

static int sirius_vpu_vpp_set_base_line(unsigned int value)
{
	VPU_VPP_SET_BASE_LINE(siriusvpu_reg->rPP_FRAME_STRIDE, value);
	return 0;
}

static int sirius_vpu_disp_get_buff_id(void)
{
	return VPU_DISP_GET_BUFF_ID(siriusvpu_reg->rPP_DISP_R_PTR);
}

static int sirius_vpu_disp_set_rst(void)
{
	VPU_DISP_SET_RST( siriusvpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int sirius_vpu_disp_clr_rst(void)
{
	VPU_DISP_CLR_RST( siriusvpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int sirius_vpu_disp_get_view_active_cnt(void)
{
	return VPU_DISP_GET_VIEW_ACTIVE_CNT(siriusvpu_reg->rSCAN_LINE);
}

static int sirius_vpu_disp_clr_field_error_int(void)
{
	VPU_DISP_CLR_FILED_ERROR_INT(siriusvpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int sirius_vpu_disp_clr_field_start_int(void)
{
	VPU_DISP_CLR_FILED_START_INT(siriusvpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int sirius_vpu_disp_field_error_int_en(void)
{
	VPU_DISP_FILED_ERROR_INT_EN( siriusvpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int sirius_vpu_disp_field_start_int_en(void)
{
	VPU_DISP_FILED_START_INT_EN( siriusvpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int field_int_enabled(void)
{
	return VPU_DISP_FILED_START_INT_ENABLED( siriusvpu_reg->rPP_DISP_CTRL );
}

static int sirius_vpu_disp_field_error_int_dis(void)
{
	VPU_DISP_FILED_ERROR_INT_DIS(siriusvpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int sirius_vpu_disp_field_start_int_dis(void)
{
	VPU_DISP_FILED_START_INT_DIS(siriusvpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int sirius_vpu_disp_get_buff_cnt(void)
{
	return VPU_DISP_GET_BUFF_CNT(siriusvpu_reg->rPP_DISP_CTRL);
}

static int sirius_vpu_disp_get_view_field(void)
{
	return VPU_DISP_GET_VIEW_FIELD(siriusvpu_reg->rSCAN_LINE);
}

static int sirius_vpu_disp_get_display_buf(int i)
{
	return (int)siriusvpu_reg->rDISP_CTRL[i];
}

extern int sirius_spp_enable(int enable);
extern int sirius_vpp_enable(int enable);
static int sirius_vpu_disp_layer_enable(GxLayerID layer, int enable)
{
	if (siriusvpu_info == NULL)
		return 0;

	if (!siriusvpu_info->vout_is_ready) {
		siriusvpu_info->layer[layer].enable = enable;
		return 0;
	}

	switch (layer)
	{
	case GX_LAYER_VPP:
		sirius_vpp_enable(enable);
		break;
	case GX_LAYER_SPP:
		sirius_spp_enable(enable);
		break;
	default:
		return -1;
	}
	return 0;
}


static int sirius_vpu_vpp_set_stream_ratio(unsigned int ratio)
{
	return sirius_vpp_set_stream_ratio(ratio);
}

static int sirius_vpu_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	return sirius_vpp_get_actual_viewrect(view_rect);
}

static int sirius_vpu_regist_surface(GxVpuProperty_RegistSurface *param)
{
	unsigned long flags;
	SiriusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	VPU_SPIN_LOCK();
	sm->showing         = 0;
	sm->cur_time        = 0;
	sm->ready_time      = 0;
	sm->ready           = NULL;
	sm->interrupt_en    = 0;
	sm->force_flip_gate = 1;
	gx_memcpy(sm->surfaces, param->surfaces, sizeof(param->surfaces));
	VPU_SPIN_UNLOCK();
	return 0;
}

static int sirius_vpu_unregist_surface(GxVpuProperty_UnregistSurface *param)
{
	int i, j;
	unsigned long flags;
	SiriusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	VPU_SPIN_LOCK();
	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		for(j = 0; j < GXVPU_MAX_SURFACE_NUM; j++)
			if(param->surfaces[j] == sm->surfaces[i])
				sm->surfaces[i] = NULL;
	}
	VPU_SPIN_UNLOCK();
	return 0;
}

static int sirius_vpu_get_idle_surface(GxVpuProperty_GetIdleSurface *param)
{
	int idle;
	unsigned long flags;
	SiriusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	VPU_SPIN_LOCK();
	idle = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
	param->idle_surface = sm->surfaces[idle];
	if(sm->surfaces[idle]) {
		sm->surfaces[idle]->dirty = 0;
		gx_smp_mb();
	}
	VPU_SPIN_UNLOCK();

	return 0;
}

static int sirius_vpu_flip_surface(GxVpuProperty_FlipSurface *param)
{
	unsigned long flags;
	SiriusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	if(param && param->surface) {
		VPU_SPIN_LOCK();
		((SiriusVpuSurface*)(param->surface))->dirty = 1;
		gx_smp_mb();
		VPU_SPIN_UNLOCK();

		if(param->surface == sm->ready) {
			if(sm->cur_time - sm->ready_time >= sm->force_flip_gate && field_int_enabled()) {
				while(sm->ready->dirty);
				//gx_printf("#vpu_isr_cnt:%d\n", sm->cur_time - sm->ready_time);
			}
		}
		else {
			sm->ready      = param->surface;
			sm->ready_time = sm->cur_time;
		}
	}

	if(!sm->interrupt_en) {
		sirius_vpu_disp_field_start_int_en();
		sirius_vpu_disp_field_error_int_en();
		sm->interrupt_en = 1;
	}
	return 0;
}

static int sirius_vpu_pan_display(GxLayerID layer_id, void *buffer)
{
	SiriusVpuLayer *layer;
	GXAV_ASSERT(buffer != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	layer = &siriusvpu_info->layer[layer_id];
	if(layer->ops->set_main_surface != NULL) {
		return layer->ops->set_pan_display(buffer);
	}

	return -1;
}

static int sirius_vpu_isr_callback(void *arg)
{
	int ret = 0;
	int ready;
	GxVpuProperty_LayerMainSurface main_surface;
	SiriusSurfaceManager *sm;
	int i, layers[] = {GX_LAYER_OSD, GX_LAYER_SPP};
	unsigned long flags;

	if(VPU_CLOSED()){
		sirius_vpu_disp_clr_field_error_int();
		sirius_vpu_disp_clr_field_start_int();
		sirius_vpu_disp_field_start_int_dis();
		sirius_vpu_disp_field_error_int_dis();
		return -1;
	}

	VPU_SPIN_LOCK();
	if (siriusvpu_info->layer[GX_LAYER_VPP].flip_require == 1) {
		SiriusVpuLayer *layer = &siriusvpu_info->layer[GX_LAYER_VPP];
		layer->ops->set_main_surface(layer->surface);
		siriusvpu_info->layer[GX_LAYER_VPP].flip_require = 0;
		ret = 1;
	}
	for(i = 0; i < sizeof(layers)/sizeof(int); i++) {
		if((sm = GET_SURFACE_MANAGER(layers[i])) != NULL) {
			sirius_vpu_disp_clr_field_error_int();
			sirius_vpu_disp_clr_field_start_int();
			ready = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
			if(sm->surfaces[ready] && sm->surfaces[ready]->dirty) {
				main_surface.layer   = layers[i];
				main_surface.surface = sm->surfaces[ready];
				sirius_vpu_SetLayerMainSurface(&main_surface);
				sm->surfaces[ready]->dirty = 0;
				sm->showing = ready;
				gx_smp_mb();
			}
			sm->cur_time++;
		}
	}
	VPU_SPIN_UNLOCK();

	return ret;
}

static int sirius_vpu_add_region(GxVpuProperty_AddRegion *param)
{
	int ret = -1;
	SiriusVpuLayerOps *osd_ops = siriusvpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->add_region) {
		ret = osd_ops->add_region(param->surface, param->pos_x, param->pos_y);
	}

	return ret;
}

static int sirius_vpu_remove_region(GxVpuProperty_RemoveRegion *param)
{
	int ret = -1;
	SiriusVpuLayerOps *osd_ops = siriusvpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->remove_region) {
		ret = osd_ops->remove_region(param->surface);
	}

	return ret;
}

static int sirius_vpu_update_region(GxVpuProperty_UpdateRegion *param)
{
	int ret = -1;
	SiriusVpuLayerOps *osd_ops = siriusvpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->update_region) {
		ret = osd_ops->update_region(param->surface, param->pos_x, param->pos_y);
	}

	return ret;
}

static int sirius_vpu_set_osd_overlay(GxVpuProperty_OsdOverlay *param)
{
	int i = 0;
	int osd_code;

	if (param) {
		for (i = 0; i < OSD_ID_MAX; i++) {
			if (IS_OSD_LAYER(param->sel[i])) {
				if (param->sel[i] == GX_LAYER_OSD) {
					osd_code = 0;
				} else if (param->sel[i] == GX_LAYER_SOSD) {
					osd_code = 1;
				} else if (param->sel[i] == GX_LAYER_SOSD2) {
					osd_code = 2;
				} else {
					osd_code = 3;
				}
				OSD_SET_LAYER_SEL(siriusvpu_reg->rLAYER_CTRL, i, osd_code);
			}
		}
	}

	return 0;
}

static int sirius_vpu_get_osd_overlay(GxVpuProperty_OsdOverlay *param)
{
	int i = 0;
	int osd_code;

	if (param) {
		for (i = 0; i < OSD_ID_MAX; i++) {
			OSD_GET_LAYER_SEL(siriusvpu_reg->rLAYER_CTRL, i, osd_code);
			if (osd_code == 0) {
				param->sel[i] = GX_LAYER_OSD;
			} else if (osd_code == 1) {
				param->sel[i] = GX_LAYER_SOSD;
			} else if (osd_code == 2) {
				param->sel[i] = GX_LAYER_SOSD2;
			} else {
				param->sel[i] = -1;
			}
		}
	}

	return 0;
}

int sirius_vpu_reset(void)
{
	gx_interrupt_mask(VPU_INT_SRC);
	gx_mdelay(30);
	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	gxav_clock_hot_rst_clr(MODULE_TYPE_VPU);
	siriusvpu_info->reset_flag++;
	//gx_printf("\n----VPU RESET----flag = %d\n", siriusvpu_info->reset_flag);
	gx_interrupt_unmask(VPU_INT_SRC);

	return 0;
}

static struct vpu_ops sirius_vpu_ops = {
	.init                     = sirius_vpu_init                      ,
	.cleanup                  = sirius_vpu_cleanup                   ,
	.open                     = sirius_vpu_open                      ,
	.close                    = sirius_vpu_close                     ,
	.reset                    = sirius_vpu_reset                     ,

	.WPalette                 = sirius_vpu_WPalette                  ,
	.DestroyPalette           = sirius_vpu_DestroyPalette            ,
	.SurfaceBindPalette       = sirius_vpu_SurfaceBindPalette        ,
	.SetLayerViewport         = sirius_vpu_SetLayerViewport          ,
	.SetLayerMainSurface      = sirius_vpu_SetLayerMainSurface       ,
	.UnsetLayerMainSurface    = sirius_vpu_UnsetLayerMainSurface     ,
	.SetLayerEnable           = sirius_vpu_SetLayerEnable            ,
	.SetLayerAntiFlicker      = sirius_vpu_SetLayerAntiFlicker       ,
	.SetLayerOnTop            = sirius_vpu_SetLayerOnTop             ,
	.SetLayerVideoMode        = sirius_vpu_SetLayerVideoMode         ,
	.SetLayerMixConfig        = sirius_vpu_SetLayerMixConfig         ,
	.SetDestroySurface        = sirius_vpu_SetDestroySurface         ,
	.ModifySurface            = sirius_vpu_ModifySurface             ,
	.SetPalette               = sirius_vpu_SetPalette                ,
	.SetAlpha                 = sirius_vpu_SetAlpha                  ,
	.SetColorFormat           = sirius_vpu_SetColorFormat            ,
	.SetColorKey              = sirius_vpu_SetColorKey               ,
	.SetBackColor             = sirius_vpu_SetBackColor              ,
	.SetPoint                 = sirius_vpu_SetPoint                  ,
	.SetDrawLine              = sirius_vpu_SetDrawLine               ,
	.SetConvertColor          = sirius_vpu_SetConvertColor           ,
	.SetVirtualResolution     = sirius_vpu_SetVirtualResolution      ,
	.SetVbiEnable             = sirius_vpu_SetVbiEnable              ,
	.SetAspectRatio           = sirius_vpu_SetAspectRatio            ,
	.SetTvScreen              = sirius_vpu_SetTvScreen               ,
	.SetLayerEnablePatch      = sirius_vpu_SetLayerEnablePatch       ,
	.GetEntries               = sirius_vpu_GetEntries                ,
	.RPalette                 = sirius_vpu_RPalette                  ,
	.GetCreatePalette         = sirius_vpu_GetCreatePalette          ,
	.GetLayerViewport         = sirius_vpu_GetLayerViewport          ,
	.GetLayerClipport         = sirius_vpu_GetLayerClipport          ,
	.GetLayerMainSurface      = sirius_vpu_GetLayerMainSurface       ,
	.GetLayerEnable           = sirius_vpu_GetLayerEnable            ,
	.GetLayerAntiFlicker      = sirius_vpu_GetLayerAntiFlicker       ,
	.GetLayerOnTop            = sirius_vpu_GetLayerOnTop             ,
	.GetLayerVideoMode        = sirius_vpu_GetLayerVideoMode         ,
	.GetLayerCapture          = sirius_vpu_GetLayerCapture           ,
	.GetCreateSurface         = sirius_vpu_GetCreateSurface          ,
	.GetReadSurface           = sirius_vpu_GetReadSurface            ,
	.GetPalette               = sirius_vpu_GetPalette                ,
	.GetAlpha                 = sirius_vpu_GetAlpha                  ,
	.GetColorFormat           = sirius_vpu_GetColorFormat            ,
	.GetColorKey              = sirius_vpu_GetColorKey               ,
	.GetBackColor             = sirius_vpu_GetBackColor              ,
	.GetPoint                 = sirius_vpu_GetPoint                  ,
	.GetConvertColor          = sirius_vpu_GetConvertColor           ,
	.GetVirtualResolution     = sirius_vpu_GetVirtualResolution      ,
	.GetVbiEnable             = sirius_vpu_GetVbiEnable              ,
	.GetVbiCreateBuffer       = sirius_vpu_GetVbiCreateBuffer        ,
	.GetVbiReadAddress        = sirius_vpu_GetVbiReadAddress         ,
	.SetLayerClipport         = sirius_vpu_SetLayerClipport          ,
	.SetBlit                  = sirius_vpu_SetBlit                   ,
	.SetDfbBlit               = sirius_vpu_SetDfbBlit                ,
	.SetBatchDfbBlit          = sirius_vpu_SetBatchDfbBlit           ,
	.SetFillRect              = sirius_vpu_SetFillRect               ,
	.SetFillPolygon           = sirius_vpu_SetFillPolygon            ,
	.SetGAMode                = sirius_vpu_SetSetMode                ,
	.SetWaitUpdate            = sirius_vpu_SetWaitUpdate             ,
	.SetBeginUpdate           = sirius_vpu_SetBeginUpdate            ,
	.SetEndUpdate             = sirius_vpu_SetEndUpdate              ,
	.ZoomSurface              = sirius_vpu_ZoomSurface               ,
	.Complet                  = sirius_vpu_Complet                   ,
	.TurnSurface              = sirius_vpu_TurnSurface               ,

	.Ga_Interrupt             = sirius_vpu_Ga_Interrupt              ,
	.SetVoutIsready           = sirius_vpu_SetVoutIsready            ,
	.DACEnable                = sirius_vpu_DACEnable                 ,
	.DACSource                = sirius_vpu_DACSource                 ,
	.DACPower                 = sirius_vpu_DACPower                  ,

	.vpp_get_base_line        = sirius_vpu_vpp_get_base_line         ,
	.vpp_set_base_line        = sirius_vpu_vpp_set_base_line         ,
	.disp_get_buff_id         = sirius_vpu_disp_get_buff_id          ,
	.disp_set_rst             = sirius_vpu_disp_set_rst              ,
	.disp_clr_rst             = sirius_vpu_disp_clr_rst              ,
	.disp_get_view_active_cnt = sirius_vpu_disp_get_view_active_cnt  ,
	.disp_clr_field_error_int = sirius_vpu_disp_clr_field_error_int  ,
	.disp_clr_field_start_int = sirius_vpu_disp_clr_field_start_int  ,
	.disp_field_error_int_en  = sirius_vpu_disp_field_error_int_en   ,
	.disp_field_start_int_en  = sirius_vpu_disp_field_start_int_en   ,
	.disp_field_error_int_dis = sirius_vpu_disp_field_error_int_dis  ,
	.disp_field_start_int_dis = sirius_vpu_disp_field_start_int_dis  ,
	.disp_get_buff_cnt        = sirius_vpu_disp_get_buff_cnt         ,
	.disp_get_view_field      = sirius_vpu_disp_get_view_field       ,
	.disp_get_display_buf     = sirius_vpu_disp_get_display_buf      ,
	.disp_layer_enable        = sirius_vpu_disp_layer_enable         ,

	.vpp_set_stream_ratio     = sirius_vpu_vpp_set_stream_ratio      ,
	.vpp_get_actual_viewrect  = sirius_vpu_vpp_get_actual_viewrect   ,
	.vpp_play_frame           = sirius_vpp_play_frame                ,

	.RegistSurface   = sirius_vpu_regist_surface,
	.UnregistSurface = sirius_vpu_unregist_surface,
	.GetIdleSurface  = sirius_vpu_get_idle_surface,
	.FlipSurface     = sirius_vpu_flip_surface,
	.PanDisplay	 = sirius_vpu_pan_display,
	.VpuIsrCallback  = sirius_vpu_isr_callback,

	.AddRegion    = sirius_vpu_add_region,
	.RemoveRegion = sirius_vpu_remove_region,
	.UpdateRegion = sirius_vpu_update_region,

	.vpu_get_scan_line = sirius_vpu_get_scan_line,
	.svpu_config = sirius_svpu_config,
	.svpu_config_buf = sirius_svpu_config_buf,
	.svpu_get_buf    = sirius_svpu_get_buf,
	.svpu_run = sirius_svpu_run,
	.svpu_stop = sirius_svpu_stop,

	.GetOsdOverlay = sirius_vpu_get_osd_overlay,
	.SetOsdOverlay = sirius_vpu_set_osd_overlay,

	.SetGAMMAConfig = taurus_vpu_set_gamma                            ,
	.GetGAMMAConfig = taurus_vpu_get_gamma                            ,
	.SetGreenBlueConfig = taurus_vpu_set_green_blue                   ,
	.GetGreenBlueConfig = taurus_vpu_get_green_blue                   ,
	.SetEnhanceConfig = taurus_vpu_set_enhance                        ,
	.GetEnhanceConfig = taurus_vpu_get_enhance                        ,
	.SetMixFilter = taurus_vpu_set_mix_filter                         ,
	.GetMixFilter = taurus_vpu_get_mix_filter                         ,
	.SetSVPUConfig = sirius_vpu_set_svpu                              ,
	.SetLayerAutoZoom = sirius_vpu_SetLayerAutoZoom                   ,
	.GetLayerAutoZoom = sirius_vpu_GetLayerAutoZoom                   ,

	.set_triming_offset = vpu_set_triming_offset,
};

struct gxav_module_ops sirius_vpu_module;
static int gx_vpu_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	VPU_REG_BASE_ADDR = res->regs[0];
	SVPU_REG_BASE_ADDR = res->regs[1];
	GA_REG_BASE_ADDR = res->regs[2];

	GA_INT_SRC  = sirius_vpu_module.irqs[0] = res->irqs[0];
	VPU_INT_SRC = res->irqs[1];
	sirius_vpu_module.interrupts[res->irqs[0]] = gx_vpu_ga_interrupt;

	return (0);
}

struct gxav_module_ops sirius_vpu_module = {
	.module_type = GXAV_MOD_VPU,
	.count = 1,
	.irqs = {12, -1},
	.irq_names = {"ga"},
	.irq_flags = {-1},
	.setup = gx_vpu_setup,
	.init = gx_vpu_init,
	.cleanup = gx_vpu_cleanup,
	.open = gx_vpu_open,
	.close = gx_vpu_close,
	.set_property = gx_vpu_set_property,
	.get_property = gx_vpu_get_property,
	.interrupts[12] = gx_vpu_ga_interrupt,
	.priv = &sirius_vpu_ops,
};

