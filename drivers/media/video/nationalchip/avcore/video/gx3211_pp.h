#ifndef __PP_REG_H__
#define __PP_REG_H__
#include "gxav_bitops.h"

struct gx3211_pp_reg {
	unsigned int rPP_DINTL_WEIGHT; // 0x00
	unsigned int rPP_DNOIS_PARAMS; // 0x04
	unsigned int rPP_DINTL_PARAMS; // 0x08

	unsigned int rPP_Y_PRE_ADDR;   // 0x0C
	unsigned int rPP_Y_CUR_ADDR;   // 0x10
	unsigned int rPP_Y_NXT_ADDR;   // 0x14
	unsigned int rPP_Y_NX2_ADDR;   // 0x18
	unsigned int rPP_Y_CURW_ADDR;  // 0x1C
	unsigned int rPP_Y_NXTW_ADDR;  // 0x20
	unsigned int rPP_Y_NXTD_ADDR;  // 0x24
	unsigned int rPP_Y_NX2D_ADDR;  // 0x28
	unsigned int rPP_Y_CURZ_ADDR;  // 0x2C
	unsigned int rPP_Y_NXTZ_ADDR;  // 0x30

	unsigned int rPP_UV_PRE_ADDR;  // 0x34
	unsigned int rPP_UV_CUR_ADDR;  // 0x38
	unsigned int rPP_UV_NXT_ADDR;  // 0x3C
	unsigned int rPP_UV_NX2_ADDR;  // 0x40
	unsigned int rPP_UV_CURW_ADDR; // 0x44
	unsigned int rPP_UV_NXTW_ADDR; // 0x48
	unsigned int rPP_UV_NXTD_ADDR; // 0x4C
	unsigned int rPP_UV_NX2D_ADDR; // 0x50
	unsigned int rPP_UV_CURZ_ADDR; // 0x54
	unsigned int rPP_UV_NXTZ_ADDR; // 0x58

	unsigned int rPP_REF_FRAME_SIZE;     // 0x5C
	unsigned int rPP_REF_FRAME_STRIDE;   // 0x60
	unsigned int rPP_DINTL_FRAME_STRIDE; // 0x64
	unsigned int rPP_DNOIS_FRAME_STRIDE; // 0x68
	unsigned int rPP_ZOOM_FRAME_STRIDE;  // 0x6C
	unsigned int rPP_INT_CTRL;           // 0x70
};

#define bPP_START (0)
#define PP_START(reg)\
	do {\
		REG_CLR_BIT(&(reg), bPP_START);\
		REG_SET_BIT(&(reg), bPP_START);\
	}while(0)
#define PP_STOP(reg)\
		REG_CLR_BIT(&(reg), bPP_START);\

#define bPP_INT_EN (1)
#define PP_INT_ENABLE(reg)\
		REG_SET_BIT(&(reg), bPP_INT_EN);
#define PP_INT_DISABLE(reg)\
		REG_CLR_BIT(&(reg), bPP_INT_EN);

#define bPP_INT_CLR (3)
#define PP_INT_CLR(reg)\
		REG_SET_BIT(&(reg), bPP_INT_CLR);

#define bPP_ENDIAN (4)
#define mPP_ENDIAN (0x3<<bPP_ENDIAN)
#define PP_SET_ENDIAN(reg, endian)\
	REG_SET_FIELD(&(reg), mPP_ENDIAN, endian, bPP_ENDIAN)

typedef enum {
	MODE_NONE                = 0,
	MODE_DENOISE             = 1,
	MODE_DEINTERLACE         = 2,
	MODE_DENOISE_DEINTERLACE = 3,
	MODE_DEINTERLACE_ZOOM    = 4,
	MODE_INTERLACE_ZOOM      = 5,
	MODE_PROGRESSIVE_ZOOM    = 6,
}ProcessMode;
#define bPP_Y_MODE (6)
#define mPP_Y_MODE (0x7<<bPP_Y_MODE)
#define PP_SET_Y_MODE(reg, mode)\
	REG_SET_FIELD(&(reg), mPP_Y_MODE, mode, bPP_Y_MODE)
#define bPP_UV_MODE (9)
#define mPP_UV_MODE (0x7<<bPP_UV_MODE)
#define PP_SET_UV_MODE(reg, mode)\
	REG_SET_FIELD(&(reg), mPP_UV_MODE, mode, bPP_UV_MODE)

#define PP_SET_PROCESS_MODE(reg, y_mode, uv_mode)\
	do {\
		PP_SET_Y_MODE(reg, y_mode);\
		PP_SET_UV_MODE(reg, uv_mode);\
	}while(0)

typedef enum {
	ZOOM_NONE    = 0,
	ZOOM_HALF    = 1,
	ZOOM_QUARTER = 2,
}ZoomLevel;
#define bPP_ZOOMLEVEL_H (12)
#define mPP_ZOOMLEVEL_H (0x3<<bPP_ZOOMLEVEL_H)
#define PP_SET_ZOOMLEVEL_H(reg, level)\
	REG_SET_FIELD(&(reg), mPP_ZOOMLEVEL_H, level, bPP_ZOOMLEVEL_H)

#define bPP_ZOOMLEVEL_V (14)
#define mPP_ZOOMLEVEL_V (0x3<<bPP_ZOOMLEVEL_H)
#define PP_SET_ZOOMLEVEL_V(reg, level)\
	REG_SET_FIELD(&(reg), mPP_ZOOMLEVEL_V, level, bPP_ZOOMLEVEL_V)


#define bPP_HEIGHT (0)
#define mPP_HEIGHT (0xfff<<bPP_HEIGHT)
#define bPP_WIDTH  (12)
#define mPP_WIDTH  (0xfff<<bPP_WIDTH)
#define PP_SET_FRAMESIZE(reg, w, h)\
	do {\
		REG_SET_FIELD(&(reg), mPP_WIDTH,  w, bPP_WIDTH);\
		REG_SET_FIELD(&(reg), mPP_HEIGHT, h, bPP_HEIGHT);\
	}while(0)

#define bPP_Y_STRIDE  (0)
#define mPP_Y_STRIDE  (0xfff<<bPP_Y_STRIDE)
#define bPP_UV_STRIDE (12)
#define mPP_UV_STRIDE (0xfff<<bPP_UV_STRIDE)
#define PP_SET_STRIDE(reg, y_stride, uv_stride)\
	do {\
		REG_SET_FIELD(&(reg), mPP_Y_STRIDE,  y_stride,  bPP_Y_STRIDE);\
		REG_SET_FIELD(&(reg), mPP_UV_STRIDE, uv_stride, bPP_UV_STRIDE);\
	}while(0)

#define PP_SET_ADDR(reg, addr)\
	REG_SET_VAL(&(reg), addr)
#endif
