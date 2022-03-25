#include "kernelcalls.h"
#include "porting.h"
#include "firewall.h"
#include "gxav_bitops.h"
#include "cygnus_vpu.h"
#include "vout_hal.h"
#include "vpu_color.h"
#include "cygnus_ga.h"
#include "cygnus_vpu_internel.h"
#include "kernelcalls.h"
#include "clock_hal.h"
#include "video.h"
#include "vpu_module.h"
#include "video_display.h"
#include "mempool.h"
#include "log_printf.h"
#include "cygnus_svpu.h"
#include "hdr.h"

// Base Address
static unsigned int VPU_REG_BASE_ADDR = 0x8A800000;

// Interrup Number
unsigned int VPU_INT_SRC;

// VPU info
CygnusVpu *cygnusvpu_info = NULL;
gx_spin_lock_t cygnusvpu_spin_lock;

unsigned int cygnus_osd_filter_table[OSD_FLICKER_TABLE_LEN] = {
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

extern CygnusVpuLayerOps cygnus_osd_ops;
extern CygnusVpuLayerOps cygnus_sosd_ops;
extern CygnusVpuLayerOps cygnus_sosd2_ops;
extern CygnusVpuLayerOps cygnus_vpp_ops;
extern CygnusVpuLayerOps cygnus_spp_ops;
extern CygnusVpuLayerOps cygnus_pp2_ops;
//extern CygnusVpuLayerOps cygnus_bkg_ops;

#define M_DIV_N(m, n) \
	((m)%(n)==0 ? (m)/(n) : (m)/(n)+1)

#define MAX_TMP_SURFACE_NUM (10)
#define VPU_CLOSED() (cygnusvpu_info == NULL)

#define SURFACE_MEM_POOL_ALLOCZ(p, size) \
	{                                \
		p = gxav_mem_pool_allocz(cygnusvpu_info->memory_pool);  \
		if(NULL == p) {                    \
			p = gx_malloc(size);       \
		}             \
	}

#define SURFACE_MEM_POOL_FREE(p)  \
	{   \
		if(gxav_mem_pool_free(cygnusvpu_info->memory_pool, p)) {   \
			gx_free(p);   \
			p = NULL;    \
		}    \
	}

static int remove_surface_from_list(CygnusVpuSurface *surface)
{
	CygnusVpuSurface *now   = cygnusvpu_info->surface;
	CygnusVpuSurface *prev  = cygnusvpu_info->surface;

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
	if(now == cygnusvpu_info->surface)
		cygnusvpu_info->surface = prev->next ;
//	gx_free(now);
	SURFACE_MEM_POOL_FREE(now);

	return 0;
}

static int is_in_surface_list(CygnusVpuSurface *surface)
{
	CygnusVpuSurface *now   = cygnusvpu_info->surface;

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

int cygnus_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
int cygnus_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
static int modify_surface_from_list(GxVpuProperty_ModifySurface *property)
{
	GxVpuProperty_LayerMainSurface MainSurface = {0};
	CygnusVpuSurface *surface = NULL;
	int i = 0;

	surface = (CygnusVpuSurface *)(property->surface);
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
			cygnus_vpu_GetLayerMainSurface(&MainSurface);
			if(MainSurface.surface == surface) {
				cygnus_vpu_SetLayerMainSurface(&MainSurface);
				break;
			}
		}
		return (true);
	} else {
		return (false);
	}
}

int cygnus_vpu_GetVirtualResolution(GxVpuProperty_Resolution *property)
{
	if(cygnusvpu_info != NULL){
		property->xres = cygnusvpu_info->virtual_resolution.xres;
		property->yres = cygnusvpu_info->virtual_resolution.yres;
	}
	else{
		property->xres = 1280;
		property->yres = 720;
	}

	return 0;
}

int cygnus_vpu_GetActualResolution(GxVpuProperty_Resolution *property)
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

int cygnus_vpu_VirtualToActual(int virtual, int referance, int horizontal)
{
	GxVpuProperty_Resolution virtual_resolution;
	cygnus_vpu_GetVirtualResolution(&virtual_resolution);

	if(horizontal)
		return virtual*referance/virtual_resolution.xres;
	else
		return virtual*referance/virtual_resolution.yres;
}

int cygnus_vpu_GetCreateSurface(GxVpuProperty_CreateSurface *property)
{
	unsigned int bpp;
	CygnusVpuSurface *surface = NULL;
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

//	surface = gx_malloc(sizeof(CygnusVpuSurface));
	SURFACE_MEM_POOL_ALLOCZ((surface), (sizeof(CygnusVpuSurface)));
	if(surface == NULL) {
		VPU_PRINTF("[vpu] GetCreateSurface error:[gx_malloc surface return 0] !\n");
		return -1;
	}
	gx_memset(surface, 0, sizeof(CygnusVpuSurface));

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
	surface->next = cygnusvpu_info->surface;
	cygnusvpu_info->surface = surface;

	return 0;
}

int cygnus_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);

static void cygnus_check_layer_surface(CygnusVpuSurface *surface)
{
	CygnusSurfaceManager *sm = GET_SURFACE_MANAGER(GX_LAYER_OSD);
	GxVpuProperty_LayerMainSurface main_surface = {0};
	int i = 0;

	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		if(surface == sm->surfaces[i]) {
			sm->surfaces[i] = NULL;
		}
	}

	for(i = GX_LAYER_OSD; i < GX_LAYER_MAX; i++) {
		main_surface.layer = i;
		cygnus_vpu_GetLayerMainSurface(&main_surface);
		if(main_surface.surface == surface) {
			cygnus_vpu_UnsetLayerMainSurface(&main_surface);
		}
	}
}

int cygnus_vpu_SetDestroySurface(GxVpuProperty_DestroySurface *property)
{
	unsigned long flags;

	VPU_SPIN_LOCK();
	cygnus_check_layer_surface(property->surface);
	VPU_SPIN_UNLOCK();
	return remove_surface_from_list(property->surface);
}

int cygnus_vpu_ModifySurface(GxVpuProperty_ModifySurface *property)
{
	return modify_surface_from_list(property);
}

void CygnusVpu_SetPoint(GxVpuProperty_Point *point)
{
	unsigned int addr;
	CygnusVpuSurface *surface = (CygnusVpuSurface*)point->surface;

	addr = (unsigned int)surface->buffer +
		((gx_color_get_bpp(surface->color_format) * (point->point.y * surface->width + point->point.x)) >> 3);

	if(IS_INVAILD_POINT(&point->point, surface->width, surface->height))
		return;

	gx_color_pixelcolor_write((void*)addr, surface->color_format, &(point->color));
}

