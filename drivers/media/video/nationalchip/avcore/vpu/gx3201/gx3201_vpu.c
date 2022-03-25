#include "kernelcalls.h"
#include "porting.h"
#include "gxav_bitops.h"
#include "gx3201_vpu.h"
#include "gx3201_vout.h"
#include "gx3201_ga.h"
#include "gx3201_vpu_internel.h"
#include "kernelcalls.h"
#include "clock_hal.h"
#include "vout_hal.h"
#include "vpu_hal.h"
#include "video.h"
#include "video_display.h"
#include "vpu_module.h"
#include "vpu_color.h"

Gx3201Vpu *gx3201vpu_info = NULL;
volatile Gx3201VpuReg *gx3201vpu_reg  = NULL;
static volatile Gx3201SvpuReg *gx3201svpu_reg = NULL;

static unsigned int gx3201_osd_filter_table[OSD_FLICKER_TABLE_LEN] = {
	0x808000,0x7e8200,0x7c8400,0x7a8600,
	0x788800,0x768a00,0x748c00,0x728e00,
	0x709000,0x6e9200,0x6c9400,0x6a9600,
	0x689800,0x669a00,0x649c00,0x629e00,
	0x60a000,0x5ea200,0x5ca400,0x5aa600,
	0x58a800,0x56aa00,0x54ac00,0x52ae00,
	0x50b000,0x4eb200,0x4cb400,0x4ab600,
	0x48b800,0x46ba00,0x44bc00,0x42be00,
	0x40c000,0x3ec200,0x3cc400,0x3ac600,
	0x38c800,0x36ca00,0x34cc00,0x32ce00,
	0x30d000,0x2ed200,0x2cd400,0x2ad600,
	0x28d800,0x26da00,0x24dc00,0x22de00,
	0x20e000,0x1ee200,0x1ce400,0x1ae600,
	0x18e800,0x16ea00,0x14ec00,0x12ee00,
	0x10f000,0x0ef200,0x0cf400,0x0af600,
	0x08f800,0x06fa00,0x04fc00,0x02fe00,
};

extern Gx3201VpuLayerOps gx3201_osd_ops;
extern Gx3201VpuLayerOps gx3201_vpp_ops;
extern Gx3201VpuLayerOps gx3201_spp_ops;
extern Gx3201VpuLayerOps gx3201_bkg_ops;

#define M_DIV_N(m, n) \
	((m)%(n)==0 ? (m)/(n) : (m)/(n)+1)


#define MAX_TMP_SURFACE_NUM (10)

#define VPU_CLOSED() (gx3201vpu_info == NULL)

static int remove_surface_from_list(Gx3201VpuSurface *surface)
{
	Gx3201VpuSurface *now   = gx3201vpu_info->surface;
	Gx3201VpuSurface *prev  = gx3201vpu_info->surface;

	if(surface == NULL)
		return -1;

	while ((now) && (now != surface)) {
		prev = now;
		now = now->next;
	}
	if(now == NULL)
		return -1;
	prev->next = now->next;

	if(now->palette)
		gx_free(now->palette);
	if(!now->prealloced && now->buffer) {
		gx_page_free(now->buffer, now->width * now->height * gx_color_get_bpp(now->color_format) >> 3);
	}
	if(now == gx3201vpu_info->surface)
		gx3201vpu_info->surface = prev->next ;
	gx_free(now);

	return 0;
}

int gx3201_vpu_GetVirtualResolution(GxVpuProperty_Resolution *property)
{
	if(gx3201vpu_info != NULL){
		property->xres = gx3201vpu_info->virtual_resolution.xres;
		property->yres = gx3201vpu_info->virtual_resolution.yres;
	}
	else{
		property->xres = 1280;
		property->yres = 720;
	}

	return 0;
}

int gx3201_vpu_GetActualResolution(GxVpuProperty_Resolution *property)
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
	case GXAV_VOUT_PAL_M:
		actual_resolution.xres = 720;
		actual_resolution.yres = 480;
		break;
	case GXAV_VOUT_PAL:
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

int gx3201_vpu_VirtualToActual(int virtual, int referance, int horizontal)
{
	GxVpuProperty_Resolution virtual_resolution;
	gx3201_vpu_GetVirtualResolution(&virtual_resolution);

	if(horizontal)
		return virtual*referance/virtual_resolution.xres;
	else
		return virtual*referance/virtual_resolution.yres;
}

int gx3201_vpu_GetCreateSurface(GxVpuProperty_CreateSurface *property)
{
	unsigned int bpp;
	Gx3201VpuSurface *surface = NULL;
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

	surface = gx_malloc(sizeof(Gx3201VpuSurface));
	if(surface == NULL) {
		VPU_PRINTF("[vpu] GetCreateSurface error:[gx_malloc surface return 0] !\n");
		return -1;
	}
	gx_memset(surface, 0, sizeof(Gx3201VpuSurface));

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
				gx_free(surface);
				return -1;
			}

			property->buffer = gx_page_malloc(size);
			if(property->buffer == NULL) {
				gx_free(surface);
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
	surface->next = gx3201vpu_info->surface;
	gx3201vpu_info->surface = surface;

	return 0;
}

int gx3201_vpu_SetDestroySurface(GxVpuProperty_DestroySurface *property)
{
	return remove_surface_from_list(property->surface);
}

void Gx3201Vpu_SetPoint(GxVpuProperty_Point *point)
{
	unsigned int addr;
	Gx3201VpuSurface *surface = (Gx3201VpuSurface*)point->surface;

	addr = (unsigned int)surface->buffer +
		((gx_color_get_bpp(surface->color_format) * (point->point.y * surface->width + point->point.x)) >> 3);

	if(IS_INVAILD_POINT(&point->point, surface->width, surface->height))
		return;

	gx_color_pixelcolor_write((void*)addr, surface->color_format, &(point->color));
}

int Gx3201Vpu_GetPoint(GxVpuProperty_Point *point)
{
	void *addr;
	unsigned int value;
	unsigned char byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
	Gx3201VpuSurface *surface = (Gx3201VpuSurface*)point->surface;

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

int Gx3201Vpu_DrawLine(GxVpuProperty_DrawLine *line)
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
				Gx3201Vpu_SetPoint(&point);
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
				Gx3201Vpu_SetPoint(&point);
			}
			width--;
		}
	}

	return 0;
}

static int gx3201ce_layerid_get(GxLayerID gx_layer_id)
{
	enum {
		CE_LAYER_MIX = 0,
		CE_LAYER_VPP = 1,
		CE_LAYER_SPP = 3,
		CE_LAYER_OSD = 2,
	};
	switch (gx_layer_id) {
	case GX_LAYER_OSD:
		return CE_LAYER_OSD;
	case GX_LAYER_SPP:
		return CE_LAYER_SPP;
	case GX_LAYER_VPP:
		return CE_LAYER_VPP;
	case GX_LAYER_MIX_ALL:
		return CE_LAYER_MIX;
	default:
		return -1;
	}
}

