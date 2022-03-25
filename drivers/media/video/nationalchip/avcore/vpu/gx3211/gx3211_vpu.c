#include "kernelcalls.h"
#include "porting.h"
#include "gxav_bitops.h"
#include "gx3211_vpu.h"
#include "vout_hal.h"
#include "vpu_color.h"
#include "gx3211_ga.h"
#include "gx3211_svpu.h"
#include "gx3211_vpu_internel.h"
#include "kernelcalls.h"
#include "clock_hal.h"
#include "video.h"
#include "vpu_module.h"
#include "video_display.h"
#include "gx3211_svpu.h"

Gx3211Vpu *gx3211vpu_info = NULL;
volatile Gx3211VpuReg *gx3211vpu_reg = NULL;

static unsigned int gx3211_osd_filter_table[OSD_FLICKER_TABLE_LEN] = {
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

extern Gx3211VpuLayerOps gx3211_osd_ops;
extern Gx3211VpuLayerOps gx3211_sosd_ops;
extern Gx3211VpuLayerOps gx3211_vpp_ops;
extern Gx3211VpuLayerOps gx3211_spp_ops;
extern Gx3211VpuLayerOps gx3211_bkg_ops;

#define M_DIV_N(m, n) \
	((m)%(n)==0 ? (m)/(n) : (m)/(n)+1)

#define MAX_TMP_SURFACE_NUM (10)
#define VPU_CLOSED() (gx3211vpu_info == NULL)

static int remove_surface_from_list(Gx3211VpuSurface *surface)
{
	Gx3211VpuSurface *now   = gx3211vpu_info->surface;
	Gx3211VpuSurface *prev  = gx3211vpu_info->surface;

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
		if(now->color_format == GX_COLOR_FMT_YCBCR420 ||
		   now->color_format == GX_COLOR_FMT_YCBCR420_Y_UV ||
		   now->color_format == GX_COLOR_FMT_YCBCR420_Y_U_V) {
			gx_page_free(now->buffer, now->width * (now->height + (now->height >> 1)));
		}else {
			gx_page_free(now->buffer, now->width * now->height * gx_color_get_bpp(now->color_format) >> 3);
		}
	}
	if(now == gx3211vpu_info->surface)
		gx3211vpu_info->surface = prev->next ;
	gx_free(now);

	return 0;
}

static int is_in_surface_list(Gx3211VpuSurface *surface)
{
	Gx3211VpuSurface *now   = gx3211vpu_info->surface;

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

int gx3211_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
int gx3211_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
static int modify_surface_from_list(GxVpuProperty_ModifySurface *property)
{
	GxVpuProperty_LayerMainSurface MainSurface = {0};
	Gx3211VpuSurface *surface = NULL;
	int i = 0;

	surface = (Gx3211VpuSurface *)(property->surface);
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
		}

		if(-1 != property->color_format) {
			surface->color_format = property->color_format;
		}
		if(-1 != property->mode) {
			surface->surface_mode = property->mode;
		}
		for(i = 0; i < GX_LAYER_MAX; i++) {
			MainSurface.layer = i;
			gx3211_vpu_GetLayerMainSurface(&MainSurface);
			if(MainSurface.surface == surface) {
				gx3211_vpu_SetLayerMainSurface(&MainSurface);
				break;
			}
		}
		return (true);
	} else {
		return (false);
	}
}

int gx3211_vpu_GetVirtualResolution(GxVpuProperty_Resolution *property)
{
	if(gx3211vpu_info != NULL){
		property->xres = gx3211vpu_info->virtual_resolution.xres;
		property->yres = gx3211vpu_info->virtual_resolution.yres;
	}
	else{
		property->xres = 1280;
		property->yres = 720;
	}

	return 0;
}

int gx3211_vpu_GetActualResolution(GxVpuProperty_Resolution *property)
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

int gx3211_vpu_VirtualToActual(int virtual, int referance, int horizontal)
{
	GxVpuProperty_Resolution virtual_resolution;
	gx3211_vpu_GetVirtualResolution(&virtual_resolution);

	if(horizontal)
		return virtual*referance/virtual_resolution.xres;
	else
		return virtual*referance/virtual_resolution.yres;
}

int gx3211_vpu_ActualToVirtual(int actual, int referance, int horizontal)
{
	Gx3211VpuVppPriv *info;
	GxVpuProperty_Resolution actual_referance;
	if(gx3211vpu_info && gx3211vpu_info->layer[GX_LAYER_VPP].priv) {
		info = gx3211vpu_info->layer[GX_LAYER_VPP].priv;
		actual_referance.xres	= info->frame_width;
		actual_referance.yres   = info->frame_height;

		if(horizontal)
			return (actual*referance + actual_referance.xres -1)/actual_referance.xres;
		else
			return (actual*referance + actual_referance.yres -1)/actual_referance.yres;
	}
	return 0;
}
int gx3211_vpu_GetCreateSurface(GxVpuProperty_CreateSurface *property)
{
	unsigned int bpp;
	Gx3211VpuSurface *surface = NULL;
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

	surface = gx_malloc(sizeof(Gx3211VpuSurface));
	if(surface == NULL) {
		VPU_PRINTF("[vpu] GetCreateSurface error:[gx_malloc surface return 0] !\n");
		return -1;
	}
	gx_memset(surface, 0, sizeof(Gx3211VpuSurface));

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
	surface->next = gx3211vpu_info->surface;
	gx3211vpu_info->surface = surface;

	return 0;
}

int gx3211_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
int gx3211_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property);
static void gx3211_check_layer_surface(Gx3211VpuSurface *surface)
{
	Gx3211SurfaceManager *sm = GET_SURFACE_MANAGER(GX_LAYER_OSD);
	GxVpuProperty_LayerMainSurface main_surface = {0};
	int i = 0;

	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		if(surface == sm->surfaces[i]) {
			sm->surfaces[i] = NULL;
		}
	}

	for(i = GX_LAYER_OSD; i < GX_LAYER_MAX; i++) {
		main_surface.layer = i;
		gx3211_vpu_GetLayerMainSurface(&main_surface);
		if(main_surface.surface == surface) {
			gx3211_vpu_UnsetLayerMainSurface(&main_surface);
		}
	}
}

int gx3211_vpu_SetDestroySurface(GxVpuProperty_DestroySurface *property)
{
	gx3211_check_layer_surface(property->surface);
	return remove_surface_from_list(property->surface);
}

int gx3211_vpu_ModifySurface(GxVpuProperty_ModifySurface *property)
{
	return modify_surface_from_list(property);
}

void Gx3211Vpu_SetPoint(GxVpuProperty_Point *point)
{
	unsigned int addr;
	Gx3211VpuSurface *surface = (Gx3211VpuSurface*)point->surface;

	addr = (unsigned int)surface->buffer +
		((gx_color_get_bpp(surface->color_format) * (point->point.y * surface->width + point->point.x)) >> 3);

	if(IS_INVAILD_POINT(&point->point, surface->width, surface->height))
		return;

	gx_color_pixelcolor_write((void*)addr, surface->color_format, &(point->color));
}

int Gx3211Vpu_GetPoint(GxVpuProperty_Point *point)
{
	void *addr;
	unsigned int value;
	unsigned char byte0 = 0, byte1 = 0, byte2 = 0, byte3 = 0;
	Gx3211VpuSurface *surface = (Gx3211VpuSurface*)point->surface;

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

int Gx3211Vpu_DrawLine(GxVpuProperty_DrawLine *line)
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
				Gx3211Vpu_SetPoint(&point);
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
				Gx3211Vpu_SetPoint(&point);
			}
			width--;
		}
	}

	return 0;
}

enum {
	CE_LAYER_MIX  = 0,
	CE_LAYER_VPP  = 1,
	CE_LAYER_OSD  = 2,
	CE_LAYER_SPP  = 3,
	CE_LAYER_SOSD = 4,
};
static int gx3211ce_layerid_get(GxLayerID gx_layer_id)
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

static void _save_y_uv_to_uyvy(void *buffer, Gx3211VpuSurface *surface)
{
	char *src = (char *)buffer, *y_addr = NULL, *uv_addr = NULL;
	char *dst = surface->buffer;
	int u_value = 0, v_value = 0, u_key[2] = {0}, v_key[2] = {0};
	int width = 0, height = 0, i = 0, j = 0, line = 0;

	width = gx3211_svpu_get_sce_length();
	height = gx3211_svpu_get_sce_height();

	y_addr = src;
	uv_addr = src + VOUT_BUF_SIZE / 3;

	
	if(CHIP_IS_GX6605S) {
		_adjust_byte_sequence(y_addr, width * height);
		for(i = 0; i < height; i++) {
			_adjust_byte_sequence(uv_addr + i * width, width);
		}
	} else {
		_adjust_byte_sequence(src, VOUT_BUF_SIZE);
	}

	for(i = 0; i < height; i++) {
		if(CHIP_IS_GX6605S) {
			line = i / 2;
		} else {
			line = i;
		}
		for(j = 0; j < width; j++) {
			if (CHIP_IS_GX6605S) {
				dst[(i * surface->width + j) * 2] = uv_addr[i * width + j];
			} else {
				if(CHIP_IS_GX6605S) {
					u_key[0] = uv_addr[line * width + (j / 2) * 2];
					u_key[1] = uv_addr[(line + 1) * width + (j / 2) * 2];
					u_value = (u_key[0] + u_key[1]) / 2;
				} else {
					u_value = uv_addr[line * width + j];
				}

				if(CHIP_IS_GX6605S) {
					v_key[0] = uv_addr[line * width + (j / 2) * 2 + 1];
					v_key[1] = uv_addr[(line + 1) * width + (j / 2) * 2 + 1];
					v_value = (v_key[0] + v_key[1]) / 2;
				} else {
					v_value = uv_addr[VOUT_BUF_SIZE / 3 + line * width + j];
				}
				if(j % 2) {
					dst[(i * surface->width + j) * 2] = v_value;
				} else {
					dst[(i * surface->width + j) * 2] = u_value;
				}
			}
			dst[(i * surface->width + j) * 2 + 1] = y_addr[i * width + j];
		}
	}
}

