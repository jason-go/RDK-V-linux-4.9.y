#ifndef __SIRIUS_VPU_REG_H__
#define __SIRIUS_VPU_REG_H__

#include "gxav_bitops.h"
#define VPU_MPW  (0)
#define VPU_NRE  (1)

//#define VPU_REG_BASE_ADDR         (0x89500000)
//#define VPU_REG_BASE_ADDR         (0x8A800000)

#define OSD_ADDR_MASK             (0xFFFFFFFF)
#define OSD_BASE_LINE_MASK        (0x0001FFF)
#define SPP_ADDR_MASK             (0xFFFFFFFF)
#define VPP_ADDR_MASK             (0xFFFFFFFF)
#define VBI_ADDR_MASK             (0xFFFFFFF)
#define CE_ADDR_MASK              (0xFFFFFFFF)

struct osd_reg {
	unsigned int rOSD_CTRL;//0x0090
	unsigned int rOSD_FIRST_HEAD_PTR;
	unsigned int rOSD_VIEW_SIZE;
	unsigned int rOSD_ZOOM;
	unsigned int rOSD_COLOR_KEY;//0x00A0
	unsigned int rOSD_ALPHA_5551;
	unsigned int rOSD_PHASE_SIGN;
	unsigned int rOSD_POSITION;
	unsigned int rOSD_PARA;//0x00B0
	unsigned int rOSD_PHASE_0_H;
	unsigned int rOSD_PHASE_0_V;
	unsigned int rOSD_PHASE_BIAS;
};

typedef struct _enhance_reg {
	unsigned int rGAMMA_COEF00; //0x4110
	unsigned int rGAMMA_COEF01;
	unsigned int rGAMMA_COEF02;
	unsigned int rGAMMA_COEF03;
	unsigned int rGAMMA_COEF04;

	unsigned int rGAMMA_RESERVED0[3];

	unsigned int rGAMMA_COEF10; //0x4130
	unsigned int rGAMMA_COEF11;
	unsigned int rGAMMA_COEF12;
	unsigned int rGAMMA_COEF13;
	unsigned int rGAMMA_COEF14;

	unsigned int rGAMMA_RESERVED1[3];

	unsigned int rGAMMA_COEF20; //0x4150
	unsigned int rGAMMA_COEF21;
	unsigned int rGAMMA_COEF22;
	unsigned int rGAMMA_COEF23;
	unsigned int rGAMMA_COEF24; //0x4160

	unsigned int rG_ENHANCEMENT_CTL; //0x4164
	unsigned int rB_STRETCH_CTL;
	unsigned int rENH_SRC_SEL;
	unsigned int rPRM_LTI_CFG_0; //0x4170
	unsigned int rPRM_LTI_CFG_1;
	unsigned int rPRM_PEAKING_CFG_0;
	unsigned int rPRM_PEAKING_CFG_1;
	unsigned int rPRM_CTI_CFG_0; //0x4180
	unsigned int rPRM_CTI_CFG_1;
	unsigned int rPRM_BLEC_BRT_CFG;
	unsigned int rPRM_HUE_STA_CON_CFG;
	unsigned int rPRM_HEC_CFG_0; //0x4190
	unsigned int rPRM_HEC_CFG_1;
	unsigned int rPRM_HEC_CFG_2;
	unsigned int rPRM_HEC_CFG_3;
	unsigned int rPRM_HEC_CFG_4; //0x41A0
	unsigned int rSCN_BLEC_BRT_HUE_CFG;
	unsigned int rSCN_STA_CON_CFG;
	unsigned int rMIX_Y_FIR_PARA0;
	unsigned int rMIX_Y_FIR_PARA1; //0x41B0
	unsigned int rMIX_C_FIR_PARA0;
	unsigned int rMIX_C_FIR_PARA1;
}enhance_reg;

typedef struct sirius_vpu_reg {
	unsigned int rPP_CTRL;//0x0000
	unsigned int rPP_V_PHASE;
	unsigned int rPP_POSITION;
	unsigned int rPP_SOURCE_SIZE;
	unsigned int rPP_VIEW_SIZE;//0x0010
	unsigned int rPP_ZOOM;
	unsigned int rPP_FRAME_STRIDE;
	unsigned int rPP_FILTER_SIGN;
	unsigned int rPP_PHASE_0_H;//0x0020
	unsigned int rPP_PHASE_0_V;
	unsigned int rPP_DISP_CTRL;
	unsigned int rPP_DISP_R_PTR;
	unsigned int rPP_BACK_COLOR;//0x0030
	unsigned int rRESERVED_0[3];//0x0034~0x003C

	unsigned int rPIC_CTRL;//0x0040
	unsigned int rPIC_V_PHASE;
	unsigned int rPIC_POSITION;
	unsigned int rPIC_SOURCE_SIZE;
	unsigned int rPIC_VIEW_SIZE;//0x0050
	unsigned int rPIC_ZOOM;
	unsigned int rPIC_PARA;
	unsigned int rPIC_FILTER_SIGN;
	unsigned int rPIC_PHASE_0_H;//0x0060
	unsigned int rPIC_PHASE_0_V;
	unsigned int rPIC_Y_TOP_ADDR;
	unsigned int rPIC_Y_BOTTOM_ADDR;
	unsigned int rPIC_UV_TOP_ADDR;//0x0070
	unsigned int rPIC_UV_BOTTOM_ADDR;
	unsigned int rPIC_BACK_COLOR;
	unsigned int rRESERVED_1[5];//0x007C~0x008C

	struct osd_reg osd_0; //0x0090~0x00BC

	unsigned int rCAP_CTRL;//0x00C0
	unsigned int rCAP_ADDR;
	unsigned int rCAP_HEIGHT;
	unsigned int rCAP_WIDTH;
	unsigned int rCAP_BUF_CTRL;//0xD0
	unsigned int rRESERVED_2[2];//0x00D4~0x00D8

	unsigned int rMIX_CTRL2;
	unsigned int rCHIPTEST;//0x00E0
	unsigned int rSCAN_LINE;
	unsigned int rSYS_PARA;
	unsigned int rRESERVED_3[1];//0x00EC
	unsigned int rLAYER_CTRL;
	unsigned int rEMPTY_GATE_1;
	unsigned int rEMPTY_GATE_2;
	unsigned int rFULL_GATE;
	unsigned int rBUFFER_INT;//0x0100
	unsigned int rPP_JPG_MIX_CTRL;
	unsigned int rCRC0;
	unsigned int rCRC1;
	unsigned int rOSD_EN_DLY;//0x0110
	unsigned int rRESERVED_4[3];//0x0114~0x011C

	unsigned int rVBI_CTRL;//0x0120
	unsigned int rVBI_FIRST_ADDR;
	unsigned int rVBI_ADDR;
	unsigned int rRESERVED_5[1];

	unsigned int rGIAN_DAC;//0x0130
	unsigned int rPOWER_DOWN;
	unsigned int rPOWER_DOWN_BYSELF;
	unsigned int rRESERVED_6[1];

	unsigned int rPP_FILTER_PARA[4]; //0x0140~0x014C
	unsigned int rJPG_FILTER_PARA[4];//0x0150~0x015C
	unsigned int rOSD_FILTER_PARA[4];//0x0160~0x016C
	unsigned int rSOSD_FILTER_PARA[4];//0x0170~0x017C
	unsigned int rSOSD2_FILTER_PARA[4];//0x0180~0x018C

	struct osd_reg osd_1; //0x0190~0x01BC
	struct osd_reg osd_2; //0x01C0~0x01EC

	unsigned int rRESERVED_7[3]; //0x01F0~0x01F8
	unsigned int rMODULE_EDITION;//0x01FC

	unsigned int rPP_PHASE_PARA_H[64];//0x0200~0x02FC
	unsigned int rPP_PHASE_PARA_V[64];//0x0300~0x03FC
	unsigned int rOSD_PHASE_PARA[64];//0x0400~0x04FC

	unsigned int rDISP_CTRL[8][8];//0x0500~0x05FC

	unsigned int rPIC_PHASE_PARA_H[64];//0x0600~0x06FC
	unsigned int rPIC_PHASE_PARA_V[64];//0x0700~0x07FC
	unsigned int rSOSD_PHASE_PARA[64]; //0x0800~0x08FC
	unsigned int rSOSD2_PHASE_PARA[64]; //0x0900~0x09FC

	// Reserve for Video Out
	unsigned int rRESERVED_VOUT[3524]; //0x0a00~0x410C
	enhance_reg enhance; // 0x4110
}SiriusVpuReg;

/*****************************************************************************/
/*                                   VPP                                     */
/*****************************************************************************/


//VPP_CTRL
#define bVPU_VPP_EN               (0)
#define bVPU_VPP_ZOOM_MODE        (1)
#define bVPU_VPP_V_DOWNSCALE_EN   (2)
#define bVPU_VPP_H_DOWNSCALE_EN      (3)
#define bVPU_VPP_FIELD_START      (4)
#define bVPU_VPP_MODE_UV          (6)

#define bVPU_VPP_H_PHASE_BIAS     (8)
#define bVPU_VPP_VT_PHASE_BIAS    (0)
#define bVPU_VPP_VB_PHASE_BIAS    (16)
#define bVPU_VPP_BLUR_EN          (30)

#define mVPU_VPP_EN               (0x01<<bVPU_VPP_EN )
#define mVPU_VPP_ZOOM_MODE        (0x01<<bVPU_VPP_ZOOM_MODE)
#define mVPU_VPP_MODE_UV          (0x03<<bVPU_VPP_MODE_UV)
#define mVPU_VPP_H_PHASE_BIAS     (0x7F<<bVPU_VPP_H_PHASE_BIAS)
#define mVPU_VPP_VT_PHASE_BIAS    (0x7FFF<<bVPU_VPP_VT_PHASE_BIAS)
#define mVPU_VPP_VB_PHASE_BIAS    (0x7FFF<<bVPU_VPP_VB_PHASE_BIAS)

#define VPU_VPP_ENABLE(reg) do{\
	if(!REG_GET_BIT(&(reg),bVPU_VPP_EN)) \
		REG_SET_BIT(&(reg),bVPU_VPP_EN);\
}while(0)

#define VPU_VPP_DISABLE(reg) do{\
	while(REG_GET_BIT(&(reg),bVPU_VPP_EN))\
		REG_CLR_BIT(&(reg),bVPU_VPP_EN);\
}while(0)

#define bVPU_VPP_V_DOWNSCALE   (28)
#define mVPU_VPP_V_DOWNSCALE   (0x3<<bVPU_VPP_V_DOWNSCALE)
#define VPU_VPP_V_DOWNSCALE_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_VPP_V_DOWNSCALE_EN)
#define VPU_VPP_V_DOWNSCALE_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_VPP_V_DOWNSCALE_EN)
#define VPU_VPP_V_DOWNSCALE(reg, value) \
	REG_SET_FIELD(&(reg), mVPU_VPP_V_DOWNSCALE, value, bVPU_VPP_V_DOWNSCALE)

#define bVPU_VPP_H_DOWNSCALE   (12)
#define mVPU_VPP_H_DOWNSCALE   (0x3<<bVPU_VPP_H_DOWNSCALE)
#define VPU_VPP_H_DOWNSCALE_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_VPP_H_DOWNSCALE_EN)
#define VPU_VPP_H_DOWNSCALE_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_VPP_H_DOWNSCALE_EN)
#define VPU_VPP_H_DOWNSCALE(reg, value) \
	REG_SET_FIELD(&(reg), mVPU_VPP_H_DOWNSCALE, value, bVPU_VPP_H_DOWNSCALE)

#define VPU_VPP_SET_PP_PHASE_0_H(reg, val)\
	REG_SET_VAL(&(reg), val)
#define VPU_VPP_SET_PP_PHASE_0_V(reg, val)\
	REG_SET_VAL(&(reg), val)

#define VPU_VPP_ZOOM_MODE_OUT(reg) \
	REG_SET_BIT(&(reg),bVPU_VPP_ZOOM_MODE)
#define VPU_VPP_ZOOM_MODE_IN(reg) \
	REG_CLR_BIT(&(reg),bVPU_VPP_ZOOM_MODE)

#define VPU_VPP_SET_FIELD_MODE_UV_FIELD(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_MODE_UV, value,bVPU_VPP_MODE_UV)

#define VPU_VPP_BLUR_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_VPP_BLUR_EN)
#define VPU_VPP_BLUR_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_VPP_BLUR_EN)

#define VPU_VPP_SET_H_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_H_PHASE_BIAS,value,bVPU_VPP_H_PHASE_BIAS)

#define VPU_VPP_SET_VT_PHASE(reg,value) \
	REG_SET_FIELD(&(reg), mVPU_VPP_VT_PHASE_BIAS,value,bVPU_VPP_VT_PHASE_BIAS)

#define VPU_VPP_SET_VB_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_VB_PHASE_BIAS,value,bVPU_VPP_VB_PHASE_BIAS)

//VPP_DISP_CTRL
#define bVPU_VPP_DISP_CTRL_RST               (12)

#define VPU_VPP_DISP_CTRL_RESET_SET(reg) do{\
		REG_SET_BIT(&(reg),bVPU_VPP_DISP_CTRL_RST);\
}while(0)

#define VPU_VPP_DISP_CTRL_RESET_CLR(reg) do{\
		REG_CLR_BIT(&(reg),bVPU_VPP_DISP_CTRL_RST);\
}while(0)

//VPP_ZOOM
#define VPU_VPP_ZOOM_WIDTH (12)
#define VPU_VPP_ZOOM_NONE (1 << VPU_VPP_ZOOM_WIDTH)
#define VPU_VPP_ZOOM_OUT_MAX (VPU_VPP_ZOOM_NONE << 2)

#define bVPU_VPP_VZOOM            (0)
#define bVPU_VPP_HZOOM            (16)
#define bVPU_VPP_VT_PHASE_BIAS_H  (28)
#define bVPU_VPP_VB_PHASE_BIAS_H  (30)
#define mVPU_VPP_VZOOM            (0x7FFF<<bVPU_VPP_VZOOM)
#define mVPU_VPP_HZOOM            (0x7FFF<<bVPU_VPP_HZOOM)
#define mVPU_VPP_VT_PHASE_BIAS_H  (0x3<<bVPU_VPP_VT_PHASE_BIAS_H)
#define mVPU_VPP_VB_PHASE_BIAS_H  (0x3<<bVPU_VPP_VB_PHASE_BIAS_H)

#define VPU_VPP_SET_ZOOM(reg,hzoom,vzoom) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_VPP_HZOOM,hzoom,bVPU_VPP_HZOOM);\
		REG_SET_FIELD(&(reg),mVPU_VPP_VZOOM,vzoom,bVPU_VPP_VZOOM);\
	}while (0)