int CygnusVpu_GetPoint(GxVpuProperty_Point *point)
{
	void *addr;
	unsigned int value;
	unsigned char byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
	CygnusVpuSurface *surface = (CygnusVpuSurface*)point->surface;

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

int CygnusVpu_DrawLine(GxVpuProperty_DrawLine *line)
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
				CygnusVpu_SetPoint(&point);
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
				CygnusVpu_SetPoint(&point);
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
static int cygnusce_layerid_get(GxLayerID gx_layer_id)
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

int CygnusVpu_Capture(GxVpuProperty_LayerCapture *capture)
{
	int i, ce_layer_id;
	CygnusVpuCe   ce;
	unsigned char *src ,*dst;
	unsigned int  width ,line ,request_block = 0;
	GxAvRect dst_rect = {0};

#if 0
	ce.layer    = capture->layer;
	ce.left     = capture->rect.x;
	ce.right    = capture->rect.x + capture->rect.width - 1;
	ce.top      = capture->rect.y;
	ce.bottom   = capture->rect.y + capture->rect.height - 1;
	ce.buffer   = ((CygnusVpuSurface *) capture->surface)->buffer;
	if (capture->layer == GX_LAYER_MIX_VPP_BKG) {
		ce_layer_id = CE_LAYER_VPP;
		VPU_CAP_SET_MIX_BKG_ENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
	}
	else if(capture->layer == GX_LAYER_MIX_SPP_BKG){
		ce_layer_id = CE_LAYER_SPP;
		VPU_CAP_SET_MIX_BKG_ENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
	}
	else {
		ce_layer_id = cygnusce_layerid_get(capture->layer);
		VPU_CAP_SET_MIX_BKG_DISENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
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

	VPU_VPP_SET_REQUEST_BLOCK(cygnusvpu_reg->rPP_FRAME_STRIDE, request_block);
	VPU_CAP_SET_LEVEL(cygnusvpu_reg->rCAP_CTRL, ce_layer_id);
	VPU_CAP_SET_PIC_ADDR(cygnusvpu_reg->rCAP_ADDR, (unsigned int)gx_virt_to_phys((unsigned int)ce.buffer));
	VPU_CAP_SET_PIC_HORIZONTAL(cygnusvpu_reg->rCAP_WIDTH, ce.left, ce.right);
	VPU_CAP_SET_PIC_VERTICAL(cygnusvpu_reg->rCAP_HEIGHT, ce.top, ce.bottom);
	VPU_CAP_START(cygnusvpu_reg->rCAP_CTRL);
	VPU_CAP_STOP(cygnusvpu_reg->rCAP_CTRL);

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
#endif
	if((capture->rect.width < ((CygnusVpuSurface *)capture->surface)->width) ||
	   ((capture->rect.height < ((CygnusVpuSurface *)capture->surface)->height)))
		return -1;

	ce_enable(0);
	ce_set_format(((CygnusVpuSurface *)capture->surface)->color_format);
	ce_set_mode(CE_MODE_MANUAL);
	ce_set_layer(1<< capture->layer);
	ce_set_buf((CygnusVpuSurface *)capture->surface);

	dst_rect.x = dst_rect.y = 0;
	dst_rect.width = ((CygnusVpuSurface *)capture->surface)->width;
	dst_rect.height = ((CygnusVpuSurface *)capture->surface)->height;

	ce_zoom(&capture->rect, &dst_rect);
	ce_enable(1);
	gx_dcache_clean_range(0, 0);

	return 0;
}

extern int CygnusVpu_Zoom(GxLayerID layer);
int cygnus_vpu_SetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	CygnusVpuLayer *layer = NULL;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	cygnus_vpu_GetVirtualResolution(&virtual_resolution);
	if((IS_INVAILD_LAYER(property->layer))
			|| ((cygnusvpu_info->layer[property->layer].auto_zoom) && (IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres)))
			|| (IS_NULL(cygnusvpu_info->layer[property->layer].surface)))
		return -1;

	layer = &cygnusvpu_info->layer[property->layer];

	layer->view_rect = property->rect;
	if (property->layer == GX_LAYER_VPP
			&& (cygnusvpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		cygnus_vpp_zoom_require();
	} else if(property->layer == GX_LAYER_SPP
			&& (cygnusvpu_info->layer[GX_LAYER_SPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		;
	} else {
		gx_mutex_lock(&cygnusvpu_info->mutex);
		if(cygnusvpu_info->vout_is_ready)
			CygnusVpu_Zoom(property->layer);
		gx_mutex_unlock(&cygnusvpu_info->mutex);
	}

	return 0;
}

int cygnus_vpu_SetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	CygnusVpuLayer *layer;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	cygnus_vpu_GetVirtualResolution(&virtual_resolution);
	if( (IS_INVAILD_LAYER(property->layer))||
			(IS_NULL(cygnusvpu_info))||
			(IS_NULL(cygnusvpu_info->layer[property->layer].surface)))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
		return -1;

	layer = &cygnusvpu_info->layer[property->layer];
	layer->clip_rect.x      = property->rect.x;
	layer->clip_rect.y      = property->rect.y;
	layer->clip_rect.width  = property->rect.width;
	layer->clip_rect.height = property->rect.height;

	if (property->layer == GX_LAYER_VPP &&
			(cygnusvpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO){
		return cygnus_vpp_zoom_require();
	}

	return 0;
}

static int cygnus_vpu_disp_field_start_int_en(void);
int cygnus_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	CygnusVpuLayer *layer;
	CygnusVpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	if(false == is_in_surface_list(property->surface)) {
		return (-1);
	}

	surface = (CygnusVpuSurface*)property->surface;
	layer = &cygnusvpu_info->layer[property->layer];
	if(layer->ops->set_main_surface != NULL) {
		if((surface->width * gx_color_get_bpp(surface->color_format) >> 3) % 8 != 0)
			return -1;

		layer->surface = (CygnusVpuSurface *) property->surface;
		layer->surface->layer = layer;

		if (property->layer == GX_LAYER_VPP && GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
			cygnus_vpu_disp_field_start_int_en();
			cygnusvpu_info->layer[GX_LAYER_VPP].flip_require = 1;
			while(cygnusvpu_info->layer[GX_LAYER_VPP].flip_require);
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

int cygnus_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	CygnusVpuLayer *layer;
	CygnusVpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	surface = (CygnusVpuSurface*)property->surface;
	layer   = &cygnusvpu_info->layer[property->layer];
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
	CygnusVpuLayerOps *layer_ops;\
	GXAV_ASSERT(property != NULL);\
	if (IS_INVAILD_LAYER(layer_id) || cygnusvpu_info == NULL)\
	return -1;\
	layer_ops = cygnusvpu_info->layer[layer_id].ops;\
	if (layer_ops->func != NULL) {\
		cygnusvpu_info->layer[layer_id].attr = value;\
		return layer_ops->func(value);\
	}\
	return -1;\
}\

static int cygnusVpu_LayerEnable(GxLayerID layer, int enable)
{
	if (IS_INVAILD_LAYER(layer))
		return -1;
	if (cygnusvpu_info->layer[layer].ops->set_enable)
		cygnusvpu_info->layer[layer].ops->set_enable(enable);
	return 0;
}

int cygnus_vpu_SetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GXAV_ASSERT(property != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	if (property->enable == 0) {
		if(property->layer == GX_LAYER_VPP) {
			gx_video_ppclose_require(0);
		}
		cygnusVpu_LayerEnable(property->layer, 0);
		cygnusvpu_info->layer[property->layer].enable = 0;
	}
	else {
		if (cygnusvpu_info->layer[property->layer].surface == NULL)
			return -1;

		if (cygnusvpu_info->vout_is_ready) {
			if(property->layer == GX_LAYER_VPP) {
				gx_video_ppopen_require(0);
			}
			cygnusVpu_LayerEnable(property->layer, 1);
		}
		cygnusvpu_info->layer[property->layer].enable = 1;
	}


	return 0;
}

int cygnus_vpu_SetLayerEnablePatch(GxVpuProperty_LayerEnable *property)
{
	RUN_LAYER_FUNC(property->layer, set_enable, enable, property->enable);
}

int cygnus_vpu_SetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	RUN_LAYER_FUNC(property->layer, set_anti_flicker, anti_flicker_en, property->enable);
}

int cygnus_vpu_SetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	RUN_LAYER_FUNC(property->layer, set_on_top, on_top, property->enable);
}

int cygnus_vpu_SetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	RUN_LAYER_FUNC(property->layer, set_video_mode, video_mode, property->mode);
}

#if 0
int cygnus_vpu_SetLayerMixConfig(GxVpuProperty_LayerMixConfig *property)
{
	if(property->covermode_en) {
		/* covermode en */
		VPU_MIX_LAYER_COVERMODE_ENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
		/* spp alpha config */
		if(property->spp_alpha.type == GX_ALPHA_GLOBAL) {
			VPU_MIX_SPP_GLOBAL_ALPHA_ENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
			VPU_MIX_SET_SPP_GLOBAL_ALPHA(cygnusvpu_reg->rPP_JPG_MIX_CTRL, property->spp_alpha.value);
		}
		else {
			VPU_MIX_SPP_GLOBAL_ALPHA_DISENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
		}
		/* vpp alpha config */
		VPU_MIX_SET_VPP_GLOBAL_ALPHA(cygnusvpu_reg->rPP_JPG_MIX_CTRL, property->vpp_alpha_value);
	}
	else {
		VPU_MIX_LAYER_COVERMODE_DISENABLE(cygnusvpu_reg->rPP_JPG_MIX_CTRL);
	}

	return 0;
}
#endif

#define ACTIVE_SURFACE_CHANGE(layer_id, func,value)\
{\
	CygnusVpuLayerOps *layer_ops;\
	if(IS_INVAILD_LAYER(layer_id) || cygnusvpu_info == NULL)\
	return -1;\
	layer_ops = cygnusvpu_info->layer[layer_id].ops;\
	if(layer_ops->func != NULL)\
	return layer_ops->func(value);\
	return -1;\
}

#define RUN_SURFACE_FUNC(surface_handle, func, attr,value)\
{\
	CygnusVpuSurface *surface;\
	if(IS_NULL(surface_handle) || cygnusvpu_info == NULL)\
	return -1;\
	surface = (CygnusVpuSurface *) (surface_handle);\
	surface->attr = value;\
	if(IS_MAIN_SURFACE(surface))\
	ACTIVE_SURFACE_CHANGE(surface->layer->id,func,value);\
	return 0;\
}

int cygnus_vpu_SetPalette(GxVpuProperty_Palette *property)
{
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
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

int cygnus_vpu_SetAlpha(GxVpuProperty_Alpha *property)
{
	if(property->alpha.type == GX_ALPHA_GLOBAL && property->alpha.value >=256)
		return -1;
	if(property->alpha.type != GX_ALPHA_GLOBAL && property->alpha.type != GX_ALPHA_PIXEL)
		return -1;

	RUN_SURFACE_FUNC(property->surface, set_alpha, alpha, property->alpha);
}

int cygnus_vpu_SetColorFormat(GxVpuProperty_ColorFormat *property)
{
	if(property->format<GX_COLOR_FMT_CLUT1 || property->format>GX_COLOR_FMT_YCBCR420_UV)
		return -1;
	RUN_SURFACE_FUNC(property->surface, set_color_format, color_format, property->format);
}

int cygnus_vpu_SetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	CygnusVpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if (!IS_INVAILD_LAYER(property->layer)) {
		layer = &cygnusvpu_info->layer[property->layer];
		if(layer->ops->set_bg_color != NULL) {
			layer->bg_color = property->color;
			ret = layer->ops->set_bg_color(property->color);
		}
	}

	return ret;
}

int cygnus_vpu_SetColorKey(GxVpuProperty_ColorKey *property)
{
	CygnusVpuSurface *surface;
	CygnusVpuLayerOps *layer_ops;

	GXAV_ASSERT(property != NULL);
	if (IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
	surface->color_key = property->color;
	surface->color_key_en = property->enable;
	if (IS_MAIN_SURFACE(surface)) {
		if (IS_INVAILD_LAYER(surface->layer->id))
			return -1;
		layer_ops = cygnusvpu_info->layer[surface->layer->id].ops;

		if (layer_ops->set_color_key_mode != NULL)
			layer_ops->set_color_key_mode(property->mode, property->ext_alpha);
		if (layer_ops->set_color_key != NULL)
			return layer_ops->set_color_key(&surface->color_key, surface->color_key_en);
	}

	return 0;
}

int cygnus_vpu_SetPoint(GxVpuProperty_Point *property)
{
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;
	CygnusVpu_SetPoint(property);

	return 0;
}

int cygnus_vpu_SetDrawLine(GxVpuProperty_DrawLine *property)
{
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->start, surface->width, surface->height)
			|| IS_INVAILD_POINT(&property->end, surface->width, surface->height))
		return -1;

	return CygnusVpu_DrawLine(property);
}

