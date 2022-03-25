#ifndef __GX3201_PP_H__
#define __GX3201_PP_H__

struct gx3201_pp_reg{
	unsigned int    rINTER_DN_CFG0;    // 0x00
	unsigned int    rINTER_DN_CFG1;    // 0x04
	unsigned int    rINTER_DN_CFG2;    // 0x08
	unsigned int    rDINTERLACE_CFG0;  // 0x0C
	unsigned int    rDINTERLACE_CFG1;  // 0x10
	unsigned int    rDINTERLACE_CFG2;  // 0x14
	unsigned int    rPP_Y_PRE_ADDR;    // 0x18
	unsigned int    rPP_Y_PRC_ADDR;    // 0x1C
	unsigned int    rPP_Y_CUR_ADDR;    // 0x20
	unsigned int    rPP_Y_NEXT_ADDR;   // 0x24
	unsigned int    rPP_Y_NEXT2_ADDR;  // 0x28
	unsigned int    rPP_Y_DN_ADDR0;    // 0x2C
	unsigned int    rPP_Y_DNINT_ADDR0; // 0x30
	unsigned int    rPP_Y_DN_ADDR1;    // 0x34
	unsigned int    rPP_Y_DNINT_ADDR1; // 0x38

	unsigned int    rCLIPPED_X_START;  // 0x3C
	unsigned int    rCLIPPED_X_END;    // 0x40
	unsigned int    rCLIPPED_Y_START;  // 0x44
	unsigned int    rCLIPPED_Y_END;    // 0x48
	unsigned int    rPIC_REC;          // 0x4C
	unsigned int    rFRAME_BUF_STRIDE; // 0x50
	unsigned int    rPP_INT_CTRL;      // 0x54

	unsigned int    rPP_U_PRE_ADDR;    // 0x58
	unsigned int    rPP_U_CUR_ADDR;    // 0x5C
	unsigned int    rPP_U_NXT_ADDR;    // 0x60
	unsigned int    rPP_U_NX2_ADDR;    // 0x64
	unsigned int    rPP_U_DN_ADDR0;    // 0x68
	unsigned int    rPP_U_DN_ADDR1;    // 0x6C
	unsigned int    rPP_V_PRE_ADDR;    // 0x70
	unsigned int    rPP_V_CUR_ADDR;    // 0x74
	unsigned int    rPP_V_NXT_ADDR;    // 0x78
	unsigned int    rPP_V_NX2_ADDR;    // 0x7C
	unsigned int    rPP_V_DN_ADDR0;    // 0x80
	unsigned int    rPP_V_DN_ADDR1;    // 0x84
	unsigned int    rRESERVED_A[30];

	unsigned int    rPP_ZOOM_CFG;			// 0x100
	unsigned int    rPP_ZOOM_PIC_INFO;
	unsigned int    rPP_ZOOM_PIC_SIZE;
	unsigned int    rPP_ZOOM_PIC_STRIDE;
	unsigned int    rPP_ZOOM_YCUR_ADDR;		// 0x110
	unsigned int    rPP_ZOOM_YCUR_STORE_ADDR;
	unsigned int    rPP_ZOOM_UVCUR_ADDR;
	unsigned int    rPP_ZOOM_UVCUR_STORE_ADDR;
	unsigned int    rPP_ZOOM_VCUR_ADDR;		// 0x120
	unsigned int    rPP_ZOOM_VCUR_STORE_ADDR;
	unsigned int    rPP_ZOOM_BANDWIDTH_CTRL;	// 0x128
};

#define bPP_START_NEW                               (0)
#define PP_START_NEW_ENABLE(reg) \
	REG_SET_BIT(&(reg),bPP_START_NEW)
#define PP_START_NEW_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bPP_START_NEW)

#define bPP_DEMOTION_EN                             (1)
#define PP_DEMOTION_ENABLE(reg) \
	REG_SET_BIT(&(reg),bPP_DEMOTION_EN)
#define PP_DEMOTION_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bPP_DEMOTION_EN)

#define bPP_CAL_END_INT_EN                          (2)
#define PP_CAL_END_INT_ENABLE(reg) \
	REG_SET_BIT(&(reg),bPP_CAL_END_INT_EN)
#define PP_CAL_END_INT_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bPP_CAL_END_INT_EN)

#define bPP_INT_STATUS				    (3)
#define bPP_INT_CLEAN                               (4)
#define PP_INT_STATUS_CLR(reg) \
	do {\
		do{\
			REG_SET_BIT(&(reg),bPP_INT_CLEAN) ;\
		}while(REG_GET_BIT(&(reg),bPP_INT_STATUS)); \
	}while(0)