#define VPU_VPP_SET_PHASE_H(reg,vt,vb) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_VPP_VT_PHASE_BIAS_H,vt,bVPU_VPP_VT_PHASE_BIAS_H); \
		REG_SET_FIELD(&(reg),mVPU_VPP_VB_PHASE_BIAS_H,vb,bVPU_VPP_VB_PHASE_BIAS_H); \
	}while (0)

//VPP_SIZE
#define bVPU_VPP_PP_SIZE_WIDTH (0)
#define bVPU_VPP_PP_SIZE_HIGH  (16)
#define mVPU_VPP_PP_SIZE_WIDTH (0x7FF << bVPU_VPP_PP_SIZE_WIDTH)
#define mVPU_VPP_PP_SIZE_HIGH  (0x7FF << bVPU_VPP_PP_SIZE_HIGH)

#define VPU_VPP_SET_PP_SIZE(reg,width,high) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_VPP_PP_SIZE_WIDTH,width,bVPU_VPP_PP_SIZE_WIDTH); \
		REG_SET_FIELD(&(reg),mVPU_VPP_PP_SIZE_HIGH,high,bVPU_VPP_PP_SIZE_HIGH); \
	}while (0)

//VPP_SOURCE_SIZE
#define bVPU_VPP_PP_SOURCE_SIZE_WIDTH (0)
#define bVPU_VPP_PP_SOURCE_SIZE_HIGH  (16)
#define mVPU_VPP_PP_SOURCE_SIZE_WIDTH (0xFFF << bVPU_VPP_PP_SOURCE_SIZE_WIDTH)
#define mVPU_VPP_PP_SOURCE_SIZE_HIGH  (0x7FF << bVPU_VPP_PP_SOURCE_SIZE_HIGH)

#define VPU_VPP_SET_PP_SOURCE_SIZE(reg,width,high) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_VPP_PP_SOURCE_SIZE_WIDTH,width,bVPU_VPP_PP_SOURCE_SIZE_WIDTH); \
		REG_SET_FIELD(&(reg),mVPU_VPP_PP_SOURCE_SIZE_HIGH,high,bVPU_VPP_PP_SOURCE_SIZE_HIGH); \
	}while(0)
#define VPU_VPP_GET_PP_SOURCE_WIDTH(reg) \
		REG_GET_FIELD(&(reg),mVPU_VPP_PP_SOURCE_SIZE_WIDTH, bVPU_VPP_PP_SOURCE_SIZE_WIDTH); \

//VPP_POSITION
#define bVPU_VPP_PP_POSITION_X (0)
#define bVPU_VPP_PP_POSITION_Y (16)
#define mVPU_VPP_PP_POSITION_X (0x7FF << bVPU_VPP_PP_POSITION_X)
#define mVPU_VPP_PP_POSITION_Y (0x7FF << bVPU_VPP_PP_POSITION_Y)

#define VPU_VPP_SET_PP_POSITION(reg,x,y) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_VPP_PP_POSITION_X,x,bVPU_VPP_PP_POSITION_X); \
		REG_SET_FIELD(&(reg),mVPU_VPP_PP_POSITION_Y,y,bVPU_VPP_PP_POSITION_Y); \
	}while (0)

//VPP_LEN_PHASE
#define bVPU_VPP_REQ_LENGTH       (16)
#define bVPU_VPP_INTP_PHASE       (16)

#define mVPU_VPP_REQ_LENGTH       (0x07FF<<bVPU_VPP_REQ_LENGTH) //Ö»ÓÐ[8:0]
#define mVPU_VPP_INTP_PHASE       (0x00FF<<bVPU_VPP_INTP_PHASE)

#define VPU_VPP_SET_LINE_REQ_LENGTH(reg,value) \
        REG_SET_FIELD(&(reg),mVPU_VPP_REQ_LENGTH,value,bVPU_VPP_REQ_LENGTH)

#define VPU_VPP_SET_INTP_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_INTP_PHASE,value,bVPU_VPP_INTP_PHASE)

//VPP_BACK_COLOR
#define bVPU_VPP_BACK_COLOR       (0)
#define mVPU_VPP_BACK_COLOR       (0xFFFFFF<<bVPU_VPP_BACK_COLOR)
#define VPU_VPP_SET_BG_COLOR(reg,y,cb,cr) do{\
	unsigned int Reg ; \
	Reg = ((unsigned int)cr&0xFF)|(((unsigned int)cb&0xFF)<<8)|(((unsigned int)y&0xFF)<<16);\
	REG_SET_VAL(&(reg),Reg); \
}while(0)

//SPP_BACK_COLOR
#define bVPU_SPP_BACK_COLOR       (0)
#define mVPU_SPP_BACK_COLOR       (0xFFFFFF<<bVPU_SPP_BACK_COLOR)
#define VPU_SPP_SET_BG_COLOR(reg,y,cb,cr) do{\
	unsigned int Reg ; \
	Reg = ((unsigned int)cr&0xFF)|(((unsigned int)cb&0xFF)<<8)|(((unsigned int)y&0xFF)<<16);\
	REG_SET_VAL(&(reg),Reg); \
}while(0)

//MIX_CTRL
#define  bVPU_SPP_ALPHA           (1)
#define  mVPU_SPP_ALPHA           (0x7F<<bVPU_SPP_ALPHA)

#define VPU_SPP_SET_MIXWEIGHT(reg,value) \
	REG_SET_FIELD(&(reg), mVPU_SPP_ALPHA, value, bVPU_SPP_ALPHA)

#define bVPU_MIX_COVERMODE_EN			(31)
#define bVPU_MIX_SPP_GLOBAL_ALPHA_EN	(30)
#define bVPU_MIX_SPP_GLOBAL_ALPHA		(8)
#define mVPU_MIX_SPP_GLOBAL_ALPHA		(0xff<<bVPU_MIX_SPP_GLOBAL_ALPHA)
#define bVPU_MIX_VPP_GLOBAL_ALPHA		(16)
#define mVPU_MIX_VPP_GLOBAL_ALPHA		(0xff<<bVPU_MIX_VPP_GLOBAL_ALPHA)

#define VPU_MIX_LAYER_COVERMODE_ENABLE(reg) \
	REG_SET_BIT(&(reg), bVPU_MIX_COVERMODE_EN)
#define VPU_MIX_LAYER_COVERMODE_DISENABLE(reg) \
	REG_CLR_BIT(&(reg), bVPU_MIX_COVERMODE_EN)

#define VPU_MIX_SPP_GLOBAL_ALPHA_ENABLE(reg) \
	REG_SET_BIT(&(reg), bVPU_MIX_SPP_GLOBAL_ALPHA_EN)
#define VPU_MIX_SPP_GLOBAL_ALPHA_DISENABLE(reg) \
	REG_CLR_BIT(&(reg), bVPU_MIX_SPP_GLOBAL_ALPHA_EN)
#define VPU_MIX_SET_SPP_GLOBAL_ALPHA(reg, value) \
	REG_SET_FIELD(&(reg), mVPU_MIX_SPP_GLOBAL_ALPHA, value, bVPU_MIX_SPP_GLOBAL_ALPHA)

#define VPU_MIX_SET_VPP_GLOBAL_ALPHA(reg, value) \
	REG_SET_FIELD(&(reg), mVPU_MIX_VPP_GLOBAL_ALPHA, value, bVPU_MIX_VPP_GLOBAL_ALPHA)

//PHASE
#define VPU_VPP_SET_PHASE(reg,i,Value) \
	REG_SET_VAL(&(reg[i]),Value)

//PP_FRAME_STRIDE
#define bVPU_VPP_FRAME_STRIDE (0)
#define mVPU_VPP_FRAME_STRIDE (0x3FFF<<bVPU_VPP_FRAME_STRIDE)
#define VPU_VPP_SET_BASE_LINE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_FRAME_STRIDE,value,bVPU_VPP_FRAME_STRIDE)
#define VPU_VPP_GET_BASE_LINE(reg) \
    REG_GET_FIELD(&(reg), mVPU_VPP_FRAME_STRIDE, bVPU_VPP_FRAME_STRIDE)

//pp_request_block
#define bVPU_VPP_REQUEST_BLOCK (16)
#define mVPU_VPP_REQUEST_BLOCK (0x7FF<<bVPU_VPP_REQUEST_BLOCK)
#define VPU_VPP_SET_REQUEST_BLOCK(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_REQUEST_BLOCK,value,bVPU_VPP_REQUEST_BLOCK)
#define VPU_VPP_GET_REQUEST_BLOCK(reg) \
    REG_GET_FIELD(&(reg), mVPU_VPP_REQUEST_BLOCK, bVPU_VPP_REQUEST_BLOCK)

//PP_Y_TOP_ADDR
#define bVPU_VPP_Y_TOP_ADDR          (0)
#define mVPU_VPP_Y_TOP_ADDR          (VPP_ADDR_MASK<<bVPU_VPP_Y_TOP_ADDR)
#define VPU_VPP_SET_Y_TOP_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_Y_TOP_ADDR,value,bVPU_VPP_Y_TOP_ADDR)
//PP_Y_BOTTOM_ADDR
#define bVPU_VPP_Y_BOTTOM_ADDR          (0)
#define mVPU_VPP_Y_BOTTOM_ADDR          (VPP_ADDR_MASK<<bVPU_VPP_Y_BOTTOM_ADDR)
#define VPU_VPP_SET_Y_BOTTOM_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_Y_BOTTOM_ADDR,value,bVPU_VPP_Y_BOTTOM_ADDR)
//PP_UV_TOP_ADDR
#define bVPU_VPP_UV_TOP_ADDR          (0)
#define mVPU_VPP_UV_TOP_ADDR          (VPP_ADDR_MASK<<bVPU_VPP_UV_TOP_ADDR)
#define VPU_VPP_SET_UV_TOP_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_UV_TOP_ADDR,value,bVPU_VPP_UV_TOP_ADDR)
//PP_UV_BOTTOM_ADDR
#define bVPU_VPP_UV_BOTTOM_ADDR          (0)
#define mVPU_VPP_UV_BOTTOM_ADDR          (VPP_ADDR_MASK<<bVPU_VPP_UV_BOTTOM_ADDR)
#define VPU_VPP_SET_UV_BOTTOM_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_UV_BOTTOM_ADDR,value,bVPU_VPP_UV_BOTTOM_ADDR)

#define bVPU_VPP_PLAY_MODE (1)
#define mVPU_VPP_PLAY_MODE (0x1 << bVPU_VPP_PLAY_MODE)
#define VPU_VPP_SET_PLAY_MODE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_PLAY_MODE,value,bVPU_VPP_PLAY_MODE)

//BUFFER_CTRL1
#define bVPU_VPP_REQ_DATA_LEN     (16)
#define mVPU_VPP_REQ_DATA_LEN     (0x7FF<<bVPU_VPP_REQ_DATA_LEN)

#define VPU_VPP_SET_BUFFER_REQ_DATA_LEN(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_REQ_DATA_LEN, value,bVPU_VPP_REQ_DATA_LEN)

//BUFFER_CTRL2
#define bVPU_OSD_BUFF_LEN         (0)
#define bVPU_VPP_LOCK             (16)
#define bVPU_VPP_AV_MODE          (19)
#define bVPU_BUFF_STATE_DELAY     (16)

#define mVPU_OSD_BUFF_LEN         (0x3ff<<bVPU_OSD_BUFF_LEN)
#define mVPU_VPP_LOCK             (0x07<<bVPU_VPP_LOCK)
#define mVPU_VPP_AV_MODE          (0x01<<bVPU_VPP_AV_MODE)
#define mVPU_BUFF_STATE_DELAY     (0xFF<<bVPU_BUFF_STATE_DELAY)

#define VPU_VPP_SET_LOCK(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_VPP_LOCK,value,bVPU_VPP_LOCK)

#define VPU_VPP_SET_AV_MODE_MPEG(reg) \
	REG_CLR_BIT(&(reg),bVPU_VPP_AV_MODE)

#define VPU_VPP_SET_AV_MODE_H264(reg) \
	REG_SET_BIT(&(reg),bVPU_VPP_AV_MODE)

#define VPU_VPP_PRI_PARAM(reg,v1,v2,v3) do{\
	REG_SET_FIELD(&(reg),mVPU_OSD_BUFF_LEN,v1,bVPU_OSD_BUFF_LEN);\
	REG_SET_FIELD(&(reg),mVPU_VPP_LOCK ,v2,bVPU_VPP_LOCK);\
	REG_SET_FIELD(&(reg),mVPU_BUFF_STATE_DELAY,v3,bVPU_BUFF_STATE_DELAY);\
}while(0)

#define VPU_VPP_BUFFER_PRI_LEVEL1(reg1,reg2)  do{\
	reg1 = 0x02000200;\
	VPU_VPP_PRI_PARAM(reg2,0x80,0x04,0x40);\
}while(0)

#define VPU_VPP_BUFFER_PRI_LEVEL2(reg1,reg2) do{\
	reg1 = 0x02000200;\
	VPU_VPP_PRI_PARAM(reg2,0x80,0x05,0x40);\
}while(0)

#define VPU_VPP_BUFFER_PRI_LEVEL3(reg1,reg2) do{\
	reg1 = 0x02000200;\
	VPU_VPP_PRI_PARAM(reg2,0x80,0x05,0x40);\
}while(0)

#define VPU_VPP_BUFFER_PRI_LEVEL4(reg1,reg2) do{\
        reg1 = 0x02000200;\
        VPU_VPP_PRI_PARAM(reg2,0x80,0x07,0x40);\
}while(0)

/*****************************************************************************/
/*                                   BKG                                     */
/*****************************************************************************/
//MIX_CTRL
#define VPU_BKG_SET_COLOR(reg,y,cb,cr) do{\
	unsigned int Reg ; \
	Reg = ((unsigned char)(y)<<8)|((unsigned char)(cb)<<16)|((unsigned char)(cr)<<24);\
	(reg) = ((reg) & 0xFF)|(Reg & 0xFFFFFF00) ;\
}while(0)

/*****************************************************************************/
/*                                   OSD                                     */
/*****************************************************************************/