#define GET_SURFACE(_src_is_ksurface, _src, _dst)\
	do{\
		typeof(_src) src = (_src);\
		typeof(_dst) dst = (_dst);\
		typeof(_src_is_ksurface) src_is_ksurface = (_src_is_ksurface);\
		gx_memset(dst, 0, sizeof(CygnusVpuSurface));\
		if(src_is_ksurface){\
			gx_memcpy(dst, src, sizeof(CygnusVpuSurface));\
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

int cygnus_vpu_SetBlit(GxVpuProperty_Blit *property)
{
	int i, ret=-1;
	CygnusVpuSurface *sura, *surb, *surd;
	static GxVpuProperty_Blit kproperty;
	static CygnusVpuSurface isrca, isrcb,  idst;

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
			CygnusVpuSurface *sur_tmp;
			GxVpuProperty_Blit tmp_property;

			sur_tmp = (CygnusVpuSurface*)gx_malloc(sizeof(CygnusVpuSurface)* 3);
			gx_memcpy(&sur_tmp[0], sura, sizeof(CygnusVpuSurface));
			gx_memcpy(&sur_tmp[1], surb, sizeof(CygnusVpuSurface));
			gx_memcpy(&sur_tmp[2], surd, sizeof(CygnusVpuSurface));
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

				ret = cygnusga_blit(&tmp_property);
			}
			gx_free(sur_tmp);
		}
		else {
			ret = cygnusga_blit(&kproperty);
		}
	}

	return ret;
}






int cygnus_vpu_SetDfbBlit(GxVpuProperty_DfbBlit *property)
{
	int i, ret=-1;
	CygnusVpuSurface *sura, *surb, *surd;
	static GxVpuProperty_DfbBlit kproperty;
	static CygnusVpuSurface isrca, isrcb,  idst;

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
		CygnusVpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (CygnusVpuSurface*)gx_malloc(sizeof(CygnusVpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(CygnusVpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(CygnusVpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(CygnusVpuSurface));
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

			ret = cygnusga_blit(&tmp_property);
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
				if(cygnus_vpu_GetCreateSurface(&tmp_dst)) {
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
				cygnusga_dfb_blit(&kproperty);

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
			ret = cygnusga_dfb_blit(&kproperty);
out:
			/* destroy tmp surfaces */
			for(i = 0; i < tmp_cnt; i++) {
				if(destroy_surface[i].surface != NULL)
					cygnus_vpu_SetDestroySurface(&destroy_surface[i]);
			}
		} else {
			ret = cygnusga_dfb_blit(&kproperty);
		}
		return ret;
	}
	return ret;
}

int cygnus_vpu_SetBatchDfbBlit(GxVpuProperty_BatchDfbBlit *property)
{
	int i, ret=-1;
	CygnusVpuSurface *sura, *surb, *surd;
	static GxVpuProperty_BatchDfbBlit kproperty;
	static CygnusVpuSurface isrca, isrcb,  idst;

	GXAV_ASSERT(property != NULL);
	if( IS_NULL(property->basic.srca.surface)|| IS_NULL(property->basic.srcb.surface)|| IS_NULL(property->basic.dst.surface) )
		return -1;

	/* copy surface */
	gx_memcpy(&kproperty, property, sizeof(GxVpuProperty_BatchDfbBlit));
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

	if(IS_INVALID_ALU_MODE(property->basic.mode)) {
		VPU_PRINTF("BLIT ERROR:invaild alu mode \n");
		return -1;
	}

	if(GX_COLOR_FMT_YCBCR420_Y_UV == sura->color_format) {
		CygnusVpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (CygnusVpuSurface*)gx_malloc(sizeof(CygnusVpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(CygnusVpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(CygnusVpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(CygnusVpuSurface));
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

			ret = cygnusga_blit(&tmp_property);
		}
		gx_free(sur_tmp);
	} else {
		ret = cygnusga_batch_dfb_blit(&kproperty);
		return ret;
	}
	return ret;
}

int cygnus_vpu_SetFillRect(GxVpuProperty_FillRect *property)
{
	static CygnusVpuSurface surface;
	static GxVpuProperty_FillRect kproperty;

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

	return cygnusga_fillrect(&kproperty);
}

int cygnus_vpu_SetFillPolygon(GxVpuProperty_FillPolygon *property)
{
#if 0
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_GA_COLOR(surface->color_format))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, surface->width, surface->height))
		return -1;

	return cygnusga_fillpolygon(property);
#else
	return 0;
#endif
}

int cygnus_vpu_SetSetMode(GxVpuProperty_SetGAMode *property)
{
	GXAV_ASSERT(property != NULL);
	return cygnusga_setmode(property);
}

int cygnus_vpu_SetWaitUpdate(GxVpuProperty_WaitUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return cygnusga_wait(property);
}

int cygnus_vpu_SetBeginUpdate(GxVpuProperty_BeginUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return cygnusga_begin(property);
}

int cygnus_vpu_SetEndUpdate(GxVpuProperty_EndUpdate *property)
{
	return cygnusga_end(property);
}

int cygnus_vpu_SetConvertColor(GxVpuProperty_ConvertColor *property)
{
	GXAV_ASSERT(property != NULL);
	return 0;
}

int cygnus_vpu_SetVirtualResolution(GxVpuProperty_Resolution *property)
{
	GXAV_ASSERT(property != NULL);
	cygnusvpu_info->virtual_resolution.xres = property->xres;
	cygnusvpu_info->virtual_resolution.yres = property->yres;

	cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect.width  = property->xres;
	cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect.height = property->yres;

	return 0;
}

int cygnus_vpu_SetAspectRatio(GxVpuProperty_AspectRatio *property)
{
	GXAV_ASSERT(property != NULL);
	if((property->AspectRatio > VP_AUTO)
			|| (property->AspectRatio < VP_UDEF))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	cygnusvpu_info->layer[GX_LAYER_SPP].spec=property->AspectRatio;
	cygnusvpu_info->layer[GX_LAYER_VPP].spec=property->AspectRatio;
	cygnus_vpp_zoom_require();

	return 0;
}

int cygnus_vpu_SetTvScreen(GxVpuProperty_TvScreen *property)
{
	GxVpuProperty_LayerViewport  vpp_viewport, spp_viewport;

	GXAV_ASSERT(property != NULL);
	if((property->Screen > SCREEN_16X9) || (property->Screen < SCREEN_4X3))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	spp_viewport.layer= GX_LAYER_SPP;
	spp_viewport.rect = cygnusvpu_info->layer[GX_LAYER_SPP].view_rect;
	cygnusvpu_info->layer[GX_LAYER_SPP].screen = property->Screen;
	cygnus_vpu_SetLayerViewport(&spp_viewport);

	vpp_viewport.layer= GX_LAYER_VPP;
	vpp_viewport.rect = cygnusvpu_info->layer[GX_LAYER_VPP].view_rect;
	cygnusvpu_info->layer[GX_LAYER_VPP].screen = property->Screen;
	cygnus_vpu_SetLayerViewport(&vpp_viewport);

	return 0;
}

int cygnus_vpu_SetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	GXAV_ASSERT(property != NULL);
	cygnusvpu_info->vbi.enable = property->enable;
	cygnus_vpu_VbiEnable(&cygnusvpu_info->vbi);

	return 0;
}

