#include "region.h"
#include "kernelcalls.h"
#include "gxav_vpu_propertytypes.h"

#include "vpu_color.h"
#include "cygnus_vpu_reg.h"
#include "cygnus_vpu_internel.h"

#if REGION_POOL_EN
# include "mpool.h"
#endif

extern struct osd_reg*     get_reg_by_layer_id(GxLayerID layer_id);
extern struct osd_region** cygnus_get_region_head_by_layer_id(GxLayerID layer_id);

static int region_set_alpha      (OsdRegion *region, GxAlpha alpha);
static int region_set_palette    (OsdRegion *region, GxPalette *palette);
static int region_set_colorkey   (OsdRegion *region, GxColor color, int enable, GxLayerID layer_id);
static int region_set_colorformat(OsdRegion *region, GxColorFormat format, GxLayerID layer_id);

static OsdRegion* region_alloc (void);
static void       region_free  (OsdRegion *region);
static void       region_config(GxLayerID layer_id, OsdRegion *region, VpuSurface *surface, unsigned pos_x, unsigned pos_y);
static int        region_submit(GxLayerID layer_id, OsdRegion *region);
static OsdRegion* region_revoke(GxLayerID layer_id, VpuSurface *surface);


static int region_set_alpha(OsdRegion *region, GxAlpha alpha)
{
	if(alpha.type == GX_ALPHA_GLOBAL) {
		VPU_OSD_GLOBAL_ALHPA_ENABLE(region->word1);
		VPU_OSD_SET_MIX_WEIGHT(region->word1, alpha.value);
		VPU_OSD_SET_ALPHA_RATIO_DISABLE(region->word7);
	} else {
		VPU_OSD_GLOBAL_ALHPA_DISABLE(region->word1);
		//patch, 0xFF should equal with ratio_disable, but actually not
		if(alpha.value == 0xFF) {
			VPU_OSD_SET_ALPHA_RATIO_DISABLE(region->word7);
		} else {
			VPU_OSD_SET_ALPHA_RATIO_VALUE(region->word7, alpha.value);
			VPU_OSD_SET_ALPHA_RATIO_ENABLE(region->word7);
		}
	}

	return 0;
}

static int region_set_palette(OsdRegion *region, GxPalette *palette)
{
	int clut_len;

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
	VPU_OSD_SET_CLUT_PTR(region->word2, gx_virt_to_phys((unsigned int)palette->entries));
	VPU_OSD_SET_CLUT_LENGTH(region->word1, clut_len);
	VPU_OSD_CLUT_UPDATA_ENABLE(region->word1);

	return 0;
}

static int region_set_colorkey_en(OsdRegion *region, int enable)
{
	if(enable == 0) {
		VPU_OSD_COLOR_KEY_DISABLE(region->word1);
	} else {
		VPU_OSD_COLOR_KEY_ENABLE(region->word1);
	}

	return 0;
}

static int region_set_colorkey(OsdRegion *region, GxColor color, int enable, GxLayerID layer_id)
{
	extern int cygnus_osdx_color_key(GxLayerID layer_id, GxColor *color, int enable);

	region_set_colorkey_en(region, enable);
	return cygnus_osdx_color_key(layer_id, &color, enable);
}

#define IS_YUV_COLOR_SPACE(format) \
	(format == GX_COLOR_FMT_YCBCRA6442 ||\
	 format == GX_COLOR_FMT_AYCBCR2644 ||\
	 format == GX_COLOR_FMT_ACRCBY2446 ||\
	 format == GX_COLOR_FMT_YUVA8888   ||\
	 format == GX_COLOR_FMT_AYUV8888   ||\
	 format == GX_COLOR_FMT_AVUY8888 )

#define IS_AYUV(format) \
	 (format == GX_COLOR_FMT_AYCBCR2644 || format == GX_COLOR_FMT_AYUV8888)

#define IS_AVUY(format) \
	 (format == GX_COLOR_FMT_ACRCBY2446 || format == GX_COLOR_FMT_AVUY8888)

