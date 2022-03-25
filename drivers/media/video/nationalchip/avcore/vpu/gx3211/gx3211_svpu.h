#ifndef __GX3211_SVPU_H__
#define __GX3211_SVPU_H__

#include "vpu_hal.h"
#include "gxav_bitops.h"
#include "gxav_vout_propertytypes.h"

#define SVPU_REG_BASE_ADDR         (0x04900000)

#define SVPU_SURFACE_WIDTH         (720)
#define SVPU_SURFACE_HEIGHT        (576)
#define SVPU_SURFACE_BPP           (24)

#define SVPU_SURFACE_NUM           (3)
#define SVPU_SURFACE_SIZE          ((SVPU_SURFACE_WIDTH*SVPU_SURFACE_HEIGHT*SVPU_SURFACE_BPP)>>3)
#define SVPU_SURFACE_TOTLE_SIZE    ((SVPU_SURFACE_SIZE*SVPU_SURFACE_NUM)

#define SVPU_SET_FLIKER_FLITER(reg, Value) \
	REG_SET_VAL(&(reg), Value)

typedef struct svpu_buf {
	unsigned int y_topfield_addr;
	unsigned int y_bottomfield_addr;
	unsigned int u_topfield_addr;
	unsigned int u_bottomfield_addr;
	unsigned int v_topfield_addr;
	unsigned int v_bottomfield_addr;
}SvpuBuf;

typedef struct gx3211_svpu_reg {
	SvpuBuf      rBUF[SVPU_SURFACE_NUM];//0x0000~0x0044
	unsigned int rRESERVED_0[2];//0x0048~0x004C

	unsigned int rCAP_CTRL;//0x0050
	unsigned int rRESERVED_1[1];
	unsigned int rCAP_H;
	unsigned int rCAP_V;
	unsigned int rSCE_ZOOM;//0x0060
	unsigned int rSCE_ZOOM1;
	unsigned int rREQ_LENGTH;
	unsigned int rRESERVED_2[1];
	unsigned int rZOOM_PHASE;//0x0070
	unsigned int rSYS_PARA;
	unsigned int rPARA_UPDATE;
	unsigned int rRESERVED_3[1];
	unsigned int rDISP_REQ_BLOCK;//0x0080
	unsigned int rDISP_CTRL;
	unsigned int rVIEW_V;
	unsigned int rVIEW_H;
	unsigned int rVPU_FRAME_MODE;//0x0090
	unsigned int rRESERVED_4[27];
	unsigned int rSCE_PHASE_PARA[64];//0x0100~0x01FC
}Gx3211SvpuReg;

#define bSVPU_CAPMODE     (24)
#define mSVPU_CAPMODE     (0x1f<<bSVPU_CAPMODE)
#define SVPU_SET_CAPMODE(reg, capmode)\
	REG_SET_FIELD(&(reg), mSVPU_CAPMODE, capmode, bSVPU_CAPMODE)
#define bSVPU_CAPMODE_EN  (31)
#define SVPU_SET_CAPMODE_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_CAPMODE_EN);
#define SVPU_SET_CAPMODE_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_CAPMODE_EN);

#define bSVPU_CAPLEVEL    (1)
#define mSVPU_CAPLEVEL    (0x1<<bSVPU_CAPLEVEL)
#define SVPU_SET_CAPLEVEL(reg, caplevel)\
	REG_SET_FIELD(&(reg), mSVPU_CAPLEVEL, caplevel, bSVPU_CAPLEVEL)

typedef enum {
	SVPU_YUV_444,
	SVPU_YUV_422 = 3,
}SvpuYUVMode;

#define bSVPU_YUVMODE    (8)
#define mSVPU_YUVMODE    (0x3<<bSVPU_YUVMODE)
#define SVPU_SET_CAP_YUVMODE(reg, yuvmode)\
	REG_SET_FIELD(&(reg), mSVPU_YUVMODE, yuvmode, bSVPU_YUVMODE)