#define GET_LAYER_PROPERTY(attr,layer_attr)\
{\
	GXAV_ASSERT(property != NULL);\
	if(IS_INVAILD_LAYER(property->layer))\
	return -1;\
	if( cygnusvpu_info!=NULL ){\
		property->attr  = cygnusvpu_info->layer[property->layer].layer_attr ;\
		return 0;\
	}\
	else\
	return -1; \
}

#define GET_SURFACE_PROPERTY(attr,surface_attr)\
{\
	CygnusVpuSurface *surface;\
	GXAV_ASSERT(property != NULL);\
	if(IS_NULL(property->surface))\
	return -1;\
	surface = (CygnusVpuSurface *)(property->surface);\
	property->attr  = surface->surface_attr;\
	return 0;\
}

int cygnus_vpu_GetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	GET_LAYER_PROPERTY(rect, view_rect);
}

int cygnus_vpu_GetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	GET_LAYER_PROPERTY(rect, clip_rect);
}

int cygnus_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	GET_LAYER_PROPERTY(surface, surface);
}

int cygnus_vpu_GetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GET_LAYER_PROPERTY(enable, enable);
}

int cygnus_vpu_GetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	GET_LAYER_PROPERTY(enable, anti_flicker_en);
}

int cygnus_vpu_GetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	GET_LAYER_PROPERTY(enable, on_top);
}

int cygnus_vpu_GetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	GET_LAYER_PROPERTY(mode, video_mode);
}

int cygnus_vpu_GetLayerCapture(GxVpuProperty_LayerCapture *property)
{
#define IS_POINT_IN_RECT(point,rect)\
	((point.x>=rect.x)&&(point.y>=rect.y)&&(point.x<=rect.x+rect.width)&&(point.y<=rect.y+rect.height))

	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution	referance, virtual_resolution;
	CygnusVpuSurface *surface = (CygnusVpuSurface*)property->surface;
	GxVpuProperty_LayerCapture  cap_property;
	CygnusVpuLayer *layer;
	GxLayerID       layer_id;
	GxAvRect        capture_rect = {0};
	GxAvPoint       end_point    = {0};
	GxAvPoint       point_topleft= {0}, point_topright={0}, point_bottomleft={0}, point_bottomright={0};

	GXAV_ASSERT(property != NULL);
	GXAV_ASSERT(property->surface != NULL);

	cygnus_vpu_GetVirtualResolution(&virtual_resolution);
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
		if((surface->color_format!=GX_COLOR_FMT_YCBCR422) &&
		   (surface->color_format!=GX_COLOR_FMT_YCBCR422_Y_UV) &&
		   (surface->color_format!=GX_COLOR_FMT_YCBCR444_Y_U_V)) {
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

			layer = &(cygnusvpu_info->layer[layer_id]);
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

		cygnus_vpu_GetActualResolution(&referance);
		cap_property.layer       =  property->layer;
		cap_property.rect.x      =  cygnus_vpu_VirtualToActual(capture_rect.x, referance.xres, 1);
		cap_property.rect.y      =  cygnus_vpu_VirtualToActual(capture_rect.y, referance.yres, 0);
		cap_property.rect.width  =  cygnus_vpu_VirtualToActual(capture_rect.width,  referance.xres, 1);
		cap_property.rect.height =  cygnus_vpu_VirtualToActual(capture_rect.height, referance.yres, 0);
		cap_property.surface = (void*)property->surface;
		cap_property.svpu_cap = property->svpu_cap;

		return CygnusVpu_Capture(&cap_property);
	}
}

int cygnus_vpu_SetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	GXAV_ASSERT(property != NULL);

	cygnusvpu_info->layer[property->layer].auto_zoom = property->auto_zoom;

	return (0);
}

int cygnus_vpu_GetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	GXAV_ASSERT(property != NULL);

	property->auto_zoom = cygnusvpu_info->layer[property->layer].auto_zoom;

	return (0);
}

int cygnus_vpu_GetCreatePalette(GxVpuProperty_CreatePalette *property)
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
	property->palette = (GxPalette *)gx_malloc(property->palette_num * (sizeof(GxPalette) + num_entries * sizeof(GxColor)) + 8 * property->palette_num);
	if(NULL == property->palette) {
		gx_printf("[vpu] CreatePalette malloc NULL ,error! \n");
		return -1;
	}

	gx_palette = property->palette;
	for (i=0; i<property->palette_num; i++) {
		gx_palette->num_entries = num_entries;
		gx_palette->entries= (GxColor*)((unsigned int)(gx_palette)+sizeof(GxPalette));
	if((int)gx_palette->entries % 8) {
		gx_palette->entries = (GxColor *)(((int)gx_palette->entries / 8 + 1) * 8);
	}
		gx_palette = (GxPalette*)((unsigned int)gx_palette + sizeof(GxPalette) + num_entries * sizeof(GxColor));
	}

	return 0;
}

int cygnus_vpu_DestroyPalette(GxVpuProperty_DestroyPalette *property)
{
	if(property->palette!=NULL)
		gx_free(property->palette);
	property->palette = NULL;

	return 0;
}