int Gx3201Vpu_Capture(GxVpuProperty_LayerCapture *capture)
{
	int i, ce_layer_id;
	Gx3201VpuCe   ce;
	unsigned char *src ,*dst;
	unsigned int  width ,line ,request_block = 0;

	ce.layer    = capture->layer;
	ce.left     = capture->rect.x;
	ce.right    = capture->rect.x + capture->rect.width - 1;
	ce.top      = capture->rect.y;
	ce.bottom   = capture->rect.y + capture->rect.height - 1;
	ce.buffer   = ((Gx3201VpuSurface *) capture->surface)->buffer;
	ce_layer_id = gx3201ce_layerid_get(capture->layer);

	request_block = (ce.right - ce.left)/2/128*128;
	if(request_block < 128) {
		request_block = 128;
	}
	else if(request_block > 896) {
		request_block = 896;
	}

	REG_SET_FIELD(&(gx3201vpu_reg->rBUFF_CTRL1),0x7FF<<0,request_block,0);
	VPU_CAP_SET_LEVEL(gx3201vpu_reg->rCAP_CTRL, ce_layer_id);
	VPU_CAP_SET_PIC_ADDR(gx3201vpu_reg->rCAP_ADDR, (unsigned int)gx_virt_to_phys((unsigned int)ce.buffer));
	VPU_CAP_SET_PIC_HORIZONTAL(gx3201vpu_reg->rCAP_WIDTH, ce.left, ce.right);
	VPU_CAP_SET_PIC_VERTICAL(gx3201vpu_reg->rCAP_HEIGHT, ce.top, ce.bottom);
	VPU_CAP_START(gx3201vpu_reg->rCAP_CTRL);
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

extern int Gx3201Vpu_Zoom(GxLayerID layer);
int gx3201_vpu_SetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	Gx3201VpuLayer *layer = NULL;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	gx3201_vpu_GetVirtualResolution(&virtual_resolution);
	if((IS_INVAILD_LAYER(property->layer))
			|| (IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
			|| (IS_NULL(gx3201vpu_info->layer[property->layer].surface)))
		return -1;

	layer = &gx3201vpu_info->layer[property->layer];

	layer->view_rect = property->rect;
	if(property->layer == GX_LAYER_VPP
			&& (gx3201vpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		gx_video_zoom_require(0);
	}
	else if(property->layer == GX_LAYER_SPP
			&& (gx3201vpu_info->layer[GX_LAYER_SPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		gx_video_zoom_require(1);
	}
	else {
		gx_mutex_lock(&gx3201vpu_info->mutex);
		if(gx3201vpu_info->vout_is_ready)
			Gx3201Vpu_Zoom(property->layer);
		gx_mutex_unlock(&gx3201vpu_info->mutex);
	}

	return 0;
}

int gx3201_vpu_SetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	Gx3201VpuLayer *layer;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	gx3201_vpu_GetVirtualResolution(&virtual_resolution);
	if( (IS_INVAILD_LAYER(property->layer))||
			(IS_NULL(gx3201vpu_info))||
			(IS_NULL(gx3201vpu_info->layer[property->layer].surface)))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
		return -1;

	layer = &gx3201vpu_info->layer[property->layer];
	layer->clip_rect.x      = property->rect.x;
	layer->clip_rect.y      = property->rect.y;
	layer->clip_rect.width  = property->rect.width;
	layer->clip_rect.height = property->rect.height;

	if(property->layer == GX_LAYER_VPP &&
			(gx3201vpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO){
		return gx_video_zoom_require(0);
	}
	else if(property->layer == GX_LAYER_OSD) {
		;
	}

	return 0;
}

int gx3201_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	Gx3201VpuLayer *layer;
	Gx3201VpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	surface = (Gx3201VpuSurface*)property->surface;
	layer = &gx3201vpu_info->layer[property->layer];
	if(layer->ops->set_main_surface != NULL) {
		if((surface->width * gx_color_get_bpp(surface->color_format) >> 3) % 8 != 0)
			return -1;
		layer->surface = (Gx3201VpuSurface *) property->surface;
		layer->surface->layer = layer;
		layer->clip_rect.x = 0 ;
		layer->clip_rect.y = 0 ;
		layer->clip_rect.width  = layer->surface->width;
		layer->clip_rect.height = layer->surface->height;
		return layer->ops->set_main_surface(layer->surface);
	}

	return -1;
}

int gx3201_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	Gx3201VpuLayer *layer;
	Gx3201VpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	surface = (Gx3201VpuSurface*)property->surface;
	layer   = &gx3201vpu_info->layer[property->layer];
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

#define RUN_LAYER_FUNC(layer_id, func, attr,value) { \
	Gx3201VpuLayerOps *layer_ops;    \
	GXAV_ASSERT(property != NULL);   \
	if (IS_INVAILD_LAYER(layer_id) || gx3201vpu_info == NULL)  \
	return -1;               \
	layer_ops = gx3201vpu_info->layer[layer_id].ops;      \
	if (layer_ops->func != NULL) {                        \
		gx3201vpu_info->layer[layer_id].attr = value; \
		return layer_ops->func(value);                \
	}                                                     \
	return -1;                                            \
}

static int gx3201Vpu_LayerEnable(GxLayerID layer, int enable)
{
	if (IS_INVAILD_LAYER(layer))
		return -1;
	if (gx3201vpu_info->layer[layer].ops &&
		gx3201vpu_info->layer[layer].ops->set_enable)
		gx3201vpu_info->layer[layer].ops->set_enable(enable);
	return 0;
}

int gx3201_vpu_SetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GXAV_ASSERT(property != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	if (property->enable == 0) {
		gx_mutex_lock(&gx3201vpu_info->mutex);
		if(property->layer == GX_LAYER_VPP) {
			gx_video_ppclose_require(0);
		}
		gx3201Vpu_LayerEnable(property->layer, 0);
		gx3201vpu_info->layer[property->layer].enable = 0;
		gx_mutex_unlock(&gx3201vpu_info->mutex);
	}
	else {
		if (gx3201vpu_info->layer[property->layer].surface == NULL)
			return -1;

		gx_mutex_lock(&gx3201vpu_info->mutex);
		if (gx3201vpu_info->vout_is_ready) {
			if(property->layer == GX_LAYER_VPP) {
				gx_video_ppopen_require(0);
			}
			else {
				gx3201Vpu_LayerEnable(property->layer, 1);
			}
		}
		gx3201vpu_info->layer[property->layer].enable = 1;
		gx_mutex_unlock(&gx3201vpu_info->mutex);
	}


	return 0;
}

int gx3201_vpu_SetLayerEnablePatch(GxVpuProperty_LayerEnable *property)
{
	RUN_LAYER_FUNC(property->layer, set_enable, enable, property->enable);
}

int gx3201_vpu_SetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	RUN_LAYER_FUNC(property->layer, set_anti_flicker, anti_flicker_en, property->enable);
}

int gx3201_vpu_SetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	RUN_LAYER_FUNC(property->layer, set_on_top, on_top, property->enable);
}

int gx3201_vpu_SetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	RUN_LAYER_FUNC(property->layer, set_video_mode, video_mode, property->mode);
}

#define ACTIVE_SURFACE_CHANGE(layer_id, func,value)\
{\
	Gx3201VpuLayerOps *layer_ops;\
	if(IS_INVAILD_LAYER(layer_id) || gx3201vpu_info == NULL)\
	return -1;\
	layer_ops = gx3201vpu_info->layer[layer_id].ops;\
	if(layer_ops->func != NULL)\
	return layer_ops->func(value);\
	return -1;\
}

#define RUN_SURFACE_FUNC(surface_handle, func, attr,value)\
{\
	Gx3201VpuSurface *surface;\
	if(IS_NULL(surface_handle) || gx3201vpu_info == NULL)\
	return -1;\
	surface = (Gx3201VpuSurface *) (surface_handle);\
	surface->attr = value;\
	if(IS_MAIN_SURFACE(surface))\
	ACTIVE_SURFACE_CHANGE(surface->layer->id,func,value);\
	return 0;\
}

int gx3201_vpu_SetPalette(GxVpuProperty_Palette *property)
{
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	if(surface->palette == NULL)
		surface->palette = (GxPalette *) gx_malloc(sizeof(GxPalette));
	if(surface->palette == NULL)
		return -1;

	gx_memcpy(surface->palette, &(property->palette), sizeof(GxPalette));
	if(IS_MAIN_SURFACE(surface)) {
		ACTIVE_SURFACE_CHANGE(surface->layer->id, set_palette, surface->palette);
	}

	return 0;
}