//OSD_CTRL
#define bVPU_OSD_EN               (0)
#define bVPU_OSD_VT_PHASE         (8)
#define bVPU_OSD_HZOOM            (0)
#define bVPU_OSD_VZOOM            (16)
#define bVPU_OSD_ZOOM_MODE_EN_IPS (25)
#define bVPU_OSD_HDOWN_SAMPLE_EN  (28)
#define bVPU_OSD_ANTI_FLICKER     (29)
#define bVPU_OSD_ANTI_FLICKER_CBCR (31)
#define mVPU_OSD_EN               (0x1<<bVPU_OSD_EN)
#define mVPU_OSD_VT_PHASE         (0x3FFF<<bVPU_OSD_VT_PHASE)
#define mVPU_OSD_VZOOM            (0x3FFF<<bVPU_OSD_VZOOM)
#define mVPU_OSD_HZOOM            (0x3FFF<<bVPU_OSD_HZOOM)
#define mVPU_OSD_HDOWN_SAMPLE_EN  (0x1<<bVPU_OSD_HDOWN_SAMPLE_EN)

/*
 * sequences ....... value
 * DCBA_HGFE .......   0
 * EFGH_ABCD .......   1
 * HGFE_DCBA .......   2
 * ABCD_EFGH .......   3
 *
 * reg: 0xa48000c8 [21:20]
 * understanding:
 * If points on a line are p0,p1,p2,p3...
 * And the color value of them are vale0, vale1, vale2, vale3, ...
 * VPU read 8 bytes each time.
 * 1)32 bpp VPU defined vale0 = (A<<24)|(B<<16)|(C<<8)|(D<<0) ...
 *      example: p0 = 0x11223344,
 *      gdb>>x /x &p0
 *      gdb>>0x11223344
 *      so we should config: ABCD_EFGH
 * 2)16 bpp VPU defined vale0 = (A<<8 )|(B<<0), vale1 = (C<<8)|(D<<0) ...
 *      example:  p0 = 0x1122, p1 = 0x3344
 *      gdb>>x /x &p0
 *      gdb>>0x33441122
 *      so we should config: CDAB_GHEF
 * 3)8  bpp VPU defined vale0 = (A<<0), vale1 = (B<<0), vale2 = (C<<0) ...
 *      example: p0 = 0x11, p1 = 0x22, p2 = 0x33, p3 = 0x44
 *      gdb>>x /x &p0
 *      gdb>>0x44332211
 *      so we should config: DCBA_HGFE
 */
#define bVPU_OSD_RW_BYTESEQ_HIGH   (22)
#define bVPU_OSD_RW_BYTESEQ_LOW    (20)
#define mVPU_OSD_RW_BYTESEQ_LOW    (0x3<<bVPU_OSD_RW_BYTESEQ_LOW)
#define VPU_OSD_SET_DATA_RW_BYTESEQ(reg, byte_seq)\
do\
{\
	REG_SET_FIELD(&(reg), mVPU_OSD_RW_BYTESEQ_LOW, byte_seq&0x3, bVPU_OSD_RW_BYTESEQ_LOW);\
	if(byte_seq>>2)\
		REG_SET_BIT(&(reg), bVPU_OSD_RW_BYTESEQ_HIGH);\
	else\
		REG_CLR_BIT(&(reg), bVPU_OSD_RW_BYTESEQ_HIGH);\
}while(0)

#define bVPU_OSD_REGIONHEAD_BYTESEQ   (12)
#define mVPU_OSD_REGIONHEAD_BYTESEQ   (0x7<<bVPU_OSD_REGIONHEAD_BYTESEQ)
#define VPU_OSD_SET_REGIONHEAD_BYTESEQ(reg, byte_seq)\
do\
{\
	REG_SET_FIELD(&(reg), mVPU_OSD_REGIONHEAD_BYTESEQ, byte_seq&0x7, bVPU_OSD_REGIONHEAD_BYTESEQ);\
}while(0)

#define bVPU_SOSD_RW_BYTESEQ_HIGH   (22)
#define bVPU_SOSD_RW_BYTESEQ_LOW    (20)
#define mVPU_SOSD_RW_BYTESEQ_LOW    (0x3<<bVPU_SOSD_RW_BYTESEQ_LOW)
#define VPU_SOSD_SET_DATA_RW_BYTESEQ(reg, byte_seq)\
do\
{\
	REG_SET_FIELD(&(reg), mVPU_SOSD_RW_BYTESEQ_LOW, byte_seq&0x3, bVPU_SOSD_RW_BYTESEQ_LOW);\
	if(byte_seq>>2)\
		REG_SET_BIT(&(reg), bVPU_SOSD_RW_BYTESEQ_HIGH);\
	else\
		REG_CLR_BIT(&(reg), bVPU_SOSD_RW_BYTESEQ_HIGH);\
}while(0)

#define bVPU_SOSD_REGIONHEAD_BYTESEQ   (12)
#define mVPU_SOSD_REGIONHEAD_BYTESEQ   (0x7<<bVPU_SOSD_REGIONHEAD_BYTESEQ)
#define VPU_SOSD_SET_REGIONHEAD_BYTESEQ(reg, byte_seq)\
do\
{\
	REG_SET_FIELD(&(reg), mVPU_SOSD_REGIONHEAD_BYTESEQ, byte_seq&0x7, bVPU_SOSD_REGIONHEAD_BYTESEQ);\
}while(0)

#define REG_SET_BIT_ABCD_EFGH	GX_SET_BIT
#define REG_SET_BIT_DCBA_HGFE	GX_SET_BIT_E
#define REG_SET_BIT_CDAB_GHEF(reg, bit)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		tmpVal |=(1<<(bit));\
		tmpVal = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		gx_iowrite32(tmpVal, reg);\
	}while(0)
#define OSD_HEAD_SET_BIT(reg, bit) \
	do{\
		ByteSequence byteseq = ABCD_EFGH;\
		/*
		extern SiriusVpu *siriusvpu_info;\
		ByteSequence byteseq = ((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq;
		*/\
		switch(byteseq)\
		{\
		case ABCD_EFGH:\
			REG_SET_BIT_ABCD_EFGH(reg, bit);\
			break;\
		case DCBA_HGFE:\
			REG_SET_BIT_DCBA_HGFE(reg, bit);\
			break;\
		case CDAB_GHEF:\
			REG_SET_BIT_CDAB_GHEF(reg, bit);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define REG_CLR_BIT_ABCD_EFGH	GX_CLR_BIT
#define REG_CLR_BIT_DCBA_HGFE	GX_CLR_BIT_E
#define REG_CLR_BIT_CDAB_GHEF(reg, bit)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		tmpVal &= (~(1<<(bit)));\
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		gx_iowrite32(tmpVal, reg);\
	}while(0)
#define OSD_HEAD_CLR_BIT(reg, bit) \
	do{\
		ByteSequence byteseq = ABCD_EFGH;\
		/*
		extern SiriusVpu *siriusvpu_info;\
		ByteSequence byteseq = ((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq;
		*/\
		switch(byteseq)\
		{\
		case ABCD_EFGH:\
			REG_CLR_BIT_ABCD_EFGH(reg, bit);\
			break;\
		case DCBA_HGFE:\
			REG_CLR_BIT_DCBA_HGFE(reg, bit);\
			break;\
		case CDAB_GHEF:\
			REG_CLR_BIT_CDAB_GHEF(reg, bit);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define REG_SET_FEILD_ABCD_EFGH	GX_SET_FEILD
#define REG_SET_FEILD_DCBA_HGFE	GX_SET_FEILD_E
#define REG_SET_FEILD_CDAB_GHEF(reg,mask,val,offset)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		tmpVal &=  ~(mask);\
		tmpVal |=  ((val) << (offset)) & (mask);\
		tmpVal  = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		gx_iowrite32(tmpVal, reg);\
	}while(0)
#define OSD_HEAD_SET_FEILD(reg,mask,val,offset)\
	do{\
		ByteSequence byteseq = ABCD_EFGH;\
		/*
		extern SiriusVpu *siriusvpu_info;\
		ByteSequence byteseq = ((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq;
		*/\
		switch(byteseq)\
		{\
		case ABCD_EFGH:\
			REG_SET_FEILD_ABCD_EFGH(reg,mask,val,offset);\
			break;\
		case DCBA_HGFE:\
			REG_SET_FEILD_DCBA_HGFE(reg,mask,val,offset);\
			break;\
		case CDAB_GHEF:\
			REG_SET_FEILD_CDAB_GHEF(reg,mask,val,offset);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define REG_GET_FEILD_ABCD_EFGH	GX_GET_FEILD
#define REG_GET_FEILD_DCBA_HGFE	GX_GET_FEILD_E
#define REG_GET_FEILD_CDAB_GHEF(reg,mask,val,offset)\
	do{\
		unsigned int tmpVal  = gx_ioread32(reg); \
		tmpVal = ((tmpVal&0xffff)<<16)|((tmpVal>>16)&0xffff);\
		(val)  = (tmpVal & (mask)) >> (offset);\
	}while(0)
#define OSD_HEAD_GET_FEILD(reg,mask,val,offset)\
	do{\
		ByteSequence byteseq = ABCD_EFGH;\
		/*
		extern SiriusVpu *siriusvpu_info;\
		ByteSequence byteseq = ((SiriusVpuOsdPriv*)siriusvpu_info->layer[GX_LAYER_OSD].priv)->regionhead_byte_seq;
		*/\
		switch(byteseq)\
		{\
		case ABCD_EFGH:\
			REG_GET_FEILD_ABCD_EFGH(reg,mask,val,offset);\
			break;\
		case DCBA_HGFE:\
			REG_GET_FEILD_DCBA_HGFE(reg,mask,val,offset);\
			break;\
		case CDAB_GHEF:\
			REG_GET_FEILD_CDAB_GHEF(reg,mask,val,offset);\
			break;\
		default:\
			break;\
		}\
	}while(0)

#define VPU_OSD_ENABLE(reg) do{\
	while(!REG_GET_BIT(&(reg),bVPU_OSD_EN)){\
		REG_SET_BIT(&(reg),bVPU_OSD_EN);\
	}\
}while(0)

#define VPU_OSD_DISABLE(reg) do{\
	while(REG_GET_BIT(&(reg),bVPU_OSD_EN)){\
		REG_CLR_BIT(&(reg),bVPU_OSD_EN);\
	}\
}while(0)

#define VPU_OSD_ZOOM_WIDTH (12)
#define VPU_OSD_ZOOM_NONE (1 << VPU_OSD_ZOOM_WIDTH)

#define VPU_OSD_SET_VZOOM(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_VZOOM,value,bVPU_OSD_VZOOM)
#define VPU_OSD_GET_VZOOM(reg) \
	REG_GET_FIELD(&(reg), mVPU_OSD_VZOOM, bVPU_OSD_VZOOM)
#define VPU_OSD_SET_HZOOM(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_HZOOM,value,bVPU_OSD_HZOOM)
#define VPU_OSD_GET_HZOOM(reg) \
	REG_GET_FIELD(&(reg), mVPU_OSD_HZOOM, bVPU_OSD_HZOOM)
#define VPU_OSD_SET_VTOP_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_VT_PHASE,value,bVPU_OSD_VT_PHASE)
#define VPU_OSD_SET_ZOOM_MODE(reg)  \
	REG_SET_BIT(&(reg),bVPU_OSD_ANTI_FLICKER_CBCR)
#define VPU_OSD_CLR_ZOOM_MODE(reg)  \
	REG_CLR_BIT(&(reg),bVPU_OSD_ANTI_FLICKER_CBCR)
#define VPU_OSD_SET_ZOOM_MODE_EN_IPS(reg) \
	REG_SET_BIT(&(reg),bVPU_OSD_ZOOM_MODE_EN_IPS)
#define VPU_OSD_CLR_ZOOM_MODE_EN_IPS(reg) \
	REG_CLR_BIT(&(reg),bVPU_OSD_ZOOM_MODE_EN_IPS)

#define VPU_OSD_H_DOWNSCALE_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_OSD_HDOWN_SAMPLE_EN)
#define VPU_OSD_H_DOWNSCALE_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_OSD_HDOWN_SAMPLE_EN)

#define VPU_OSD_PP_ANTI_FLICKER_ENABLE(reg)	 REG_SET_BIT(&(reg),bVPU_OSD_ANTI_FLICKER)
#define VPU_OSD_PP_ANTI_FLICKER_DISABLE(reg) REG_CLR_BIT(&(reg),bVPU_OSD_ANTI_FLICKER)

//OSD_FIRST_HEAD_PTR
#define bVPU_OSD_FIRST_HEAD       (0)
#define mVPU_OSD_FIRST_HEAD       (OSD_ADDR_MASK<<bVPU_OSD_FIRST_HEAD)

#define VPU_OSD_SET_FIRST_HEAD(reg,value)\
	REG_SET_FIELD(&(reg),mVPU_OSD_FIRST_HEAD,value,bVPU_OSD_FIRST_HEAD)

//OSD_PHASE_0
#define VPU_OSD_PHASE_0_ZOOM_COEF (0xFF0100)
#define bVPU_OSD_PHASE_0       (0)
#define mVPU_OSD_PHASE_0       (0xFFFFFF<<bVPU_OSD_PHASE_0)
#define VPU_OSD_SET_PHASE_0(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_PHASE_0,value,bVPU_OSD_PHASE_0)
#define VPU_OSD_GET_PHASE_0(reg) \
	REG_GET_FIELD(&(reg), mVPU_OSD_PHASE_0, bVPU_OSD_PHASE_0)

//OSD_ALPHA_5551
#define bVPU_OSD_ALPHA_5551 (0)
#define mVPU_OSD_ALPHA_5551	(0xFFFF)
#define VPU_OSD_SET_ALPHA_5551(reg, value) \
	REG_SET_FIELD(&(reg), mVPU_OSD_ALPHA_5551,value,bVPU_OSD_ALPHA_5551)

//BUFFER_CTRL2
#define bVPU_OSD_REQ_DATA_LEN     (0)
#define bVPU_BUFF_STATE_DELAY     (16)
#define mVPU_OSD_REQ_DATA_LEN     (0x7FF<<bVPU_OSD_REQ_DATA_LEN)
#define mVPU_BUFF_STATE_DELAY     (0xFF<<bVPU_BUFF_STATE_DELAY)

#define VPU_OSD_SET_BUFFER_REQ_DATA_LEN(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_OSD_REQ_DATA_LEN,value,bVPU_OSD_REQ_DATA_LEN)

#define VPU_SET_BUFF_STATE_DELAY(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_BUFF_STATE_DELAY,value,bVPU_BUFF_STATE_DELAY)

//OSD_COLOR_KEY
#define VPU_OSD_SET_COLOR_KEY(reg,R,G,B,A) do{\
	unsigned int Reg ; \
	Reg = (((unsigned char)R)<<24)|(((unsigned char)G)<<16)|(((unsigned char)B)<<8)|((unsigned char)A);\
	reg = Reg ;\
}while(0)

//rFLICKER
#define VPU_OSD_SET_FLIKER_FLITER(reg,i,Value) \
	REG_SET_VAL(&(reg[i]),Value)

//OSD_POSITION
#define bVPU_OSD_POSITION_X (0)
#define bVPU_OSD_POSITION_Y (16)
#define mVPU_OSD_POSITION_X (0x3FF << bVPU_OSD_POSITION_X)
#define mVPU_OSD_POSITION_Y (0x3FF << bVPU_OSD_POSITION_Y)

#define VPU_OSD_SET_POSITION(reg,x,y) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_OSD_POSITION_X,x,bVPU_OSD_POSITION_X); \
		REG_SET_FIELD(&(reg),mVPU_OSD_POSITION_Y,y,bVPU_OSD_POSITION_Y); \
	}while (0)