int cygnus_vpu_SurfaceBindPalette(GxVpuProperty_SurfaceBindPalette *property)
{
	CygnusVpuSurface* surface = (CygnusVpuSurface*)(property->surface);;
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
int cygnus_vpu_WPalette(GxVpuProperty_RWPalette *property)
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
int cygnus_vpu_RPalette(GxVpuProperty_RWPalette *property)
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

int cygnus_vpu_GetEntries(GxVpuProperty_GetEntries *property)
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

int cygnus_vpu_GetReadSurface(GxVpuProperty_ReadSurface *property)
{
	CygnusVpuSurface *surface = NULL;
	CygnusVpuSurface *now = NULL;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(cygnusvpu_info))
		return -1;
	surface = (CygnusVpuSurface *) property->surface;
	if(surface == NULL)
		return -1;

	now = cygnusvpu_info->surface;
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

int cygnus_vpu_GetPalette(GxVpuProperty_Palette *property)
{
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;
	surface = (CygnusVpuSurface *) (property->surface);
	if(surface->palette == NULL)
		return -1;

	gx_memcpy(&property->palette, surface->palette, sizeof(GxPalette));
	return 0;
}

int cygnus_vpu_GetAlpha(GxVpuProperty_Alpha *property)
{
	GET_SURFACE_PROPERTY(alpha, alpha);
}

int cygnus_vpu_GetColorFormat(GxVpuProperty_ColorFormat *property)
{
	GET_SURFACE_PROPERTY(format, color_format);
}

int cygnus_vpu_GetColorKey(GxVpuProperty_ColorKey *property)
{
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
	property->color = surface->color_key;
	property->enable= surface->color_key_en;

	return 0;
}

int cygnus_vpu_GetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	CygnusVpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &cygnusvpu_info->layer[property->layer];
		property->color = layer->bg_color;
		ret = 0;
	}

	return ret;
}

int cygnus_vpu_GetPoint(GxVpuProperty_Point *property)
{
	CygnusVpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (CygnusVpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;

	CygnusVpu_GetPoint(property);
	return 0;
}

int cygnus_vpu_GetConvertColor(GxVpuProperty_ConvertColor *property)
{
	return 0;
}

int cygnus_vpu_GetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	property->enable = cygnusvpu_info->vbi.enable;
	return 0;
}

int cygnus_vpu_GetVbiCreateBuffer(GxVpuProperty_VbiCreateBuffer *property)
{
	if(property->unit_num == 0)
		return -1;
	if(property->buffer == NULL) {
		property->buffer = gx_malloc((property->unit_data_len + 1) * 4 *property->unit_num);
		if(property->buffer == NULL)
			return -1;
	}

	cygnus_vpu_VbiCreateBuffer(&cygnusvpu_info->vbi);
	return 0;
}

int cygnus_vpu_GetVbiReadAddress(GxVpuProperty_VbiReadAddress *property)
{
	cygnus_vpu_VbiGetReadPtr(&cygnusvpu_info->vbi);
	property->read_address = cygnusvpu_info->vbi.read_address;
	return 0;
}

int cygnus_vpu_ZoomSurface(GxVpuProperty_ZoomSurface *property)
{
	unsigned int ret=0;
	CygnusVpuSurface src_surface, dst_surface;
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
	ret = cygnusga_zoom(&kproperty);

	return ret ;
}

int cygnus_vpu_TurnSurface(GxVpuProperty_TurnSurface *property)
{
	CygnusVpuSurface src_surface, dst_surface;
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

	return cygnusga_turn(&kproperty);
}

int cygnus_vpu_Complet(GxVpuProperty_Complet *property)
{
	GXAV_ASSERT(property != NULL);
	return cygnusga_complet(property);
}

int cygnus_vpu_Ga_Interrupt(int irq)
{
	return cygnusga_interrupt(irq);
}

static void vpp_disp_controler_init(DispControler *controler)
{
	int i;
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	if (controler) {
		controler->buf_to_wr = 0;
		for(i = 0; i < MAX_DISPBUF_NUM; i++)
			controler->unit[i] = (struct disp_unit*)reg->rDISP_CTRL[i];
	}
}

static void vpu_init_auto_zoom(CygnusVpuLayer *layer)
{
	int i = 0;

	for(i = 0; i < GX_LAYER_MAX; i++) {
		layer[i].auto_zoom = true;
	}
}

int cygnus_vpu_open(void)
{
	unsigned int value;
	CygnusVpuOsdPriv  *osd_priv  = NULL;
	CygnusVpuSosdPriv *sosd_priv = NULL;
	CygnusVpuSosdPriv *sosd2_priv = NULL;
	CygnusVpuVppPriv  *vpp_priv  = NULL;

	VPU_DBG("[CYGNUS]:vpu Open start!\n");
	cygnusvpu_info = (CygnusVpu *) gx_malloc(sizeof(CygnusVpu));
	if(cygnusvpu_info == NULL) {
		VPU_PRINTF("Open Error: cygnusvpu_info MALLOC\n");
		return -1;
	}

	gx_memset(cygnusvpu_info, 0, sizeof(CygnusVpu));
	cygnusvpu_info->layer[GX_LAYER_OSD].ops  = &cygnus_osd_ops;
	cygnusvpu_info->layer[GX_LAYER_VPP].ops  = &cygnus_vpp_ops;
	cygnusvpu_info->layer[GX_LAYER_SPP].ops  = &cygnus_spp_ops;
	cygnusvpu_info->layer[GX_LAYER_VPP2].ops = &cygnus_pp2_ops;
	cygnusvpu_info->layer[GX_LAYER_BKG].ops  = NULL; //&cygnus_bkg_ops;

	cygnusvpu_info->layer[GX_LAYER_OSD].id   = GX_LAYER_OSD;
	cygnusvpu_info->layer[GX_LAYER_VPP].id   = GX_LAYER_VPP;
	cygnusvpu_info->layer[GX_LAYER_SPP].id   = GX_LAYER_SPP;
	cygnusvpu_info->layer[GX_LAYER_VPP2].id   = GX_LAYER_VPP2;
	cygnusvpu_info->layer[GX_LAYER_BKG].id   = GX_LAYER_BKG;

	cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.x = 0;
	cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.y = 0;
	cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.width = CYGNUS_MAX_XRES;
	cygnusvpu_info->layer[GX_LAYER_VPP].view_rect.height= CYGNUS_MAX_YRES;
	cygnusvpu_info->layer[GX_LAYER_VPP].clip_rect = cygnusvpu_info->layer[GX_LAYER_VPP].view_rect;

	cygnusvpu_info->layer[GX_LAYER_VPP2].view_rect.x = 0;
	cygnusvpu_info->layer[GX_LAYER_VPP2].view_rect.y = 0;
	cygnusvpu_info->layer[GX_LAYER_VPP2].view_rect.width = 720;
	cygnusvpu_info->layer[GX_LAYER_VPP2].view_rect.height= 576;
	cygnusvpu_info->layer[GX_LAYER_VPP2].clip_rect = cygnusvpu_info->layer[GX_LAYER_VPP2].view_rect;

	vpu_init_auto_zoom(cygnusvpu_info->layer);

	//alloc osd priv
	osd_priv = (CygnusVpuOsdPriv*)gx_dma_malloc(sizeof(CygnusVpuOsdPriv));
	if(osd_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)osd_priv, 0, sizeof(CygnusVpuOsdPriv));
	cygnusvpu_info->layer[GX_LAYER_OSD].priv = osd_priv;
#if REGION_POOL_EN
	osd_priv->region_pool = mpool_create(sizeof(OsdRegion), 8);
	if (osd_priv->region_pool == NULL) {
		VPU_PRINTF("Open Error: RegionPool ALLOC\n");
	}
#endif

	//alloc vpp priv
	vpp_priv = (CygnusVpuVppPriv*)gx_malloc(sizeof(CygnusVpuVppPriv));
	if(vpp_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)vpp_priv, 0, sizeof(CygnusVpuVppPriv));
	cygnusvpu_info->layer[GX_LAYER_VPP].priv = vpp_priv;
	vpp_disp_controler_init(&vpp_priv->controler);

	{
		clock_params clock;
		clock.vpu.div = 1;
		clock.vpu.clock = 7;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
		clock.vpu.clock = 30;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
	}
	cygnusvpu_info->memory_pool = gxav_mem_pool_init(sizeof(CygnusVpuSurface), CYGNUS_MEM_POOL_SIZE);
	cygnusga_open();

	vpu_int_en(VPU_INT_FIELD_START);

	ce_init();
	hdr_init();

	cygnusvpu_info->reset_flag = 1;
	gx_mutex_init(&cygnusvpu_info->mutex);
	return 0;
}