int gx3201_vpu_SetAlpha(GxVpuProperty_Alpha *property)
{
	if(property->alpha.type == GX_ALPHA_GLOBAL && property->alpha.value >=128)
		return -1;
	if(property->alpha.type != GX_ALPHA_GLOBAL && property->alpha.type != GX_ALPHA_PIXEL)
		return -1;

	RUN_SURFACE_FUNC(property->surface, set_alpha, alpha, property->alpha);
}

int gx3201_vpu_SetColorFormat(GxVpuProperty_ColorFormat *property)
{
	if(property->format<GX_COLOR_FMT_CLUT1 || property->format>GX_COLOR_FMT_YCBCR420_UV)
		return -1;
	RUN_SURFACE_FUNC(property->surface, set_color_format, color_format, property->format);
}

int gx3201_vpu_SetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	Gx3201VpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &gx3201vpu_info->layer[property->layer];
		if(layer->ops->set_bg_color != NULL) {
			layer->bg_color = property->color;
			ret = layer->ops->set_bg_color(property->color);
		}
	}

	return ret;
}

int gx3201_vpu_SetColorKey(GxVpuProperty_ColorKey *property)
{
	Gx3201VpuSurface *surface;
	Gx3201VpuLayerOps *layer_ops;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	surface->color_key = property->color;
	surface->color_key_en = property->enable;
	if(IS_MAIN_SURFACE(surface)) {
		if(IS_INVAILD_LAYER(surface->layer->id))
			return -1;
		layer_ops = gx3201vpu_info->layer[surface->layer->id].ops;
		if(layer_ops->set_color_key != NULL) {
			return layer_ops->set_color_key(&surface->color_key, surface->color_key_en);
		}
	}

	return 0;
}

int gx3201_vpu_SetPoint(GxVpuProperty_Point *property)
{
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;
	Gx3201Vpu_SetPoint(property);

	return 0;
}

int gx3201_vpu_SetDrawLine(GxVpuProperty_DrawLine *property)
{
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->start, surface->width, surface->height)
			|| IS_INVAILD_POINT(&property->end, surface->width, surface->height))
		return -1;

	return Gx3201Vpu_DrawLine(property);
}

#define GET_SURFACE(_src_is_ksurface, _src, _dst)\
	do{\
		typeof(_src) src = (_src);\
		typeof(_dst) dst = (_dst);\
		typeof(_src_is_ksurface) src_is_ksurface = (_src_is_ksurface);\
		gx_memset(dst, 0, sizeof(Gx3201VpuSurface));\
		if(src_is_ksurface){\
			gx_memcpy(dst, src, sizeof(Gx3201VpuSurface));\
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

int gx3201_vpu_SetBlit(GxVpuProperty_Blit *property)
{
	int i, ret=-1;
	Gx3201VpuSurface *sura, *surb, *surd;
	static GxVpuProperty_Blit kproperty;
	static Gx3201VpuSurface isrca, isrcb,  idst;

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
	if( IS_NULL(sura->buffer) || IS_INVAILD_GA_COLOR(sura->color_format)||
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

	if( kproperty.srca.rect.width == kproperty.srcb.rect.width &&
			kproperty.srca.rect.height== kproperty.srcb.rect.height) {
		if(GX_COLOR_FMT_YCBCR420_Y_UV == sura->color_format) {
			Gx3201VpuSurface *sur_tmp;
			GxVpuProperty_Blit tmp_property;

			sur_tmp = (Gx3201VpuSurface*)gx_malloc(sizeof(Gx3201VpuSurface)* 3);
			gx_memcpy(&sur_tmp[0], sura, sizeof(Gx3201VpuSurface));
			gx_memcpy(&sur_tmp[1], surb, sizeof(Gx3201VpuSurface));
			gx_memcpy(&sur_tmp[2], surd, sizeof(Gx3201VpuSurface));
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

				ret = gx3201ga_blit(&tmp_property);
			}
			gx_free(sur_tmp);
		}
		else {
			ret = gx3201ga_blit(&kproperty);
		}
	}

	return ret;
}






int gx3201_vpu_SetDfbBlit(GxVpuProperty_DfbBlit *property)
{
	int i, ret=-1;
	Gx3201VpuSurface *sura, *surb, *surd;
	static GxVpuProperty_DfbBlit kproperty;
	static Gx3201VpuSurface isrca, isrcb,  idst;

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
		Gx3201VpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (Gx3201VpuSurface*)gx_malloc(sizeof(Gx3201VpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(Gx3201VpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(Gx3201VpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(Gx3201VpuSurface));
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

			ret = gx3201ga_blit(&tmp_property);
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
				if(gx3201_vpu_GetCreateSurface(&tmp_dst)) {
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
				gx3201ga_dfb_blit(&kproperty);

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
			ret = gx3201ga_dfb_blit(&kproperty);
out:
			/* destroy tmp surfaces */
			for(i = 0; i < tmp_cnt; i++) {
				if(destroy_surface[i].surface != NULL)
					gx3201_vpu_SetDestroySurface(&destroy_surface[i]);
			}
		} else {
			ret = gx3201ga_dfb_blit(&kproperty);
		}
		return ret;
	}
	return ret;
}


int gx3201_vpu_SetFillRect(GxVpuProperty_FillRect *property)
{
	static Gx3201VpuSurface surface;
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

	return gx3201ga_fillrect(&kproperty);
}

int gx3201_vpu_SetFillPolygon(GxVpuProperty_FillPolygon *property)
{
#if 0
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_GA_COLOR(surface->color_format))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, surface->width, surface->height))
		return -1;

	return gx3201ga_fillpolygon(property);
#else
	return 0;
#endif
}

int gx3201_vpu_SetBeginUpdate(GxVpuProperty_BeginUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return gx3201ga_begin(property);
}

int gx3201_vpu_SetEndUpdate(GxVpuProperty_EndUpdate *property)
{
	return gx3201ga_end(property);
}

int gx3201_vpu_SetConvertColor(GxVpuProperty_ConvertColor *property)
{
	GXAV_ASSERT(property != NULL);
	return 0;
}

int gx3201_vpu_SetVirtualResolution(GxVpuProperty_Resolution *property)
{
	GXAV_ASSERT(property != NULL);
	gx3201vpu_info->virtual_resolution.xres = property->xres;
	gx3201vpu_info->virtual_resolution.yres = property->yres;

	return 0;
}

int gx3201_vpu_SetAspectRatio(GxVpuProperty_AspectRatio *property)
{
	GXAV_ASSERT(property != NULL);
	if((property->AspectRatio > VP_16X9)
			|| (property->AspectRatio < VP_UDEF))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	gx3201vpu_info->layer[GX_LAYER_SPP].spec=property->AspectRatio;
	gx3201vpu_info->layer[GX_LAYER_VPP].spec=property->AspectRatio;
	gx_video_zoom_require(0);

	return 0;
}

int gx3201_vpu_SetTvScreen(GxVpuProperty_TvScreen *property)
{
	GxVpuProperty_LayerViewport  vpp_viewport, spp_viewport;

	GXAV_ASSERT(property != NULL);
	if((property->Screen > SCREEN_16X9) || (property->Screen < SCREEN_4X3))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	spp_viewport.layer= GX_LAYER_SPP;
	spp_viewport.rect = gx3201vpu_info->layer[GX_LAYER_SPP].view_rect;
	gx3201vpu_info->layer[GX_LAYER_SPP].screen = property->Screen;
	gx3201_vpu_SetLayerViewport(&spp_viewport);

	vpp_viewport.layer= GX_LAYER_VPP;
	vpp_viewport.rect = gx3201vpu_info->layer[GX_LAYER_VPP].view_rect;
	gx3201vpu_info->layer[GX_LAYER_VPP].screen = property->Screen;
	gx3201_vpu_SetLayerViewport(&vpp_viewport);

	return 0;
}

int gx3201_vpu_SetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	GXAV_ASSERT(property != NULL);
	gx3201vpu_info->vbi.enable = property->enable;
	gx3201_vpu_VbiEnable(&gx3201vpu_info->vbi);

	return 0;
}