int Gx3211Vpu_Capture(GxVpuProperty_LayerCapture *capture)
{
	int i, ce_layer_id;
	Gx3211VpuCe   ce;
	unsigned char *src ,*dst;
	unsigned int  width ,line ,request_block = 0;

	if(capture->svpu_cap) {
		SvpuSurfaceInfo buf_info;

		memset(&buf_info, 0, sizeof(SvpuSurfaceInfo));
		gx3211vpu_info->svpu_tmp_buffer[0] = gx_page_malloc(VOUT_BUF_SIZE);
		gx3211vpu_info->svpu_tmp_buffer[1] = gx3211vpu_info->svpu_tmp_buffer[0];
		gx3211vpu_info->svpu_tmp_buffer[2] = gx3211vpu_info->svpu_tmp_buffer[1];
		gx3211vpu_info->svpu_tmp_buffer[3] = gx3211vpu_info->svpu_tmp_buffer[2];
		if(NULL == gx3211vpu_info->svpu_tmp_buffer[0]) {
			return (-1);
		}

		gx3211_svpu_get_buf(&buf_info);
		for(i = 0; i < 4; i++) {
			gx3211vpu_info->svpu_buffer[i] = (void *)(buf_info.buf_addrs[i]);
		}
		if(gx3211vpu_info->svpu_tmp_buffer[0] && gx3211vpu_info->svpu_buffer[0]) {
			GxVpuProperty_BeginUpdate Begin = {0};
			GxVpuProperty_EndUpdate End = {0};
			GxVpuProperty_CreateSurface src = {0}, dst = {0};
			GxVpuProperty_Blit Blit;
			GxVpuProperty_DestroySurface DesSurface = {0};

			src.width = 720 * 3;
			src.height = VOUT_BUF_SIZE / src.width;
			src.format = GX_COLOR_FMT_CLUT8;
			src.mode = GX_SURFACE_MODE_IMAGE;
			src.buffer = gx3211vpu_info->svpu_buffer[0];
			gxav_vpu_GetCreateSurface(&src);

			dst.width = 720 * 3;
			dst.height = VOUT_BUF_SIZE / dst.width;
			dst.format = GX_COLOR_FMT_CLUT8;
			dst.mode = GX_SURFACE_MODE_IMAGE;
			dst.buffer = gx3211vpu_info->svpu_tmp_buffer[0];
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
		}
		if(gx3211vpu_info->svpu_tmp_buffer[0]) {
			_save_y_uv_to_uyvy(gx3211vpu_info->svpu_tmp_buffer[0],
					   (Gx3211VpuSurface *) capture->surface);
			gx_page_free(gx3211vpu_info->svpu_tmp_buffer[0], VOUT_BUF_SIZE);
			gx3211vpu_info->svpu_tmp_buffer[0] = NULL;
		}

		return (0);
	}

	ce.layer    = capture->layer;
	ce.left     = capture->rect.x;
	ce.right    = capture->rect.x + capture->rect.width - 1;
	ce.top      = capture->rect.y;
	ce.bottom   = capture->rect.y + capture->rect.height - 1;
	ce.buffer   = ((Gx3211VpuSurface *) capture->surface)->buffer;
	if(capture->layer == GX_LAYER_MIX_VPP_BKG) {
		ce_layer_id = CE_LAYER_VPP;
		VPU_CAP_SET_MIX_BKG_ENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
	}
	else if(capture->layer == GX_LAYER_MIX_SPP_BKG){
		ce_layer_id = CE_LAYER_SPP;
		VPU_CAP_SET_MIX_BKG_ENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
	}
	else {
		ce_layer_id = gx3211ce_layerid_get(capture->layer);
		VPU_CAP_SET_MIX_BKG_DISENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
	}

	request_block = (ce.right - ce.left)/2/128*128;
	if(request_block < 128) {
		request_block = 128;
	}
	else if(request_block > 896) {
		request_block = 896;
	}

	REG_SET_FIELD(&(gx3211vpu_reg->rBUFF_CTRL1),0x7FF<<0,request_block,0);

	VPU_CAP_SET_LEVEL(gx3211vpu_reg->rCAP_CTRL, ce_layer_id);
	VPU_CAP_SET_PIC_ADDR(gx3211vpu_reg->rCAP_ADDR, (unsigned int)gx_virt_to_phys((unsigned int)ce.buffer));
	VPU_CAP_SET_PIC_HORIZONTAL(gx3211vpu_reg->rCAP_WIDTH, ce.left, ce.right);
	VPU_CAP_SET_PIC_VERTICAL(gx3211vpu_reg->rCAP_HEIGHT, ce.top, ce.bottom);
	VPU_CAP_START(gx3211vpu_reg->rCAP_CTRL);
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

extern int Gx3211Vpu_Zoom(GxLayerID layer);
int gx3211_vpu_SetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	Gx3211VpuLayer *layer = NULL;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	gx3211_vpu_GetVirtualResolution(&virtual_resolution);
	if((IS_INVAILD_LAYER(property->layer))
			|| ((gx3211vpu_info->layer[property->layer].auto_zoom) && (IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres)))
			|| (IS_NULL(gx3211vpu_info->layer[property->layer].surface)))
		return -1;

	layer = &gx3211vpu_info->layer[property->layer];

	layer->view_rect = property->rect;
	if(property->layer == GX_LAYER_VPP
			&& (gx3211vpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		gx3211_vpp_zoom_require();
	} else if(property->layer == GX_LAYER_SPP
			&& (gx3211vpu_info->layer[GX_LAYER_SPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO) {
		gx3211_vpp_zoom_require();
	} else {
		gx_mutex_lock(&gx3211vpu_info->mutex);
		if(gx3211vpu_info->vout_is_ready)
			Gx3211Vpu_Zoom(property->layer);
		gx_mutex_unlock(&gx3211vpu_info->mutex);
	}

	return 0;
}

int gx3211_vpu_SetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	Gx3211VpuLayer *layer;
	GxVpuProperty_Resolution virtual_resolution;

	GXAV_ASSERT(property != NULL);
	gx3211_vpu_GetVirtualResolution(&virtual_resolution);
	if( (IS_INVAILD_LAYER(property->layer))||
			(IS_NULL(gx3211vpu_info))||
			(IS_NULL(gx3211vpu_info->layer[property->layer].surface)))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, virtual_resolution.xres, virtual_resolution.yres))
		return -1;

	layer = &gx3211vpu_info->layer[property->layer];
	layer->clip_rect.x      = property->rect.x;
	layer->clip_rect.y      = property->rect.y;
	layer->clip_rect.width  = property->rect.width;
	layer->clip_rect.height = property->rect.height;

	if(property->layer == GX_LAYER_VPP &&
			(gx3211vpu_info->layer[GX_LAYER_VPP].surface)->surface_mode == GX_SURFACE_MODE_VIDEO){
		return gx3211_vpp_zoom_require();
	}

	return 0;
}

static int gx3211_vpu_pan_display(GxLayerID layer_id, void *buffer)
{
	Gx3211VpuLayer *layer;
	GXAV_ASSERT(buffer != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	layer = &gx3211vpu_info->layer[layer_id];
	if(layer->ops->set_main_surface != NULL) {
		return layer->ops->set_pan_display(buffer);
	}

	return -1;
}

static int gx3211_vpu_disp_field_start_int_en(void);
static int gx3211_vpu_disp_field_error_int_en(void);
int gx3211_vpu_SetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	Gx3211VpuLayer *layer;
	Gx3211VpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	if(false == is_in_surface_list(property->surface)) {
		return (-1);
	}

	surface = (Gx3211VpuSurface*)property->surface;
	layer = &gx3211vpu_info->layer[property->layer];
	if(layer->ops->set_main_surface != NULL) {
		if((surface->width * gx_color_get_bpp(surface->color_format) >> 3) % 8 != 0)
			return -1;

		layer->surface = (Gx3211VpuSurface *) property->surface;
		layer->surface->layer = layer;

		if (property->layer == GX_LAYER_VPP && GX_SURFACE_MODE_IMAGE == surface->surface_mode) {
			gx3211_vpu_disp_field_start_int_en();
			gx3211_vpu_disp_field_error_int_en();
			gx3211vpu_info->layer[GX_LAYER_VPP].flip_require = 1;
			while(gx3211vpu_info->layer[GX_LAYER_VPP].flip_require);
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

int gx3211_vpu_UnsetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	Gx3211VpuLayer *layer;
	Gx3211VpuSurface *surface;
	GXAV_ASSERT(property != NULL);
	if(IS_INVAILD_LAYER(property->layer) || (property->surface == NULL))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	surface = (Gx3211VpuSurface*)property->surface;
	layer   = &gx3211vpu_info->layer[property->layer];
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
	Gx3211VpuLayerOps *layer_ops;\
	GXAV_ASSERT(property != NULL);\
	if (IS_INVAILD_LAYER(layer_id) || gx3211vpu_info == NULL)\
	return -1;\
	layer_ops = gx3211vpu_info->layer[layer_id].ops;\
	if (layer_ops->func != NULL) {\
		gx3211vpu_info->layer[layer_id].attr = value;\
		return layer_ops->func(value);\
	}\
	return -1;\
}\

static int gx3211Vpu_LayerEnable(GxLayerID layer, int enable)
{
	if (IS_INVAILD_LAYER(layer))
		return -1;
	if (gx3211vpu_info->layer[layer].ops->set_enable)
		gx3211vpu_info->layer[layer].ops->set_enable(enable);
	return 0;
}

int gx3211_vpu_SetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GXAV_ASSERT(property != NULL);

	if(VPU_CLOSED()){
		return -1;
	}

	if (property->enable == 0) {
		if(property->layer == GX_LAYER_VPP) {
			gx_video_ppclose_require(0);
		}
		gx3211Vpu_LayerEnable(property->layer, 0);
		gx3211vpu_info->layer[property->layer].enable = 0;
	}
	else {
		if (gx3211vpu_info->layer[property->layer].surface == NULL)
			return -1;

		if (gx3211vpu_info->vout_is_ready) {
			if(property->layer == GX_LAYER_VPP) {
				gx_video_ppopen_require(0);
			}
			else {
				gx3211Vpu_LayerEnable(property->layer, 1);
			}
		}
		gx3211vpu_info->layer[property->layer].enable = 1;
	}


	return 0;
}

int gx3211_vpu_SetLayerEnablePatch(GxVpuProperty_LayerEnable *property)
{
	RUN_LAYER_FUNC(property->layer, set_enable, enable, property->enable);
}

int gx3211_vpu_SetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	RUN_LAYER_FUNC(property->layer, set_anti_flicker, anti_flicker_en, property->enable);
}

int gx3211_vpu_SetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	RUN_LAYER_FUNC(property->layer, set_on_top, on_top, property->enable);
}