int cygnus_vpu_close(void)
{
	CygnusVpuOsdPriv **osd_priv  = NULL;
	CygnusVpuOsdPriv **sosd_priv = NULL;
	CygnusVpuVppPriv **vpp_priv  = NULL;

	if(cygnusvpu_info) {
		CygnusVpuSurface *next;
		CygnusVpuSurface *now = cygnusvpu_info->surface;

		while (now != NULL) {
			next = now->next;
			if(remove_surface_from_list(now) != 0)
				break;
			now = next;
		}

		//free osd priv
		osd_priv = (CygnusVpuOsdPriv**)(&(cygnusvpu_info->layer[GX_LAYER_OSD].priv));
		if(*osd_priv) {
#if REGION_POOL_EN
			if ((*osd_priv)->region_pool) {
				mpool_destroy((*osd_priv)->region_pool);
			}
#endif
			gx_dma_free((unsigned char *)*osd_priv, sizeof(CygnusVpuOsdPriv));
			*osd_priv = NULL;
		}

		//free sosd priv
		sosd_priv = (CygnusVpuSosdPriv**)(&(cygnusvpu_info->layer[GX_LAYER_SOSD].priv));
		if(*sosd_priv) {
			gx_dma_free((unsigned char *)*sosd_priv, sizeof(CygnusVpuSosdPriv));
			*sosd_priv = NULL;
		}

		//free vpp priv
		vpp_priv = (CygnusVpuVppPriv**)(&(cygnusvpu_info->layer[GX_LAYER_VPP].priv));
		if(*vpp_priv) {
			gx_free(*vpp_priv);
			*vpp_priv = NULL;
		}
		if(cygnusvpu_info->memory_pool) {
			gxav_mem_pool_destroy(cygnusvpu_info->memory_pool);
			cygnusvpu_info->memory_pool = 0;
		}

		gx_mutex_destroy(&cygnusvpu_info->mutex);
		gx_free(cygnusvpu_info);
		cygnusvpu_info = NULL;
	}

	vpu_all_layers_en(0);
	vpu_vbi_en(0);
	VPU_DBG("[CYGNUS]:Close OK!\n");

	return 0;
}

static void cygnus_VpuIOUnmap(void)
{
	cygnus_vpu_reg_iounmap();
	return ;
}

static int cygnus_VpuIORemap(void)
{
	return cygnus_vpu_reg_ioremap();
}

static int cygnus_vpu_init(void)
{
	unsigned int i;
	GxVpuMixerLayers cfg = {0};

	if(0 != cygnus_VpuIORemap()) {
		return -1;
	}

	cygnusga_init();
	vpu_all_layers_en(1);

	cfg.id = HD_MIXER;
	cfg.layers[2] =  GX_LAYER_OSD;
	cfg.layers[1] =  GX_LAYER_SPP;
	cfg.layers[0] =  GX_LAYER_VPP;
	vpu_mixer_set_layers(&cfg);

	/*### 加载各种系数, 调用各个子模块的init*/
#if 0 
	VPU_VPP_SET_REQUEST_BLOCK(cygnusvpu_reg->rPP_FRAME_STRIDE, 0x100);
	VPU_SPP_SET_BUFFER_REQ_DATA_LEN(cygnusvpu_reg->rPIC_PARA, 0x100);
	VPU_VPP_SET_ZOOM(cygnusvpu_reg->rPP_ZOOM, 0x1000, 0x1000);
	VPU_VPP_SET_PP_SIZE(cygnusvpu_reg->rPP_VIEW_SIZE, CYGNUS_MAX_XRES, CYGNUS_MAX_YRES);
	VPU_VPP_SET_FIELD_MODE_UV_FIELD(cygnusvpu_reg->rPP_CTRL,0x1);
	VPU_SPP_SET_BG_COLOR(cygnusvpu_reg->rPIC_BACK_COLOR, 0x10, 0x80, 0x80);
	for (i = 0; i < OSD_FLICKER_TABLE_LEN; i++) {
		VPU_OSD_SET_FLIKER_FLITER(cygnusvpu_reg->rOSD_PHASE_PARA, i, cygnus_osd_filter_table[i]);
		VPU_OSD_SET_FLIKER_FLITER(cygnusvpu_reg->rSOSD_PHASE_PARA, i, cygnus_osd_filter_table[i]);
		VPU_OSD_SET_FLIKER_FLITER(cygnusvpu_reg->rSOSD2_PHASE_PARA, i, cygnus_osd_filter_table[i]);
	}
#endif

	VPU_SPIN_LOCK_INIT();
	return 0;
}

static int cygnus_vpu_cleanup(void)
{
	cygnus_vpu_close();
	cygnusga_cleanup();
	cygnus_VpuIOUnmap();
	return 0;
}

static int cygnus_vpu_SetVoutIsready(int ready)
{
	if(cygnusvpu_info != NULL) {
		gx_mutex_lock(&cygnusvpu_info->mutex);
		cygnusvpu_info->vout_is_ready = ready;
		if(ready) {
			gx_video_disp_patch(0);
			//re-zoom all layer
			if(cygnusvpu_info->layer[GX_LAYER_OSD].auto_zoom) {
				CygnusVpu_Zoom(GX_LAYER_OSD);
			}

			if(cygnusvpu_info->layer[GX_LAYER_SOSD].auto_zoom) {
				CygnusVpu_Zoom(GX_LAYER_SOSD);
			}

			if(cygnusvpu_info->layer[GX_LAYER_SPP].auto_zoom) {
				CygnusVpu_Zoom(GX_LAYER_SPP);
			}

			if(cygnusvpu_info->layer[GX_LAYER_VPP].auto_zoom) {
				cygnus_vpp_zoom_require();
			}
			vpu_all_layers_en(1);
			gx_mutex_unlock(&cygnusvpu_info->mutex);
		}
		else {
			unsigned long flags;
			gx_mutex_unlock(&cygnusvpu_info->mutex);
			VPU_SPIN_LOCK();
			vpu_all_layers_en(0);
			gx_video_ppclose_require(0);
			VPU_SPIN_UNLOCK();
			while(!cygnus_vpu_check_layers_close()) gx_msleep(10);
		}
		return 0;
	}

	return -1;
}

int cygnus_vpu_DACEnable(GxVpuDacID dac_id, int enable)
{
	return vpu_dac_en(dac_id, enable);

}

int cygnus_vpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source)
{
	/*### 确认该函数是否是选择hd_mixer/sd_mixer, 是否也是如下逻辑*/
#if 0
	if(cygnusvpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if(source == VPU_DAC_SRC_VPU || source == VPU_DAC_SRC_SVPU) {
			if(source == VPU_DAC_SRC_VPU) {
				if(dac_id&VPU_DAC_0) {
					REG_CLR_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 0);
					gxav_clock_set_video_dac_clock_source(0, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_1) {
					REG_CLR_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 1);
					gxav_clock_set_video_dac_clock_source(1, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_2) {
					REG_CLR_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 2);
					gxav_clock_set_video_dac_clock_source(2, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_3) {
					REG_CLR_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 3);
					gxav_clock_set_video_dac_clock_source(3, VIDEO_DAC_SRC_VPU);
				}
			} else {
				if(dac_id&VPU_DAC_0) {
					REG_SET_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 0);
					gxav_clock_set_video_dac_clock_source(0, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_1) {
					REG_SET_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 1);
					gxav_clock_set_video_dac_clock_source(1, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_2) {
					REG_SET_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 2);
					gxav_clock_set_video_dac_clock_source(2, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_3) {
					REG_SET_BIT(&cygnusvpu_reg->rPOWER_DOWN_BYSELF, 3);
					gxav_clock_set_video_dac_clock_source(3, VIDEO_DAC_SRC_SVPU);
				}
			}
			return 0;
		}
	}
#endif

	return -1;
}

int cygnus_vpu_DACPower(GxVpuDacID dac_id)
{
	int ret = 0;
#if 0
	if (cygnusvpu_reg)
		gx_printf("\n%s:%d, reg = %p, dac_id = %d, dac-reg = 0x%x\n", __func__, __LINE__, cygnusvpu_reg, dac_id, cygnusvpu_reg->rPOWER_DOWN);
	else
		gx_printf("\n%s:%d, reg = %p, dac_id = %d\n", __func__, __LINE__, cygnusvpu_reg, dac_id);
#endif

	return vpu_dac_status(dac_id);
}

static int cygnus_vpu_disp_get_buff_id(void)
{
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	return REG_GET_FIELD(&(reg->rPP_DISP_R_PTR), m_DISP_R_PTR, b_DISP_R_PTR);
}