#define GET_LAYER_PROPERTY(attr,layer_attr)\
{\
	GXAV_ASSERT(property != NULL);\
	if(IS_INVAILD_LAYER(property->layer))\
	return -1;\
	if( gx3201vpu_info!=NULL ){\
		property->attr  = gx3201vpu_info->layer[property->layer].layer_attr ;\
		return 0;\
	}\
	else\
	return -1; \
}

#define GET_SURFACE_PROPERTY(attr,surface_attr)\
{\
	Gx3201VpuSurface *surface;\
	GXAV_ASSERT(property != NULL);\
	if(IS_NULL(property->surface))\
	return -1;\
	surface = (Gx3201VpuSurface *)(property->surface);\
	property->attr  = surface->surface_attr;\
	return 0;\
}

int gx3201_vpu_GetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	GET_LAYER_PROPERTY(rect, view_rect);
}

int gx3201_vpu_GetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	GET_LAYER_PROPERTY(rect, clip_rect);
}

int gx3201_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	GET_LAYER_PROPERTY(surface, surface);
}

int gx3201_vpu_GetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GET_LAYER_PROPERTY(enable, enable);
}

int gx3201_vpu_GetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	GET_LAYER_PROPERTY(enable, anti_flicker_en);
}

int gx3201_vpu_GetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	GET_LAYER_PROPERTY(enable, on_top);
}

int gx3201_vpu_GetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	GET_LAYER_PROPERTY(mode, video_mode);
}

int gx3201_vpu_GetLayerCapture(GxVpuProperty_LayerCapture *property)
{
#define IS_POINT_IN_RECT(point,rect)\
	((point.x>=rect.x)&&(point.y>=rect.y)&&(point.x<=rect.x+rect.width)&&(point.y<=rect.y+rect.height))

	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution	referance, virtual_resolution;
	Gx3201VpuSurface *surface = (Gx3201VpuSurface*)property->surface;
	GxVpuProperty_LayerCapture  cap_property;
	Gx3201VpuLayer *layer;
	GxLayerID       layer_id;
	GxAvRect        capture_rect = {0};
	GxAvPoint       end_point    = {0};
	GxAvPoint       point_topleft= {0}, point_topright={0}, point_bottomleft={0}, point_bottomright={0};

	GXAV_ASSERT(property != NULL);
	GXAV_ASSERT(property->surface != NULL);

	gx3201_vpu_GetVirtualResolution(&virtual_resolution);
	if(IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
		return -1;

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if((dvemode.mode>=GXAV_VOUT_NULL_MAX)&&(dvemode.mode<GXAV_VOUT_PAL))
		return -1;

	layer_id = property->layer;
	if(layer_id == GX_LAYER_VPP) {
		struct cap_frame frame;
		if(surface->color_format!=GX_COLOR_FMT_YCBCR420 && surface->color_format!=GX_COLOR_FMT_YCBCR420_Y_UV) {
			gx_printf("invalid color format for capture surface!\n");
			return -1;
		}
		frame.buf_addr = (unsigned)surface->buffer;
		frame.buf_size = (surface->width*surface->height*gx_color_get_bpp(GX_COLOR_FMT_YCBCR420))>>3;
		if(gx_video_cap_frame(0, &frame) != 0) {
			gx_printf("invalid width or height for capture surface!\n");
			return -1;
		}
		return 0;
	}
	else {
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

			layer = &(gx3201vpu_info->layer[layer_id]);
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
		else if(layer_id == GX_LAYER_MIX_ALL){
			capture_rect.x = property->rect.x;
			capture_rect.y = property->rect.y;
			capture_rect.width = property->rect.width;
			capture_rect.height = property->rect.height;
		}
		else {
			return -1;
		}

		gx3201_vpu_GetActualResolution(&referance);
		cap_property.layer       =  property->layer;
		cap_property.rect.x      =  gx3201_vpu_VirtualToActual(capture_rect.x, referance.xres, 1);
		cap_property.rect.y      =  gx3201_vpu_VirtualToActual(capture_rect.y, referance.yres, 0);
		cap_property.rect.width  = (gx3201_vpu_VirtualToActual(capture_rect.width,  referance.xres, 1)>>3)<<3;
		cap_property.rect.height =  gx3201_vpu_VirtualToActual(capture_rect.height, referance.yres, 0);
		cap_property.surface = (void*)property->surface;

		return Gx3201Vpu_Capture(&cap_property);
	}
}

int gx3201_vpu_GetCreatePalette(GxVpuProperty_CreatePalette *property)
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

int gx3201_vpu_DestroyPalette(GxVpuProperty_DestroyPalette *property)
{
	if(property->palette!=NULL)
		gx_free(property->palette);
	property->palette = NULL;

	return 0;
}

int gx3201_vpu_SurfaceBindPalette(GxVpuProperty_SurfaceBindPalette *property)
{
	Gx3201VpuSurface* surface = (Gx3201VpuSurface*)(property->surface);;
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
int gx3201_vpu_WPalette(GxVpuProperty_RWPalette *property)
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
	if(u_gx_palette->num_entries <= num_entries)
		gx_copy_from_user(k_gx_palette->entries, u_gx_palette->entries, u_gx_palette->num_entries * sizeof(GxColor));

	return 0;
}

/*from kernel to user*/
int gx3201_vpu_RPalette(GxVpuProperty_RWPalette *property)
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

int gx3201_vpu_GetEntries(GxVpuProperty_GetEntries *property)
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

int gx3201_vpu_GetReadSurface(GxVpuProperty_ReadSurface *property)
{
	Gx3201VpuSurface *surface = NULL;
	Gx3201VpuSurface *now = NULL;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(gx3201vpu_info))
		return -1;
	surface = (Gx3201VpuSurface *) property->surface;
	if(surface == NULL)
		return -1;

	now = gx3201vpu_info->surface;
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

int gx3201_vpu_GetPalette(GxVpuProperty_Palette *property)
{
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;
	surface = (Gx3201VpuSurface *) (property->surface);
	if(surface->palette == NULL)
		return -1;

	gx_memcpy(&property->palette, surface->palette, sizeof(GxPalette));
	return 0;
}

int gx3201_vpu_GetAlpha(GxVpuProperty_Alpha *property)
{
	GET_SURFACE_PROPERTY(alpha, alpha);
}

int gx3201_vpu_GetColorFormat(GxVpuProperty_ColorFormat *property)
{
	GET_SURFACE_PROPERTY(format, color_format);
}

int gx3201_vpu_GetColorKey(GxVpuProperty_ColorKey *property)
{
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	property->color = surface->color_key;
	property->enable= surface->color_key_en;

	return 0;
}

int gx3201_vpu_GetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	Gx3201VpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &gx3201vpu_info->layer[property->layer];
		property->color = layer->bg_color;
		ret = 0;
	}

	return ret;
}

int gx3201_vpu_GetPoint(GxVpuProperty_Point *property)
{
	Gx3201VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3201VpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;

	Gx3201Vpu_GetPoint(property);
	return 0;
}

int gx3201_vpu_GetConvertColor(GxVpuProperty_ConvertColor *property)
{
	return 0;
}

int gx3201_vpu_GetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	property->enable = gx3201vpu_info->vbi.enable;
	return 0;
}

int gx3201_vpu_GetVbiCreateBuffer(GxVpuProperty_VbiCreateBuffer *property)
{
	if(property->unit_num == 0)
		return -1;
	if(property->buffer == NULL) {
		property->buffer = gx_malloc((property->unit_data_len + 1) * 4 *property->unit_num);
		if(property->buffer == NULL)
			return -1;
	}

	gx3201_vpu_VbiCreateBuffer(&gx3201vpu_info->vbi);
	return 0;
}