//OSD_SIZE
#define bVPU_OSD_VIEW_SIZE_WIDTH (0)
#define bVPU_OSD_VIEW_SIZE_HIGH  (16)
#define mVPU_OSD_VIEW_SIZE_WIDTH (0x7FF << bVPU_OSD_VIEW_SIZE_WIDTH)
#define mVPU_OSD_VIEW_SIZE_HIGH  (0x7FF << bVPU_OSD_VIEW_SIZE_HIGH)
#define VPU_OSD_SET_VIEW_SIZE(reg,width,high) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_OSD_VIEW_SIZE_WIDTH,width,bVPU_OSD_VIEW_SIZE_WIDTH); \
		REG_SET_FIELD(&(reg),mVPU_OSD_VIEW_SIZE_HIGH,high,bVPU_OSD_VIEW_SIZE_HIGH); \
	}while (0)

//OSD_HEAD_WORD1
#define bVPU_OSD_CLUT_SWITCH      (0)
#define bVPU_OSD_CLUT_LENGTH      (8)
#define bVPU_OSD_COLOR_MODE       (10)
#define bVPU_OSD_CLUT_UPDATA_EN   (13)
#define bVPU_OSD_FLIKER_FLITER_EN (14)
#define bVPU_OSD_COLOR_KEY_EN     (15)
#define bVPU_OSD_MIX_WEIGHT       (16)
#define bVPU_OSD_GLOBAL_ALPHA_EN  (23)
#define bVPU_OSD_TRUE_COLOR_MODE  (24)
#define bVPU_OSD_ARGB_CONVERT     (26)
#define bVPU_OSD_ABGR_CONVERT     (27)
#define bVPU_OSD_YUV_MODE         (28)

#define mVPU_OSD_CLUT_SWITCH      (0xFF << bVPU_OSD_CLUT_SWITCH)
#define mVPU_OSD_CLUT_LENGTH      (0x3  << bVPU_OSD_CLUT_LENGTH)
#define mVPU_OSD_COLOR_MODE       (0x7  << bVPU_OSD_COLOR_MODE)
#define mVPU_OSD_CLUT_UPDATA_EN   (0x1  << bVPU_OSD_CLUT_UPDATA_EN)
#define mVPU_OSD_FLIKER_FLITER_EN (0x1  << bVPU_OSD_FLIKER_FLITER_EN)
#define mVPU_OSD_COLOR_KEY_EN     (0x1  << bVPU_OSD_COLOR_KEY_EN)
#define mVPU_OSD_MIX_WEIGHT       (0x7F << bVPU_OSD_MIX_WEIGHT)
#define mVPU_OSD_GLOBAL_ALPHA_EN  (0x1  << bVPU_OSD_GLOBAL_ALPHA_EN)
#define mVPU_OSD_TRUE_COLOR_MODE  (0x3  << bVPU_OSD_TRUE_COLOR_MODE)
#define mVPU_OSD_ARGB_CONVERT     (0x1  << bVPU_OSD_ARGB_CONVERT)
#define mVPU_OSD_ABGR_CONVERT     (0x1  << bVPU_OSD_ABGR_CONVERT)
#define mVPU_OSD_YUV_MODE         (0x1  << bVPU_OSD_YUV_MODE)

#define VPU_OSD_COLOR_KEY_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_COLOR_KEY_EN)

#define VPU_OSD_COLOR_KEY_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_COLOR_KEY_EN)

#define VPU_OSD_CLUT_UPDATA_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_CLUT_UPDATA_EN)

#define VPU_OSD_CLUT_UPDATA_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_CLUT_UPDATA_EN)

#define VPU_OSD_FLIKER_FLITER_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_FLIKER_FLITER_EN)

#define VPU_OSD_FLIKER_FLITER_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_FLIKER_FLITER_EN)

#define VPU_OSD_GLOBAL_ALHPA_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_GLOBAL_ALPHA_EN)

#define VPU_OSD_GLOBAL_ALHPA_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_GLOBAL_ALPHA_EN)

#define VPU_OSD_SET_MIX_WEIGHT(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_MIX_WEIGHT,value,bVPU_OSD_MIX_WEIGHT)

#define VPU_OSD_SET_COLOR_TYPE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_COLOR_MODE,value,bVPU_OSD_COLOR_MODE)

#define VPU_OSD_GET_COLOR_TYPE(reg,value) \
	OSD_HEAD_GET_FEILD(&(reg), mVPU_OSD_COLOR_MODE, value, bVPU_OSD_COLOR_MODE)

#define VPU_OSD_SET_TRUE_COLOR_MODE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_TRUE_COLOR_MODE,value,bVPU_OSD_TRUE_COLOR_MODE)

#define VPU_OSD_SET_CLUT_LENGTH(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_CLUT_LENGTH,value,bVPU_OSD_CLUT_LENGTH)

#define VPU_OSD_SET_CLUT_SWITCH(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_CLUT_SWITCH,value,bVPU_OSD_CLUT_SWITCH)

#define VPU_OSD_SET_ARGB_CONVERT(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_ARGB_CONVERT,value,bVPU_OSD_ARGB_CONVERT)
#define VPU_OSD_SET_ABGR_CONVERT(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_ABGR_CONVERT,value,bVPU_OSD_ABGR_CONVERT)

#define VPU_OSD_SET_YUV_MODE(reg, value)\
	OSD_HEAD_SET_FEILD(&(reg), mVPU_OSD_YUV_MODE, value, bVPU_OSD_YUV_MODE)

//OSD_HEAD_WORD2
#define bVPU_OSD_CLUT_PTR         (0)
#define mVPU_OSD_CLUT_PTR         (OSD_ADDR_MASK<<bVPU_OSD_CLUT_PTR)

#define VPU_OSD_SET_CLUT_PTR(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg), mVPU_OSD_CLUT_PTR,value,bVPU_OSD_CLUT_PTR)

//OSD_HEAD_WORD3
#define bVPU_OSD_LEFT             (0)
#define bVPU_OSD_RIGHT            (16)
#define mVPU_OSD_LEFT             (0x7FF<<bVPU_OSD_LEFT)
#define mVPU_OSD_RIGHT            (0x7FF<<bVPU_OSD_RIGHT)

#define VPU_OSD_SET_WIDTH(reg,left,right) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_LEFT,left,bVPU_OSD_LEFT); \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_RIGHT,right,bVPU_OSD_RIGHT)

//OSD_HEAD_WORD4
#define bVPU_OSD_TOP              (0)
#define bVPU_OSD_BOTTOM           (16)
#define mVPU_OSD_TOP              (0x7FF<<bVPU_OSD_TOP)
#define mVPU_OSD_BOTTOM           (0x7FF<<bVPU_OSD_BOTTOM)

#define VPU_OSD_SET_HEIGHT(reg,top,bottom) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_TOP,top,bVPU_OSD_TOP); \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_BOTTOM,bottom,bVPU_OSD_BOTTOM)

//OSD_HEAD_WORD5
#define bVPU_OSD_DATA_ADDR         (0)
#define mVPU_OSD_DATA_ADDR         (OSD_ADDR_MASK<<bVPU_OSD_DATA_ADDR)

#define VPU_OSD_SET_DATA_ADDR(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_DATA_ADDR,value,bVPU_OSD_DATA_ADDR)

 //OSD_HEAD_WORD6
#define bVPU_OSD_NEXT_PTR         (0)
#define bVPU_OSD_LIST_END         (31)

#define mVPU_OSD_NEXT_PTR         (OSD_ADDR_MASK<<bVPU_OSD_NEXT_PTR)
#define mVPU_OSD_LIST_END         (0x1<<bVPU_OSD_LIST_END)

#define VPU_OSD_LIST_END_ENABLE(reg) \
        OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_LIST_END)
#define VPU_OSD_LIST_END_DISABLE(reg) \
        OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_LIST_END)

#define VPU_OSD_SET_NEXT_PTR(reg,value) \
        OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_NEXT_PTR,value,bVPU_OSD_NEXT_PTR)

//OSD_HEAD_WORD7
#define bVPU_OSD_BASE_LINE         (0)
#define mVPU_OSD_BASE_LINE         (OSD_BASE_LINE_MASK<<bVPU_OSD_BASE_LINE)
#define bVPU_OSD_ALPHA_RATIO       (16)
#define mVPU_OSD_ALPHA_RATIO       (0xFF<<bVPU_OSD_ALPHA_RATIO)
#define bVPU_OSD_ALPHA_RATIO_EN    (24)
#define VPU_OSD_SET_BASE_LINE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_BASE_LINE,value,bVPU_OSD_BASE_LINE)
#define VPU_OSD_SET_ALPHA_RATIO_ENABLE(reg) \
	OSD_HEAD_SET_BIT(&(reg),bVPU_OSD_ALPHA_RATIO_EN)
#define VPU_OSD_SET_ALPHA_RATIO_DISABLE(reg) \
	OSD_HEAD_CLR_BIT(&(reg),bVPU_OSD_ALPHA_RATIO_EN)
#define VPU_OSD_SET_ALPHA_RATIO_VALUE(reg,value) \
	OSD_HEAD_SET_FEILD(&(reg),mVPU_OSD_ALPHA_RATIO,value,bVPU_OSD_ALPHA_RATIO)


/*****************************************************************************/
/*                                   SOSD                                     */
/*****************************************************************************/
#define SOSD_FLICKER_TABLE_LEN     (64)

//SOSD_CTRL
#define bVPU_SOSD_ON_TOP_EN (30)
#define VPU_SOSD_SET_ON_TOP_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_SOSD_ON_TOP_EN)
#define VPU_SOSD_SET_ON_TOP_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_SOSD_ON_TOP_EN)
#define VPU_SOSD_GET_ON_TOP_EN(reg) \
	REG_GET_BIT(&(reg),bVPU_SOSD_ON_TOP_EN)

#define VPU_SOSD_GET_VZOOM VPU_OSD_GET_VZOOM
#define VPU_SOSD_GET_HZOOM VPU_OSD_GET_HZOOM
#define VPU_SOSD_SET_VZOOM VPU_OSD_SET_VZOOM
#define VPU_SOSD_SET_HZOOM VPU_OSD_SET_HZOOM

#define VPU_SOSD_PP_ANTI_FLICKER_ENABLE VPU_OSD_PP_ANTI_FLICKER_ENABLE
#define VPU_SOSD_CBCR_ANTI_FLICKER_ENABLE VPU_OSD_CBCR_ANTI_FLICKER_ENABLE
#define VPU_SOSD_PP_ANTI_FLICKER_DISABLE VPU_OSD_PP_ANTI_FLICKER_DISABLE
#define VPU_SOSD_CBCR_ANTI_FLICKER_DISABLE VPU_OSD_CBCR_ANTI_FLICKER_DISABLE
#define VPU_SOSD_ZOOM_NONE VPU_OSD_ZOOM_NONE

#define VPU_SOSD_DISABLE VPU_OSD_DISABLE
#define VPU_SOSD_ENABLE VPU_OSD_ENABLE

#define VPU_SOSD_H_DOWNSCALE_ENABLE VPU_OSD_H_DOWNSCALE_ENABLE
#define VPU_SOSD_H_DOWNSCALE_DISABLE VPU_OSD_H_DOWNSCALE_DISABLE

//SOSD_FIRST_HEAD
#define VPU_SOSD_SET_FIRST_HEAD VPU_OSD_SET_FIRST_HEAD

//SOSD_PHASE_0
#define VPU_SOSD_PHASE_0_ANTI_FLICKER_ENABLE VPU_OSD_PHASE_0_ANTI_FLICKER_ENABLE
#define VPU_SOSD_PHASE_0_ANTI_FLICKER_DISABLE VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE
#define VPU_SOSD_SET_PHASE_0 VPU_OSD_SET_PHASE_0
#define VPU_SOSD_GET_PHASE_0 VPU_OSD_GET_PHASE_0

//SOSD_PARA_0-63
#define VPU_SOSD_SET_FLIKER_FLITER VPU_OSD_SET_FLIKER_FLITER

//SOSD_HEAD_WORD
#define VPU_SOSD_SET_COLOR_TYPE VPU_OSD_SET_COLOR_TYPE
#define VPU_SOSD_GLOBAL_ALHPA_ENABLE VPU_OSD_GLOBAL_ALHPA_ENABLE
#define VPU_SOSD_SET_MIX_WEIGHT VPU_OSD_SET_MIX_WEIGHT
#define VPU_SOSD_GLOBAL_ALHPA_DISABLE VPU_OSD_GLOBAL_ALHPA_DISABLE
#define VPU_SOSD_SET_CLUT_PTR VPU_OSD_SET_CLUT_PTR
#define VPU_SOSD_SET_CLUT_LENGTH VPU_OSD_SET_CLUT_LENGTH
#define VPU_SOSD_CLUT_UPDATA_ENABLE VPU_OSD_CLUT_UPDATA_ENABLE
#define VPU_SOSD_SET_WIDTH VPU_OSD_SET_WIDTH
#define VPU_SOSD_SET_HEIGHT VPU_OSD_SET_HEIGHT
#define VPU_SOSD_SET_DATA_ADDR VPU_OSD_SET_DATA_ADDR
#define VPU_SOSD_LIST_END_ENABLE VPU_OSD_LIST_END_ENABLE
#define VPU_SOSD_SET_BASE_LINE VPU_OSD_SET_BASE_LINE

/*****************************************************************************/
/*                                   SPP                                     */
/*****************************************************************************/
//MIX_CTRL
#define bVPU_SPP_ON_TOP           (0)
#define mVPU_SPP_ON_TOP           (0x1<<bVPU_SPP_ON_TOP)
#define SIRIUS_VPU_SPP_ON_TOP(reg) \
        REG_SET_BIT(&(reg),bVPU_SPP_ON_TOP)
#define SIRIUS_VPU_SPP_ON_BOTTOM(reg) \
        REG_CLR_BIT(&(reg),bVPU_SPP_ON_TOP)