static int cygnus_vpu_disp_set_rst(void)
{
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);
	REG_SET_FIELD(&(reg->rPP_DISP_CTL), m_DISP_RST, 1, b_DISP_RST);
	return 0;
}

static int cygnus_vpu_disp_clr_rst(void)
{
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);
	REG_SET_FIELD(&(reg->rPP_DISP_CTL), m_DISP_RST, 0, b_DISP_RST);
	return 0;
}

static int cygnus_vpu_disp_get_view_active_cnt(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return REG_GET_FIELD(&(reg->rSYS_STATUS), m_ACTIVE_LINE_COUNTER, b_ACTIVE_LINE_COUNTER);
}


static int cygnus_vpu_disp_clr_field_start_int(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return vpu_int_clr(VPU_INT_FIELD_START);
}

static int cygnus_vpu_disp_field_start_int_en(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return vpu_int_en(VPU_INT_FIELD_START);
}

static int field_int_enabled(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return vpu_int_en_status(VPU_INT_FIELD_START);
}

static int cygnus_vpu_disp_field_start_int_dis(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return vpu_int_dis(VPU_INT_FIELD_START);
}

static int cygnus_vpu_disp_get_buff_cnt(void)
{
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);

	return REG_GET_FIELD(&(reg->rPP_DISP_CTL), m_DISP_BUFFER_CNT, b_DISP_BUFFER_CNT);
}

static int cygnus_vpu_disp_get_view_field(void)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return REG_GET_BIT(&(reg->rSYS_STATUS), b_TOP_FIELD);
}

static int cygnus_vpu_disp_get_display_buf(int i)
{
	struct cygnus_vpu_pp_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_PP0);
	return (int)reg->rDISP_CTRL[i];
}

extern int cygnus_spp_enable(int enable);
extern int cygnus_vpp_enable(int enable);
static int cygnus_vpu_disp_layer_enable(GxLayerID layer, int enable)
{
	if (cygnusvpu_info == NULL)
		return 0;

	if (!cygnusvpu_info->vout_is_ready) {
		cygnusvpu_info->layer[layer].enable = enable;
		return 0;
	}

	switch (layer)
	{
	case GX_LAYER_VPP:
		cygnus_vpp_enable(enable);
		break;
	case GX_LAYER_SPP:
		cygnus_spp_enable(enable);
		break;
	default:
		return -1;
	}
	return 0;
}


static int cygnus_vpu_vpp_set_stream_ratio(unsigned int ratio)
{
	return cygnus_vpp_set_stream_ratio(ratio);
}

static int cygnus_vpu_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	return cygnus_vpp_get_actual_viewrect(view_rect);
}

static int cygnus_vpu_regist_surface(GxVpuProperty_RegistSurface *param)
{
	unsigned long flags;
	CygnusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

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

static int cygnus_vpu_unregist_surface(GxVpuProperty_UnregistSurface *param)
{
	int i, j;
	unsigned long flags;
	CygnusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	VPU_SPIN_LOCK();
	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		for(j = 0; j < GXVPU_MAX_SURFACE_NUM; j++)
			if(param->surfaces[j] == sm->surfaces[i])
				sm->surfaces[i] = NULL;
	}
	VPU_SPIN_UNLOCK();
	return 0;
}