int gx3211_vpu_SetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	RUN_LAYER_FUNC(property->layer, set_video_mode, video_mode, property->mode);
}

int gx3211_vpu_SetLayerMixConfig(GxVpuProperty_LayerMixConfig *property)
{
	if(property->covermode_en) {
		/* covermode en */
		VPU_MIX_LAYER_COVERMODE_ENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
		/* spp alpha config */
		if(property->spp_alpha.type == GX_ALPHA_GLOBAL) {
			VPU_MIX_SPP_GLOBAL_ALPHA_ENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
			VPU_MIX_SET_SPP_GLOBAL_ALPHA(gx3211vpu_reg->rPP_JPG_MIX_CTRL, property->spp_alpha.value);
		}
		else {
			VPU_MIX_SPP_GLOBAL_ALPHA_DISENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
		}
		/* vpp alpha config */
		VPU_MIX_SET_VPP_GLOBAL_ALPHA(gx3211vpu_reg->rPP_JPG_MIX_CTRL, property->vpp_alpha_value);
	}
	else {
		VPU_MIX_LAYER_COVERMODE_DISENABLE(gx3211vpu_reg->rPP_JPG_MIX_CTRL);
	}

	return 0;
}

#define ACTIVE_SURFACE_CHANGE(layer_id, func,value)\
{\
	Gx3211VpuLayerOps *layer_ops;\
	if(IS_INVAILD_LAYER(layer_id) || gx3211vpu_info == NULL)\
	return -1;\
	layer_ops = gx3211vpu_info->layer[layer_id].ops;\
	if(layer_ops->func != NULL)\
	return layer_ops->func(value);\
	return -1;\
}

#define RUN_SURFACE_FUNC(surface_handle, func, attr,value)\
{\
	Gx3211VpuSurface *surface;\
	if(IS_NULL(surface_handle) || gx3211vpu_info == NULL)\
	return -1;\
	surface = (Gx3211VpuSurface *) (surface_handle);\
	surface->attr = value;\
	if(IS_MAIN_SURFACE(surface))\
	ACTIVE_SURFACE_CHANGE(surface->layer->id,func,value);\
	return 0;\
}

int gx3211_vpu_SetPalette(GxVpuProperty_Palette *property)
{
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
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

int gx3211_vpu_SetAlpha(GxVpuProperty_Alpha *property)
{
	if(property->alpha.type == GX_ALPHA_GLOBAL && property->alpha.value >=256)
		return -1;
	if(property->alpha.type != GX_ALPHA_GLOBAL && property->alpha.type != GX_ALPHA_PIXEL)
		return -1;

	RUN_SURFACE_FUNC(property->surface, set_alpha, alpha, property->alpha);
}

int gx3211_vpu_SetColorFormat(GxVpuProperty_ColorFormat *property)
{
	if(property->format<GX_COLOR_FMT_CLUT1 || property->format>GX_COLOR_FMT_YCBCR420_UV)
		return -1;
	RUN_SURFACE_FUNC(property->surface, set_color_format, color_format, property->format);
}

int gx3211_vpu_SetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	Gx3211VpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &gx3211vpu_info->layer[property->layer];
		if(layer->ops->set_bg_color != NULL) {
			layer->bg_color = property->color;
			ret = layer->ops->set_bg_color(property->color);
		}
	}

	return ret;
}

int gx3211_vpu_SetColorKey(GxVpuProperty_ColorKey *property)
{
	Gx3211VpuSurface *surface;
	Gx3211VpuLayerOps *layer_ops;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
	surface->color_key = property->color;
	surface->color_key_en = property->enable;
	if(IS_MAIN_SURFACE(surface)) {
		if(IS_INVAILD_LAYER(surface->layer->id))
			return -1;
		layer_ops = gx3211vpu_info->layer[surface->layer->id].ops;
		if(layer_ops->set_color_key != NULL) {
			return layer_ops->set_color_key(&surface->color_key, surface->color_key_en);
		}
	}

	return 0;
}

int gx3211_vpu_SetPoint(GxVpuProperty_Point *property)
{
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;
	Gx3211Vpu_SetPoint(property);

	return 0;
}

int gx3211_vpu_SetDrawLine(GxVpuProperty_DrawLine *property)
{
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->start, surface->width, surface->height)
			|| IS_INVAILD_POINT(&property->end, surface->width, surface->height))
		return -1;

	return Gx3211Vpu_DrawLine(property);
}