int gx3201_vpu_GetVbiReadAddress(GxVpuProperty_VbiReadAddress *property)
{
	gx3201_vpu_VbiGetReadPtr(&gx3201vpu_info->vbi);
	property->read_address = gx3201vpu_info->vbi.read_address;
	return 0;
}

int gx3201_vpu_ZoomSurface(GxVpuProperty_ZoomSurface *property)
{
	unsigned int ret=0;
	Gx3201VpuSurface src_surface, dst_surface;
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
			IS_INVAILD_ZOOM_COLOR(src_surface.color_format)||
			IS_INVAILD_ZOOM_COLOR(dst_surface.color_format)||
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
	ret = gx3201ga_zoom(&kproperty);

	return ret ;
}

int gx3201_vpu_TurnSurface(GxVpuProperty_TurnSurface *property)
{
	Gx3201VpuSurface src_surface, dst_surface;
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

	return gx3201ga_turn(&kproperty);
}

int gx3201_vpu_Complet(GxVpuProperty_Complet *property)
{
	GXAV_ASSERT(property != NULL);
	return gx3201ga_complet(property);
}

int gx3201_vpu_Ga_Interrupt(int irq)
{
	if(irq != VPU_GA_INC_SRC) {
		VPU_PRINTF("vpu ga interrup inode error! irq_num=%x\n",irq);
		return 1;
	}

	return gx3201ga_interrupt(irq);
}

int gx3201_vpu_open(void)
{
	unsigned int value;
	Gx3201VpuOsdPriv *osd_priv;
	Gx3201VpuVppPriv *vpp_priv;

	VPU_DBG("[GX3201]:vpu Open start!\n");
	gx3201vpu_info = (Gx3201Vpu *) gx_malloc(sizeof(Gx3201Vpu));
	if(gx3201vpu_info == NULL) {
		VPU_PRINTF("Open Error: gx3201vpu_info MALLOC\n");
		return -1;
	}

	gx_memset(gx3201vpu_info, 0, sizeof(Gx3201Vpu));
	gx3201vpu_info->layer[GX_LAYER_OSD].ops = &gx3201_osd_ops;
	gx3201vpu_info->layer[GX_LAYER_VPP].ops = &gx3201_vpp_ops;
	gx3201vpu_info->layer[GX_LAYER_SPP].ops = &gx3201_spp_ops;
	gx3201vpu_info->layer[GX_LAYER_BKG].ops = &gx3201_bkg_ops;

	gx3201vpu_info->layer[GX_LAYER_OSD].id  = GX_LAYER_OSD;
	gx3201vpu_info->layer[GX_LAYER_VPP].id  = GX_LAYER_VPP;
	gx3201vpu_info->layer[GX_LAYER_SPP].id  = GX_LAYER_SPP;
	gx3201vpu_info->layer[GX_LAYER_BKG].id  = GX_LAYER_BKG;

	gx3201vpu_info->layer[GX_LAYER_VPP].view_rect.x = 0 ;
	gx3201vpu_info->layer[GX_LAYER_VPP].view_rect.y = 0 ;
	gx3201vpu_info->layer[GX_LAYER_VPP].view_rect.width = GX3201_MAX_XRES ;
	gx3201vpu_info->layer[GX_LAYER_VPP].view_rect.height= GX3201_MAX_YRES;
	gx3201vpu_info->layer[GX_LAYER_VPP].clip_rect = gx3201vpu_info->layer[GX_LAYER_VPP].view_rect;

	osd_priv = (Gx3201VpuOsdPriv*)gx_dma_malloc(sizeof(Gx3201VpuOsdPriv));
	if(osd_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)osd_priv, 0, sizeof(Gx3201VpuOsdPriv));
	gx3201vpu_info->layer[GX_LAYER_OSD].priv = osd_priv;

	vpp_priv = (Gx3201VpuVppPriv*)gx_malloc(sizeof(Gx3201VpuVppPriv));
	if(vpp_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)vpp_priv, 0, sizeof(Gx3201VpuVppPriv));
	gx3201vpu_info->layer[GX_LAYER_VPP].priv = vpp_priv;

	value = REG_GET_VAL(&(gx3201vpu_reg->rPP_DISP_CTRL))|(1<<30);
	REG_SET_VAL(&(gx3201vpu_reg->rPP_DISP_CTRL),value);

	{
		clock_params clock;
		clock.vpu.div = 1;
		clock.vpu.clock = 7;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
		clock.vpu.clock = 30;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
	}
	gx3201ga_open();

	gx_mutex_init(&gx3201vpu_info->mutex);

	return 0;
}

int gx3201_vpu_close(void)
{
	Gx3201VpuOsdPriv **osd_priv;
	Gx3201VpuVppPriv **vpp_priv;

	if(gx3201vpu_info) {
		Gx3201VpuSurface *next;
		Gx3201VpuSurface *now = gx3201vpu_info->surface;

		while (now != NULL) {
			next = now->next;
			if(remove_surface_from_list(now) != 0)
				break;
			now = next;
		}

		osd_priv = (Gx3201VpuOsdPriv**)(&(gx3201vpu_info->layer[GX_LAYER_OSD].priv));
		if(*osd_priv) {
			gx_dma_free((unsigned char *)*osd_priv, sizeof(Gx3201VpuOsdPriv));
			*osd_priv = NULL;
		}

		vpp_priv = (Gx3201VpuVppPriv**)(&(gx3201vpu_info->layer[GX_LAYER_VPP].priv));
		if(*vpp_priv) {
			gx_free(*vpp_priv);
			*vpp_priv = NULL;
		}

		gx_mutex_destroy(&gx3201vpu_info->mutex);
		gx_free(gx3201vpu_info);
		gx3201vpu_info = NULL;
	}

	VPU_VPP_DISABLE(gx3201vpu_reg->rPP_CTRL);
	VPU_SPP_DISABLE(gx3201vpu_reg->rPIC_CTRL);
	VPU_OSD_DISABLE(gx3201vpu_reg->rOSD_CTRL);
	VPU_VBI_STOP(gx3201vpu_reg->rVBI_CTRL);
	VPU_DBG("[GX3201]:Close OK!\n");

	return 0;
}

static void gx3201_VpuIOUnmap(void)
{
	if(gx3201vpu_reg) {
		gx_iounmap(gx3201vpu_reg);
		gx_release_mem_region(VPU_REG_BASE_ADDR, sizeof(Gx3201VpuReg));
		gx3201vpu_reg = NULL;
	}

	if(gx3201svpu_reg) {
		gx_iounmap(gx3201svpu_reg);
		gx_release_mem_region(SVPU_REG_BASE_ADDR, sizeof(Gx3201SvpuReg));
		gx3201svpu_reg = NULL;
	}

	return ;
}

static int gx3201_VpuIORemap(void)
{
	if(!gx_request_mem_region(VPU_REG_BASE_ADDR, sizeof(Gx3201VpuReg))) {
		VPU_PRINTF("request_mem_region failed");
		goto VPU_IOMAP_ERROR;
	}
	gx3201vpu_reg = gx_ioremap(VPU_REG_BASE_ADDR, sizeof(Gx3201VpuReg));
	if(!gx3201vpu_reg) {
		goto VPU_IOMAP_ERROR;
	}

	if(!gx_request_mem_region(SVPU_REG_BASE_ADDR, sizeof(Gx3201SvpuReg))) {
		VPU_PRINTF("request_mem_region failed");
		goto VPU_IOMAP_ERROR;
	}
	gx3201svpu_reg = gx_ioremap(SVPU_REG_BASE_ADDR, sizeof(Gx3201SvpuReg));
	if(!gx3201svpu_reg) {
		goto VPU_IOMAP_ERROR;
	}
	return 0;

VPU_IOMAP_ERROR:
	gx3201_VpuIOUnmap( );
	return -1;
}