static int cygnus_vpu_get_idle_surface(GxVpuProperty_GetIdleSurface *param)
{
	int idle;
	unsigned long flags;
	CygnusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

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

static int cygnus_vpu_flip_surface(GxVpuProperty_FlipSurface *param)
{
	unsigned long flags;
	CygnusSurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	if(param && param->surface) {
		VPU_SPIN_LOCK();
		((CygnusVpuSurface*)(param->surface))->dirty = 1;
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
		cygnus_vpu_disp_field_start_int_en();
		sm->interrupt_en = 1;
	}
	return 0;
}

static int cygnus_vpu_pan_display(GxLayerID layer_id, void *buffer)
{
	CygnusVpuLayer *layer;
	GXAV_ASSERT(buffer != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	layer = &cygnusvpu_info->layer[layer_id];
	if(layer->ops->set_main_surface != NULL) {
		return layer->ops->set_pan_display(buffer);
	}

	return -1;
}

static int cygnus_vpu_isr_callback(void *arg)
{
	int ret = 0;
	int ready;
	GxVpuProperty_LayerMainSurface main_surface;
	CygnusSurfaceManager *sm;
	int i, layers[] = {GX_LAYER_OSD, GX_LAYER_SPP};
	unsigned long flags;

	if(VPU_CLOSED()){
		cygnus_vpu_disp_clr_field_start_int();
		cygnus_vpu_disp_field_start_int_dis();
		return -1;
	}

	VPU_SPIN_LOCK();
	if (cygnusvpu_info->layer[GX_LAYER_VPP].flip_require == 1) {
		CygnusVpuLayer *layer = &cygnusvpu_info->layer[GX_LAYER_VPP];
		layer->ops->set_main_surface(layer->surface);
		cygnusvpu_info->layer[GX_LAYER_VPP].flip_require = 0;
		ret = 1;
	}
	for(i = 0; i < sizeof(layers)/sizeof(int); i++) {
		if((sm = GET_SURFACE_MANAGER(layers[i])) != NULL) {
			cygnus_vpu_disp_clr_field_start_int();
			ready = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
			if(sm->surfaces[ready] && sm->surfaces[ready]->dirty) {
				main_surface.layer   = layers[i];
				main_surface.surface = sm->surfaces[ready];
				cygnus_vpu_SetLayerMainSurface(&main_surface);
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

static int cygnus_vpu_add_region(GxVpuProperty_AddRegion *param)
{
	int ret = -1;
	CygnusVpuLayerOps *osd_ops = cygnusvpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->add_region) {
		ret = osd_ops->add_region(param->surface, param->pos_x, param->pos_y);
	}

	return ret;
}

static int cygnus_vpu_remove_region(GxVpuProperty_RemoveRegion *param)
{
	int ret = -1;
	CygnusVpuLayerOps *osd_ops = cygnusvpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->remove_region) {
		ret = osd_ops->remove_region(param->surface);
	}

	return ret;
}

static int cygnus_vpu_update_region(GxVpuProperty_UpdateRegion *param)
{
	int ret = -1;
	CygnusVpuLayerOps *osd_ops = cygnusvpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->update_region) {
		ret = osd_ops->update_region(param->surface, param->pos_x, param->pos_y);
	}

	return ret;
}

int cygnus_vpu_reset(void)
{
	gx_interrupt_mask(VPU_INT_SRC);
	gx_mdelay(30);
	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	gxav_clock_hot_rst_clr(MODULE_TYPE_VPU);
	cygnusvpu_info->reset_flag++;
	//gx_printf("\n----VPU RESET----flag = %d\n", cygnusvpu_info->reset_flag);
	gx_interrupt_unmask(VPU_INT_SRC);

	return 0;
}

int cygnus_vpu_set_mixer_layers(GxVpuProperty_MixerLayers *property)
{
	return vpu_mixer_set_layers(property);
}

int cygnus_vpu_get_mixer_layers(GxVpuProperty_MixerLayers *property)
{
	return vpu_mixer_get_layers(property);
}

extern int vpp2_set_backcolor(GxColor color);
int cygnus_vpu_set_mixer_backcolor(GxVpuProperty_MixerBackcolor *property)
{
	int ret = 0;
	if (property && property->id == CE_MIXER)
		ret = vpp2_set_backcolor(property->color);
	else
		ret = vpu_mixer_set_backcolor(property->id, property->color);

	return ret;
}

static struct vpu_ops cygnus_vpu_ops = {
	.init                     = cygnus_vpu_init                      ,
	.cleanup                  = cygnus_vpu_cleanup                   ,
	.open                     = cygnus_vpu_open                      ,
	.close                    = cygnus_vpu_close                     ,
	.reset                    = cygnus_vpu_reset                     ,

	.WPalette                 = cygnus_vpu_WPalette                  ,
	.DestroyPalette           = cygnus_vpu_DestroyPalette            ,
	.SurfaceBindPalette       = cygnus_vpu_SurfaceBindPalette        ,
	.SetLayerViewport         = cygnus_vpu_SetLayerViewport          ,
	.SetLayerMainSurface      = cygnus_vpu_SetLayerMainSurface       ,
	.UnsetLayerMainSurface    = cygnus_vpu_UnsetLayerMainSurface     ,
	.SetLayerEnable           = cygnus_vpu_SetLayerEnable            ,
	.SetLayerAntiFlicker      = cygnus_vpu_SetLayerAntiFlicker       ,
	.SetLayerOnTop            = cygnus_vpu_SetLayerOnTop             ,
	.SetLayerVideoMode        = cygnus_vpu_SetLayerVideoMode         ,
	//.SetLayerMixConfig        = cygnus_vpu_SetLayerMixConfig         ,
	.SetDestroySurface        = cygnus_vpu_SetDestroySurface         ,
	.ModifySurface            = cygnus_vpu_ModifySurface             ,
	.SetPalette               = cygnus_vpu_SetPalette                ,
	.SetAlpha                 = cygnus_vpu_SetAlpha                  ,
	.SetColorFormat           = cygnus_vpu_SetColorFormat            ,
	.SetColorKey              = cygnus_vpu_SetColorKey               ,
	.SetBackColor             = cygnus_vpu_SetBackColor              ,
	.SetPoint                 = cygnus_vpu_SetPoint                  ,
	.SetDrawLine              = cygnus_vpu_SetDrawLine               ,
	.SetConvertColor          = cygnus_vpu_SetConvertColor           ,
	.SetVirtualResolution     = cygnus_vpu_SetVirtualResolution      ,
	.SetVbiEnable             = cygnus_vpu_SetVbiEnable              ,
	.SetAspectRatio           = cygnus_vpu_SetAspectRatio            ,
	.SetTvScreen              = cygnus_vpu_SetTvScreen               ,
	.SetLayerEnablePatch      = cygnus_vpu_SetLayerEnablePatch       ,
	.GetEntries               = cygnus_vpu_GetEntries                ,
	.RPalette                 = cygnus_vpu_RPalette                  ,
	.GetCreatePalette         = cygnus_vpu_GetCreatePalette          ,
	.GetLayerViewport         = cygnus_vpu_GetLayerViewport          ,
	.GetLayerClipport         = cygnus_vpu_GetLayerClipport          ,
	.GetLayerMainSurface      = cygnus_vpu_GetLayerMainSurface       ,
	.GetLayerEnable           = cygnus_vpu_GetLayerEnable            ,
	.GetLayerAntiFlicker      = cygnus_vpu_GetLayerAntiFlicker       ,
	.GetLayerOnTop            = cygnus_vpu_GetLayerOnTop             ,
	.GetLayerVideoMode        = cygnus_vpu_GetLayerVideoMode         ,
	.GetLayerCapture          = cygnus_vpu_GetLayerCapture           ,
	.GetCreateSurface         = cygnus_vpu_GetCreateSurface          ,
	.GetReadSurface           = cygnus_vpu_GetReadSurface            ,
	.GetPalette               = cygnus_vpu_GetPalette                ,
	.GetAlpha                 = cygnus_vpu_GetAlpha                  ,
	.GetColorFormat           = cygnus_vpu_GetColorFormat            ,
	.GetColorKey              = cygnus_vpu_GetColorKey               ,
	.GetBackColor             = cygnus_vpu_GetBackColor              ,
	.GetPoint                 = cygnus_vpu_GetPoint                  ,
	.GetConvertColor          = cygnus_vpu_GetConvertColor           ,
	.GetVirtualResolution     = cygnus_vpu_GetVirtualResolution      ,
	.GetVbiEnable             = cygnus_vpu_GetVbiEnable              ,
	.GetVbiCreateBuffer       = cygnus_vpu_GetVbiCreateBuffer        ,
	.GetVbiReadAddress        = cygnus_vpu_GetVbiReadAddress         ,
	.SetLayerClipport         = cygnus_vpu_SetLayerClipport          ,
	.SetBlit                  = cygnus_vpu_SetBlit                   ,
	.SetDfbBlit               = cygnus_vpu_SetDfbBlit                ,
	.SetBatchDfbBlit          = cygnus_vpu_SetBatchDfbBlit           ,
	.SetFillRect              = cygnus_vpu_SetFillRect               ,
	.SetFillPolygon           = cygnus_vpu_SetFillPolygon            ,
	.SetGAMode                = cygnus_vpu_SetSetMode                ,
	.SetWaitUpdate            = cygnus_vpu_SetWaitUpdate             ,
	.SetBeginUpdate           = cygnus_vpu_SetBeginUpdate            ,
	.SetEndUpdate             = cygnus_vpu_SetEndUpdate              ,
	.ZoomSurface              = cygnus_vpu_ZoomSurface               ,
	.Complet                  = cygnus_vpu_Complet                   ,
	.TurnSurface              = cygnus_vpu_TurnSurface               ,

	.Ga_Interrupt             = cygnus_vpu_Ga_Interrupt              ,
	.SetVoutIsready           = cygnus_vpu_SetVoutIsready            ,
	.DACEnable                = cygnus_vpu_DACEnable                 ,
	.DACSource                = cygnus_vpu_DACSource                 ,
	.DACPower                 = cygnus_vpu_DACPower                  ,

	.disp_get_buff_id         = cygnus_vpu_disp_get_buff_id          ,
	.disp_set_rst             = cygnus_vpu_disp_set_rst              ,
	.disp_clr_rst             = cygnus_vpu_disp_clr_rst              ,
	.disp_get_view_active_cnt = cygnus_vpu_disp_get_view_active_cnt  ,
	.disp_clr_field_start_int = cygnus_vpu_disp_clr_field_start_int  ,
	.disp_field_start_int_en  = cygnus_vpu_disp_field_start_int_en   ,
	.disp_field_start_int_dis = cygnus_vpu_disp_field_start_int_dis  ,
	.disp_get_buff_cnt        = cygnus_vpu_disp_get_buff_cnt         ,
	.disp_get_view_field      = cygnus_vpu_disp_get_view_field       ,
	.disp_get_display_buf     = cygnus_vpu_disp_get_display_buf      ,
	.disp_layer_enable        = cygnus_vpu_disp_layer_enable         ,

	.vpp_set_stream_ratio     = cygnus_vpu_vpp_set_stream_ratio      ,
	.vpp_get_actual_viewrect  = cygnus_vpu_vpp_get_actual_viewrect   ,
	.vpp_play_frame           = cygnus_vpp_play_frame                ,

	.RegistSurface   = cygnus_vpu_regist_surface,
	.UnregistSurface = cygnus_vpu_unregist_surface,
	.GetIdleSurface  = cygnus_vpu_get_idle_surface,
	.FlipSurface     = cygnus_vpu_flip_surface,
	.PanDisplay	 = cygnus_vpu_pan_display,
	.VpuIsrCallback  = cygnus_vpu_isr_callback,

	.AddRegion    = cygnus_vpu_add_region,
	.RemoveRegion = cygnus_vpu_remove_region,
	.UpdateRegion = cygnus_vpu_update_region,

	.vpu_get_scan_line = cygnus_vpu_get_scan_line,
	.SetLayerAutoZoom = cygnus_vpu_SetLayerAutoZoom                   ,
	.GetLayerAutoZoom = cygnus_vpu_GetLayerAutoZoom                   ,

	.SetMixerLayers    = cygnus_vpu_set_mixer_layers,
	.GetMixerLayers    = cygnus_vpu_get_mixer_layers,
	.SetMixerBackcolor = cygnus_vpu_set_mixer_backcolor,

	.svpu_config     = cygnus_svpu_config,
//	.svpu_config_buf = cygnus_svpu_config_buf,
//	.svpu_get_buf    = cygnus_svpu_get_buf,
	.svpu_run        = cygnus_svpu_run,
	.svpu_stop       = cygnus_svpu_stop,


};

struct gxav_module_ops cygnus_vpu_module;
static int gx_vpu_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	VPU_REG_BASE_ADDR = res->regs[0];

	VPU_INT_SRC = res->irqs[1];
	cygnus_vpu_module.interrupts[res->irqs[0]] = gx_vpu_ga_interrupt;

	return (0);
}

struct gxav_module_ops cygnus_vpu_module = {
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
	.priv = &cygnus_vpu_ops,
};