#define GET_SURFACE(_src_is_ksurface, _src, _dst)\
	do{\
		typeof(_src) src = (_src);\
		typeof(_dst) dst = (_dst);\
		typeof(_src_is_ksurface) src_is_ksurface = (_src_is_ksurface);\
		gx_memset(dst, 0, sizeof(Gx3211VpuSurface));\
		if(src_is_ksurface){\
			gx_memcpy(dst, src, sizeof(Gx3211VpuSurface));\
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

int gx3211_vpu_SetBeginUpdate(GxVpuProperty_BeginUpdate *property);
int gx3211_vpu_SetEndUpdate(GxVpuProperty_EndUpdate *property);

static void _blit_uv_data(GxVpuProperty_Blit *property, Gx3211VpuSurface *sura)
{
	GxVpuProperty_CreateSurface  src_a8 = {0}, dst_a8 = {0};
	GxVpuProperty_DestroySurface destroy_surface = {0};
	GxVpuProperty_BeginUpdate begin = {0};
	GxVpuProperty_EndUpdate end = {0};
	GxVpuProperty_Blit tmp_blit;
	int ret = 0;

	src_a8.format = GX_COLOR_FMT_Y8;
	src_a8.width = property->srca.rect.width / 2;
	src_a8.height = property->srca.rect.height / 2;
	src_a8.mode = GX_SURFACE_MODE_IMAGE;
	src_a8.buffer = NULL;
	gx3211_vpu_GetCreateSurface(&src_a8);
	memcpy(src_a8.buffer,
			sura->buffer + property->srca.rect.width * property->srca.rect.height,
			src_a8.width * src_a8.height);

	dst_a8.format = GX_COLOR_FMT_Y8;
	dst_a8.width = property->srca.rect.width;
	dst_a8.height = property->srca.rect.height / 2;
	dst_a8.mode = GX_SURFACE_MODE_IMAGE;
	dst_a8.buffer = NULL;
	gx3211_vpu_GetCreateSurface(&dst_a8);
	memset(dst_a8.buffer, 0, dst_a8.width * dst_a8.height);

	begin.max_job_num = 2;
	gx3211_vpu_SetBeginUpdate(&begin);
	gx_memcpy(&tmp_blit, property, sizeof(GxVpuProperty_Blit));
	tmp_blit.srca.rect.x = tmp_blit.srca.rect.y = 0;
	tmp_blit.srca.rect.width = property->srca.rect.width / 2;
	tmp_blit.srca.rect.height = property->srca.rect.height / 2;
	tmp_blit.srca.surface = src_a8.surface;
	tmp_blit.srcb.rect.x = tmp_blit.srcb.rect.y = 0;
	tmp_blit.srcb.rect.width = property->srcb.rect.width / 2;
	tmp_blit.srcb.rect.height = property->srcb.rect.height / 2;
	tmp_blit.srcb.surface = dst_a8.surface;
	tmp_blit.dst = tmp_blit.srcb;
	ret = gx3211ga_blit(&tmp_blit);
	gx3211_vpu_SetEndUpdate(&end);

	property->srca.cct = dst_a8.surface;

	memcpy(src_a8.buffer,
	       sura->buffer + property->srca.rect.width * property->srca.rect.height +
			property->srca.rect.width * property->srca.rect.height / 4,
			src_a8.width * src_a8.height);

	dst_a8.format = GX_COLOR_FMT_Y8;
	dst_a8.width = property->srca.rect.width;
	dst_a8.height = property->srca.rect.height / 2;
	dst_a8.mode = GX_SURFACE_MODE_IMAGE;
	dst_a8.buffer = NULL;
	gx3211_vpu_GetCreateSurface(&dst_a8);
	memset(dst_a8.buffer, 0, dst_a8.width * dst_a8.height);

	begin.max_job_num = 2;
	gx3211_vpu_SetBeginUpdate(&begin);
	gx_memcpy(&tmp_blit, property, sizeof(GxVpuProperty_Blit));
	tmp_blit.srca.rect.x = tmp_blit.srca.rect.y = 0;
	tmp_blit.srca.rect.width = property->srca.rect.width / 2;
	tmp_blit.srca.rect.height = property->srca.rect.height / 2;
	tmp_blit.srca.surface = src_a8.surface;
	tmp_blit.srcb.rect.x = tmp_blit.srcb.rect.y = 0;
	tmp_blit.srcb.rect.width = property->srcb.rect.width / 2;
	tmp_blit.srcb.rect.height = property->srcb.rect.height / 2;
	tmp_blit.srcb.surface = dst_a8.surface;
	tmp_blit.dst = tmp_blit.srcb;
	ret = gx3211ga_blit(&tmp_blit);
	gx3211_vpu_SetEndUpdate(&end);

	property->srcb.cct = dst_a8.surface;

	destroy_surface.surface = src_a8.surface;
	gx3211_vpu_SetDestroySurface(&destroy_surface);
}

int gx3211_vpu_SetBlit(GxVpuProperty_Blit *property)
{
	int i, ret=-1;
	Gx3211VpuSurface *sura, *surb, *surd;
	GxVpuProperty_Blit kproperty;
	Gx3211VpuSurface isrca, isrcb,  idst;

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

	if( (kproperty.srca.rect.width == kproperty.srcb.rect.width &&
	    kproperty.srca.rect.height== kproperty.srcb.rect.height) ||
	    (GX_ALU_PAVE == property->mode)) {
		if((GX_COLOR_FMT_YCBCR420_Y_UV == sura->color_format) && (GX_COLOR_FMT_YCBCR420_Y_UV == surd->color_format)) {
			Gx3211VpuSurface *sur_tmp;
			GxVpuProperty_Blit tmp_property;

			sur_tmp = (Gx3211VpuSurface*)gx_malloc(sizeof(Gx3211VpuSurface)* 3);
			gx_memcpy(&sur_tmp[0], sura, sizeof(Gx3211VpuSurface));
			gx_memcpy(&sur_tmp[1], surb, sizeof(Gx3211VpuSurface));
			gx_memcpy(&sur_tmp[2], surd, sizeof(Gx3211VpuSurface));
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

				ret = gx3211ga_blit(&tmp_property);
			}
			gx_free(sur_tmp);
		}
		else {
			if(sura->color_format == GX_COLOR_FMT_YCBCR420_Y_U_V) {
				_blit_uv_data(&kproperty, sura);
			}
			ret = gx3211ga_blit(&kproperty);
			if(sura->color_format == GX_COLOR_FMT_YCBCR420_Y_U_V) {
				GxVpuProperty_DestroySurface destroy_surface = {0};

				destroy_surface.surface = kproperty.srca.cct;
				gx3211_vpu_SetDestroySurface(&destroy_surface);

				destroy_surface.surface = kproperty.srcb.cct;
				gx3211_vpu_SetDestroySurface(&destroy_surface);
			}
		}
	}

	return ret;
}

int gx3211_vpu_SetDfbBlit(GxVpuProperty_DfbBlit *property)
{
	int i, ret=-1;
	Gx3211VpuSurface *sura, *surb, *surd;
	GxVpuProperty_DfbBlit kproperty;
	Gx3211VpuSurface isrca, isrcb,  idst;

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
		Gx3211VpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (Gx3211VpuSurface*)gx_malloc(sizeof(Gx3211VpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(Gx3211VpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(Gx3211VpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(Gx3211VpuSurface));
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

			ret = gx3211ga_blit(&tmp_property);
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
				if(gx3211_vpu_GetCreateSurface(&tmp_dst)) {
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
				gx3211ga_dfb_blit(&kproperty);

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
			ret = gx3211ga_dfb_blit(&kproperty);
out:
			/* destroy tmp surfaces */
			for(i = 0; i < tmp_cnt; i++) {
				if(destroy_surface[i].surface != NULL)
					gx3211_vpu_SetDestroySurface(&destroy_surface[i]);
			}
		} else {
			ret = gx3211ga_dfb_blit(&kproperty);
		}
		return ret;
	}
	return ret;
}

int gx3211_vpu_SetBatchDfbBlit(GxVpuProperty_BatchDfbBlit *property)
{
	int i, ret=-1;
	Gx3211VpuSurface *sura, *surb, *surd;
	GxVpuProperty_BatchDfbBlit *kproperty;// Too big
	Gx3211VpuSurface isrca, isrcb,  idst;

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
		Gx3211VpuSurface *sur_tmp;
		GxVpuProperty_Blit tmp_property;

		sur_tmp = (Gx3211VpuSurface*)gx_malloc(sizeof(Gx3211VpuSurface)* 3);
		gx_memcpy(&sur_tmp[0], sura, sizeof(Gx3211VpuSurface));
		gx_memcpy(&sur_tmp[1], surb, sizeof(Gx3211VpuSurface));
		gx_memcpy(&sur_tmp[2], surd, sizeof(Gx3211VpuSurface));
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

			ret = gx3211ga_blit(&tmp_property);
		}
		gx_free(sur_tmp);
	} else {
		ret = gx3211ga_batch_dfb_blit(kproperty);
		gx_free(kproperty);
		return ret;
	}
	gx_free(kproperty);
	return ret;
}

int gx3211_vpu_SetFillRect(GxVpuProperty_FillRect *property)
{
	Gx3211VpuSurface surface;
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

	return gx3211ga_fillrect(&kproperty);
}

int gx3211_vpu_SetFillPolygon(GxVpuProperty_FillPolygon *property)
{
#if 0
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
	if(IS_VIDEO_SURFACE(surface)
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_GA_COLOR(surface->color_format))
		return -1;
	if(IS_INVAILD_RECT(&property->rect, surface->width, surface->height))
		return -1;

	return gx3211ga_fillpolygon(property);
#else
	return 0;
#endif
}

int gx3211_vpu_SetSetMode(GxVpuProperty_SetGAMode *property)
{
	GXAV_ASSERT(property != NULL);
	return gx3211ga_setmode(property);
}

int gx3211_vpu_SetWaitUpdate(GxVpuProperty_WaitUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return gx3211ga_wait(property);
}

int gx3211_vpu_SetBeginUpdate(GxVpuProperty_BeginUpdate *property)
{
	GXAV_ASSERT(property != NULL);
	return gx3211ga_begin(property);
}

int gx3211_vpu_SetEndUpdate(GxVpuProperty_EndUpdate *property)
{
	return gx3211ga_end(property);
}

int gx3211_vpu_SetConvertColor(GxVpuProperty_ConvertColor *property)
{
	GXAV_ASSERT(property != NULL);
	return 0;
}

int gx3211_vpu_SetVirtualResolution(GxVpuProperty_Resolution *property)
{
	GXAV_ASSERT(property != NULL);
	gx3211vpu_info->virtual_resolution.xres = property->xres;
	gx3211vpu_info->virtual_resolution.yres = property->yres;

	return 0;
}

int gx3211_vpu_SetAspectRatio(GxVpuProperty_AspectRatio *property)
{
	GXAV_ASSERT(property != NULL);
	if((property->AspectRatio > VP_AUTO)
			|| (property->AspectRatio < VP_UDEF))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	gx3211vpu_info->layer[GX_LAYER_SPP].spec=property->AspectRatio;
	gx3211vpu_info->layer[GX_LAYER_VPP].spec=property->AspectRatio;
	gx3211_vpp_zoom_require();

	return 0;
}

int gx3211_vpu_SetTvScreen(GxVpuProperty_TvScreen *property)
{
	GxVpuProperty_LayerViewport  vpp_viewport, spp_viewport;

	GXAV_ASSERT(property != NULL);
	if((property->Screen > SCREEN_16X9) || (property->Screen < SCREEN_4X3))
		return -1;

	if(VPU_CLOSED()){
		return -1;
	}

	spp_viewport.layer= GX_LAYER_SPP;
	spp_viewport.rect = gx3211vpu_info->layer[GX_LAYER_SPP].view_rect;
	gx3211vpu_info->layer[GX_LAYER_SPP].screen = property->Screen;
	gx3211_vpu_SetLayerViewport(&spp_viewport);

	vpp_viewport.layer= GX_LAYER_VPP;
	vpp_viewport.rect = gx3211vpu_info->layer[GX_LAYER_VPP].view_rect;
	gx3211vpu_info->layer[GX_LAYER_VPP].screen = property->Screen;
	gx3211_vpu_SetLayerViewport(&vpp_viewport);

	return 0;
}

int gx3211_vpu_SetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	GXAV_ASSERT(property != NULL);
	gx3211vpu_info->vbi.enable = property->enable;
	gx3211_vpu_VbiEnable(&gx3211vpu_info->vbi);

	return 0;
}

#define GET_LAYER_PROPERTY(attr,layer_attr)\
{\
	GXAV_ASSERT(property != NULL);\
	if(IS_INVAILD_LAYER(property->layer))\
	return -1;\
	if( gx3211vpu_info!=NULL ){\
		property->attr  = gx3211vpu_info->layer[property->layer].layer_attr ;\
		return 0;\
	}\
	else\
	return -1; \
}

#define GET_SURFACE_PROPERTY(attr,surface_attr)\
{\
	Gx3211VpuSurface *surface;\
	GXAV_ASSERT(property != NULL);\
	if(IS_NULL(property->surface))\
	return -1;\
	surface = (Gx3211VpuSurface *)(property->surface);\
	property->attr  = surface->surface_attr;\
	return 0;\
}

int gx3211_vpu_GetLayerViewport(GxVpuProperty_LayerViewport *property)
{
	GET_LAYER_PROPERTY(rect, view_rect);
}

int gx3211_vpu_GetLayerClipport(GxVpuProperty_LayerClipport *property)
{
	GET_LAYER_PROPERTY(rect, clip_rect);
}

int gx3211_vpu_GetLayerMainSurface(GxVpuProperty_LayerMainSurface *property)
{
	GET_LAYER_PROPERTY(surface, surface);
}

int gx3211_vpu_GetLayerEnable(GxVpuProperty_LayerEnable *property)
{
	GET_LAYER_PROPERTY(enable, enable);
}

int gx3211_vpu_GetLayerAntiFlicker(GxVpuProperty_LayerAntiFlicker *property)
{
	GET_LAYER_PROPERTY(enable, anti_flicker_en);
}

int gx3211_vpu_GetLayerOnTop(GxVpuProperty_LayerOnTop *property)
{
	GET_LAYER_PROPERTY(enable, on_top);
}

int gx3211_vpu_GetLayerVideoMode(GxVpuProperty_LayerVideoMode *property)
{
	GET_LAYER_PROPERTY(mode, video_mode);
}

int gx3211_vpu_GetLayerCapture(GxVpuProperty_LayerCapture *property)
{
#define IS_POINT_IN_RECT(point,rect)\
	((point.x>=rect.x)&&(point.y>=rect.y)&&(point.x<=rect.x+rect.width)&&(point.y<=rect.y+rect.height))

	struct vout_dvemode dvemode;
	GxVpuProperty_Resolution	referance, virtual_resolution;
	Gx3211VpuSurface *surface = (Gx3211VpuSurface*)property->surface;
	GxVpuProperty_LayerCapture  cap_property;
	Gx3211VpuLayer *layer;
	GxLayerID       layer_id;
	int ret = 0;
	GxAvRect        capture_rect = {0};
	GxAvPoint       end_point    = {0};
	GxAvPoint       point_topleft= {0}, point_topright={0}, point_bottomleft={0}, point_bottomright={0};

	GXAV_ASSERT(property != NULL);
	GXAV_ASSERT(property->surface != NULL);

	gx3211_vpu_GetVirtualResolution(&virtual_resolution);
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

			layer = &(gx3211vpu_info->layer[layer_id]);
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

		gx3211_vpu_GetActualResolution(&referance);
		cap_property.layer       =  property->layer;
		cap_property.rect.x      =  gx3211_vpu_VirtualToActual(capture_rect.x, referance.xres, 1);
		cap_property.rect.y      =  gx3211_vpu_VirtualToActual(capture_rect.y, referance.yres, 0);
		cap_property.rect.width  = (gx3211_vpu_VirtualToActual(capture_rect.width,  referance.xres, 1)>>3)<<3;
		cap_property.rect.height =  gx3211_vpu_VirtualToActual(capture_rect.height, referance.yres, 0);
		cap_property.surface = (void*)property->surface;
		cap_property.svpu_cap = property->svpu_cap;

		ret = Gx3211Vpu_Capture(&cap_property);
		return (ret);
	}
}

