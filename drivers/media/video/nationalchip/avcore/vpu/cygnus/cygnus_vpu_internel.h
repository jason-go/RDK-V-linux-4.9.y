#ifndef __CYGNUS_VPU_INTERNEL_H__
#define __CYGNUS_VPU_INTERNEL_H__

#define REGION_POOL_EN (1)

#include "vpu_hal.h"
#include "region.h"
#if REGION_POOL_EN
# include "mpool.h"
#endif

#define CYGNUS_MEM_POOL_SIZE     (500)

#define CYGNUS_MAX_XRES     (1920)
#define CYGNUS_MAX_YRES     (1080)

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
    (OsdRegion**)&(((CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_OSD].priv)->region_head)
#define SOSD_HEADPTR \
    (OsdRegion**)&(((CygnusVpuSosdPriv*)cygnusvpu_info->layer[GX_LAYER_SOSD].priv)->region_head)

#define OSD_HEADPTR_PHYS(layer_id) \
	(unsigned int)(gx_dma_to_phys((unsigned int)((((CygnusVpuOsdPriv*)cygnusvpu_info->layer[layer_id].priv)->region_head))))
#define SOSD_HEADPTR_PHYS \
	(unsigned int)(gx_dma_to_phys((unsigned int)((CygnusVpuOsdPriv*)cygnusvpu_info->layer[GX_LAYER_SOSD].priv)->region_head))

#define GET_SURFACE_MANAGER(layer_id)\
    (&(cygnusvpu_info->sm[layer_id]))

typedef enum {
    VPU_CLUT8_LEN = 0,
    VPU_CLUT4_LEN ,
    VPU_CLUT2_LEN ,
    VPU_CLUT1_LEN ,
}CygnusVpuClutLen;

typedef struct gx_vpu_layer     CygnusVpuLayer;
typedef struct gx_vpu_surface   CygnusVpuSurface;
typedef struct gx_vpu_layer_ops CygnusVpuLayerOps;

typedef struct {
	GxLayerID           layer;
	unsigned int        left;
	unsigned int        right;
	unsigned int        top;
	unsigned int        bottom;
	GxColorFormat       format;
	void                *buffer;
}CygnusVpuCe;

typedef struct {
	void*               data_address;       /*Vbi buffer */
	void*               read_address;       /*Read addr*/
	unsigned int        unit_length;        /*num in one unit*/
	int                 enable;             /*Vbi start*/
}CygnusVpuVbi;

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
	CygnusVpuSurface  *ready;
	CygnusVpuSurface  *surfaces[GXVPU_MAX_SURFACE_NUM];
}CygnusSurfaceManager;

typedef struct {
	int                         vout_is_ready;
	int                         reset_flag;
	CygnusVpuLayer              layer[GX_LAYER_MAX];
	CygnusVpuSurface            *surface;
	CygnusVpuCe                 ce;
	CygnusVpuVbi                vbi;
	GxVpuProperty_Resolution    virtual_resolution;
	gx_mutex_t                  mutex;
	CygnusSurfaceManager        sm[GX_LAYER_MAX];
	void *svpu_buffer[4];
	void *svpu_tmp_buffer[4];
	int memory_pool;
}CygnusVpu;

typedef struct {
#if REGION_POOL_EN
	void         *region_pool;
#endif
	OsdRegion    *region_head;
	ByteSequence data_byte_seq;
	ByteSequence regionhead_byte_seq;
}CygnusVpuOsdPriv;

typedef CygnusVpuOsdPriv CygnusVpuSosdPriv;

typedef struct {
    GxVpuLayerPlayMode   play_mode;
    unsigned int    stream_ratio;
    unsigned int    frame_width;
    unsigned int    frame_height;
	DispControler   controler;
}CygnusVpuVppPriv;

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

#define IS_OSD_LAYER(layer) \
	(layer == GX_LAYER_OSD || layer == GX_LAYER_SOSD || layer == GX_LAYER_SOSD2)

extern CygnusVpu *cygnusvpu_info;
extern gx_spin_lock_t cygnusvpu_spin_lock;

extern	int cygnus_vpu_VirtualToActual(int virtual, int referance, int horizontal);
extern	int cygnus_vpu_GetActualResolution(GxVpuProperty_Resolution * property);
extern	int cygnus_vpu_GetVirtualResolution(GxVpuProperty_Resolution * property);

extern void cygnus_vpu_VbiEnable         (CygnusVpuVbi *vbi);
extern void cygnus_vpu_VbiGetReadPtr     (CygnusVpuVbi *vbi);
extern int  cygnus_vpu_VbiCreateBuffer   (CygnusVpuVbi *vbi);

#define VPU_SPIN_LOCK_INIT()   gx_spin_lock_init(&cygnusvpu_spin_lock);
#define VPU_SPIN_LOCK()        gx_spin_lock_irqsave(&cygnusvpu_spin_lock, flags);
#define VPU_SPIN_UNLOCK()      gx_spin_unlock_irqrestore(&cygnusvpu_spin_lock, flags);
#define VPU_SPIN_LOCK_UNINIT(id) (void)0

int ce_init(void);
int ce_set_format(GxColorFormat format);
enum {
	CE_MODE_AUTO,
	CE_MODE_MANUAL,
};
int ce_set_mode(int mode);
int ce_set_layer(int layer_sel);
int ce_set_buf(CygnusVpuSurface *surface);
int ce_zoom(GxAvRect *clip, GxAvRect *view);
int ce_enable(int en);
int ce_bind_to_vpp2(int en);

enum {
	VPU_INT_ALL_LAYER_IDLE,
	VPU_INT_LINE,
	VPU_INT_FIELD_START,
	VPU_INT_FRAME_START,
	VPU_INT_FIELD_END,
	VPU_INT_FRAME_END,
	VPU_INT_PP0,
	VPU_INT_PP1,
	VPU_INT_PIC,
	VPU_INT_OSD0,
	VPU_INT_OSD1,
	VPU_INT_PP2,
	VPU_INT_CE,
};
int vpu_int_en(int bit);
int vpu_int_clr(int bit);
int vpu_int_dis(int bit);
int vpu_int_en_status(int bit);
int vpu_all_layers_en(int en);

int vpu_vbi_en(int en);

int vpu_dac_en(GxVpuDacID dac_id, int en);
int vpu_dac_status(GxVpuDacID dac_id);
int vpu_dac_set_source(int dac_id, GxVpuMixerID src);

#if 0
typedef enum {
	HD_MIXER,
	SD_MIXER,
} VpuMixerID;

struct mixer_layer_config {
	VpuMixerID id;
	GxLayerID  layers[5];/*逻辑层layer0至layer4*/
};
#endif

int vpu_mixer_set_layers(GxVpuMixerLayers *cfg);
int vpu_mixer_get_layers(GxVpuMixerLayers *cfg);
int vpu_mixer_set_backcolor(GxVpuMixerID id, GxColor color);
#endif
