#ifndef __CYGNUS_VPU_REG_H__
#define __CYGNUS_VPU_REG_H__

#include "gxav_bitops.h"

#include "ce_reg.h"
#include "sys_reg.h"
#include "osd_reg.h"
#include "pic_reg.h"
#include "pp_reg.h"
#include "pp2_reg.h"
#include "hdr_reg.h"

#define OSD_ADDR_MASK      (0xFFFFFFFF)
#define OSD_BASE_LINE_MASK (0x0001FFF)

#define VPU_VPP_ZOOM_WIDTH (12)
#define VPU_VPP_ZOOM_NONE (1 << VPU_VPP_ZOOM_WIDTH)
#define VPU_VPP_ZOOM_OUT_MAX (VPU_VPP_ZOOM_NONE << 2)

#define VPU_SPP_ZOOM_WIDTH (12)
#define VPU_SPP_ZOOM_NONE (1 << VPU_SPP_ZOOM_WIDTH)
#define VPU_SPP_ZOOM_OUT_MAX (VPU_SPP_ZOOM_NONE << 1)

//PIC_PARA
enum {
	VPU_SPP_PLAY_MODE_FIELD = 0,
	VPU_SPP_PLAY_MODE_FRAME,
};

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

enum {
	CYGNUS_VPU_SUB_SYS   = 0,
	CYGNUS_VPU_SUB_PP0   = 1,
	CYGNUS_VPU_SUB_PP1   = 2,
	CYGNUS_VPU_SUB_PIC   = 3,
	CYGNUS_VPU_SUB_OSD0  = 4,
	CYGNUS_VPU_SUB_OSD1  = 5,
	CYGNUS_VPU_SUB_PP2   = 6,
	CYGNUS_VPU_SUB_CE    = 7,
	CYGNUS_VPU_SUB_VOUT0 = 8,
	CYGNUS_VPU_SUB_VOUT1 = 9,
	CYGNUS_VPU_SUB_HDR   = 10,
	CYGNUS_VPU_SUB_MAX,
};

int cygnus_vpu_reg_iounmap(void);
int cygnus_vpu_reg_ioremap(void);
void* get_reg_by_sub_id(int id);

#endif