int gx3211_vpu_SetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	GXAV_ASSERT(property != NULL);

	gx3211vpu_info->layer[property->layer].auto_zoom = property->auto_zoom;

	return (0);
}

int gx3211_vpu_GetLayerAutoZoom(GxVpuProperty_LayerAutoZoom *property)
{
	GXAV_ASSERT(property != NULL);

	property->auto_zoom = gx3211vpu_info->layer[property->layer].auto_zoom;

	return (0);
}

int gx3211_vpu_SetGAMMACorrect(GxVpuProperty_GAMMACorrect *property)
{
	int gamma_coef[GAMMA_COEF_MAX][4] = {
		{0x5443301b, 0x93847464, 0xcbbdafa1, 0x00f3e6d8}, //0.8
		{0x49382715, 0x89796959, 0xc5b6a798, 0x00f1e3d4}, //0.9
		{0x40302010, 0x80706050, 0xc0b0a090, 0x00f0e0d0}, //1.0
		{0x3728190c, 0x77675747, 0xbaa99887, 0x00eeddcb}, //1.1
		{0x30221509, 0x6f5e4e3f, 0xb5a39180, 0x00ecdac7}, //1.2
	};

	GXAV_ASSERT(property != NULL);

	if(property->value >= GAMMA_COEF_MAX) {
		return (-1);
	}

	if((gxcore_chip_probe() == GXAV_ID_GX6605S) &&
	   (gxcore_chip_sub_probe() == 1)) {
		if((property->enable)) {
			REG_SET_VAL(&(gx3211vpu_reg->rPPGAMMA_COEF[0]), 1);
			REG_SET_VAL(&(gx3211vpu_reg->rPPGAMMA_COEF[1]), gamma_coef[property->value][0]);
			REG_SET_VAL(&(gx3211vpu_reg->rPPGAMMA_COEF[2]), gamma_coef[property->value][1]);
			REG_SET_VAL(&(gx3211vpu_reg->rPPGAMMA_COEF[3]), gamma_coef[property->value][2]);
			REG_SET_VAL(&(gx3211vpu_reg->rPPGAMMA_COEF[4]), gamma_coef[property->value][3]);
		} else {
			REG_SET_VAL(&(gx3211vpu_reg->rPPGAMMA_COEF[0]), 0);
		}
	}
	return (0);
}

int gx3211_vpu_GetCreatePalette(GxVpuProperty_CreatePalette *property)
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

int gx3211_vpu_DestroyPalette(GxVpuProperty_DestroyPalette *property)
{
	if(property->palette!=NULL)
		gx_free(property->palette);
	property->palette = NULL;

	return 0;
}

int gx3211_vpu_SurfaceBindPalette(GxVpuProperty_SurfaceBindPalette *property)
{
	Gx3211VpuSurface* surface = (Gx3211VpuSurface*)(property->surface);;
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
int gx3211_vpu_WPalette(GxVpuProperty_RWPalette *property)
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
int gx3211_vpu_RPalette(GxVpuProperty_RWPalette *property)
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

int gx3211_vpu_GetEntries(GxVpuProperty_GetEntries *property)
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

int gx3211_vpu_GetReadSurface(GxVpuProperty_ReadSurface *property)
{
	Gx3211VpuSurface *surface = NULL;
	Gx3211VpuSurface *now = NULL;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(gx3211vpu_info))
		return -1;
	surface = (Gx3211VpuSurface *) property->surface;
	if(surface == NULL)
		return -1;

	now = gx3211vpu_info->surface;
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

int gx3211_vpu_GetPalette(GxVpuProperty_Palette *property)
{
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;
	surface = (Gx3211VpuSurface *) (property->surface);
	if(surface->palette == NULL)
		return -1;

	gx_memcpy(&property->palette, surface->palette, sizeof(GxPalette));
	return 0;
}

int gx3211_vpu_GetAlpha(GxVpuProperty_Alpha *property)
{
	GET_SURFACE_PROPERTY(alpha, alpha);
}

int gx3211_vpu_GetColorFormat(GxVpuProperty_ColorFormat *property)
{
	GET_SURFACE_PROPERTY(format, color_format);
}

int gx3211_vpu_GetColorKey(GxVpuProperty_ColorKey *property)
{
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
	property->color = surface->color_key;
	property->enable= surface->color_key_en;

	return 0;
}

int gx3211_vpu_GetBackColor(GxVpuProperty_BackColor *property)
{
	int ret = -1;
	Gx3211VpuLayer *layer;

	GXAV_ASSERT(property != NULL);
	if(!IS_INVAILD_LAYER(property->layer)) {
		layer = &gx3211vpu_info->layer[property->layer];
		property->color = layer->bg_color;
		ret = 0;
	}

	return ret;
}

int gx3211_vpu_GetPoint(GxVpuProperty_Point *property)
{
	Gx3211VpuSurface *surface;

	GXAV_ASSERT(property != NULL);
	if(IS_NULL(property->surface))
		return -1;

	surface = (Gx3211VpuSurface *) (property->surface);
	if((IS_VIDEO_SURFACE(surface))
			|| IS_NULL(surface->buffer)
			|| IS_INVAILD_IMAGE_COLOR(surface->color_format)
			|| IS_INVAILD_POINT(&property->point, surface->width, surface->height))
		return -1;

	Gx3211Vpu_GetPoint(property);
	return 0;
}

int gx3211_vpu_GetConvertColor(GxVpuProperty_ConvertColor *property)
{
	return 0;
}

int gx3211_vpu_GetVbiEnable(GxVpuProperty_VbiEnable *property)
{
	property->enable = gx3211vpu_info->vbi.enable;
	return 0;
}

int gx3211_vpu_GetVbiCreateBuffer(GxVpuProperty_VbiCreateBuffer *property)
{
	if(property->unit_num == 0)
		return -1;
	if(property->buffer == NULL) {
		property->buffer = gx_malloc((property->unit_data_len + 1) * 4 *property->unit_num);
		if(property->buffer == NULL)
			return -1;
	}

	gx3211_vpu_VbiCreateBuffer(&gx3211vpu_info->vbi);
	return 0;
}

int gx3211_vpu_GetVbiReadAddress(GxVpuProperty_VbiReadAddress *property)
{
	gx3211_vpu_VbiGetReadPtr(&gx3211vpu_info->vbi);
	property->read_address = gx3211vpu_info->vbi.read_address;
	return 0;
}

int gx3211_vpu_ZoomSurface(GxVpuProperty_ZoomSurface *property)
{
	unsigned int ret=0;
	Gx3211VpuSurface src_surface, dst_surface;
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
	ret = gx3211ga_zoom(&kproperty);

	return ret ;
}

int gx3211_vpu_TurnSurface(GxVpuProperty_TurnSurface *property)
{
	Gx3211VpuSurface src_surface, dst_surface;
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

	return gx3211ga_turn(&kproperty);
}

int gx3211_vpu_Complet(GxVpuProperty_Complet *property)
{
	GXAV_ASSERT(property != NULL);
	return gx3211ga_complet(property);
}

int gx3211_vpu_Ga_Interrupt(int irq)
{
	if(irq != VPU_GA_INC_SRC) {
		VPU_PRINTF("vpu ga interrup inode error! irq_num=%x\n",irq);
		return 1;
	}

	return gx3211ga_interrupt(irq);
}

static void vpp_disp_controler_init(DispControler *controler)
{
	int i;

	if (controler) {
		controler->buf_to_wr = 0;
		for(i = 0; i < MAX_DISPBUF_NUM; i++)
			controler->unit[i] = (struct disp_unit*)gx3211vpu_reg->rDISP_CTRL[i];;
	}
}

static void vpu_init_auto_zoom(Gx3211VpuLayer *layer)
{
	int i = 0;

	for(i = 0; i < GX_LAYER_MAX; i++) {
		layer[i].auto_zoom = true;
	}
}

int gx3211_vpu_open(void)
{
	unsigned int value;
	Gx3211VpuOsdPriv  *osd_priv  = NULL;
	Gx3211VpuSosdPriv *sosd_priv = NULL;
	Gx3211VpuVppPriv  *vpp_priv  = NULL;

	gx3211vpu_info = (Gx3211Vpu *) gx_malloc(sizeof(Gx3211Vpu));
	if(gx3211vpu_info == NULL) {
		VPU_PRINTF("Open Error: gx3211vpu_info MALLOC\n");
		return -1;
	}

	gx_memset(gx3211vpu_info, 0, sizeof(Gx3211Vpu));
	gx3211vpu_info->layer[GX_LAYER_OSD].ops  = &gx3211_osd_ops;
	gx3211vpu_info->layer[GX_LAYER_SOSD].ops = &gx3211_sosd_ops;
	gx3211vpu_info->layer[GX_LAYER_VPP].ops  = &gx3211_vpp_ops;
	gx3211vpu_info->layer[GX_LAYER_SPP].ops  = &gx3211_spp_ops;
	gx3211vpu_info->layer[GX_LAYER_BKG].ops  = &gx3211_bkg_ops;

	gx3211vpu_info->layer[GX_LAYER_OSD].id   = GX_LAYER_OSD;
	gx3211vpu_info->layer[GX_LAYER_SOSD].id  = GX_LAYER_SOSD;
	gx3211vpu_info->layer[GX_LAYER_VPP].id   = GX_LAYER_VPP;
	gx3211vpu_info->layer[GX_LAYER_SPP].id   = GX_LAYER_SPP;
	gx3211vpu_info->layer[GX_LAYER_BKG].id   = GX_LAYER_BKG;

	gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.x = 0;
	gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.y = 0;
	gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.width = GX3211_MAX_XRES;
	gx3211vpu_info->layer[GX_LAYER_VPP].view_rect.height= GX3211_MAX_YRES;
	gx3211vpu_info->layer[GX_LAYER_VPP].clip_rect = gx3211vpu_info->layer[GX_LAYER_VPP].view_rect;

	vpu_init_auto_zoom(gx3211vpu_info->layer);

	//alloc osd priv
	osd_priv = (Gx3211VpuOsdPriv*)gx_dma_malloc(sizeof(Gx3211VpuOsdPriv));
	if(osd_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)osd_priv, 0, sizeof(Gx3211VpuOsdPriv));
	gx3211vpu_info->layer[GX_LAYER_OSD].priv = osd_priv;
#if REGION_POOL_EN
	osd_priv->region_pool = mpool_create(sizeof(OsdRegion), 8);
	if (osd_priv->region_pool == NULL) {
		VPU_PRINTF("Open Error: RegionPool ALLOC\n");
	}
#endif

	//alloc sosd priv
	sosd_priv = (Gx3211VpuSosdPriv*)gx_dma_malloc(sizeof(Gx3211VpuSosdPriv));
	if(sosd_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)sosd_priv, 0, sizeof(Gx3211VpuSosdPriv));
	gx3211vpu_info->layer[GX_LAYER_SOSD].priv = sosd_priv;

	//alloc vpp priv
	vpp_priv = (Gx3211VpuVppPriv*)gx_malloc(sizeof(Gx3211VpuVppPriv));
	if(vpp_priv == NULL) {
		VPU_PRINTF("Open Error: MemoryBlock MALLOC\n");
		return -1;
	}
	gx_memset((void*)vpp_priv, 0, sizeof(Gx3211VpuVppPriv));
	gx3211vpu_info->layer[GX_LAYER_VPP].priv = vpp_priv;
	vpp_disp_controler_init(&vpp_priv->controler);

	value = REG_GET_VAL(&(gx3211vpu_reg->rPP_DISP_CTRL))|(1<<30);
	REG_SET_VAL(&(gx3211vpu_reg->rPP_DISP_CTRL),value);

	{
		clock_params clock;
		clock.vpu.div = 1;
		clock.vpu.clock = 7;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
		clock.vpu.clock = 30;
		gxav_clock_setclock(MODULE_TYPE_VPU, &clock);
	}
	gx3211ga_open();

	gx3211vpu_info->reset_flag = 1;
	gx_mutex_init(&gx3211vpu_info->mutex);
	return 0;
}