#define mVPU_SPP_TOP_MASK                       (0x000000FF)
#define bVPU_SPP_TOP_OFFSET                     (0)
#define TAURUS_VPU_SPP_ON_TOP(reg) \
	REG_SET_FIELD(&(reg),mVPU_SPP_TOP_MASK,0x01,bVPU_SPP_TOP_OFFSET)
#define TAURUS_VPU_SPP_ON_BOTTOM(reg) \
	REG_SET_FIELD(&(reg),mVPU_SPP_TOP_MASK,0x10,bVPU_SPP_TOP_OFFSET)

//PIC_CTRL
#define bVPU_SPP_EN               (0)
#define bVPU_SPP_V_DOWNSCALE      (2)
#define bVPU_SPP_H_DOWNSCALE      (3)
#define bVPU_SPP_VT_PHASE_BIAS    (0)
#define bVPU_SPP_VB_PHASE_BIAS    (16)
#define bVPU_SPP_PLAY_MODE        (29)

#define mVPU_SPP_EN               (0x01   << bVPU_SPP_EN)
#define mVPU_SPP_V_DOWNSCALE      (0x01   << bVPU_SPP_V_DOWNSCALE)
#define mVPU_SPP_H_DOWNSCALE      (0x01   << bVPU_SPP_H_DOWNSCALE)
#define mVPU_SPP_VT_PHASE_BIAS    (0x7FFF << bVPU_SPP_VT_PHASE_BIAS)
#define mVPU_SPP_VB_PHASE_BIAS    (0x7FFF << bVPU_SPP_VB_PHASE_BIAS)
#define mVPU_SPP_PLAY_MODE        (0x01   << bVPU_SPP_PLAY_MODE)


#define VPU_SPP_ENABLE(reg) do{             \
	while(!REG_GET_BIT(&(reg),bVPU_SPP_EN)){ \
		REG_SET_BIT(&(reg),bVPU_SPP_EN);      \
	}\
}while(0) \

#define VPU_SPP_DISABLE(reg) do{           \
	while(REG_GET_BIT(&(reg),bVPU_SPP_EN)){ \
		REG_CLR_BIT(&(reg),bVPU_SPP_EN);     \
	}\
}while(0)

#define VPU_SPP_V_DOWNSCALE_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_SPP_V_DOWNSCALE)
#define VPU_SPP_V_DOWNSCALE_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_SPP_V_DOWNSCALE)

#define VPU_SPP_H_DOWNSCALE_ENABLE(reg) \
	REG_SET_BIT(&(reg),bVPU_SPP_H_DOWNSCALE)
#define VPU_SPP_H_DOWNSCALE_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bVPU_SPP_H_DOWNSCALE)

#define VPU_SPP_SET_SPP_PHASE_0_H(reg, val)\
    REG_SET_VAL(&(reg), val);
#define VPU_SPP_SET_SPP_PHASE_0_V(reg, val)\
    REG_SET_VAL(&(reg), val);

#define VPU_SPP_SET_VT_PHASE(reg,value) \
	REG_SET_FIELD(&(reg), mVPU_SPP_VT_PHASE_BIAS,value,bVPU_SPP_VT_PHASE_BIAS)

#define VPU_SPP_SET_VB_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_VB_PHASE_BIAS,value,bVPU_SPP_VB_PHASE_BIAS)

#define VPU_SPP_SET_PLAY_MODE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_PLAY_MODE,value,bVPU_SPP_PLAY_MODE)

//PIC_PARA
enum {
	VPU_SPP_PLAY_MODE_FIELD = 0,
	VPU_SPP_PLAY_MODE_FRAME,
};

enum {
	VPU_SPP_COLOR_TYPE_420 = 0,
	VPU_SPP_COLOR_TYPE_422_Y_CBCR,
	VPU_SPP_COLOR_TYPE_422_CBYCRY,
	VPU_SPP_COLOR_TYPE_6442_YCBCRA,
};

#define bVPU_SPP_BASE_LINE         (0)
#define mVPU_SPP_BASE_LINE         (0x1FFF<<bVPU_SPP_BASE_LINE)
#define VPU_SPP_SET_BASE_LINE(reg,value) \
	GX_SET_FEILD(&(reg),mVPU_SPP_BASE_LINE,value,bVPU_SPP_BASE_LINE)

#define bVPU_SPP_COLOR_TYPE	(30)
#define mVPU_SPP_COLOR_TYPE	(0x3 << bVPU_SPP_COLOR_TYPE)
#define VPU_SPP_SET_COLOR_TYPE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_COLOR_TYPE,value,bVPU_SPP_COLOR_TYPE)

//PIC_Y_TOP_ADDR
#define bVPU_SPP_Y_TOP_ADDR          (0)
#define mVPU_SPP_Y_TOP_ADDR          (SPP_ADDR_MASK<<bVPU_SPP_Y_TOP_ADDR)
#define VPU_SPP_SET_Y_TOP_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_Y_TOP_ADDR,(unsigned int)gx_virt_to_phys(value),bVPU_SPP_Y_TOP_ADDR)
//PIC_Y_BOTTOM_ADDR
#define bVPU_SPP_Y_BOTTOM_ADDR          (0)
#define mVPU_SPP_Y_BOTTOM_ADDR          (SPP_ADDR_MASK<<bVPU_SPP_Y_BOTTOM_ADDR)
#define VPU_SPP_SET_Y_BOTTOM_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_Y_BOTTOM_ADDR,(unsigned int)gx_virt_to_phys(value),bVPU_SPP_Y_BOTTOM_ADDR)
//PIC_UV_TOP_ADDR
#define bVPU_SPP_UV_TOP_ADDR          (0)
#define mVPU_SPP_UV_TOP_ADDR          (SPP_ADDR_MASK<<bVPU_SPP_UV_TOP_ADDR)
#define VPU_SPP_SET_UV_TOP_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_UV_TOP_ADDR,(unsigned int)gx_virt_to_phys(value),bVPU_SPP_UV_TOP_ADDR)
//PIC_UV_BOTTOM_ADDR
#define bVPU_SPP_UV_BOTTOM_ADDR          (0)
#define mVPU_SPP_UV_BOTTOM_ADDR          (SPP_ADDR_MASK<<bVPU_SPP_UV_BOTTOM_ADDR)
#define VPU_SPP_SET_UV_BOTTOM_ADDR(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_UV_BOTTOM_ADDR,(unsigned int)gx_virt_to_phys(value),bVPU_SPP_UV_BOTTOM_ADDR)

//PIC_SIZE
#define bVPU_SPP_PIC_SIZE_WIDTH (0)
#define bVPU_SPP_PIC_SIZE_HIGH  (16)
#define mVPU_SPP_PIC_SIZE_WIDTH (0x7FF << bVPU_SPP_PIC_SIZE_WIDTH)
#define mVPU_SPP_PIC_SIZE_HIGH  (0x7FF << bVPU_SPP_PIC_SIZE_HIGH)

#define VPU_SPP_SET_PIC_SIZE(reg,width,high) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_SPP_PIC_SIZE_WIDTH,width,bVPU_SPP_PIC_SIZE_WIDTH); \
		REG_SET_FIELD(&(reg),mVPU_SPP_PIC_SIZE_HIGH,high,bVPU_SPP_PIC_SIZE_HIGH); \
	}while (0)

//PIC_SOURECE_SIZE
#define bVPU_SPP_PIC_SOURCE_SIZE_WIDTH (0)
#define bVPU_SPP_PIC_SOURCE_SIZE_HIGH  (16)
#define mVPU_SPP_PIC_SOURCE_SIZE_WIDTH (0xFFF << bVPU_SPP_PIC_SOURCE_SIZE_WIDTH)
#define mVPU_SPP_PIC_SOURCE_SIZE_HIGH  (0x7FF << bVPU_SPP_PIC_SOURCE_SIZE_HIGH)

#define VPU_SPP_SET_PIC_SOURCE_SIZE(reg,width,high) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_SPP_PIC_SOURCE_SIZE_WIDTH,width,bVPU_SPP_PIC_SOURCE_SIZE_WIDTH); \
		REG_SET_FIELD(&(reg),mVPU_SPP_PIC_SOURCE_SIZE_HIGH,high,bVPU_SPP_PIC_SOURCE_SIZE_HIGH); \
	}while (0)

//PIC_POSITION
#define bVPU_SPP_PIC_POSITION_X (0)
#define bVPU_SPP_PIC_POSITION_Y (16)
#define mVPU_SPP_PIC_POSITION_X (0x7FF << bVPU_SPP_PIC_POSITION_X)
#define mVPU_SPP_PIC_POSITION_Y (0x7FF << bVPU_SPP_PIC_POSITION_Y)

#define VPU_SPP_SET_PIC_POSITION(reg,x,y) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_SPP_PIC_POSITION_X,x,bVPU_SPP_PIC_POSITION_X); \
		REG_SET_FIELD(&(reg),mVPU_SPP_PIC_POSITION_Y,y,bVPU_SPP_PIC_POSITION_Y); \
	}while (0)

//PIC_ZOOM
#define VPU_SPP_ZOOM_WIDTH (12)
#define VPU_SPP_ZOOM_NONE (1 << VPU_SPP_ZOOM_WIDTH)
#define VPU_SPP_ZOOM_OUT_MAX (VPU_SPP_ZOOM_NONE << 2)

#define bVPU_SPP_VZOOM            (0)
#define bVPU_SPP_HZOOM            (16)
#define mVPU_SPP_VZOOM            (0x7FFF<<bVPU_SPP_VZOOM)
#define mVPU_SPP_HZOOM            (0x7FFF<<bVPU_SPP_HZOOM)

#define VPU_SPP_SET_ZOOM(reg,hzoom,vzoom) \
	do { \
		REG_SET_FIELD(&(reg),mVPU_SPP_HZOOM,hzoom,bVPU_SPP_HZOOM); \
		REG_SET_FIELD(&(reg),mVPU_SPP_VZOOM,vzoom,bVPU_SPP_VZOOM); \
	}while (0)

//PIC_LEN_PHASE
#define bVPU_SPP_REQ_LENGTH       (0)
#define bVPU_SPP_INTP_PHASE       (16)

#define mVPU_SPP_REQ_LENGTH       (0x03FF<<bVPU_SPP_REQ_LENGTH)
#define mVPU_SPP_INTP_PHASE       (0x00FF<<bVPU_SPP_INTP_PHASE)

#define VPU_SPP_SET_LINE_REQ_LENGTH(reg,value) \
        REG_SET_FIELD(&(reg),mVPU_SPP_REQ_LENGTH,value,bVPU_SPP_REQ_LENGTH)

#define VPU_SPP_SET_INTP_PHASE(reg,value) \
	REG_SET_FIELD(&(reg),mVPU_SPP_INTP_PHASE,value,bVPU_SPP_INTP_PHASE)

//BUFFER_CTRL1
#define bVPU_SPP_REQ_DATA_LEN     (16)
#define mVPU_SPP_REQ_DATA_LEN     (0x7FF<<bVPU_SPP_REQ_DATA_LEN)

#define VPU_SPP_SET_BUFFER_REQ_DATA_LEN(reg,value) \
        REG_SET_FIELD(&(reg),mVPU_SPP_REQ_DATA_LEN,value,bVPU_SPP_REQ_DATA_LEN)

//PIC_PARA_V/H
#define VPU_SPP_SET_PHASE(reg,i,Value) \
	REG_SET_VAL(&(reg[i]),Value)

/*****************************************************************************/
/*                                   CE                                      */
/*****************************************************************************/
//CAP_CTRL
#define bVPU_CAP_EN               (0)
#define bVPU_CAP_LEVEL            (1)
#define bVPU_CAP_END              (4)
#define mVPU_CAP_EN               (0x1<<bVPU_CAP_EN)
#define mVPU_CAP_LEVEL            (0x7<<bVPU_CAP_LEVEL)
#define mVPU_CAP_END              (0x1<<bVPU_CAP_END)

#define VPU_CAP_SET_LEVEL(reg,level) \
        REG_SET_FIELD(&(reg),mVPU_CAP_LEVEL,level,bVPU_CAP_LEVEL)

#define VPU_CAP_START(reg) do{\
        REG_CLR_BIT(&(reg),bVPU_CAP_END);\
        REG_CLR_BIT(&(reg),bVPU_CAP_EN); \
        REG_SET_BIT(&(reg),bVPU_CAP_EN); \
        while(!REG_GET_BIT(&(reg),bVPU_CAP_END));\
}while(0)
#define VPU_CAP_STOP(reg)\
	REG_CLR_BIT(&(reg),bVPU_CAP_EN);\

//CAP_ADDR
#define bVPU_CAP_ADDR             (0)
#define mVPU_CAP_ADDR             (CE_ADDR_MASK<<bVPU_CAP_ADDR)

#define VPU_CAP_SET_PIC_ADDR(reg,addr) \
        REG_SET_FIELD(&(reg),mVPU_CAP_ADDR, addr,bVPU_CAP_ADDR)

//CAP_HEIGHT
#define bVPU_CAP_TOP              (0)
#define bVPU_CAP_BOTTOM           (16)
#define mVPU_CAP_TOP              (0x7FF<<bVPU_CAP_TOP)
#define mVPU_CAP_BOTTOM           (0x7FF<<bVPU_CAP_BOTTOM)

#define VPU_CAP_SET_PIC_VERTICAL(reg,top,bottom) \
        REG_SET_FIELD(&(reg),mVPU_CAP_TOP,top,bVPU_CAP_TOP);\
        REG_SET_FIELD(&(reg),mVPU_CAP_BOTTOM,bottom,bVPU_CAP_BOTTOM)

//CAP_WIDTH
#define bVPU_CAP_LEFT             (0)
#define bVPU_CAP_RIGHT            (16)
#define mVPU_CAP_RIGHT            (0x7FF<<bVPU_CAP_RIGHT)
#define mbVPU_CAP_LEFT            (0x7FF<<bVPU_CAP_LEFT)

#define VPU_CAP_SET_PIC_HORIZONTAL(reg,left,right) \
        REG_SET_FIELD(&(reg),mbVPU_CAP_LEFT,left,bVPU_CAP_LEFT);\
        REG_SET_FIELD(&(reg),mVPU_CAP_RIGHT,right,bVPU_CAP_RIGHT)

#define bVPU_CAP_MIX_BKG_EN			(29)
#define VPU_CAP_SET_MIX_BKG_ENABLE(reg) \
        REG_SET_BIT(&(reg), bVPU_CAP_MIX_BKG_EN)
#define VPU_CAP_SET_MIX_BKG_DISENABLE(reg) \
        REG_CLR_BIT(&(reg), bVPU_CAP_MIX_BKG_EN)