#define bSVPU_W_REQBLOCK  (16)
#define mSVPU_W_REQBLOCK  (0x7ff<<bSVPU_W_REQBLOCK)
#define SVPU_SET_BUFS_WRITE_REQBLOCK(reg, reqblock)\
	REG_SET_FIELD(&(reg), mSVPU_W_REQBLOCK, reqblock, bSVPU_W_REQBLOCK)

#define bSVPU_R_REQBLOCK  (16)
#define mSVPU_R_REQBLOCK  (0x7ff<<bSVPU_R_REQBLOCK)
#define SVPU_SET_BUFS_READ_REQBLOCK(reg, reqblock)\
	REG_SET_FIELD(&(reg), mSVPU_R_REQBLOCK, reqblock, bSVPU_R_REQBLOCK)

#define bSVPU_CAP_EN      (0)
#define SVPU_CAP_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_CAP_EN);
#define SVPU_CAP_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_CAP_EN);

#define bSVPU_DISP_EN     (31)
#define SVPU_DISP_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_DISP_EN);
#define SVPU_DISP_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_DISP_EN);

#define bSVPU_PARA_UPDATE (1)
#define SVPU_PARA_UPDATE(reg)\
	do {\
		REG_SET_VAL(&(reg), 1);\
		REG_SET_VAL(&(reg), 0);\
	}while(0)

#define bSVPU_SPP_EN     (31)
#define SVPU_SPP_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_SPP_EN);
#define SVPU_SPP_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_SPP_EN);

#define bSVPU_VDOWNSCALE_EN      (4)
#define SVPU_VDOWNSCALE_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_VDOWNSCALE_EN)
#define SVPU_VDOWNSCALE_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_VDOWNSCALE_EN)

#define bSVPU_CAP_LEFT    (0)
#define mSVPU_CAP_LEFT    (0x7ff<<bSVPU_CAP_LEFT)
#define SVPU_SET_CAP_LEFT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_LEFT, value, bSVPU_CAP_LEFT)

#define bSVPU_CAP_RIGHT    (16)
#define mSVPU_CAP_RIGHT    (0x7ff<<bSVPU_CAP_RIGHT)
#define SVPU_SET_CAP_RIGHT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_RIGHT, value, bSVPU_CAP_RIGHT)

#define bSVPU_CAP_TOP    (0)
#define mSVPU_CAP_TOP    (0x7ff<<bSVPU_CAP_TOP)
#define SVPU_SET_CAP_TOP(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_TOP, value, bSVPU_CAP_TOP)

#define bSVPU_CAP_BOTTOM    (16)
#define mSVPU_CAP_BOTTOM    (0x7ff<<bSVPU_CAP_BOTTOM)
#define SVPU_SET_CAP_BOTTOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_CAP_BOTTOM, value, bSVPU_CAP_BOTTOM)

#define bSVPU_SCE_VZOOM    (0)
#define mSVPU_SCE_VZOOM    (0x3fff<<bSVPU_SCE_VZOOM)
#define SVPU_SET_SCE_VZOOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_VZOOM, value, bSVPU_SCE_VZOOM)

#define bSVPU_SCE_HZOOM    (16)
#define mSVPU_SCE_HZOOM    (0x3fff<<bSVPU_SCE_HZOOM)
#define SVPU_SET_SCE_HZOOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_HZOOM, value, bSVPU_SCE_HZOOM)

#define bSVPU_SCE_ZOOM_LENGTH    (0)
#define mSVPU_SCE_ZOOM_LENGTH    (0x7ff<<bSVPU_SCE_ZOOM_LENGTH)
#define SVPU_SET_SCE_ZOOM_LENGTH(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_ZOOM_LENGTH, value, bSVPU_SCE_ZOOM_LENGTH)
#define SVPU_GET_SCE_ZOOM_LENGTH(reg)\
	REG_GET_FIELD(&(reg), mSVPU_SCE_ZOOM_LENGTH, bSVPU_SCE_ZOOM_LENGTH)