int gx3211_vpu_close(void)
{
	Gx3211VpuOsdPriv **osd_priv  = NULL;
	Gx3211VpuOsdPriv **sosd_priv = NULL;
	Gx3211VpuVppPriv **vpp_priv  = NULL;

	if(gx3211vpu_info) {
		Gx3211VpuSurface *next;
		Gx3211VpuSurface *now = gx3211vpu_info->surface;

		while (now != NULL) {
			next = now->next;
			if(remove_surface_from_list(now) != 0)
				break;
			now = next;
		}

		//free osd priv
		osd_priv = (Gx3211VpuOsdPriv**)(&(gx3211vpu_info->layer[GX_LAYER_OSD].priv));
		if(*osd_priv) {
#if REGION_POOL_EN
			if ((*osd_priv)->region_pool) {
				mpool_destroy((*osd_priv)->region_pool);
			}
#endif
			gx_dma_free((unsigned char *)*osd_priv, sizeof(Gx3211VpuOsdPriv));
			*osd_priv = NULL;
		}

		//free sosd priv
		sosd_priv = (Gx3211VpuSosdPriv**)(&(gx3211vpu_info->layer[GX_LAYER_SOSD].priv));
		if(*sosd_priv) {
			gx_dma_free((unsigned char *)*sosd_priv, sizeof(Gx3211VpuSosdPriv));
			*sosd_priv = NULL;
		}

		//free vpp priv
		vpp_priv = (Gx3211VpuVppPriv**)(&(gx3211vpu_info->layer[GX_LAYER_VPP].priv));
		if(*vpp_priv) {
			gx_free(*vpp_priv);
			*vpp_priv = NULL;
		}

		gx_mutex_destroy(&gx3211vpu_info->mutex);
		gx_free(gx3211vpu_info);
		gx3211vpu_info = NULL;
	}

	VPU_VPP_DISABLE(gx3211vpu_reg->rPP_CTRL);
	VPU_SPP_DISABLE(gx3211vpu_reg->rPIC_CTRL);
	VPU_OSD_DISABLE(gx3211vpu_reg->rOSD_CTRL);
	VPU_VBI_STOP(gx3211vpu_reg->rVBI_CTRL);
	VPU_DBG("[GX3211]:Close OK!\n");

	return 0;
}

static void gx3211_VpuIOUnmap(void)
{
	if(gx3211vpu_reg) {
		gx_iounmap(gx3211vpu_reg);
		gx_release_mem_region(VPU_REG_BASE_ADDR, sizeof(Gx3211VpuReg));
		gx3211vpu_reg = NULL;
	}

	return ;
}

static int gx3211_VpuIORemap(void)
{
	if(!gx_request_mem_region(VPU_REG_BASE_ADDR, sizeof(Gx3211VpuReg))) {
		VPU_PRINTF("request_mem_region failed");
		goto VPU_IOMAP_ERROR;
	}
	gx3211vpu_reg = gx_ioremap(VPU_REG_BASE_ADDR, sizeof(Gx3211VpuReg));
	if(!gx3211vpu_reg) {
		goto VPU_IOMAP_ERROR;
	}
	return 0;

VPU_IOMAP_ERROR:
	gx3211_VpuIOUnmap();
	return -1;
}

static int gx3211_vpu_init(void)
{
	unsigned int i;

	if(0 != gx3211_VpuIORemap()) {
		return -1;
	}

	gx3211ga_init();
	gx3211_svpu_init();
	//VPU_VPP_DISABLE(gx3211vpu_reg->rPP_CTRL);
	//VPU_SPP_DISABLE(gx3211vpu_reg->rPIC_CTRL);
	//VPU_OSD_DISABLE(gx3211vpu_reg->rOSD_CTRL);
	VPU_VPP_SET_BUFFER_REQ_DATA_LEN(gx3211vpu_reg->rBUFF_CTRL1, 0x100);
	VPU_SPP_SET_BUFFER_REQ_DATA_LEN(gx3211vpu_reg->rPIC_PARA, 0x100);
	VPU_VPP_SET_ZOOM(gx3211vpu_reg->rPP_ZOOM, 0x1000, 0x1000);
	VPU_VPP_SET_PP_SIZE(gx3211vpu_reg->rPP_VIEW_SIZE, GX3211_MAX_XRES, GX3211_MAX_YRES);
	VPU_VPP_SET_FIELD_MODE_UV_FIELD(gx3211vpu_reg->rPP_CTRL,0x1);
	VPU_VPP_SET_BG_COLOR(gx3211vpu_reg->rPP_BACK_COLOR, 0x10, 0x80, 0x80);
	VPU_SPP_SET_BG_COLOR(gx3211vpu_reg->rPIC_BACK_COLOR, 0x10, 0x80, 0x80);
	for (i = 0; i < OSD_FLICKER_TABLE_LEN; i++)
		VPU_OSD_SET_FLIKER_FLITER(gx3211vpu_reg->rOSD_PHASE_PARA, i, gx3211_osd_filter_table[i]);

	return 0;
}

static int gx3211_vpu_cleanup(void)
{
	gx3211_svpu_cleanup();
	gx3211_vpu_close();
	gx3211ga_cleanup();

	gx3211_VpuIOUnmap();
	return 0;
}


void gx3211_vpu_SetBufferStateDelay(int v)
{
	VPU_SET_BUFF_STATE_DELAY(gx3211vpu_reg->rBUFF_CTRL2, v);
}

static int gx3211_vpu_SetVoutIsready(int ready)
{
	if(gx3211vpu_info != NULL) {
		gx_mutex_lock(&gx3211vpu_info->mutex);
		gx3211vpu_info->vout_is_ready = ready;
		if(ready) {
			gx_video_disp_patch(0);
			//re-zoom all layer
			if (gx3211vpu_info->layer[GX_LAYER_OSD].auto_zoom) {
				Gx3211Vpu_Zoom(GX_LAYER_OSD);
			}

			if (gx3211vpu_info->layer[GX_LAYER_SOSD].auto_zoom) {
				Gx3211Vpu_Zoom(GX_LAYER_SOSD);
			}

			if (gx3211vpu_info->layer[GX_LAYER_SPP].auto_zoom) {
				Gx3211Vpu_Zoom(GX_LAYER_SPP);
			}

			if (gx3211vpu_info->layer[GX_LAYER_VPP].auto_zoom) {
				gx3211_vpp_zoom_require();
			}

			//enable on needs
			if (gx3211vpu_info->layer[GX_LAYER_OSD].enable) {
				gx3211Vpu_LayerEnable(GX_LAYER_OSD, 1);
			}
			if (gx3211vpu_info->layer[GX_LAYER_SOSD].enable) {
				gx3211Vpu_LayerEnable(GX_LAYER_SOSD, 1);
			}
			if (gx3211vpu_info->layer[GX_LAYER_SPP].enable) {
				gx3211Vpu_LayerEnable(GX_LAYER_SPP, 1);
			}
			if(gx3211vpu_info->layer[GX_LAYER_VPP].enable) {
				gx_video_ppopen_require(0);
			}
			gx_mutex_unlock(&gx3211vpu_info->mutex);
		}
		else {
			gx_mutex_unlock(&gx3211vpu_info->mutex);
			gx_interrupt_disable();
			gx3211Vpu_LayerEnable(GX_LAYER_OSD, 0);
			gx3211Vpu_LayerEnable(GX_LAYER_SOSD, 0);
			gx3211Vpu_LayerEnable(GX_LAYER_SPP, 0);
			gx3211Vpu_LayerEnable(GX_LAYER_VPP, 0);
			gx_video_ppclose_require(0);
			gx_interrupt_enable();
			while(!gx3211_vpu_check_layers_close()) gx_msleep(10);
		}
		return 0;
	}

	return -1;
}