/*****************************************************************************/
/*                                   VBI                                     */
/*****************************************************************************/
//VBI_CTRL
#define bVPU_VBI_READ_ADDR        (0)
#define bVPU_VBI_DATA_LEN         (1)
#define bVPU_VBI_ENABLE           (0)
#define mVPU_VBI_DATA_LEN         (0x1F << bVPU_VBI_DATA_LEN)
#define mVPU_VBI_READ_ADDR        (0x3FFFFFF << bVPU_VBI_READ_ADDR)

#define VPU_VBI_START(reg) \
        REG_SET_BIT(&(reg),bVPU_VBI_ENABLE)
#define VPU_VBI_STOP(reg) \
        REG_CLR_BIT(&(reg),bVPU_VBI_ENABLE)
#define VPU_VBI_SET_DATA_LEN(reg,value) \
        REG_SET_FIELD(&(reg), mVPU_VBI_DATA_LEN, value, bVPU_VBI_DATA_LEN)

//VBI_ADDR
#define bVPU_VBI_DATA_ADDR        (0)
#define mVPU_VBI_DATA_ADDR        (0xFFFFFFFF << bVPU_VBI_DATA_ADDR)

#define VPU_VBI_SET_DATA_ADDR(reg,value) \
        REG_SET_FIELD(&(reg), mVPU_VBI_DATA_ADDR, value, bVPU_VBI_DATA_ADDR);

#define VPU_VBI_GET_READ_ADDR(reg,value) do{\
        int val ;\
        val = REG_GET_FIELD(&(reg), mVPU_VBI_DATA_ADDR,bVPU_VBI_DATA_ADDR);\
        value =(void*)val;\
}while(0)

// rDISP_CTRL
#define bVPU_DISP_CTRL_FILED_START        (0)
#define bVPU_DISP_CTRL_FILED_ERROR        (1)
#define bVPU_DISP_CTRL_FILED_ERROR_INT_EN (10)
#define bVPU_DISP_CTRL_FILED_START_INT_EN (11)
#define bVPU_DISP_CTRL_RST                (12)
#define bVPU_DISP_BUFF_CNT                (16)
#define bVPU_DISP_BUFF_ID                (3)

#define mVPU_DISP_CTRL_FILED_ERROR        (0x1  << bVPU_DISP_CTRL_FILED_ERROR)
#define mVPU_DISP_CTRL_FILED_START        (0x1  << bVPU_DISP_CTRL_FILED_START)
#define mVPU_DISP_CTRL_FILED_ERROR_INT_EN (0x1  << bVPU_DISP_CTRL_FILED_ERROR_INT_EN)
#define mVPU_DISP_CTRL_FILED_START_INT_EN (0x1  << bVPU_DISP_CTRL_FILED_START_INT_EN)
#define mVPU_DISP_BUFF_CNT                (0xF  << bVPU_DISP_BUFF_CNT)
#define mVPU_DISP_BUFF_ID                (0x7  << bVPU_DISP_BUFF_ID)

#define VPU_DISP_CLR_FILED_ERROR_INT(reg) \
do{\
    unsigned int value = 0; \
    value = REG_GET_VAL(&(reg)); \
    value &= ~(0x3<<0 ); \
    value |= (1<<bVPU_DISP_CTRL_FILED_ERROR); \
    REG_SET_VAL(&(reg),value); \
}while(0)

#define VPU_DISP_CLR_FILED_START_INT(reg) \
do{\
    unsigned int value = 0; \
    value = REG_GET_VAL(&(reg)); \
    value &= ~(0x3<<0 ); \
    value |= (1<<bVPU_DISP_CTRL_FILED_START); \
    REG_SET_VAL(&(reg),value); \
}while(0)

#define VPU_DISP_FILED_ERROR_INT_EN(reg) \
do{\
    unsigned int value = 0; \
    value = REG_GET_VAL(&(reg)); \
    value &= ~(0x3<<0 ); \
    value |= (1<<bVPU_DISP_CTRL_FILED_ERROR_INT_EN); \
	REG_SET_VAL(&(reg),value); \
}while(0)
#define VPU_DISP_FILED_ERROR_INT_DIS(reg) \
do {\
    unsigned int value = 0; \
    value = REG_GET_VAL(&(reg)); \
    value &= ~(0x3<<0 ); \
    value &= ~(1<<bVPU_DISP_CTRL_FILED_ERROR_INT_EN); \
    REG_SET_VAL(&(reg),value);\
}while(0)

#define VPU_DISP_FILED_START_INT_EN(reg) \
do{\
    unsigned int value = 0; \
    value = REG_GET_VAL(&(reg)); \
    value &= ~(0x3<<0 ); \
    value |= (1<<bVPU_DISP_CTRL_FILED_START_INT_EN); \
	REG_SET_VAL(&(reg),value); \
}while(0)
#define VPU_DISP_FILED_START_INT_DIS(reg) \
do{\
    unsigned int value = 0; \
    value = REG_GET_VAL(&(reg)); \
    value &= ~(0x3<<0 ); \
    value &= ~(1<<bVPU_DISP_CTRL_FILED_START_INT_EN); \
	REG_SET_VAL(&(reg),value); \
}while(0)
#define VPU_DISP_FILED_START_INT_ENABLED(reg)\
	REG_GET_BIT(&(reg), bVPU_DISP_CTRL_FILED_START_INT_EN)

#define VPU_DISP_SET_RST(reg) \
	REG_SET_BIT(&(reg),bVPU_DISP_CTRL_RST)

#define VPU_DISP_CLR_RST(reg) \
	REG_CLR_BIT(&(reg),bVPU_DISP_CTRL_RST)

#define VPU_DISP_GET_BUFF_CNT(reg) \
	REG_GET_FIELD(&(reg), mVPU_DISP_BUFF_CNT, bVPU_DISP_BUFF_CNT)

#define VPU_DISP_GET_BUFF_ID(reg) \
	REG_GET_FIELD(&(reg), mVPU_DISP_BUFF_ID, bVPU_DISP_BUFF_ID)

// TV_DISP_VIEW
#define bVPU_TV_DISP_VIEW_ACTIVE_CNT (0)
#define bVPU_TV_DISP_VIEW_FIELD      (11)

#define mVPU_TV_DISP_VIEW_ACTIVE_CNT (0x7ff << bVPU_TV_DISP_VIEW_ACTIVE_CNT)
#define mVPU_TV_DISP_VIEW_FIELD      (0x1   << bVPU_TV_DISP_VIEW_FIELD)

#define VPU_DISP_GET_VIEW_ACTIVE_CNT(reg)  \
	REG_GET_FIELD(&(reg),mVPU_TV_DISP_VIEW_ACTIVE_CNT,bVPU_TV_DISP_VIEW_ACTIVE_CNT)

#define VPU_DISP_GET_VIEW_FIELD(reg) \
	REG_GET_BIT(&(reg),bVPU_TV_DISP_VIEW_FIELD)

#define VPU_DAC_ENABLE(reg, dac_id)\
do {\
	REG_SET_BIT(&(reg), dac_id);\
	REG_SET_BIT(&(reg), 4);\
}while(0)
#define VPU_DAC_DISABLE(reg, dac_id)\
do {\
	REG_CLR_BIT(&(reg), dac_id);\
	if (CHIP_IS_TAURUS)\
		REG_CLR_BIT(&(reg), 4);\
}while(0)

#define bVPU_OSD_OVERLAY_TOP    (8)
#define bVPU_OSD_OVERLAY_MIDDLE (4)
#define bVPU_OSD_OVERLAY_BOTTOM (0)
#define mVPU_OSD_OVERLAY_TOP    (0x3<<bVPU_OSD_OVERLAY_TOP)
#define mVPU_OSD_OVERLAY_MIDDLE (0x3<<bVPU_OSD_OVERLAY_MIDDLE)
#define mVPU_OSD_OVERLAY_BOTTOM (0x3<<bVPU_OSD_OVERLAY_BOTTOM)

#define OSD_SET_LAYER_SEL(reg, overlay_id, layer)\
	do {\
		if (overlay_id == OSD_ID_TOP) {\
			REG_SET_FIELD(&(reg), mVPU_OSD_OVERLAY_TOP, layer, bVPU_OSD_OVERLAY_TOP);\
		} else if (overlay_id == OSD_ID_MIDDLE) {\
			REG_SET_FIELD(&(reg), mVPU_OSD_OVERLAY_MIDDLE, layer, bVPU_OSD_OVERLAY_MIDDLE);\
		} else if (overlay_id == OSD_ID_BOTTOM) {\
			REG_SET_FIELD(&(reg), mVPU_OSD_OVERLAY_BOTTOM, layer, bVPU_OSD_OVERLAY_BOTTOM);\
		}\
	} while(0)

#define OSD_GET_LAYER_SEL(reg, overlay_id, layer)\
	do {\
		if (overlay_id == OSD_ID_TOP) {\
			layer = REG_GET_FIELD(&(reg), mVPU_OSD_OVERLAY_TOP, bVPU_OSD_OVERLAY_TOP);\
		} else if (overlay_id == OSD_ID_MIDDLE) {\
			layer = REG_GET_FIELD(&(reg), mVPU_OSD_OVERLAY_MIDDLE, bVPU_OSD_OVERLAY_MIDDLE);\
		} else if (overlay_id == OSD_ID_BOTTOM) {\
			layer = REG_GET_FIELD(&(reg), mVPU_OSD_OVERLAY_BOTTOM, bVPU_OSD_OVERLAY_BOTTOM);\
		}\
	} while(0)

#endif

/*Enhance*/

#define bVPU_EHN_GAMMA_R_BYPASS               (0)
#define bVPU_EHN_GAMMA_R_CSC709               (24)
#define bVPU_EHN_GAMMA_R_CRC                  (28)
#define EHN_SET_GAMMA_R_COEF0(reg, bypass_en, csc709_en, crc_en) \
	do {\
		EHN_SET_GAMMA_COEF0(reg, bypass_en); \
		if(csc709_en) {\
			REG_SET_BIT(&(reg), bVPU_EHN_GAMMA_R_CSC709);\
		}\
		if(crc_en) {\
			REG_SET_BIT(&(reg), bVPU_EHN_GAMMA_R_CRC);\
		}\
	}while(0)

#define EHN_GET_GAMMA_R_COEF0(reg, bypass_en, csc709_en, crc_en) \
	do {\
		EHN_GET_GAMMA_COEF0(reg, bypass_en); \
		csc709_en = REG_GET_BIT(&(reg), bVPU_EHN_GAMMA_R_CSC709);\
		crc_en = REG_GET_BIT(&(reg), bVPU_EHN_GAMMA_R_CRC);\
	}while(0)

#define EHN_SET_GAMMA_COEF0(reg, bypass_en) \
	do {\
		if(bypass_en) {\
			REG_SET_BIT(&(reg), bVPU_EHN_GAMMA_R_BYPASS);\
		}else{ \
			REG_CLR_BIT(&(reg), bVPU_EHN_GAMMA_R_BYPASS);\
		}\
	}while(0)

#define EHN_GET_GAMMA_COEF0(reg, bypass_en) \
	do {\
		bypass_en = REG_GET_BIT(&(reg), bVPU_EHN_GAMMA_R_BYPASS);\
	}while(0)


#define EHN_SET_GAMMA_COEF(reg, coef0, coef1, coef2, coef3) \
	do {\
		int val = (coef3 << 24) | (coef2 << 16) | (coef1 << 8) | coef0; \
		REG_SET_VAL(&(reg), val); \
	}while(0)

#define EHN_GET_GAMMA_COEF(reg, coef0, coef1, coef2, coef3) \
	do {\
		int val = REG_GET_VAL(&(reg)); \
		coef3 = (val >> 24) & 0x000000FF; \
		coef2 = (val >> 16) & 0x000000FF; \
		coef3 = (val >> 8) & 0x000000FF; \
		coef3 = (val >> 24) & 0x000000FF; \
	}while(0)

#define bVPU_EHN_GREEN_GAIN   (0)
#define mVPU_EHN_GREEN_GAIN   (0xFF<<bVPU_EHN_GREEN_GAIN)
#define bVPU_EHN_GREEN_BAND   (8)
#define mVPU_EHN_GREEN_BAND   (0x7F<<bVPU_EHN_GREEN_BAND)
#define bVPU_EHN_GREEN_PHASE  (16)
#define mVPU_EHN_GREEN_PHASE  (0x1FF<<bVPU_EHN_GREEN_PHASE)
#define EHN_GREEN_SET(reg, param) \
	do {\
		REG_SET_FIELD(&(reg), mVPU_EHN_GREEN_GAIN, param->green_enhance_gain, bVPU_EHN_GREEN_GAIN);\
		REG_SET_FIELD(&(reg), mVPU_EHN_GREEN_BAND, param->green_enhance_band, bVPU_EHN_GREEN_BAND);\
		REG_SET_FIELD(&(reg), mVPU_EHN_GREEN_PHASE, param->green_phase, bVPU_EHN_GREEN_PHASE);\
	}while(0);
#define EHN_GREEN_GET(reg, param) \
	do {\
		param->green_enhance_gain = REG_GET_FIELD(&(reg), mVPU_EHN_GREEN_GAIN, bVPU_EHN_GREEN_GAIN);\
		param->green_enhance_band = REG_GET_FIELD(&(reg), mVPU_EHN_GREEN_BAND, bVPU_EHN_GREEN_BAND);\
		param->green_phase = REG_GET_FIELD(&(reg), mVPU_EHN_GREEN_PHASE, bVPU_EHN_GREEN_PHASE);\
	}while(0);

#define bVPU_EHN_BLUE_CHROMA_GAIN   (0)
#define mVPU_EHN_BLUE_CHROMA_GAIN   (0xFF<<bVPU_EHN_BLUE_CHROMA_GAIN)
#define bVPU_EHN_BLUE_LUMA_REFER    (8)
#define mVPU_EHN_BLUE_LUMA_REFER    (0xFF<<bVPU_EHN_BLUE_LUMA_REFER)
#define bVPU_EHN_BLUE_LUMA_BAND     (16)
#define mVPU_EHN_BLUE_LUMA_BAND     (0x3F<<bVPU_EHN_BLUE_LUMA_BAND)
#define EHN_BLUE_SET(reg, param) \
	do {\
		REG_SET_FIELD(&(reg), mVPU_EHN_BLUE_CHROMA_GAIN, param->choma_gain, bVPU_EHN_BLUE_CHROMA_GAIN);\
		REG_SET_FIELD(&(reg), mVPU_EHN_BLUE_LUMA_REFER, param->luma_reference, bVPU_EHN_BLUE_LUMA_REFER);\
		REG_SET_FIELD(&(reg), mVPU_EHN_BLUE_LUMA_BAND, param->luma_band, bVPU_EHN_BLUE_LUMA_BAND);\
	}while(0);