#define bPP_UV_INTERLACED                           (5)
#define PP_UV_INTERLACED(reg) \
	REG_SET_BIT(&(reg),bPP_UV_INTERLACED)
#define PP_UV_DE_INTERLACED(reg) \
	REG_CLR_BIT(&(reg),bPP_UV_INTERLACED)

#define bPP_ENDIAN_MODE                             (6)
#define mPP_ENDIAN_MODE                             (0x03 << bPP_ENDIAN_MODE   )
#define PP_ENDIAN_MODE_SET(reg,value) \
	REG_SET_FIELD(&(reg), mPP_ENDIAN_MODE, value, bPP_ENDIAN_MODE)

#define bPP_WINDOW_MODE                             (8)
#define mPP_WINDOW_MODE                             (0x03 << bPP_ENDIAN_MODE   )
#define PP_WINDOW_MODE_SET(reg,value) \
	REG_SET_FIELD(&(reg), mPP_WINDOW_MODE, value, bPP_WINDOW_MODE)

#define bPP_Y_WORK_MODE                            (10)
#define mPP_Y_WOR_MODE                             (0x03 << bPP_Y_WORK_MODE   )
#define PP_Y_WOR_MODE_SET(reg,value) \
	REG_SET_FIELD(&(reg), mPP_Y_WOR_MODE, value, bPP_Y_WORK_MODE)

#define bPP_UV_WORK_MODE                          (12)
#define PP_UV_DOMOTION_ENABLE(reg) \
	REG_SET_BIT(&(reg),bPP_UV_WORK_MODE)
#define PP_UV_DOMOTION_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bPP_UV_WORK_MODE)

#define PP_ADDR_SET(reg,value)\
	REG_SET_VAL(&(reg), value)

#define bPP_ZOOM_OUT_HX		(0)
#define bPP_ZOOM_OUT_VX		(4)
#define bPP_ZOOM_OUT_EN		(31)
#define mPP_ZOOM_OUT_HX		(0x7<<bPP_ZOOM_OUT_HX)
#define mPP_ZOOM_OUT_VX		(0x7<<bPP_ZOOM_OUT_VX)
#define PP_SET_H_MINIFICATION(reg,value)\
	REG_SET_FIELD(&(reg), mPP_ZOOM_OUT_HX, value, bPP_ZOOM_OUT_HX)
#define PP_SET_V_MINIFICATION(reg,value)\
	REG_SET_FIELD(&(reg), mPP_ZOOM_OUT_VX, value, bPP_ZOOM_OUT_VX)
#define PP_SET_ZOOM_OUT_ENABLE(reg) \
	REG_SET_BIT(&(reg),bPP_ZOOM_OUT_EN)
#define PP_SET_ZOOM_OUT_DISABLE(reg) \
	REG_CLR_BIT(&(reg),bPP_ZOOM_OUT_EN)

#define bPP_TOP_FIELD_FIRST	(0)
#define mPP_TOP_FIELD_FIRST	(1<<bPP_TOP_FIELD_FIRST)
#define bPP_FRAME_PIC		(4)
#define mPP_FRAME_PIC		(1<<bPP_FRAME_PIC)
#define PP_SET_TOP_FIELD_FIRST(reg,value) \
	REG_SET_FIELD(&(reg), mPP_TOP_FIELD_FIRST, value, bPP_TOP_FIELD_FIRST)
#define PP_SET_FRAME_PIC(reg,value) \
	REG_SET_FIELD(&(reg), mPP_FRAME_PIC, value, bPP_FRAME_PIC)

#define PP_SET_ZOOM_FRAME_IN_INFO(reg,value) \
	REG_SET_VAL(&(reg), value)
#define PP_SET_ZOOM_FRAME_IN_STRIDE(reg,value) \
	REG_SET_VAL(&(reg), value)

#define PP_SET_Y_CUR_ADDR(reg, value) \
	REG_SET_VAL(&(reg), value)
#define PP_SET_UV_CUR_ADDR(reg, value) \
	REG_SET_VAL(&(reg), value)
#define PP_SET_Y_CUR_STORE_ADDR(reg, value) \
	REG_SET_VAL(&(reg), value)
#define PP_SET_UV_CUR_STORE_ADDR(reg, value) \
	REG_SET_VAL(&(reg), value)

#endif