static int region_set_colorformat(OsdRegion *region, GxColorFormat format, GxLayerID layer_id)
{
	int ret = 0;
	int true_color_mode;
	int format_code = 0;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);

	if (format <= GX_COLOR_FMT_CLUT8) {
		REG_CLR_BIT(&(reg->rOSD_CTRL), b_OSD_ZOOM_MODE_EN_IPS);
	} else {
		REG_SET_BIT(&(reg->rOSD_CTRL), b_OSD_ZOOM_MODE_EN_IPS);
	}

	if (IS_YUV_COLOR_SPACE(format)) {
		if (layer_id != GX_LAYER_OSD && layer_id != GX_LAYER_SOSD && layer_id != GX_LAYER_SOSD2) {
			ret = -1;
			goto exit;
		}

		VPU_OSD_SET_YUV_MODE(region->word1, 1);
		format_code = (gx_color_get_bpp(format) == 32 ? 7 : 6);
		VPU_OSD_SET_COLOR_TYPE(region->word1, format_code);
		VPU_OSD_SET_ARGB_CONVERT(region->word1, IS_AYUV(format));
		VPU_OSD_SET_ABGR_CONVERT(region->word1, IS_AVUY(format));
	} else {
		VPU_OSD_SET_YUV_MODE(region->word1, 0);
		if((format >= GX_COLOR_FMT_RGBA8888)&&(format <= GX_COLOR_FMT_BGR888)) {
			true_color_mode = format - GX_COLOR_FMT_RGBA8888;
			VPU_OSD_SET_COLOR_TYPE(region->word1, 0x7);
			VPU_OSD_SET_TRUE_COLOR_MODE(region->word1, true_color_mode);
			if(GX_COLOR_FMT_ABGR8888 == format)
				VPU_OSD_SET_ABGR_CONVERT(region->word1, 1);
			else
				VPU_OSD_SET_ABGR_CONVERT(region->word1, 0);
		} else {
			if( (format==GX_COLOR_FMT_ARGB4444)||
					(format==GX_COLOR_FMT_ARGB1555)||
					(format==GX_COLOR_FMT_ARGB8888))
				VPU_OSD_SET_ARGB_CONVERT(region->word1,1);
			else
				VPU_OSD_SET_ARGB_CONVERT(region->word1,0);

			if(format == GX_COLOR_FMT_ABGR8888 || format == GX_COLOR_FMT_ABGR4444 || format == GX_COLOR_FMT_ABGR1555)
				VPU_OSD_SET_ABGR_CONVERT(region->word1, 1);
			else
				VPU_OSD_SET_ABGR_CONVERT(region->word1, 0);

			switch(format) {
			case GX_COLOR_FMT_ABGR4444:
			case GX_COLOR_FMT_ARGB4444:
				format_code = GX_COLOR_FMT_RGBA4444;
				break;
			case GX_COLOR_FMT_ABGR1555:
			case GX_COLOR_FMT_ARGB1555:
				format_code = GX_COLOR_FMT_RGBA5551;
				break;
			case GX_COLOR_FMT_ABGR8888:
			case GX_COLOR_FMT_ARGB8888:
				format_code = GX_COLOR_FMT_RGBA8888;
				break;
			default:
				format_code = format;
				break;
			}
			VPU_OSD_SET_COLOR_TYPE(region->word1, format_code);
		}
	}

exit:
	return ret;
}

static OsdRegion* region_alloc(void)
{
#ifdef NO_OS
#include <common/get_mem_info.h>
	extern unsigned int gx_virt_to_dma(unsigned int virt);
	struct mem_info info = {OSD_BUF, 0, 0};
	get_mem_info(&info);
	info.addr = gx_virt_to_dma(info.addr);
	OsdRegion *region = (OsdRegion*)(info.addr + info.len - 1024);
#else
#if REGION_POOL_EN
	CygnusVpuOsdPriv *priv = (CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_OSD].priv;
	OsdRegion *region = (OsdRegion*)mpool_alloc(priv->region_pool, sizeof(OsdRegion));
#else
	OsdRegion *region = (OsdRegion*)gx_dma_malloc(sizeof(OsdRegion));
#endif
#endif

	if (region) {
		gx_memset(region, 0, sizeof(OsdRegion));
	}

	return region;
}