#define EHN_BLUE_GET(reg, param) \
	do {\
		param->choma_gain = REG_GET_FIELD(&(reg), mVPU_EHN_BLUE_CHROMA_GAIN, bVPU_EHN_BLUE_CHROMA_GAIN);\
		param->luma_reference = REG_GET_FIELD(&(reg), mVPU_EHN_BLUE_LUMA_REFER, bVPU_EHN_BLUE_LUMA_REFER);\
		param->luma_band = REG_GET_FIELD(&(reg), mVPU_EHN_BLUE_LUMA_BAND, bVPU_EHN_BLUE_LUMA_BAND);\
	}while(0);

#define EHN_SRC_LAYER_VPP                0x01
#define EHN_SRC_LAYER_SPP                0x02
#define EHN_SRC_LAYER_OSD                0x04
#define EHN_SRC_LAYER_SOSD               0x08
#define EHN_SRC_LAYER_SOSD2              0x10

#define bVPU_EHN_PRM_SRC_SEL   (0)
#define mVPU_EHN_PRM_SRC_SEL   (0x1F<<bVPU_EHN_PRM_SRC_SEL)
#define bVPU_EHN_SCN_SRC_SEL   (8)
#define mVPU_EHN_SCN_SRC_SEL   (0x1F<<bVPU_EHN_SCN_SRC_SEL)
#define EHN_SRC_SEL(reg, val, flag) \
	do {\
		if(0 == flag) {\
			REG_SET_FIELD(&(reg), mVPU_EHN_PRM_SRC_SEL, val, bVPU_EHN_PRM_SRC_SEL); \
		}else{ \
			REG_SET_FIELD(&(reg), mVPU_EHN_SCN_SRC_SEL, val, bVPU_EHN_SCN_SRC_SEL); \
		}\
	}while(0);

#define bVPU_EHN_LTI_EN   (24)
#define bVPU_EHN_LTI_TOT_LIMIT   (16)
#define mVPU_EHN_LTI_TOT_LIMIT   (0xFF<<bVPU_EHN_LTI_TOT_LIMIT)
#define bVPU_EHN_LTI_TOT_CORE    (8)
#define mVPU_EHN_LTI_TOT_CORE    (0xFF<<bVPU_EHN_LTI_TOT_CORE)
#define bVPU_EHN_LTI_DIF_CORE    (0)
#define mVPU_EHN_LTI_DIF_CORE    (0xFF<<bVPU_EHN_LTI_DIF_CORE)
#define EHN_LTI_CFG0_SET(reg, en, tot_limit, tot_core, dif_core) \
	do { \
		if(en) { \
			REG_SET_BIT(&(reg), bVPU_EHN_LTI_EN); \
		} else {\
			REG_CLR_BIT(&(reg), bVPU_EHN_LTI_EN); \
		} \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_TOT_LIMIT, tot_limit, bVPU_EHN_LTI_TOT_LIMIT); \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_TOT_CORE, tot_core, bVPU_EHN_LTI_TOT_CORE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_DIF_CORE, dif_core, bVPU_EHN_LTI_DIF_CORE); \
	}while(0);
#define EHN_LTI_CFG0_GET(reg, en, tot_limit, tot_core, dif_core) \
	do { \
		en = REG_GET_BIT(&(reg), bVPU_EHN_LTI_EN); \
		tot_limit = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_TOT_LIMIT, bVPU_EHN_LTI_TOT_LIMIT); \
		tot_core = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_TOT_CORE, bVPU_EHN_LTI_TOT_CORE); \
		dif_core = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_DIF_CORE, bVPU_EHN_LTI_DIF_CORE); \
	}while(0);
#define bVPU_EHN_LTI_GAIN_DLY        (24)
#define mVPU_EHN_LTI_GAIN_DLY        (0x7<<bVPU_EHN_LTI_GAIN_DLY)
#define bVPU_EHN_LTI_TOT_GAIN        (16)
#define mVPU_EHN_LTI_TOT_GAIN        (0x3F<<bVPU_EHN_LTI_TOT_GAIN)
#define bVPU_EHN_LTI_LVL_GAIN        (8)
#define mVPU_EHN_LTI_LVL_GAIN        (0x3F<<bVPU_EHN_LTI_LVL_GAIN)
#define bVPU_EHN_LTI_POS_GAIN        (0)
#define mVPU_EHN_LTI_POS_GAIN        (0x3F<<bVPU_EHN_LTI_POS_GAIN)
#define EHN_LTI_CFG1_SET(reg, gain_delay, tot_gain, lvl_gain, pos_gain) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_GAIN_DLY, gain_delay, bVPU_EHN_LTI_GAIN_DLY); \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_TOT_GAIN, tot_gain, bVPU_EHN_LTI_TOT_GAIN); \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_LVL_GAIN, lvl_gain, bVPU_EHN_LTI_LVL_GAIN); \
		REG_SET_FIELD(&(reg), mVPU_EHN_LTI_POS_GAIN, pos_gain, bVPU_EHN_LTI_POS_GAIN); \
	}while(0);
#define EHN_LTI_CFG1_GET(reg, gain_delay, tot_gain, lvl_gain, pos_gain) \
	do { \
		gain_delay = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_GAIN_DLY, bVPU_EHN_LTI_GAIN_DLY); \
		tot_gain = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_TOT_GAIN, bVPU_EHN_LTI_TOT_GAIN); \
		lvl_gain = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_LVL_GAIN, bVPU_EHN_LTI_LVL_GAIN); \
		pos_gain = REG_GET_FIELD(&(reg), mVPU_EHN_LTI_POS_GAIN, bVPU_EHN_LTI_POS_GAIN); \
	}while(0);

#define bVPU_EHN_PEAKING_ENABLE      (24)
#define bVPU_EHN_PEAKING_FRQ_CORE    (16)
#define mVPU_EHN_PEAKING_FRQ_CORE    (0xFF<<bVPU_EHN_PEAKING_FRQ_CORE)
#define bVPU_EHN_PEAKING_FIR_BK      (8)
#define mVPU_EHN_PEAKING_FIR_BK      (0x1F<<bVPU_EHN_PEAKING_FIR_BK)
#define bVPU_EHN_PEAKING_FIR_HK      (0)
#define mVPU_EHN_PEAKING_FIR_HK      (0x1F<<bVPU_EHN_PEAKING_FIR_HK)
#define EHN_PEAKING_CFG0_SET(reg, en, frequency_core, filter_bk, filter_hk) \
	do { \
		if(en) { \
			REG_SET_BIT(&(reg), bVPU_EHN_PEAKING_ENABLE); \
		} else { \
			REG_CLR_BIT(&(reg), bVPU_EHN_PEAKING_ENABLE); \
		} \
		REG_SET_FIELD(&(reg), mVPU_EHN_PEAKING_FRQ_CORE, frequency_core, bVPU_EHN_PEAKING_FRQ_CORE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_PEAKING_FIR_BK, filter_bk, bVPU_EHN_PEAKING_FIR_BK); \
		REG_SET_FIELD(&(reg), mVPU_EHN_PEAKING_FIR_HK, filter_hk, bVPU_EHN_PEAKING_FIR_HK); \
	}while(0);
#define EHN_PEAKING_CFG0_GET(reg, en, frequency_core, filter_bk, filter_hk) \
	do { \
		en = REG_GET_BIT(&(reg), bVPU_EHN_PEAKING_ENABLE); \
		frequency_core = REG_GET_FIELD(&(reg), mVPU_EHN_PEAKING_FRQ_CORE, bVPU_EHN_PEAKING_FRQ_CORE); \
		filter_bk = REG_GET_FIELD(&(reg), mVPU_EHN_PEAKING_FIR_BK, bVPU_EHN_PEAKING_FIR_BK); \
		filter_hk = REG_GET_FIELD(&(reg), mVPU_EHN_PEAKING_FIR_HK, bVPU_EHN_PEAKING_FIR_HK); \
	}while(0);

#define bVPU_EHN_PEAKING_FRQ_TH      (8)
#define mVPU_EHN_PEAKING_FRQ_TH      (0x0F<<bVPU_EHN_PEAKING_FRQ_TH)
#define bVPU_EHN_PEAKING_WIN_TH      (0)
#define mVPU_EHN_PEAKING_WIN_TH      (0xFF<<bVPU_EHN_PEAKING_WIN_TH)
#define EHN_PEAKING_CFG1_SET(reg, frequency_th, window_th) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_PEAKING_FRQ_TH, frequency_th, bVPU_EHN_PEAKING_FRQ_TH); \
		REG_SET_FIELD(&(reg), mVPU_EHN_PEAKING_WIN_TH, window_th, bVPU_EHN_PEAKING_WIN_TH); \
	}while(0);
#define EHN_PEAKING_CFG1_GET(reg, frequency_th, window_th) \
	do { \
		frequency_th = REG_GET_FIELD(&(reg), mVPU_EHN_PEAKING_FRQ_TH, bVPU_EHN_PEAKING_FRQ_TH); \
		window_th =  REG_GET_FIELD(&(reg), mVPU_EHN_PEAKING_WIN_TH, bVPU_EHN_PEAKING_WIN_TH); \
	}while(0);

#define bVPU_EHN_CTI_ENABLE      (24)
#define bVPU_EHN_CTI_STEP        (20)
#define mVPU_EHN_CTI_STEP        (0x3<<bVPU_EHN_CTI_STEP)
#define bVPU_EHN_CTI_FIR_SEL     (16)
#define mVPU_EHN_CTI_FIR_SEL     (0x3<<bVPU_EHN_CTI_FIR_SEL)
#define bVPU_EHN_CTI_WINDELTA    (8)
#define mVPU_EHN_CTI_WINDELTA    (0xFF<<bVPU_EHN_CTI_WINDELTA)
#define bVPU_EHN_CTI_CORE        (0)
#define mVPU_EHN_CTI_CORE        (0xFF<<bVPU_EHN_CTI_CORE)
#define EHN_CTI_CFG0_SET(reg, en, step, filter_select, window_delta, core) \
	do { \
		if(en) { \
			REG_SET_BIT(&(reg), bVPU_EHN_CTI_ENABLE); \
		} else { \
			REG_CLR_BIT(&(reg), bVPU_EHN_CTI_ENABLE); \
		} \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_STEP, step, bVPU_EHN_CTI_STEP); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_FIR_SEL, filter_select, bVPU_EHN_CTI_FIR_SEL); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_WINDELTA, window_delta, bVPU_EHN_CTI_WINDELTA); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_CORE, core, bVPU_EHN_CTI_CORE); \
	}while(0);
#define EHN_CTI_CFG0_GET(reg, en, step, filter_select, window_delta, core) \
	do { \
		en = REG_GET_BIT(&(reg), bVPU_EHN_CTI_ENABLE); \
		step =  REG_GET_FIELD(&(reg), mVPU_EHN_CTI_STEP, bVPU_EHN_CTI_STEP); \
		filter_select = REG_GET_FIELD(&(reg), mVPU_EHN_CTI_FIR_SEL, bVPU_EHN_CTI_FIR_SEL); \
		window_delta = REG_GET_FIELD(&(reg), mVPU_EHN_CTI_WINDELTA, bVPU_EHN_CTI_WINDELTA); \
		core = REG_GET_FIELD(&(reg), mVPU_EHN_CTI_CORE, bVPU_EHN_CTI_CORE); \
	}while(0);

#define bVPU_EHN_CTI_GAIN_DLY        (16)
#define mVPU_EHN_CTI_GAIN_DLY        (0x7<<bVPU_EHN_CTI_GAIN_DLY)
#define bVPU_EHN_CTI_LVL_GAIN        (8)
#define mVPU_EHN_CTI_LVL_GAIN        (0x1F<<bVPU_EHN_CTI_LVL_GAIN)
#define bVPU_EHN_CTI_DIF_GAIN        (0)
#define mVPU_EHN_CTI_DIF_GAIN        (0x1F<<bVPU_EHN_CTI_DIF_GAIN)
#define EHN_CTI_CFG1_SET(reg, gain_delay, lvl_gain, dif_gain) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_GAIN_DLY, gain_delay, bVPU_EHN_CTI_GAIN_DLY); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_LVL_GAIN, lvl_gain, bVPU_EHN_CTI_LVL_GAIN); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CTI_DIF_GAIN, dif_gain, bVPU_EHN_CTI_DIF_GAIN); \
	}while(0);
#define EHN_CTI_CFG1_GET(reg, gain_delay, lvl_gain, dif_gain) \
	do { \
		gain_delay = REG_GET_FIELD(&(reg), mVPU_EHN_CTI_GAIN_DLY, bVPU_EHN_CTI_GAIN_DLY); \
		lvl_gain = REG_GET_FIELD(&(reg), mVPU_EHN_CTI_LVL_GAIN, bVPU_EHN_CTI_LVL_GAIN); \
		dif_gain = REG_GET_FIELD(&(reg), mVPU_EHN_CTI_DIF_GAIN, bVPU_EHN_CTI_DIF_GAIN); \
	}while(0);

#define bVPU_EHN_BRT_STEP        (16)
#define mVPU_EHN_BRT_STEP        (0xFF<<bVPU_EHN_BRT_STEP)
#define bVPU_EHN_BLEC_GAIN       (8)
#define mVPU_EHN_BLEC_GAIN       (0x1F<<bVPU_EHN_BLEC_GAIN)
#define bVPU_EHN_BLEC_TILT       (0)
#define mVPU_EHN_BLEC_TILT       (0xFF<<bVPU_EHN_BLEC_TILT)
#define EHN_BLEC_BRT_CFG_SET(reg, brt_step, blec_gain, blec_tilt) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_BRT_STEP, brt_step, bVPU_EHN_BRT_STEP); \
		REG_SET_FIELD(&(reg), mVPU_EHN_BLEC_GAIN, blec_gain, bVPU_EHN_BLEC_GAIN); \
		REG_SET_FIELD(&(reg), mVPU_EHN_BLEC_TILT, blec_tilt, bVPU_EHN_BLEC_TILT); \
	}while(0);
#define EHN_BLEC_BRT_CFG_GET(reg, brt_step, blec_gain, blec_tilt) \
	do { \
		brt_step = REG_GET_FIELD(&(reg), mVPU_EHN_BRT_STEP, bVPU_EHN_BRT_STEP); \
		blec_gain = REG_GET_FIELD(&(reg), mVPU_EHN_BLEC_GAIN, bVPU_EHN_BLEC_GAIN); \
		blec_tilt = REG_GET_FIELD(&(reg), mVPU_EHN_BLEC_TILT, bVPU_EHN_BLEC_TILT); \
	}while(0);