static int gx3201_vpu_init(void)
{
	unsigned int i;

	if(0 != gx3201_VpuIORemap()) {
		return -1;
	}

	gx3201ga_init();
	//VPU_VPP_DISABLE(gx3201vpu_reg->rPP_CTRL);
	//VPU_SPP_DISABLE(gx3201vpu_reg->rPIC_CTRL);
	//VPU_OSD_DISABLE(gx3201vpu_reg->rOSD_CTRL);
	VPU_VPP_SET_BUFFER_REQ_DATA_LEN(gx3201vpu_reg->rBUFF_CTRL1, 0x100);
	VPU_VPP_SET_ZOOM(gx3201vpu_reg->rPP_ZOOM, 0x1000, 0x1000);
	VPU_VPP_SET_PP_SIZE(gx3201vpu_reg->rPP_VIEW_SIZE, GX3201_MAX_XRES, GX3201_MAX_YRES);
	VPU_VPP_SET_FIELD_MODE_UV_FIELD(gx3201vpu_reg->rPP_CTRL,0x1);
	VPU_VPP_SET_BG_COLOR(gx3201vpu_reg->rPP_BACK_COLOR, 0x10, 0x80, 0x80);
	VPU_SPP_SET_BG_COLOR(gx3201vpu_reg->rPIC_BACK_COLOR, 0x10, 0x80, 0x80);
	for (i = 0; i < OSD_FLICKER_TABLE_LEN; i++)
		VPU_OSD_SET_FLIKER_FLITER(gx3201vpu_reg->rOSD_PARA, i, gx3201_osd_filter_table[i]);
	REG_SET_VAL(&(gx3201svpu_reg->rGAIN_DAC),0xffffffff);

	return 0;
}

static int gx3201_vpu_cleanup(void)
{
	gx3201_vpu_close();
	gx3201_VpuIOUnmap();
	gx3201ga_cleanup();

	return 0;
}

void gx3201_vpu_WaiteIdle(void)
{
	int tmp;
	unsigned int timeout = ~0;
	while(timeout--) {
		tmp = REG_GET_VAL(&(gx3201vpu_reg->rSYS_PARA));
		if((tmp&0x1)==0x1) {
			return;
		}
	}

	VPU_PRINTF("[vpu][fatal error] waite for vpu idle timeout !\n");
}

void gx3201_vpu_GetScanInfo(int *scan_line, int *top)
{
	int tmp;
	tmp = REG_GET_VAL(&(gx3201vpu_reg->rSCAN_LINE));
	*scan_line = tmp & 0x7ff;
	*top       = tmp & 0x800;
}

void Gx3201_disable_svpu(void)
{
	REG_SET_VAL(&(gx3201svpu_reg->rSVPU_CTRL),0);
}

void Gx3201_enable_svpu(int mode)
{
	int top_field = 0;
	int active_line = 0;

	REG_SET_VAL(&(gx3201svpu_reg->rDUMMY),0x00FF0100);
	REG_SET_VAL(&(gx3201svpu_reg->rPOWER_DOWN_BYSELF),1);

	gx3201_videoout_out1_enable(0);

	//gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	//gxav_clock_hot_rst_clear(MODULE_TYPE_VPU);

	REG_SET_BIT(&(gx3201svpu_reg->rZOOM_CTRL),0);
	REG_SET_BIT(&(gx3201svpu_reg->rSVPU_CTRL),1);
	REG_SET_FIELD(&(gx3201svpu_reg->rSVPU_CTRL), 0x0F<<4, mode,4);
	REG_SET_BIT(&(gx3201svpu_reg->rSVPU_CTRL),0);

	while(top_field == 0) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0x800) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0x800) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0x800) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}
	while(top_field == 0x800) {
		gx3201_vpu_GetScanInfo(&active_line,&top_field);
	}

	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	gxav_clock_hot_rst_clr(MODULE_TYPE_VPU);
	gx3201_videoout_out1_enable(1);
}

void gx3201_vpu_SetBufferStateDelay(int v)
{
	VPU_SET_BUFF_STATE_DELAY(gx3201vpu_reg->rBUFF_CTRL2, v);
}

int gx3201vpu_get_actual_viewrect(GxLayerID layer_id, int ratio, GxAvRect rect)
{
	return 0;
}

int gx3201_vpu_DACEnable(GxVpuDacID dac_id, int enable)
{
	if(gx3201svpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if(enable) {
			if(dac_id&VPU_DAC_0)
				VPU_DAC_ENABLE(gx3201svpu_reg->rPOWER_DOWN, 0);
			if(dac_id&VPU_DAC_1)
				VPU_DAC_ENABLE(gx3201svpu_reg->rPOWER_DOWN, 1);
			if(dac_id&VPU_DAC_2)
				VPU_DAC_ENABLE(gx3201svpu_reg->rPOWER_DOWN, 2);
			if(dac_id&VPU_DAC_3)
				VPU_DAC_ENABLE(gx3201svpu_reg->rPOWER_DOWN, 3);
		}
		else {
			if(dac_id&VPU_DAC_0)
				VPU_DAC_DISABLE(gx3201svpu_reg->rPOWER_DOWN, 0);
			if(dac_id&VPU_DAC_1)
				VPU_DAC_DISABLE(gx3201svpu_reg->rPOWER_DOWN, 1);
			if(dac_id&VPU_DAC_2)
				VPU_DAC_DISABLE(gx3201svpu_reg->rPOWER_DOWN, 2);
			if(dac_id&VPU_DAC_3)
				VPU_DAC_DISABLE(gx3201svpu_reg->rPOWER_DOWN, 3);
		}
		return 0;
	}

	return -1;
}

int gx3201_vpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source)
{
	if (gx3201svpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if (source == VPU_DAC_SRC_VPU || source == VPU_DAC_SRC_SVPU) {
			if (source == VPU_DAC_SRC_VPU) {
				if (dac_id&VPU_DAC_0)
					REG_CLR_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 0);
				if (dac_id&VPU_DAC_1)
					REG_CLR_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 1);
				if (dac_id&VPU_DAC_2)
					REG_CLR_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 2);
				if (dac_id&VPU_DAC_3)
					REG_CLR_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 3);
			}
			else {
				if (dac_id&VPU_DAC_0)
					REG_SET_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 0);
				if (dac_id&VPU_DAC_1)
					REG_SET_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 1);
				if (dac_id&VPU_DAC_2)
					REG_SET_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 2);
				if (dac_id&VPU_DAC_3)
					REG_SET_BIT(&gx3201svpu_reg->rPOWER_DOWN_BYSELF, 3);
			}
			return 0;
		}
	}

	return -1;
}

static int gx3201_vpu_vpp_get_base_line(void)
{
	return VPU_VPP_GET_BASE_LINE(gx3201vpu_reg->rPP_FRAME_STRIDE);
}

static int gx3201_vpu_vpp_set_base_line(unsigned int value)
{
	VPU_VPP_SET_BASE_LINE(gx3201vpu_reg->rPP_FRAME_STRIDE, value);
	return 0;
}

static int gx3201_vpu_disp_get_buff_id(void)
{
	return VPU_DISP_GET_BUFF_ID(gx3201vpu_reg->rPP_DISP_R_PTR);
}