int gx3211_vpu_DACEnable(GxVpuDacID dac_id, int enable)
{
	if(gx3211vpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if(enable) {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				VPU_DAC_ENABLE(gx3211vpu_reg->rPOWER_DOWN, 0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				VPU_DAC_ENABLE(gx3211vpu_reg->rPOWER_DOWN, 1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				VPU_DAC_ENABLE(gx3211vpu_reg->rPOWER_DOWN, 2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				VPU_DAC_ENABLE(gx3211vpu_reg->rPOWER_DOWN, 3);
			if((dac_id&VPU_DAC_ALL) == VPU_DAC_ALL)
				VPU_DAC_ENABLE(gx3211vpu_reg->rPOWER_DOWN, 4);
		}
		else {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				VPU_DAC_DISABLE(gx3211vpu_reg->rPOWER_DOWN, 0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				VPU_DAC_DISABLE(gx3211vpu_reg->rPOWER_DOWN, 1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				VPU_DAC_DISABLE(gx3211vpu_reg->rPOWER_DOWN, 2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				VPU_DAC_DISABLE(gx3211vpu_reg->rPOWER_DOWN, 3);
			if((dac_id&VPU_DAC_ALL) == VPU_DAC_ALL)
				VPU_DAC_DISABLE(gx3211vpu_reg->rPOWER_DOWN, 4);
		}
		return 0;
	}

	return -1;
}

int gx3211_vpu_DACPower(GxVpuDacID dac_id)
{
	int ret = 0;
#if 0
	if (gx3211vpu_reg)
		gx_printf("\n%s:%d, reg = %p, dac_id = %d, dac-reg = 0x%x\n", __func__, __LINE__, gx3211vpu_reg, dac_id, gx3211vpu_reg->rPOWER_DOWN);
	else
		gx_printf("\n%s:%d, reg = %p, dac_id = %d\n", __func__, __LINE__, gx3211vpu_reg, dac_id);
#endif

	if (gx3211vpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if (REG_GET_BIT(&gx3211vpu_reg->rPOWER_DOWN, 4) == 1) {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				ret |= (REG_GET_BIT(&gx3211vpu_reg->rPOWER_DOWN, 0) << VPU_DAC_0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				ret |= (REG_GET_BIT(&gx3211vpu_reg->rPOWER_DOWN, 1) << VPU_DAC_1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				ret |= (REG_GET_BIT(&gx3211vpu_reg->rPOWER_DOWN, 2) << VPU_DAC_2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				ret |= (REG_GET_BIT(&gx3211vpu_reg->rPOWER_DOWN, 3) << VPU_DAC_3);
		}
	}

	return ret;
}

int gx3211_vpu_DACSource(GxVpuDacID dac_id, GxVpuDacSrc source)
{
	if(gx3211vpu_reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if(source == VPU_DAC_SRC_VPU || source == VPU_DAC_SRC_SVPU) {
			if(source == VPU_DAC_SRC_VPU) {
				if(dac_id&VPU_DAC_0) {
					REG_CLR_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 0);
					gxav_clock_set_video_dac_clock_source(0, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_1) {
					REG_CLR_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 1);
					gxav_clock_set_video_dac_clock_source(1, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_2) {
					REG_CLR_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 2);
					gxav_clock_set_video_dac_clock_source(2, VIDEO_DAC_SRC_VPU);
				}
				if(dac_id&VPU_DAC_3) {
					REG_CLR_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 3);
					gxav_clock_set_video_dac_clock_source(3, VIDEO_DAC_SRC_VPU);
				}
			}
			else {
				if(dac_id&VPU_DAC_0) {
					REG_SET_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 0);
					gxav_clock_set_video_dac_clock_source(0, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_1) {
					REG_SET_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 1);
					gxav_clock_set_video_dac_clock_source(1, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_2) {
					REG_SET_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 2);
					gxav_clock_set_video_dac_clock_source(2, VIDEO_DAC_SRC_SVPU);
				}
				if(dac_id&VPU_DAC_3) {
					REG_SET_BIT(&gx3211vpu_reg->rPOWER_DOWN_BYSELF, 3);
					gxav_clock_set_video_dac_clock_source(3, VIDEO_DAC_SRC_SVPU);
				}
			}
			return 0;
		}
	}

	return -1;
}

static int gx3211_vpu_vpp_get_base_line(void)
{
	return VPU_VPP_GET_BASE_LINE(gx3211vpu_reg->rPP_FRAME_STRIDE);
}

static int gx3211_vpu_vpp_set_base_line(unsigned int value)
{
	VPU_VPP_SET_BASE_LINE(gx3211vpu_reg->rPP_FRAME_STRIDE, value);
	return 0;
}

static int gx3211_vpu_disp_get_buff_id(void)
{
	return VPU_DISP_GET_BUFF_ID(gx3211vpu_reg->rPP_DISP_R_PTR);
}

static int gx3211_vpu_disp_set_rst(void)
{
	VPU_DISP_SET_RST( gx3211vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int gx3211_vpu_disp_clr_rst(void)
{
	VPU_DISP_CLR_RST( gx3211vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int gx3211_vpu_disp_get_view_active_cnt(void)
{
	return VPU_DISP_GET_VIEW_ACTIVE_CNT(gx3211vpu_reg->rSCAN_LINE);
}

static int gx3211_vpu_disp_clr_field_error_int(void)
{
	VPU_DISP_CLR_FILED_ERROR_INT(gx3211vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3211_vpu_disp_clr_field_start_int(void)
{
	VPU_DISP_CLR_FILED_START_INT(gx3211vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3211_vpu_disp_field_error_int_en(void)
{
	VPU_DISP_FILED_ERROR_INT_EN( gx3211vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int gx3211_vpu_disp_field_start_int_en(void)
{
	VPU_DISP_FILED_START_INT_EN( gx3211vpu_reg->rPP_DISP_CTRL );
	return 0;
}

static int field_int_enabled(void)
{
	return VPU_DISP_FILED_START_INT_ENABLED( gx3211vpu_reg->rPP_DISP_CTRL );
}

static int gx3211_vpu_disp_field_error_int_dis(void)
{
	VPU_DISP_FILED_ERROR_INT_DIS(gx3211vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3211_vpu_disp_field_start_int_dis(void)
{
	VPU_DISP_FILED_START_INT_DIS(gx3211vpu_reg->rPP_DISP_CTRL);
	return 0;
}

static int gx3211_vpu_disp_get_buff_cnt(void)
{
	return VPU_DISP_GET_BUFF_CNT(gx3211vpu_reg->rPP_DISP_CTRL);
}

static int gx3211_vpu_disp_get_view_field(void)
{
	return VPU_DISP_GET_VIEW_FIELD(gx3211vpu_reg->rSCAN_LINE);
}

static int gx3211_vpu_disp_get_display_buf(int i)
{
	return (int)(gx3211vpu_reg->rDISP_CTRL[i]);
}

extern int gx3211_spp_enable(int enable);
extern int gx3211_vpp_enable(int enable);
static int gx3211_vpu_disp_layer_enable(GxLayerID layer, int enable)
{
	if (gx3211vpu_info == NULL)
		return 0;

	if (!gx3211vpu_info->vout_is_ready) {
		gx3211vpu_info->layer[layer].enable = enable;
		return 0;
	}

	switch (layer)
	{
	case GX_LAYER_VPP:
		gx3211_vpp_enable(enable);
		break;
	case GX_LAYER_SPP:
		gx3211_spp_enable(enable);
		break;
	default:
		return -1;
	}
	return 0;
}


static int gx3211_vpu_vpp_set_stream_ratio(unsigned int ratio)
{
	return gx3211_vpp_set_stream_ratio(ratio);
}

static int gx3211_vpu_vpp_get_actual_viewrect(GxAvRect *view_rect)
{
	return gx3211_vpp_get_actual_viewrect(view_rect);
}

static int gx3211_vpu_regist_surface(GxVpuProperty_RegistSurface *param)
{
	Gx3211SurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

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

static int gx3211_vpu_unregist_surface(GxVpuProperty_UnregistSurface *param)
{
	int i, j;
	Gx3211SurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	gx_interrupt_disable();
	for(i = 0; i < GXVPU_MAX_SURFACE_NUM; i++) {
		for(j = 0; j < GXVPU_MAX_SURFACE_NUM; j++)
			if(param->surfaces[j] == sm->surfaces[i])
				sm->surfaces[i] = NULL;
	}
	gx_interrupt_enable();
	return 0;
}

static int gx3211_vpu_get_idle_surface(GxVpuProperty_GetIdleSurface *param)
{
	int idle;
	Gx3211SurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	gx_interrupt_disable();
	idle = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
	param->idle_surface = sm->surfaces[idle];
	if(sm->surfaces[idle])
		sm->surfaces[idle]->dirty = 0;
	gx_interrupt_enable();

	return 0;
}

static int gx3211_vpu_flip_surface(GxVpuProperty_FlipSurface *param)
{
	Gx3211SurfaceManager *sm = GET_SURFACE_MANAGER(param->layer);

	if(param && param->surface) {
		gx_interrupt_disable();
		((Gx3211VpuSurface*)(param->surface))->dirty = 1;
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
		gx3211_vpu_disp_field_start_int_en();
		gx3211_vpu_disp_field_error_int_en();
		sm->interrupt_en = 1;
	}
	return 0;
}

static int gx3211_vpu_isr_callback(void *arg)
{
	int ret = 0;
	int ready;
	GxVpuProperty_LayerMainSurface main_surface;
	Gx3211SurfaceManager *sm;
	int i, layers[] = {GX_LAYER_OSD, GX_LAYER_SPP};

	if (VPU_CLOSED()){
		gx3211_vpu_disp_clr_field_error_int();
		gx3211_vpu_disp_clr_field_start_int();
		gx3211_vpu_disp_field_start_int_dis();
		gx3211_vpu_disp_field_error_int_dis();
		return -1;
	}

	if (gx3211vpu_info->layer[GX_LAYER_VPP].flip_require == 1) {
		Gx3211VpuLayer *layer = &gx3211vpu_info->layer[GX_LAYER_VPP];
		layer->ops->set_main_surface(layer->surface);
		gx3211vpu_info->layer[GX_LAYER_VPP].flip_require = 0;
		ret = 1;
	}
	for (i = 0; i < sizeof(layers)/sizeof(int); i++) {
		if((sm = GET_SURFACE_MANAGER(layers[i])) != NULL) {
			gx3211_vpu_disp_clr_field_error_int();
			gx3211_vpu_disp_clr_field_start_int();
			ready = (sm->showing+1)%GXVPU_MAX_SURFACE_NUM;
			if(sm->surfaces[ready] && sm->surfaces[ready]->dirty) {
				main_surface.layer   = layers[i];
				main_surface.surface = sm->surfaces[ready];
				gx3211_vpu_SetLayerMainSurface(&main_surface);
				sm->surfaces[ready]->dirty = 0;
				sm->surfaces[sm->showing]->dirty = 0;
				sm->showing = ready;
			}
			sm->cur_time++;
		}
	}

	return ret;
}

static int gx3211_vpu_add_region(GxVpuProperty_AddRegion *param)
{
	int ret = -1;
	Gx3211VpuLayerOps *osd_ops = gx3211vpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->add_region) {
		ret = osd_ops->add_region(param->surface, param->pos_x, param->pos_y);
	}

	return ret;
}

static int gx3211_vpu_remove_region(GxVpuProperty_RemoveRegion *param)
{
	int ret = -1;
	Gx3211VpuLayerOps *osd_ops = gx3211vpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->remove_region) {
		ret = osd_ops->remove_region(param->surface);
	}

	return ret;
}

static int gx3211_vpu_update_region(GxVpuProperty_UpdateRegion *param)
{
	int ret = -1;
	Gx3211VpuLayerOps *osd_ops = gx3211vpu_info->layer[GX_LAYER_OSD].ops;

	if (osd_ops && osd_ops->update_region) {
		ret = osd_ops->update_region(param->surface, param->pos_x, param->pos_y);
	}

	return ret;
}

int gx3211_vpu_reset(void)
{
	int vpu_int_src = 46;

	gx_interrupt_mask(vpu_int_src);
	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	gxav_clock_hot_rst_clr(MODULE_TYPE_VPU);

	//reset VPU R-Ptr
	VPU_DISP_SET_RST(gx3211vpu_reg->rPP_DISP_CTRL);
	gx_mdelay(10);
	VPU_DISP_CLR_RST(gx3211vpu_reg->rPP_DISP_CTRL);

	gx3211vpu_info->reset_flag++;
	gx_interrupt_unmask(vpu_int_src);

	return 0;
}

static struct vpu_ops gx3211_vpu_ops = {
	.init                     = gx3211_vpu_init                      ,
	.cleanup                  = gx3211_vpu_cleanup                   ,
	.open                     = gx3211_vpu_open                      ,
	.close                    = gx3211_vpu_close                     ,
	.reset                    = gx3211_vpu_reset                     ,

	.WPalette                 = gx3211_vpu_WPalette                  ,
	.DestroyPalette           = gx3211_vpu_DestroyPalette            ,
	.SurfaceBindPalette       = gx3211_vpu_SurfaceBindPalette        ,
	.SetLayerViewport         = gx3211_vpu_SetLayerViewport          ,
	.SetLayerMainSurface      = gx3211_vpu_SetLayerMainSurface       ,
	.UnsetLayerMainSurface    = gx3211_vpu_UnsetLayerMainSurface     ,
	.SetLayerEnable           = gx3211_vpu_SetLayerEnable            ,
	.SetLayerAntiFlicker      = gx3211_vpu_SetLayerAntiFlicker       ,
	.SetLayerOnTop            = gx3211_vpu_SetLayerOnTop             ,
	.SetLayerVideoMode        = gx3211_vpu_SetLayerVideoMode         ,
	.SetLayerMixConfig        = gx3211_vpu_SetLayerMixConfig         ,
	.SetDestroySurface        = gx3211_vpu_SetDestroySurface         ,
	.ModifySurface            = gx3211_vpu_ModifySurface             ,
	.SetPalette               = gx3211_vpu_SetPalette                ,
	.SetAlpha                 = gx3211_vpu_SetAlpha                  ,
	.SetColorFormat           = gx3211_vpu_SetColorFormat            ,
	.SetColorKey              = gx3211_vpu_SetColorKey               ,
	.SetBackColor             = gx3211_vpu_SetBackColor              ,
	.SetPoint                 = gx3211_vpu_SetPoint                  ,
	.SetDrawLine              = gx3211_vpu_SetDrawLine               ,
	.SetConvertColor          = gx3211_vpu_SetConvertColor           ,
	.SetVirtualResolution     = gx3211_vpu_SetVirtualResolution      ,
	.SetVbiEnable             = gx3211_vpu_SetVbiEnable              ,
	.SetAspectRatio           = gx3211_vpu_SetAspectRatio            ,
	.SetTvScreen              = gx3211_vpu_SetTvScreen               ,
	.SetLayerEnablePatch      = gx3211_vpu_SetLayerEnablePatch       ,
	.GetEntries               = gx3211_vpu_GetEntries                ,
	.RPalette                 = gx3211_vpu_RPalette                  ,
	.GetCreatePalette         = gx3211_vpu_GetCreatePalette          ,
	.GetLayerViewport         = gx3211_vpu_GetLayerViewport          ,
	.GetLayerClipport         = gx3211_vpu_GetLayerClipport          ,
	.GetLayerMainSurface      = gx3211_vpu_GetLayerMainSurface       ,
	.GetLayerEnable           = gx3211_vpu_GetLayerEnable            ,
	.GetLayerAntiFlicker      = gx3211_vpu_GetLayerAntiFlicker       ,
	.GetLayerOnTop            = gx3211_vpu_GetLayerOnTop             ,
	.GetLayerVideoMode        = gx3211_vpu_GetLayerVideoMode         ,
	.GetLayerCapture          = gx3211_vpu_GetLayerCapture           ,
	.GetCreateSurface         = gx3211_vpu_GetCreateSurface          ,
	.GetReadSurface           = gx3211_vpu_GetReadSurface            ,
	.GetPalette               = gx3211_vpu_GetPalette                ,
	.GetAlpha                 = gx3211_vpu_GetAlpha                  ,
	.GetColorFormat           = gx3211_vpu_GetColorFormat            ,
	.GetColorKey              = gx3211_vpu_GetColorKey               ,
	.GetBackColor             = gx3211_vpu_GetBackColor              ,
	.GetPoint                 = gx3211_vpu_GetPoint                  ,
	.GetConvertColor          = gx3211_vpu_GetConvertColor           ,
	.GetVirtualResolution     = gx3211_vpu_GetVirtualResolution      ,
	.GetVbiEnable             = gx3211_vpu_GetVbiEnable              ,
	.GetVbiCreateBuffer       = gx3211_vpu_GetVbiCreateBuffer        ,
	.GetVbiReadAddress        = gx3211_vpu_GetVbiReadAddress         ,
	.SetLayerClipport         = gx3211_vpu_SetLayerClipport          ,
	.SetBlit                  = gx3211_vpu_SetBlit                   ,
	.SetDfbBlit               = gx3211_vpu_SetDfbBlit                ,
	.SetBatchDfbBlit          = gx3211_vpu_SetBatchDfbBlit           ,
	.SetFillRect              = gx3211_vpu_SetFillRect               ,
	.SetFillPolygon           = gx3211_vpu_SetFillPolygon            ,
	.SetGAMode                = gx3211_vpu_SetSetMode                ,
	.SetWaitUpdate            = gx3211_vpu_SetWaitUpdate             ,
	.SetBeginUpdate           = gx3211_vpu_SetBeginUpdate            ,
	.SetEndUpdate             = gx3211_vpu_SetEndUpdate              ,
	.ZoomSurface              = gx3211_vpu_ZoomSurface               ,
	.Complet                  = gx3211_vpu_Complet                   ,
	.TurnSurface              = gx3211_vpu_TurnSurface               ,

	.Ga_Interrupt             = gx3211_vpu_Ga_Interrupt              ,
	.SetVoutIsready           = gx3211_vpu_SetVoutIsready            ,
	.DACEnable                = gx3211_vpu_DACEnable                 ,
	.DACPower                 = gx3211_vpu_DACPower                  ,
	.DACSource                = gx3211_vpu_DACSource                 ,

	.vpp_get_base_line        = gx3211_vpu_vpp_get_base_line         ,
	.vpp_set_base_line        = gx3211_vpu_vpp_set_base_line         ,
	.disp_get_buff_id         = gx3211_vpu_disp_get_buff_id          ,
	.disp_set_rst             = gx3211_vpu_disp_set_rst              ,
	.disp_clr_rst             = gx3211_vpu_disp_clr_rst              ,
	.disp_get_view_active_cnt = gx3211_vpu_disp_get_view_active_cnt  ,
	.disp_clr_field_error_int = gx3211_vpu_disp_clr_field_error_int  ,
	.disp_clr_field_start_int = gx3211_vpu_disp_clr_field_start_int  ,
	.disp_field_error_int_en  = gx3211_vpu_disp_field_error_int_en   ,
	.disp_field_start_int_en  = gx3211_vpu_disp_field_start_int_en   ,
	.disp_field_error_int_dis = gx3211_vpu_disp_field_error_int_dis  ,
	.disp_field_start_int_dis = gx3211_vpu_disp_field_start_int_dis  ,
	.disp_get_buff_cnt        = gx3211_vpu_disp_get_buff_cnt         ,
	.disp_get_view_field      = gx3211_vpu_disp_get_view_field       ,
	.disp_get_display_buf     = gx3211_vpu_disp_get_display_buf      ,
	.disp_layer_enable        = gx3211_vpu_disp_layer_enable         ,

	.vpp_set_stream_ratio     = gx3211_vpu_vpp_set_stream_ratio      ,
	.vpp_get_actual_viewrect  = gx3211_vpu_vpp_get_actual_viewrect   ,
	.vpp_play_frame           = gx3211_vpp_play_frame                ,

	.RegistSurface   = gx3211_vpu_regist_surface,
	.UnregistSurface = gx3211_vpu_unregist_surface,
	.GetIdleSurface  = gx3211_vpu_get_idle_surface,
	.FlipSurface     = gx3211_vpu_flip_surface,
	.PanDisplay      = gx3211_vpu_pan_display,
	.VpuIsrCallback  = gx3211_vpu_isr_callback,

	.AddRegion    = gx3211_vpu_add_region,
	.RemoveRegion = gx3211_vpu_remove_region,
	.UpdateRegion = gx3211_vpu_update_region,

	.vpu_get_scan_line = gx3211_vpu_get_scan_line,
	.svpu_config = gx3211_svpu_config,
	.svpu_config_buf = gx3211_svpu_config_buf,
	.svpu_get_buf    = gx3211_svpu_get_buf,
	.svpu_run = gx3211_svpu_run,
	.svpu_stop = gx3211_svpu_stop,

	.SetLayerAutoZoom         = gx3211_vpu_SetLayerAutoZoom          ,
	.GetLayerAutoZoom         = gx3211_vpu_GetLayerAutoZoom          ,

	.SetGAMMACorrect          = gx3211_vpu_SetGAMMACorrect           ,
};


struct gxav_module_ops gx3211_vpu_module = {
	.module_type = GXAV_MOD_VPU,
	.count = 1,
	.irqs = {VPU_GA_INC_SRC, -1},
	.irq_names = {"ga"},
	.irq_flags = {-1},
	.init = gx_vpu_init,
	.cleanup = gx_vpu_cleanup,
	.open = gx_vpu_open,
	.close = gx_vpu_close,
	.set_property = gx_vpu_set_property,
	.get_property = gx_vpu_get_property,
	.interrupts[VPU_GA_INC_SRC] = gx_vpu_ga_interrupt,
	.priv = &gx3211_vpu_ops,
};