#define bVPU_EHN_CON_STEP        (16)
#define mVPU_EHN_CON_STEP        (0x3F<<bVPU_EHN_CON_STEP)
#define bVPU_EHN_SAT_STEP        (8)
#define mVPU_EHN_SAT_STEP        (0x3F<<bVPU_EHN_SAT_STEP)
#define bVPU_EHN_HUE_ANGLE       (0)
#define mVPU_EHN_HUE_ANGLE       (0x7F<<bVPU_EHN_HUE_ANGLE)
#define EHN_HUE_SAT_CON_CFG_SET(reg, con_step, sat_step, hue_angle) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_CON_STEP, con_step, bVPU_EHN_CON_STEP); \
		REG_SET_FIELD(&(reg), mVPU_EHN_SAT_STEP, sat_step, bVPU_EHN_SAT_STEP); \
		REG_SET_FIELD(&(reg), mVPU_EHN_HUE_ANGLE, hue_angle, bVPU_EHN_HUE_ANGLE); \
	}while(0);
#define EHN_HUE_SAT_CON_CFG_GET(reg, con_step, sat_step, hue_angle) \
	do { \
		con_step = REG_GET_FIELD(&(reg), mVPU_EHN_CON_STEP, bVPU_EHN_CON_STEP); \
		sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_SAT_STEP, bVPU_EHN_SAT_STEP); \
		hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_HUE_ANGLE, bVPU_EHN_HUE_ANGLE); \
	}while(0);

#define bVPU_EHN_HEC_ENABLE      (4)
#define bVPU_EHN_HEC_DIFF        (0)
#define mVPU_EHN_HEC_DIFF        (0xF<<bVPU_EHN_HEC_DIFF)
#define EHN_HEC_CFG0_SET(reg, en, hec_diff) \
	do { \
		if(en) { \
			REG_SET_BIT(&(reg), bVPU_EHN_HEC_ENABLE); \
		} else  {\
			REG_CLR_BIT(&(reg), bVPU_EHN_HEC_ENABLE); \
		} \
		REG_SET_FIELD(&(reg), mVPU_EHN_HEC_DIFF, hec_diff, bVPU_EHN_HEC_DIFF); \
	}while(0);
#define EHN_HEC_CFG0_GET(reg, en, hec_diff) \
	do { \
		en = REG_GET_BIT(&(reg), bVPU_EHN_HEC_ENABLE); \
		hec_diff = REG_GET_FIELD(&(reg), mVPU_EHN_HEC_DIFF, bVPU_EHN_HEC_DIFF); \
	}while(0);

#define bVPU_EHN_GRN_SAT         (16)
#define mVPU_EHN_GRN_SAT         (0x3F<<bVPU_EHN_GRN_SAT)
#define bVPU_EHN_CYN_SAT         (8)
#define mVPU_EHN_CYN_SAT         (0x3F<<bVPU_EHN_CYN_SAT)
#define bVPU_EHN_YLW_SAT         (0)
#define mVPU_EHN_YLW_SAT         (0x3F<<bVPU_EHN_YLW_SAT)
#define EHN_HEC_CFG1_SET(reg, grn_sat_step, cyn_sat_step, ylw_sat_step) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_GRN_SAT, grn_sat_step, bVPU_EHN_GRN_SAT); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CYN_SAT, cyn_sat_step, bVPU_EHN_CYN_SAT); \
		REG_SET_FIELD(&(reg), mVPU_EHN_YLW_SAT, ylw_sat_step, bVPU_EHN_YLW_SAT); \
	}while(0);
#define EHN_HEC_CFG1_GET(reg, grn_sat_step, cyn_sat_step, ylw_sat_step) \
	do { \
		grn_sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_GRN_SAT, bVPU_EHN_GRN_SAT); \
		cyn_sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_CYN_SAT, bVPU_EHN_CYN_SAT); \
		ylw_sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_YLW_SAT, bVPU_EHN_YLW_SAT); \
	}while(0);

#define bVPU_EHN_BLU_SAT         (16)
#define mVPU_EHN_BLU_SAT         (0x3F<<bVPU_EHN_BLU_SAT)
#define bVPU_EHN_RED_SAT         (8)
#define mVPU_EHN_RED_SAT         (0x3F<<bVPU_EHN_RED_SAT)
#define bVPU_EHN_MGN_SAT         (0)
#define mVPU_EHN_MGN_SAT         (0x3F<<bVPU_EHN_MGN_SAT)
#define EHN_HEC_CFG2_SET(reg, blu_sat_step, red_sat_step, mgn_sat_step) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_BLU_SAT, blu_sat_step, bVPU_EHN_BLU_SAT); \
		REG_SET_FIELD(&(reg), mVPU_EHN_RED_SAT, red_sat_step, bVPU_EHN_RED_SAT); \
		REG_SET_FIELD(&(reg), mVPU_EHN_MGN_SAT, mgn_sat_step, bVPU_EHN_MGN_SAT); \
	}while(0);
#define EHN_HEC_CFG2_GET(reg, blu_sat_step, red_sat_step, mgn_sat_step) \
	do { \
		blu_sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_BLU_SAT, bVPU_EHN_BLU_SAT); \
		red_sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_RED_SAT, bVPU_EHN_RED_SAT); \
		mgn_sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_MGN_SAT, bVPU_EHN_MGN_SAT); \
	}while(0);

#define bVPU_EHN_GRN_HUE         (16)
#define mVPU_EHN_GRN_HUE         (0x3F<<bVPU_EHN_GRN_HUE)
#define bVPU_EHN_CYN_HUE         (8)
#define mVPU_EHN_CYN_HUE         (0x3F<<bVPU_EHN_CYN_HUE)
#define bVPU_EHN_YLW_HUE         (0)
#define mVPU_EHN_YLW_HUE         (0x3F<<bVPU_EHN_YLW_HUE)
#define EHN_HEC_CFG3_SET(reg, grn_hue_angle, cyn_hue_angle, ylw_hue_angle) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_GRN_HUE, grn_hue_angle, bVPU_EHN_GRN_HUE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_CYN_HUE, cyn_hue_angle, bVPU_EHN_CYN_HUE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_YLW_HUE, ylw_hue_angle, bVPU_EHN_YLW_HUE); \
	}while(0);
#define EHN_HEC_CFG3_GET(reg, grn_hue_angle, cyn_hue_angle, ylw_hue_angle) \
	do { \
		grn_hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_GRN_HUE, bVPU_EHN_GRN_HUE); \
		cyn_hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_CYN_HUE, bVPU_EHN_CYN_HUE); \
		ylw_hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_YLW_HUE, bVPU_EHN_YLW_HUE); \
	}while(0);

#define bVPU_EHN_BLU_HUE         (16)
#define mVPU_EHN_BLU_HUE         (0x3F<<bVPU_EHN_BLU_HUE)
#define bVPU_EHN_RED_HUE         (8)
#define mVPU_EHN_RED_HUE         (0x3F<<bVPU_EHN_RED_HUE)
#define bVPU_EHN_MGN_HUE         (0)
#define mVPU_EHN_MGN_HUE         (0x3F<<bVPU_EHN_MGN_HUE)
#define EHN_HEC_CFG4_SET(reg, blu_hue_angle, red_hue_angle, mgn_hue_angle) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_BLU_HUE, blu_hue_angle, bVPU_EHN_BLU_HUE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_RED_HUE, red_hue_angle, bVPU_EHN_RED_HUE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_MGN_HUE, mgn_hue_angle, bVPU_EHN_MGN_HUE); \
	}while(0);
#define EHN_HEC_CFG4_GET(reg, blu_hue_angle, red_hue_angle, mgn_hue_angle) \
	do { \
		blu_hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_BLU_HUE, bVPU_EHN_BLU_HUE); \
		red_hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_RED_HUE, bVPU_EHN_RED_HUE); \
		mgn_hue_angle = REG_GET_FIELD(&(reg), mVPU_EHN_MGN_HUE, bVPU_EHN_MGN_HUE); \
	}while(0);

#define bVPU_EHN_SCN_HUE_ANGLE         (24)
#define mVPU_EHN_SCN_HUE_ANGLE         (0x7F<<bVPU_EHN_SCN_HUE_ANGLE)
#define bVPU_EHN_SCN_BRT_STEP          (16)
#define mVPU_EHN_SCN_BRT_STEP          (0xFF<<bVPU_EHN_SCN_BRT_STEP)
#define bVPU_EHN_SCN_BLEC_GAIN         (8)
#define mVPU_EHN_SCN_BLEC_GAIN         (0x1F<<bVPU_EHN_SCN_BLEC_GAIN)
#define bVPU_EHN_SCN_BLEC_TILT         (0)
#define mVPU_EHN_SCN_BLEC_TILT         (0xFF<<bVPU_EHN_SCN_BLEC_TILT)
#define EHN_SCN_BLEC_BRT_HUE_SET(reg, hue_angle, brt_step, blec_gain, blec_tilt) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_SCN_HUE_ANGLE, hue_angle, bVPU_EHN_SCN_HUE_ANGLE); \
		REG_SET_FIELD(&(reg), mVPU_EHN_SCN_BRT_STEP, brt_step, bVPU_EHN_SCN_BRT_STEP); \
		REG_SET_FIELD(&(reg), mVPU_EHN_SCN_BLEC_GAIN, blec_gain, bVPU_EHN_SCN_BLEC_GAIN); \
		REG_SET_FIELD(&(reg), mVPU_EHN_SCN_BLEC_TILT, blec_tilt, bVPU_EHN_SCN_BLEC_TILT); \
	}while(0);
#define EHN_SCN_BLEC_BRT_HUE_GET(reg, hue_angle, brt_step, blec_gain, blec_tilt) \
	do { \
		hue_angle =  REG_GET_FIELD(&(reg), mVPU_EHN_SCN_HUE_ANGLE, bVPU_EHN_SCN_HUE_ANGLE); \
		brt_step = REG_GET_FIELD(&(reg), mVPU_EHN_SCN_BRT_STEP, bVPU_EHN_SCN_BRT_STEP); \
		blec_gain = REG_GET_FIELD(&(reg), mVPU_EHN_SCN_BLEC_GAIN, bVPU_EHN_SCN_BLEC_GAIN); \
		blec_tilt = REG_GET_FIELD(&(reg), mVPU_EHN_SCN_BLEC_TILT, bVPU_EHN_SCN_BLEC_TILT); \
	}while(0);

#define bVPU_EHN_SCN_CON_STEP          (8)
#define mVPU_EHN_SCN_CON_STEP          (0x3F<<bVPU_EHN_SCN_CON_STEP)
#define bVPU_EHN_SCN_SAT_STEP          (0)
#define mVPU_EHN_SCN_SAT_STEP          (0x3F<<bVPU_EHN_SCN_SAT_STEP)
#define EHN_SCN_SAT_CON_SET(reg, con_step, sat_step) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_SCN_CON_STEP, con_step, bVPU_EHN_SCN_CON_STEP); \
		REG_SET_FIELD(&(reg), mVPU_EHN_SCN_SAT_STEP, sat_step, bVPU_EHN_SCN_SAT_STEP); \
	}while(0);
#define EHN_SCN_SAT_CON_GET(reg, con_step, sat_step) \
	do { \
		con_step = REG_GET_FIELD(&(reg), mVPU_EHN_SCN_CON_STEP, bVPU_EHN_SCN_CON_STEP); \
		sat_step = REG_GET_FIELD(&(reg), mVPU_EHN_SCN_SAT_STEP, bVPU_EHN_SCN_SAT_STEP); \
	}while(0);

#define bVPU_EHN_MIX_COE0              (0)
#define mVPU_EHN_MIX_COE0              (0x3FF<<bVPU_EHN_MIX_COE0)
#define bVPU_EHN_MIX_COE1              (10)
#define mVPU_EHN_MIX_COE1              (0x3FF<<bVPU_EHN_MIX_COE1)
#define bVPU_EHN_MIX_COE2              (20)
#define mVPU_EHN_MIX_COE2              (0x3FF<<bVPU_EHN_MIX_COE2)
#define EHN_MIX_0_FILTER_SET(reg, coef0, coef1, coef2) \
	do { \
		REG_SET_FIELD(&(reg), mVPU_EHN_MIX_COE0, coef0, bVPU_EHN_MIX_COE0); \
		REG_SET_FIELD(&(reg), mVPU_EHN_MIX_COE1, coef1, bVPU_EHN_MIX_COE1); \
		REG_SET_FIELD(&(reg), mVPU_EHN_MIX_COE2, coef2, bVPU_EHN_MIX_COE2); \
	}while(0);
#define EHN_MIX_0_FILTER_GET(reg, coef0, coef1, coef2) \
	do { \
		coef0 = REG_GET_FIELD(&(reg), mVPU_EHN_MIX_COE0, bVPU_EHN_MIX_COE0); \
		coef1 = REG_GET_FIELD(&(reg), mVPU_EHN_MIX_COE1, bVPU_EHN_MIX_COE1); \
		coef2 = REG_GET_FIELD(&(reg), mVPU_EHN_MIX_COE2, bVPU_EHN_MIX_COE2); \
	}while(0);

#define bVPU_EHN_MIX_COE3              (0)
#define mVPU_EHN_MIX_COE3              (0x3FF<<bVPU_EHN_MIX_COE3)
#define bVPU_EHN_MIX_COE4              (10)
#define mVPU_EHN_MIX_COE4              (0x3FF<<bVPU_EHN_MIX_COE1)
#define bVPU_EHN_MIX_FIR_BYPASS        (20)
#define EHN_MIX_1_FILTER_SET(reg, fir_bypass, coef3, coef4) \
	do { \
		if(fir_bypass) { \
			REG_SET_BIT(&(reg), bVPU_EHN_MIX_FIR_BYPASS); \
		} else { \
			REG_CLR_BIT(&(reg), bVPU_EHN_MIX_FIR_BYPASS); \
		} \
		REG_SET_FIELD(&(reg), mVPU_EHN_MIX_COE3, coef3, bVPU_EHN_MIX_COE3); \
		REG_SET_FIELD(&(reg), mVPU_EHN_MIX_COE4, coef4, bVPU_EHN_MIX_COE4); \
	}while(0);
#define EHN_MIX_1_FILTER_GET(reg, fir_bypass, coef3, coef4) \
	do { \
		fir_bypass = REG_GET_BIT(&(reg), bVPU_EHN_MIX_FIR_BYPASS); \
		coef3 = REG_GET_FIELD(&(reg), mVPU_EHN_MIX_COE3, bVPU_EHN_MIX_COE3); \
		coef4 = REG_GET_FIELD(&(reg), mVPU_EHN_MIX_COE4, bVPU_EHN_MIX_COE4); \
	}while(0);


