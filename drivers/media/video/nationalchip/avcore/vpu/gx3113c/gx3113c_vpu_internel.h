#ifndef __GX3113c_VPU_INTERNEL_H__
#define __GX3113c_VPU_INTERNEL_H__

#include "vpu_hal.h"

#define GX3113c_MAX_XRES     (720)
#define GX3113c_MAX_YRES     (576)

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
	(OsdRegionHead*)&(((Gx3113cVpuOsdPriv*)gx3113cvpu_info->layer[GX_LAYER_OSD].priv)->region_head)

#define OSD_HEADPTR_PHYS \
	(unsigned int)(&(((Gx3113cVpuOsdPriv*)gx3113cvpu_info->layer[GX_LAYER_OSD].priv)->region_head)) - \
(unsigned int)(gx3113cvpu_info->layer[GX_LAYER_OSD].priv) + \
(unsigned int)(gx_dma_to_phys((unsigned int)(gx3113cvpu_info->layer[GX_LAYER_OSD].priv)))

#define SURFACE_MANAGER \
	(Gx3113cSurfaceManager*)&(((Gx3113cVpuOsdPriv*)gx3113cvpu_info->layer[GX_LAYER_OSD].priv)->sm)

typedef enum {
	VPU_CLUT8_LEN = 0,
	VPU_CLUT4_LEN ,
	VPU_CLUT2_LEN ,
	VPU_CLUT1_LEN ,
}Gx3113cVpuClutLen;

typedef struct gx_vpu_layer     Gx3113cVpuLayer;
typedef struct gx_vpu_surface   Gx3113cVpuSurface;
typedef struct gx_vpu_layer_ops Gx3113cVpuLayerOps;

typedef struct {
	GxLayerID           layer;
	unsigned int        left;
	unsigned int        right;
	unsigned int        top;
	unsigned int        bottom;
	GxColorFormat       format;
	void                *buffer;
}Gx3113cVpuCe;

typedef struct {
	void*               data_address;       /*Vbi buffer */
	void*               read_address;       /*Read addr*/
	unsigned int        unit_length;        /*num in one unit*/
	int                 enable;             /*Vbi start*/
}Gx3113cVpuVbi;

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
	int                       vout_is_ready;
	Gx3113cVpuLayer           layer[GX_LAYER_MAX];
	Gx3113cVpuSurface         *surface;
	Gx3113cVpuCe              ce;
	Gx3113cVpuVbi             vbi;
	GxVpuProperty_Resolution  virtual_resolution;
	gx_mutex_t                mutex;
}Gx3113cVpu;


typedef struct {
	int                interrupt_en;
	int                force_flip_gate;
	volatile int       showing;
	volatile int       cur_time;
	volatile int       ready_time;
	Gx3113cVpuSurface *ready;
	Gx3113cVpuSurface *surfaces[GXVPU_MAX_SURFACE_NUM];
}Gx3113cSurfaceManager;

typedef struct {
	OsdRegionHead         region_head;
	ByteSequence          data_byte_seq;
	ByteSequence          regionhead_byte_seq;
	Gx3113cSurfaceManager sm;
}Gx3113cVpuOsdPriv;

typedef struct {
	GxVpuLayerPlayMode   play_mode;
	unsigned int    stream_ratio;
	unsigned int    frame_width;
	unsigned int    frame_height;
}Gx3113cVpuVppPriv;

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
	(((alu) < GX_ALU_ROP_CLEAR) || ((alu) > GX_ALU_VECTOR_FONT_COPY) || ((alu) == GX_ALU_MIX_TRUE))

#define IS_REFERENCE_COLOR(color)              \
	(((color) >= GX_COLOR_FMT_CLUT1) && ((color) <= GX_COLOR_FMT_CLUT8))

#define IS_INVAILD_COLOR(color)                \
	(((color) < GX_COLOR_FMT_CLUT1) || ((color) > GX_COLOR_FMT_YCBCR444_UV))

#define IS_INVAILD_ZOOM_COLOR(color)	\
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

extern volatile Gx3113cVpuReg    *gx3113cvpu_reg;
extern          Gx3113cVpu       *gx3113cvpu_info;

extern	int gx3113c_vpu_VirtualToActual(int virtual, int referance, int horizontal);
extern	int gx3113c_vpu_GetActualResolution(GxVpuProperty_Resolution * property);
extern	int gx3113c_vpu_GetVirtualResolution(GxVpuProperty_Resolution * property);
extern	int gx3113c_vpu_patch_green_screen(void);

extern void gx3113c_vpu_VbiEnable         (Gx3113cVpuVbi *vbi);
extern void gx3113c_vpu_VbiGetReadPtr     (Gx3113cVpuVbi *vbi);
extern int  gx3113c_vpu_VbiCreateBuffer   (Gx3113cVpuVbi *vbi);

#endif