static void region_free(OsdRegion *region)
{
	if (region) {
#if REGION_POOL_EN
		CygnusVpuOsdPriv *priv = (CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_OSD].priv;
		mpool_free(priv->region_pool, region);
#else
		gx_dma_free(region, sizeof(OsdRegion));
#endif
	}
}

static void region_config(GxLayerID layer_id, OsdRegion *region, VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	if (region && surface) {
		gx_memset(region, 0, sizeof(OsdRegion));
		region->pos_x   = pos_x;
		region->pos_y   = pos_y;
		region->width   = surface->width;
		region->height  = surface->height;
		region->surface = surface;

		region_set_alpha(region, surface->alpha);
		region_set_colorkey(region, surface->color_key, surface->color_key_en, layer_id);
		region_set_colorformat(region, surface->color_format, layer_id);
		if(IS_REFERENCE_COLOR(surface->color_format) && (surface->palette)) {
			region_set_palette(region, surface->palette);
		}
		VPU_OSD_SET_WIDTH(region->word3, region->pos_x, region->width + region->pos_x - 1);
		VPU_OSD_SET_HEIGHT(region->word4, region->pos_y, region->height + region->pos_y - 1);
		VPU_OSD_SET_DATA_ADDR(region->word5, gx_virt_to_phys((unsigned int)surface->buffer));
		/*
		VPU_OSD_LIST_END_ENABLE(region->word7);
		*/
		VPU_OSD_SET_BASE_LINE(region->word7, region->width);
	}
}

static void region_remove_all(GxLayerID layer_id)
{
	struct osd_region **p_region_head = cygnus_get_region_head_by_layer_id(layer_id);
	struct osd_region *region = NULL, *next = NULL;

	region = *p_region_head;
	next = region->next;
	while(region) {
		region = next;
		if(region) {
			next = region->next;
		}
		region_free(region);
	}
	*p_region_head = NULL;
}

static int region_submit(GxLayerID layer_id, OsdRegion *region)
{
	int ret = 0;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region **p_region_head = cygnus_get_region_head_by_layer_id(layer_id);

	if (*p_region_head == NULL) {
		*p_region_head = region;
		VPU_OSD_LIST_END_ENABLE(region->word7);
		REG_SET_FIELD(&(reg->rOSD_FIRST_HEAD_PTR), m_OSD_FIRST_HEAD_PTR, OSD_HEADPTR_PHYS(layer_id), b_OSD_FIRST_HEAD_PTR);
	} else {
		OsdRegion *visit = *p_region_head;
		OsdRegion *pre = visit;
		while(visit && visit->pos_y < region->pos_y) {
			pre = visit;
			visit = visit->next;
		}

		if (visit) {
			//insert to the head
			if (visit == *p_region_head) {
				region->next  = *p_region_head;
				region->word6 = (unsigned)gx_dma_to_phys((unsigned int)(*p_region_head));
				*p_region_head = region;
				REG_SET_FIELD(&(reg->rOSD_FIRST_HEAD_PTR), m_OSD_FIRST_HEAD_PTR, OSD_HEADPTR_PHYS(layer_id), b_OSD_FIRST_HEAD_PTR);
			} else if ((region->pos_y + region->height > visit->pos_y) || (pre->pos_y + pre->height > region->pos_y)) {
				ret = -1;
				gx_printf("%s: bad region x/y/w/h!\n", __func__);
			} else {
				//insert between pre and visit
				pre->next     = region;
				pre->word6    = (unsigned)gx_dma_to_phys((unsigned int)region);
				region->next  = visit;
				region->word6 = (unsigned)gx_dma_to_phys((unsigned int)visit);
				VPU_OSD_LIST_END_DISABLE(pre->word7);
			}
		} else {
			//insert to the end
			pre->next  = region;
			pre->word6 = (unsigned)gx_dma_to_phys((unsigned int)region);
			VPU_OSD_LIST_END_DISABLE(pre->word7);
			VPU_OSD_LIST_END_ENABLE(region->word7);
		}
	}

	REG_SET_BIT(&(reg->rOSD_CTRL), b_OSD_REGION_MODE);

	return ret;
}

