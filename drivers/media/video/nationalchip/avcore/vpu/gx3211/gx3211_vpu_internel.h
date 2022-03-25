#ifndef __GX3211_VPU_INTERNEL_H__
#define __GX3211_VPU_INTERNEL_H__

#define REGION_POOL_EN (1)

#include "vpu_hal.h"
#include "region.h"
#if REGION_POOL_EN
# include "mpool.h"
#endif

#define GX3211_MAX_XRES     (1920)
#define GX3211_MAX_YRES     (1080)

#ifdef GX_VPU_DEBUG
    #define VPU_DBG(fmt, args...) \
    do { \
		gx_printf("\n[VPU][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)
#else
    #define VPU_DBG(fmt, args...)   ((void)0)
#endif

#define OSD_HEADPTR \
    (OsdRegion**)&(((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->region_head)
#define SOSD_HEADPTR \
    (OsdRegion**)&(((Gx3211VpuSosdPriv*)gx3211vpu_info->layer[GX_LAYER_SOSD].priv)->region_head)

#define OSD_HEADPTR_PHYS \
	(unsigned int)(gx_dma_to_phys((unsigned int)((((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_OSD].priv)->region_head))))
#define SOSD_HEADPTR_PHYS \
	(unsigned int)(gx_dma_to_phys((unsigned int)((Gx3211VpuOsdPriv*)gx3211vpu_info->layer[GX_LAYER_SOSD].priv)->region_head))

#define GET_SURFACE_MANAGER(layer_id)\
    (&(gx3211vpu_info->sm[layer_id]))

typedef enum {
    VPU_CLUT8_LEN = 0,
    VPU_CLUT4_LEN ,
    VPU_CLUT2_LEN ,
    VPU_CLUT1_LEN ,
}Gx3211VpuClutLen;

typedef struct gx_vpu_layer     Gx3211VpuLayer;
typedef struct gx_vpu_surface   Gx3211VpuSurface;
typedef struct gx_vpu_layer_ops Gx3211VpuLayerOps;

typedef struct {
	GxLayerID           layer;
	unsigned int        left;
	unsigned int        right;
	unsigned int        top;
	unsigned int        bottom;
	GxColorFormat       format;
	void                *buffer;
}Gx3211VpuCe;

typedef struct {
	void*               data_address;       /*Vbi buffer */
	void*               read_address;       /*Read addr*/
	unsigned int        unit_length;        /*num in one unit*/
	int                 enable;             /*Vbi start*/
}Gx3211VpuVbi;

typedef enum {
	DCBA_HGFE = 0,
	EFGH_ABCD = 1,
	HGFE_DCBA = 2,
	ABCD_EFGH = 3,
	CDAB_GHEF = 4,
	FEHG_BADC = 5,
	GHEF_CDAB = 6,
	BADC_FEHG = 7,
}ByteSequence;

typedef struct {
	int                interrupt_en;
	int                force_flip_gate;
	volatile int       showing;
	volatile int       cur_time;
	volatile int       ready_time;
	Gx3211VpuSurface  *ready;
	Gx3211VpuSurface  *surfaces[GXVPU_MAX_SURFACE_NUM];
}Gx3211SurfaceManager;

typedef struct {
	int                         vout_is_ready;
	int                         reset_flag;
	Gx3211VpuLayer              layer[GX_LAYER_MAX];
	Gx3211VpuSurface            *surface;
	Gx3211VpuCe                 ce;
	Gx3211VpuVbi                vbi;
	GxVpuProperty_Resolution    virtual_resolution;
	gx_mutex_t                  mutex;
	Gx3211SurfaceManager        sm[5];
	void *svpu_buffer[4];
	void *svpu_tmp_buffer[4];
}Gx3211Vpu;

typedef struct {
#if REGION_POOL_EN
	void         *region_pool;
#endif
	OsdRegion    *region_head;
	ByteSequence data_byte_seq;
	ByteSequence regionhead_byte_seq;
}Gx3211VpuOsdPriv;

typedef Gx3211VpuOsdPriv Gx3211VpuSosdPriv;

typedef struct {
    GxVpuLayerPlayMode   play_mode;
    unsigned int    stream_ratio;
    unsigned int    frame_width;
    unsigned int    frame_height;
	DispControler   controler;
}Gx3211VpuVppPriv;

#define IS_NULL(pointer)                       \
	(pointer == NULL)

#define IS_INVAILD_LAYER(layer)                \
	(((layer) < GX_LAYER_OSD) || ((layer) > GX_LAYER_BKG))

#define IS_MAIN_SURFACE(sur)                   \
	((sur)->layer != NULL && (sur)->layer->surface == sur)

#define IS_VIDEO_SURFACE(sur)                  \
	((sur)->surface_mode == GX_SURFACE_MODE_VIDEO)

#define IS_IMAGE_SURFACE(sur)                  \
	((sur)->surface_mode == GX_SURFACE_MODE_IMAGE)

#define IS_INVAILD_IMAGE_COLOR(color)          \
	(color == GX_COLOR_FMT_YCBCR420)

#define IS_INVAILD_GA_COLOR(color)             \
	(((color) == GX_COLOR_FMT_RGB888) || ((color) == GX_COLOR_FMT_BGR888))

#define IS_INVAILD_POINT(point,xres,yres)      \
	(((point)->x >= xres) || ((point)->y >= yres))

#define IS_INVAILD_RECT(rect,xres,yres)        \
	(((rect)->x + (rect)->width  > xres)   \
	 ||((rect)->y + (rect)->height > yres) \
	 ||((rect)->x   >= xres)               \
	 ||((rect)->y   >= yres)               \
	 ||((rect)->width   == 0)              \
	 ||((rect)->height  == 0)              \
	 ||((rect)->width   > xres)            \
	 ||((rect)->height   > yres))

#define IS_INVALID_ALU_MODE(alu)               \
	(((alu) < GX_ALU_ROP_CLEAR) || ((alu) >GX_ALU_VECTOR_FONT_COPY))

#define IS_REFERENCE_COLOR(color)              \
	(((color) >= GX_COLOR_FMT_CLUT1) && ((color) <= GX_COLOR_FMT_CLUT8))

#define IS_INVAILD_COLOR(color)                \
	(((color) < GX_COLOR_FMT_CLUT1) || ((color) > GX_COLOR_FMT_YCBCR444_UV))

#define IS_INVAILD_ZOOM_SRC_COLOR(color)	\
	(	(color != GX_COLOR_FMT_RGB565)\
	&&	(color != GX_COLOR_FMT_RGBA4444)\
	&&	(color != GX_COLOR_FMT_RGBA5551)\
	&&	(color != GX_COLOR_FMT_RGBA8888)\
	&&	(color != GX_COLOR_FMT_ARGB8888)\
	&&	(color != GX_COLOR_FMT_ARGB4444)\
	&&	(color != GX_COLOR_FMT_ABGR4444)\
	&&	(color != GX_COLOR_FMT_ARGB1555)\
	&&	(color != GX_COLOR_FMT_ABGR1555)\
	&&	(color != GX_COLOR_FMT_A8)\
	&&	(color != GX_COLOR_FMT_Y8)\
	&&	(color != GX_COLOR_FMT_YCBCR422v)\
	&&	(color != GX_COLOR_FMT_YCBCR422h)\
	&&	(color != GX_COLOR_FMT_YUVA8888)\
	&&	(color != GX_COLOR_FMT_YCBCRA6442))

#define IS_INVAILD_ZOOM_DST_COLOR(color)	\
	(	(color != GX_COLOR_FMT_RGB565)\
	&&	(color != GX_COLOR_FMT_RGBA4444)\
	&&	(color != GX_COLOR_FMT_RGBA5551)\
	&&	(color != GX_COLOR_FMT_RGBA8888)\
	&&	(color != GX_COLOR_FMT_ARGB8888)\
	&&	(color != GX_COLOR_FMT_ARGB4444)\
	&&	(color != GX_COLOR_FMT_ABGR4444)\
	&&	(color != GX_COLOR_FMT_ARGB1555)\
	&&	(color != GX_COLOR_FMT_ABGR1555)\
	&&	(color != GX_COLOR_FMT_A8)\
	&&	(color != GX_COLOR_FMT_Y8)\
	&&	(color != GX_COLOR_FMT_YUVA8888)\
	&&	(color != GX_COLOR_FMT_YCBCRA6442))

#define IS_INVAILD_TURN_COLOR(color) \
	(	(color != GX_COLOR_FMT_RGB565)\
	&&	(color != GX_COLOR_FMT_RGBA4444)\
	&&	(color != GX_COLOR_FMT_RGBA5551)\
	&&	(color != GX_COLOR_FMT_RGBA8888)\
	&&	(color != GX_COLOR_FMT_ARGB8888)\
	&&	(color != GX_COLOR_FMT_ARGB4444)\
	&&	(color != GX_COLOR_FMT_ABGR4444)\
	&&	(color != GX_COLOR_FMT_ARGB1555)\
	&&	(color != GX_COLOR_FMT_ABGR1555)\
	&&	(color != GX_COLOR_FMT_A8)\
	&&	(color != GX_COLOR_FMT_Y8)\
	&&	(color != GX_COLOR_FMT_CLUT8)\
	&&	(color != GX_COLOR_FMT_YCBCRA6442))

extern volatile Gx3211VpuReg    *gx3211vpu_reg;
extern          Gx3211Vpu       *gx3211vpu_info;

extern	int gx3211_vpu_VirtualToActual(int virtual, int referance, int horizontal);
extern	int gx3211_vpu_ActualToVirtual(int actual, int referance, int horizontal);
extern	int gx3211_vpu_GetActualResolution(GxVpuProperty_Resolution * property);
extern	int gx3211_vpu_GetVirtualResolution(GxVpuProperty_Resolution * property);

extern void gx3211_vpu_VbiEnable         (Gx3211VpuVbi *vbi);
extern void gx3211_vpu_VbiGetReadPtr     (Gx3211VpuVbi *vbi);
extern int  gx3211_vpu_VbiCreateBuffer   (Gx3211VpuVbi *vbi);

#endif