#define bSVPU_SCE_ZOOM_HEIGHT    (16)
#define mSVPU_SCE_ZOOM_HEIGHT    (0x7ff<<bSVPU_SCE_ZOOM_HEIGHT)
#define SVPU_SET_SCE_ZOOM_HEIGHT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_ZOOM_HEIGHT, value, bSVPU_SCE_ZOOM_HEIGHT)
#define SVPU_GET_SCE_ZOOM_HEIGHT(reg)\
	REG_GET_FIELD(&(reg), mSVPU_SCE_ZOOM_HEIGHT, bSVPU_SCE_ZOOM_HEIGHT)

#define bSVPU_SCE_VT_PHASE    (16)
#define mSVPU_SCE_VT_PHASE    (0x3fff<<bSVPU_SCE_VT_PHASE)
#define SVPU_SET_SCE_VT_PHASE(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_VT_PHASE, value, bSVPU_SCE_VT_PHASE)

#define bSVPU_SCE_VB_PHASE    (0)
#define mSVPU_SCE_VB_PHASE    (0x3fff<<bSVPU_SCE_VB_PHASE)
#define SVPU_SET_SCE_VB_PHASE(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SCE_VB_PHASE, value, bSVPU_SCE_VB_PHASE)

#define bSVPU_SPP_LEFT    (16)
#define mSVPU_SPP_LEFT    (0x7ff<<bSVPU_SPP_LEFT)
#define SVPU_SET_SPP_LEFT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_LEFT, value, bSVPU_SPP_LEFT)

#define bSVPU_SPP_RIGHT    (0)
#define mSVPU_SPP_RIGHT    (0x7ff<<bSVPU_SPP_RIGHT)
#define SVPU_SET_SPP_RIGHT(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_RIGHT, value, bSVPU_SPP_RIGHT)

#define bSVPU_SPP_TOP    (16)
#define mSVPU_SPP_TOP    (0x7ff<<bSVPU_SPP_TOP)
#define SVPU_SET_SPP_TOP(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_TOP, value, bSVPU_SPP_TOP)

#define bSVPU_SPP_BOTTOM    (0)
#define mSVPU_SPP_BOTTOM    (0x7ff<<bSVPU_SPP_BOTTOM)
#define SVPU_SET_SPP_BOTTOM(reg, value)\
	REG_SET_FIELD(&(reg), mSVPU_SPP_BOTTOM, value, bSVPU_SPP_BOTTOM)


/**
 * SCE module dis/en able
 * rCAP_CTRL
 */
#define bSVPU_CAP_SCE_EN     (0)
#define SVPU_CAP_SCE_ENABLE(reg)\
	REG_SET_BIT(&(reg), bSVPU_CAP_SCE_EN);
#define SVPU_CAP_SCE_DISABLE(reg)\
	REG_CLR_BIT(&(reg), bSVPU_CAP_SCE_EN);

typedef enum {
	FRAME_MODE_2 = 0,
	FRAME_MODE_3 = 1,
}SvpuFrameMode;
#define bSVPU_FRAME_MODE (0)
#define SVPU_SET_FRAME_MODE(reg, mode)\
	if(mode == FRAME_MODE_2)\
		REG_CLR_BIT(&(reg), bSVPU_FRAME_MODE);\
	else\
		REG_SET_BIT(&(reg), bSVPU_FRAME_MODE);


typedef enum {
	SVPU_LAYER_MIX,
	SVPU_LAYER_SPP,
}SvpuLayerSelect;

int gx3211_svpu_init(void);
int gx3211_svpu_cleanup(void);
int gx3211_svpu_get_sce_length(void);
int gx3211_svpu_get_sce_height(void);
int gx3211_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info);
int gx3211_svpu_config_buf(SvpuSurfaceInfo *buf_info);
int gx3211_svpu_get_buf(SvpuSurfaceInfo *buf_info);
int gx3211_svpu_run(void);
int gx3211_svpu_stop(void);
int gx3211_svpu_set_video_dac(int enable);

#endif