static OsdRegion* region_revoke(GxLayerID layer_id, VpuSurface *surface)
{
	OsdRegion *visit = NULL;
	struct cygnus_vpu_osd_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_OSD0);
	struct osd_region **p_region_head = cygnus_get_region_head_by_layer_id(layer_id);

	if (surface && *p_region_head) {
		visit = *p_region_head;
		if ((*p_region_head)->surface == surface) {
			*p_region_head = (*p_region_head)->next;
			REG_SET_FIELD(&(reg->rOSD_FIRST_HEAD_PTR), m_OSD_FIRST_HEAD_PTR, OSD_HEADPTR_PHYS(layer_id), b_OSD_FIRST_HEAD_PTR);
		} else {
			OsdRegion *pre = visit;
			while(visit && visit->surface != surface) {
				pre   = visit;
				visit = visit->next;
			}
			if (visit) {
				pre->next  = visit->next;
				pre->word6 = (unsigned)gx_dma_to_phys((unsigned int)(visit->next));
				if (pre->next == NULL) {
					VPU_OSD_LIST_END_ENABLE(pre->word7);
				}
			}
		}
	}

	return visit;
}

#if 0
	#define LS_REGION(desc)\
	do {\
		unsigned cnt = 0;\
		OsdRegion **p_region_header = OSD_HEADPTR;\
		gx_printf("\n\nls_region: %s\n", desc);\
		if (*p_region_header) {\
			OsdRegion *visit = *p_region_header;\
			while(visit) {\
				gx_printf("\nls_region: [%d] 0x%x\n", ++cnt, visit);\
				visit = visit->next;\
			}\
		}\
		gx_printf("\nls_region: totle = %d\n", cnt);\
	} while(0);
#else
	#define LS_REGION(desc) {;}
#endif

int cygnus_region_add(GxLayerID layer_id, VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	int ret = -1;
	OsdRegion *region = NULL;

	LS_REGION("before-add");
	if (surface) {
		if ((region = region_alloc()) != NULL) {
			region_config(layer_id, region, surface, pos_x, pos_y);
			ret = region_submit(layer_id, region);
		} else {
			region_remove_all(layer_id);
			if ((region = region_alloc()) != NULL) {
				region_config(layer_id, region, surface, pos_x, pos_y);
				ret = region_submit(layer_id, region);
			}
		}
	}
	LS_REGION("after-add");

	return ret;
}

int cygnus_region_reconfig(GxLayerID layer_id, OsdRegion *region, VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	int ret = -1;

	if((NULL == region) || (NULL == surface)) {
		gx_printf("------reconfig failed--------\n");
		return (ret);
	}

	LS_REGION("before-rcfg");
	region_config(layer_id, region, surface, pos_x, pos_y);
	ret = region_submit(layer_id, region);
	LS_REGION("after-rcfg");

	return (ret);
}

OsdRegion *cygnus_region_revoke(GxLayerID layer_id, VpuSurface *surface)
{
	OsdRegion *region = NULL;

	LS_REGION("before-rvk");
	if (surface) {
		region = region_revoke(layer_id, surface);
	}
	LS_REGION("after-rvk");

	return region;
}

int cygnus_region_remove(GxLayerID layer_id, VpuSurface *surface)
{
	int ret = -1;
	OsdRegion *region = NULL;

	LS_REGION("before-rm");
	if (surface) {
		if ((region = region_revoke(layer_id, surface)) != NULL) {
			region_free(region);
			ret = 0;
		}
	}
	LS_REGION("after-rm");

	return ret;
}

int cygnus_region_update(GxLayerID layer_id, VpuSurface *surface, unsigned pos_x, unsigned pos_y)
{
	int ret = -1;

	LS_REGION("before-update");
	if (surface) {
#if 0
		OsdRegion* region = NULL;
		if ((region = region_revoke(surface)) != NULL) {
			region_config(region, surface, pos_x, pos_y);
			ret = region_submit(region);
		}
#else
		OsdRegion *new_region = NULL, *old_region = NULL;
		if ( (new_region = region_alloc()) != NULL) {
			region_config(layer_id, new_region, surface, pos_x, pos_y);
			old_region = region_revoke(layer_id, surface);
			ret = region_submit(layer_id, new_region);
			region_free(old_region);
		}
#endif
	}
	LS_REGION("after-update");

	return ret;
}