static int gx3201_vpu_disp_set_rst(void)
{
	VPU_DISP_SET_RST( gx3201vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int gx3201_vpu_disp_clr_rst(void)
{
	VPU_DISP_CLR_RST( gx3201vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int gx3201_vpu_disp_get_view_active_cnt(void)
{
	return VPU_DISP_GET_VIEW_ACTIVE_CNT(gx3201vpu_reg->rSCAN_LINE);
}

static int gx3201_vpu_disp_clr_field_error_int(void)
{
	VPU_DISP_CLR_FILED_ERROR_INT(gx3201vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3201_vpu_disp_clr_field_start_int(void)
{
	VPU_DISP_CLR_FILED_START_INT(gx3201vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3201_vpu_disp_field_error_int_en(void)
{
	VPU_DISP_FILED_ERROR_INT_EN( gx3201vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int gx3201_vpu_disp_field_start_int_en(void)
{
	VPU_DISP_FILED_START_INT_EN( gx3201vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int field_int_enabled(void)
{
	return VPU_DISP_FILED_START_INT_ENABLED( gx3201vpu_reg->rPP_DISP_CTRL );
}

static int gx3201_vpu_disp_field_error_int_dis(void)
{
	VPU_DISP_FILED_ERROR_INT_DIS(gx3201vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3201_vpu_disp_field_start_int_dis(void)
{
	VPU_DISP_FILED_START_INT_DIS(gx3201vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3201_vpu_disp_get_buff_cnt(void)
{
	return VPU_DISP_GET_BUFF_CNT(gx3201vpu_reg->rPP_DISP_CTRL);
}

static int gx3201_vpu_disp_get_view_field(void)
{
	return VPU_DISP_GET_VIEW_FIELD(gx3201vpu_reg->rSCAN_LINE);
}

static int gx3201_vpu_disp_get_display_buf(int i)
{
	return (int)((unsigned int)gx3201vpu_reg->rDISP0_CTRL + i*(sizeof(DispBuf)));
}

extern int gx3201_spp_enable(int enable);
extern int gx3201_vpp_enable(int enable);
int gx3201_vpu_disp_layer_enable(GxLayerID layer, int enable)
{
	if (gx3201vpu_info == NULL)
		return 0;

	if (!gx3201vpu_info->vout_is_ready) {
		gx3201vpu_info->layer[layer].enable = enable;
		return 0;
	}

	switch (layer)
	{
	case GX_LAYER_VPP:
		gx3201_vpp_enable(enable);
		break;
	case GX_LAYER_SPP:
		gx3201_spp_enable(enable);
		break;
	default:
		return -1;
	}
	return 0;
}


static int gx3201_vpu_vpp_set_stream_ratio(unsigned int ratio)
{
	return gx3201_vpp_set_stream_ratio(ratio);
}

static int gx3201_vpu_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	return gx3201_vpp_get_actual_viewrect(view_rect);
}

static int gx3201_vpu_vpp_get_actual_cliprect(GxAvRect *clip_rect)
{
	return gx3201_vpp_get_actual_cliprect(clip_rect);
}

static int gx3201_vpu_regist_surface(GxVpuProperty_RegistSurface *param)
{
	Gx3201SurfaceManager *sm = SURFACE_MANAGER;

	gx_interrupt_disable();
	sm->showing         = 0;
	sm->cur_time        = 0;
	sm->ready_time      = 0;
	sm->ready           = NULL;
	sm->interrupt_en    = 0;
	sm->force_flip_gate = 1;
	gx_memcpy(sm->surfaces, param->surfaces, sizeof(param->surfaces));
	gx_interrupt_enable();

	return 0;
}

static int gx3201_vpu_unregist_surface(GxVpuProperty_UnregistSurface *param)
{
	int i, j;
	Gx3201SurfaceManager *sm = SURFACE_MANAGER;

	gx_interrupt_disable();
	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		for(j = 0; j < GXVPU_MAX_SURFACE_NUM; j++)
			if(param->surfaces[j] == sm->surfaces[i])
				sm->surfaces[i] = NULL;
	}
	gx_interrupt_enable();
	return 0;
}

static int gx3201_vpu_get_idle_surface(GxVpuProperty_GetIdleSurface *param)
{
	int idle;
	Gx3201SurfaceManager *sm = SURFACE_MANAGER;

	gx_interrupt_disable();
	idle = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
	param->idle_surface = sm->surfaces[idle];
	if(sm->surfaces[idle])
		sm->surfaces[idle]->dirty = 0;
	gx_interrupt_enable();

	return 0;
}

static int gx3201_vpu_flip_surface(GxVpuProperty_FlipSurface *param)
{
	Gx3201SurfaceManager *sm = SURFACE_MANAGER;

	if(param && param->surface) {
		gx_interrupt_disable();
		((Gx3201VpuSurface*)(param->surface))->dirty = 1;
		gx_interrupt_enable();

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
		gx3201_vpu_disp_field_start_int_en();
		gx3201_vpu_disp_field_error_int_en();
		sm->interrupt_en = 1;
	}
	return 0;
}

static int gx3201_vpu_isr_callback(void *arg)
{
	int ready;
	GxVpuProperty_LayerMainSurface main_surface;
	Gx3201SurfaceManager *sm;

	if(VPU_CLOSED()){
		gx3201_vpu_disp_clr_field_error_int();
		gx3201_vpu_disp_clr_field_start_int();
		gx3201_vpu_disp_field_start_int_dis();
		gx3201_vpu_disp_field_error_int_dis();
		return -1;
	}

	sm = SURFACE_MANAGER;
	gx3201_vpu_disp_clr_field_error_int();
	gx3201_vpu_disp_clr_field_start_int();
	ready = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
	if(sm->surfaces[ready] && sm->surfaces[ready]->dirty) {
		main_surface.layer   = GX_LAYER_OSD;
		main_surface.surface = sm->surfaces[ready];
		gx3201_vpu_SetLayerMainSurface(&main_surface);
		sm->surfaces[ready]->dirty = 0;
		sm->surfaces[sm->showing]->dirty = 0;
		sm->showing = ready;
	}
	sm->cur_time++;

	return 0;
}

extern unsigned int gx3201vpu_check_layers_close(void);
static int gx3201_vpu_SetVoutIsready(int ready)
{
	if(gx3201vpu_info != NULL) {
		gx_mutex_lock(&gx3201vpu_info->mutex);
		gx3201vpu_info->vout_is_ready = ready;
		if(ready) {
			gx_video_disp_patch(0);
			//re-zoom all layer
			Gx3201Vpu_Zoom(GX_LAYER_OSD);
			Gx3201Vpu_Zoom(GX_LAYER_SOSD);
			Gx3201Vpu_Zoom(GX_LAYER_SPP);
			gx_video_zoom_require(0);

			//enable on needs
			if (gx3201vpu_info->layer[GX_LAYER_OSD].enable) {
				gx3201Vpu_LayerEnable(GX_LAYER_OSD, 1);
			}
			if (gx3201vpu_info->layer[GX_LAYER_SOSD].enable) {
				gx3201Vpu_LayerEnable(GX_LAYER_SOSD, 1);
			}
			if (gx3201vpu_info->layer[GX_LAYER_SPP].enable) {
				gx3201Vpu_LayerEnable(GX_LAYER_SPP, 1);
			}
			if(gx3201vpu_info->layer[GX_LAYER_VPP].enable) {
				gx_video_ppopen_require(0);
			}
			gx_mutex_unlock(&gx3201vpu_info->mutex);
		}
		else {
			gx_mutex_unlock(&gx3201vpu_info->mutex);

			gx_interrupt_disable();
			gx3201Vpu_LayerEnable(GX_LAYER_OSD, 0);
			gx3201Vpu_LayerEnable(GX_LAYER_SOSD, 0);
			gx3201Vpu_LayerEnable(GX_LAYER_SPP, 0);
			gx3201Vpu_LayerEnable(GX_LAYER_VPP, 0);
			gx_video_ppclose_require(0);
			gx_interrupt_enable();

			while(!gx3201vpu_check_layers_close()) gx_msleep(10);
		}
		return 0;
	}

	return -1;
}

static struct vpu_ops gx3201_vpu_ops = {
	.init                     = gx3201_vpu_init                      ,
	.cleanup                  = gx3201_vpu_cleanup                   ,
	.open                     = gx3201_vpu_open                      ,
	.close                    = gx3201_vpu_close                     ,

	.WPalette                 = gx3201_vpu_WPalette                  ,
	.DestroyPalette           = gx3201_vpu_DestroyPalette            ,
	.SurfaceBindPalette       = gx3201_vpu_SurfaceBindPalette        ,
	.SetLayerViewport         = gx3201_vpu_SetLayerViewport          ,
	.SetLayerMainSurface      = gx3201_vpu_SetLayerMainSurface       ,
	.UnsetLayerMainSurface    = gx3201_vpu_UnsetLayerMainSurface     ,
	.SetLayerEnable           = gx3201_vpu_SetLayerEnable            ,
	.SetLayerAntiFlicker      = gx3201_vpu_SetLayerAntiFlicker       ,
	.SetLayerOnTop            = gx3201_vpu_SetLayerOnTop             ,
	.SetLayerVideoMode        = gx3201_vpu_SetLayerVideoMode         ,
	.SetLayerMixConfig        = NULL                                 ,
	.SetDestroySurface        = gx3201_vpu_SetDestroySurface         ,
	.SetPalette               = gx3201_vpu_SetPalette                ,
	.SetAlpha                 = gx3201_vpu_SetAlpha                  ,
	.SetColorFormat           = gx3201_vpu_SetColorFormat            ,
	.SetColorKey              = gx3201_vpu_SetColorKey               ,
	.SetBackColor             = gx3201_vpu_SetBackColor              ,
	.SetPoint                 = gx3201_vpu_SetPoint                  ,
	.SetDrawLine              = gx3201_vpu_SetDrawLine               ,
	.SetConvertColor          = gx3201_vpu_SetConvertColor           ,
	.SetVirtualResolution     = gx3201_vpu_SetVirtualResolution      ,
	.SetVbiEnable             = gx3201_vpu_SetVbiEnable              ,
	.SetAspectRatio           = gx3201_vpu_SetAspectRatio            ,
	.SetTvScreen              = gx3201_vpu_SetTvScreen               ,
	.SetLayerEnablePatch      = gx3201_vpu_SetLayerEnablePatch       ,
	.GetEntries               = gx3201_vpu_GetEntries                ,
	.RPalette                 = gx3201_vpu_RPalette                  ,
	.GetCreatePalette         = gx3201_vpu_GetCreatePalette          ,
	.GetLayerViewport         = gx3201_vpu_GetLayerViewport          ,
	.GetLayerClipport         = gx3201_vpu_GetLayerClipport          ,
	.GetLayerMainSurface      = gx3201_vpu_GetLayerMainSurface       ,
	.GetLayerEnable           = gx3201_vpu_GetLayerEnable            ,
	.GetLayerAntiFlicker      = gx3201_vpu_GetLayerAntiFlicker       ,
	.GetLayerOnTop            = gx3201_vpu_GetLayerOnTop             ,
	.GetLayerVideoMode        = gx3201_vpu_GetLayerVideoMode         ,
	.GetLayerCapture          = gx3201_vpu_GetLayerCapture           ,
	.GetCreateSurface         = gx3201_vpu_GetCreateSurface          ,
	.GetReadSurface           = gx3201_vpu_GetReadSurface            ,
	.GetPalette               = gx3201_vpu_GetPalette                ,
	.GetAlpha                 = gx3201_vpu_GetAlpha                  ,
	.GetColorFormat           = gx3201_vpu_GetColorFormat            ,
	.GetColorKey              = gx3201_vpu_GetColorKey               ,
	.GetBackColor             = gx3201_vpu_GetBackColor              ,
	.GetPoint                 = gx3201_vpu_GetPoint                  ,
	.GetConvertColor          = gx3201_vpu_GetConvertColor           ,
	.GetVirtualResolution     = gx3201_vpu_GetVirtualResolution      ,
	.GetVbiEnable             = gx3201_vpu_GetVbiEnable              ,
	.GetVbiCreateBuffer       = gx3201_vpu_GetVbiCreateBuffer        ,
	.GetVbiReadAddress        = gx3201_vpu_GetVbiReadAddress         ,
	.SetLayerClipport         = gx3201_vpu_SetLayerClipport          ,
	.SetBlit                  = gx3201_vpu_SetBlit                   ,
	.SetDfbBlit               = gx3201_vpu_SetDfbBlit                ,
	.SetFillRect              = gx3201_vpu_SetFillRect               ,
	.SetFillPolygon           = gx3201_vpu_SetFillPolygon            ,
	.SetBeginUpdate           = gx3201_vpu_SetBeginUpdate            ,
	.SetEndUpdate             = gx3201_vpu_SetEndUpdate              ,
	.ZoomSurface              = gx3201_vpu_ZoomSurface               ,
	.Complet                  = gx3201_vpu_Complet                   ,
	.TurnSurface              = gx3201_vpu_TurnSurface               ,

	.Ga_Interrupt             = gx3201_vpu_Ga_Interrupt              ,
	.SetVoutIsready           = gx3201_vpu_SetVoutIsready            ,
	.DACEnable                = gx3201_vpu_DACEnable                 ,
	.DACSource                = gx3201_vpu_DACSource                 ,

	.vpp_get_base_line        = gx3201_vpu_vpp_get_base_line         ,
	.vpp_set_base_line        = gx3201_vpu_vpp_set_base_line         ,
	.disp_get_buff_id         = gx3201_vpu_disp_get_buff_id          ,
	.disp_set_rst             = gx3201_vpu_disp_set_rst              ,
	.disp_clr_rst             = gx3201_vpu_disp_clr_rst              ,
	.disp_get_view_active_cnt = gx3201_vpu_disp_get_view_active_cnt  ,
	.disp_clr_field_error_int = gx3201_vpu_disp_clr_field_error_int  ,
	.disp_clr_field_start_int = gx3201_vpu_disp_clr_field_start_int  ,
	.disp_field_error_int_en  = gx3201_vpu_disp_field_error_int_en   ,
	.disp_field_start_int_en  = gx3201_vpu_disp_field_start_int_en   ,
	.disp_field_error_int_dis = gx3201_vpu_disp_field_error_int_dis  ,
	.disp_field_start_int_dis = gx3201_vpu_disp_field_start_int_dis  ,
	.disp_get_buff_cnt        = gx3201_vpu_disp_get_buff_cnt         ,
	.disp_get_view_field      = gx3201_vpu_disp_get_view_field       ,
	.disp_get_display_buf     = gx3201_vpu_disp_get_display_buf      ,
	.disp_layer_enable        = gx3201_vpu_disp_layer_enable         ,

	.vpp_set_stream_ratio     = gx3201_vpu_vpp_set_stream_ratio      ,
	.vpp_get_actual_viewrect  = gx3201_vpu_vpp_get_actual_viewrect   ,
	.vpp_get_actual_cliprect  = gx3201_vpu_vpp_get_actual_cliprect   ,

	.RegistSurface   = gx3201_vpu_regist_surface,
	.UnregistSurface = gx3201_vpu_unregist_surface,
	.GetIdleSurface  = gx3201_vpu_get_idle_surface,
	.FlipSurface     = gx3201_vpu_flip_surface,
	.VpuIsrCallback  = gx3201_vpu_isr_callback,
};

struct gxav_module_ops gx3201_vpu_module = {
	.module_type = GXAV_MOD_VPU,
	.count = 1,
	.irqs = {VPU_GA_INC_SRC, -1},
	.irq_flags = {-1},
	.init = gx_vpu_init,
	.cleanup = gx_vpu_cleanup,
	.open = gx_vpu_open,
	.close = gx_vpu_close,
	.set_property = gx_vpu_set_property,
	.get_property = gx_vpu_get_property,
	.interrupts[VPU_GA_INC_SRC] = gx_vpu_ga_interrupt,
	.priv = &gx3201_vpu_ops,
};

