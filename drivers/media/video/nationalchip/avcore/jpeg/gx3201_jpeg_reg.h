/*****************************************************************************
 * 						   CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2011, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	gx3201_jpeg_reg.h
 * Author    : 	lvjh
 * Project   :	GoXceed
 * Type      :	Driver
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0  	2011.12.28	      lvjh	         creation
 *****************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __GX3201_JPEG_REG_H__
#define __GX3201_JPEG_REG_H__

/* Includes --------------------------------------------------------------- */
#include "gxav_bitops.h"

/* Exported Types --------------------------------------------------------- */
struct jpeg_regs
{
	unsigned int rDECODE_CTRL; //0x00
	unsigned int rBS_BUFFER_START_ADDR;
	unsigned int rBS_BUFFER_SIZE;
	unsigned int rBS_BUFFER_ALMOST_EMPTY_TH;
	unsigned int rBS_BUFFER_WR_PTR; //0x10
	unsigned int rBS_BUFFER_RD_PTR;
	unsigned int rFRAME_BUFFER_Y_BASE_ADDR;
	unsigned int rFRAME_BUFFER_CB_BASE_ADDR;
	unsigned int rFRAME_BUFFER_CR_BASE_ADDR; //0x20
	unsigned int rFRAME_BUFFER_STRIDE;
	unsigned int rFRAME_BUFFER_MAX_PIC_SIZE;
	unsigned int rCLIP_UPPER_LEFT_COORDINATE;
	unsigned int rCLIP_LOWER_RIGHT_COORDINATE; //0x30
	unsigned int rDECODE_DEAD_OVER_TIME_GATE;
	unsigned int rDECODE_PIC_LINE_CNT;
	unsigned int rPIC_INFO;
	unsigned int rPIC_ORIGION_SIZE_INFO;
	unsigned int rPIC_WRITE_BACK_SIZE_INFO; //0x40
	unsigned int rPIC_DISPLAY_SIZE_INFO;
	unsigned int rPIC_DISPLAY_COORDINATE_INFO;
	unsigned int rPIC_INFO_WB_LAYER;  //0x50
	unsigned int rINT_STATUS;
	unsigned int rINT_ENABLE_CPU0;
	unsigned int rINT_ENABLE_CPU1;
	unsigned int rINT_ENABLE_CPU2;
	unsigned int rINT_ENABLE_CPU3;
	unsigned int rDEBUG_INFO; //0x60
};

/*****************************************************************************/
/*                                   SET CLR FIELD                           */
/*****************************************************************************/

#define JPEG_REG_SET_FIELD(reg,mask,value,offset) \
	REG_SET_FIELD(&(reg),mask, value,offset)

#define JPEG_REG_SET_BIT(reg,bit) \
	REG_SET_BIT(&(reg),bit)

#define JPEG_REG_CLR_BIT(reg,bit) \
	REG_CLR_BIT(&(reg),bit)


/*****************************************************************************/
/*                         JPEG REGISTER CONFIG                              */
/*****************************************************************************/

//rDECODE_CTRL
#define bJPEG_DECODER_RUN               (0)
#define bJPEG_DECODER_GO_ON             (1)
#define bJPEG_DECODE_BUSY               (2)
#define bJPEG_DECODE_PAUSE_SOF          (3)
#define bJPEG_DECODE_PAUSE_PER_SLICE    (4)
#define bJPEG_CLIP_MODE                 (5)
#define bJPEG_BS_BUF_DATA_END           (6)
#define bJPEG_RESAMPLE_MODE             (8)
#define bJPEG_FRAME_BUFFER_BIG_ENDIAN   (12)
#define bJPEG_BS_BUFFER_BIG_ENDIAN      (14)
#define bJPEG_RESAMPLE_RATIO_MANUAL_V   (16)
#define bJPEG_RESAMPLE_RATIO_MANUAL_H   (18)
#define bJPEG_RESAMPLE_RATIO_AUTO_V     (20)
#define bJPEG_RESAMPLE_RATIO_AUTO_H     (22)
#define bJPEG_CPU0_INT_ENABLE           (24)
#define bJPEG_CPU1_INT_ENABLE           (25)
#define bJPEG_CPU2_INT_ENABLE           (26)
#define bJPEG_CPU3_INT_ENABLE           (27)

#define mJPEG_DECODER_RUN               (0x1<<bJPEG_DECODER_RUN)
#define mJEPG_DECODER_GO_ON             (0x1<<bJPEG_DECODER_GO_ON)
#define mJPEG_DECODE_BUSY               (0x1<<bJPEG_DECODE_BUSY)
#define mJPEG_DECODE_PAUSE_SOF          (0x1<<bJPEG_DECODE_PAUSE_SOF)
#define mJPEG_DECODE_PAUSE_PER_SLICE    (0x1<<bJPEG_DECODE_PAUSE_PER_SLICE)
#define mJPEG_CLIP_MODE                 (0x1<<bJPEG_CLIP_MODE)
#define mJPEG_BS_BUF_DATA_END           (0x1<<bJPEG_BS_BUF_DATA_END)
#define mJPEG_RESAMPLE_MODE             (0x3<<bJPEG_RESAMPLE_MODE)
#define mJPEG_RESAMPLE_RATIO_MANUAL_H   (0x3<<bJPEG_RESAMPLE_RATIO_MANUAL_H)
#define mJPEG_RESAMPLE_RATIO_MANUAL_V   (0x3<<bJPEG_RESAMPLE_RATIO_MANUAL_V)
#define mJPEG_RESAMPLE_RATIO_AUTO_H     (0x3<<bJPEG_RESAMPLE_RATIO_AUTO_H)
#define mJPEG_RESAMPLE_RATIO_AUTO_V     (0x3<<bJPEG_RESAMPLE_RATIO_AUTO_V)

#define mJPEG_FRAME_BUFFER_BIG_ENDIAN   (0x3<<bJPEG_FRAME_BUFFER_BIG_ENDIAN)
#define mJPEG_BS_BUFFER_BIG_ENDIAN      (0x3<<bJPEG_BS_BUFFER_BIG_ENDIAN)
#define mJPEG_CPU0_INT_ENABLE           (0x1<<bJPEG_CPU0_INT_ENABLE)
#define mJPEG_CPU1_INT_ENABLE           (0x1<<bJPEG_CPU1_INT_ENABLE)
#define mJPEG_CPU2_INT_ENABLE           (0x1<<bJPEG_CPU2_INT_ENABLE)
#define mJPEG_CPU3_INT_ENABLE           (0x1<<bJPEG_CPU3_INT_ENABLE)

#define JPEG_DECODE_RUN_SET_START(reg) \
	REG_SET_BIT(&(reg),bJPEG_DECODER_RUN)

#define JPEG_DECODE_RUN_SET_STOP(reg) \
	REG_CLR_BIT(&(reg),bJPEG_DECODER_RUN)

#define JPEG_DECODE_RUN_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODER_RUN)

#define JPEG_DECODE_GO_ON_SET(reg) \
	REG_SET_BIT(&(reg),bJPEG_DECODER_GO_ON)

#define JPEG_DECODE_GO_ON_CLR(reg) \
	REG_CLR_BIT(&(reg),bJPEG_DECODER_GO_ON)

#define JPEG_DECODE_GO_ON_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODER_GO_ON)

#define JPEG_DECODE_BUSY_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODE_BUSY)

#define JPEG_SET_DECODE_END_FILE(reg) \
	REG_SET_BIT(&(reg),bJPEG_BS_BUF_DATA_END)

#define JPEG_CLR_DECODE_END_FILE(reg) \
	REG_CLR_BIT(&(reg),bJPEG_BS_BUF_DATA_END)

#define JPEG_DECODE_PAUSE_SOF_SET_CAN(reg) \
	REG_SET_BIT(&(reg),bJPEG_DECODE_PAUSE_SOF)

#define JPEG_DECODE_PAUSE_SOF_SET_NOT(reg) \
	REG_CLR_BIT(&(reg), bJPEG_DECODE_PAUSE_SOF)

#define JPEG_DECODE_PAUSE_SOF_GET(reg) \
	REG_GET_BIT(&(reg), bJPEG_DECODE_PAUSE_SOF)

#define JPEG_DECODE_PAUSE_PER_SLICE_SET_CAN(reg) \
	REG_SET_BIT(&(reg), bJPEG_DECODE_PAUSE_PER_SLICE)

#define JPEG_DECODE_PAUSE_PER_SLICE_SET_NOT(reg) \
	REG_CLR_BIT(&(reg), bJPEG_DECODE_PAUSE_PER_SLICE)

#define JPEG_DECODE_PAUSE_PER_SLICE_GET(reg) \
	REG_GET_BIT(&(reg), bJPEG_DECODE_PAUSE_PER_SLICE)

#define JPEG_CLIP_MODE_SET_NORMAL(reg) \
	REG_CLR_BIT(&(reg), bJPEG_CLIP_MODE)

#define JPEG_CLIP_MODE_SET_CLIP(reg) \
	REG_SET_BIT(&(reg), bJPEG_CLIP_MODE)

#define JPEG_CLIP_MODE_GET(reg) \
	REG_GET_BIT(&(reg), bJPEG_CLIP_MODE)

#define JPEG_BS_BUF_DATA_END_SET(reg) \
	REG_CLR_BIT(&(reg), bJPEG_BS_BUF_DATA_END)

#define JPEG_BS_BUF_DATA_END_CLR(reg) \
	REG_SET_BIT(&(reg), bJPEG_BS_BUF_DATA_END)

#define JPEG_BS_BUF_DATA_END_GET(reg) \
	REG_GET_BIT(&(reg), bJPEG_BS_BUF_DATA_END)

#define JPEG_RESAMPLE_MODE_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_RESAMPLE_MODE, val, bJPEG_RESAMPLE_MODE)

#define JPEG_RESAMPLE_MODE_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_RESAMPLE_MODE, bJPEG_RESAMPLE_MODE)

#define JPEG_RESAMPLE_RATIO_MANUAL_H_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_MANUAL_H, val, bJPEG_RESAMPLE_RATIO_MANUAL_H)

#define JPEG_RESAMPLE_RATIO_MANUAL_H_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_MANUAL_H, bJPEG_RESAMPLE_RATIO_MANUAL_H)

#define JPEG_RESAMPLE_RATIO_MANUAL_V_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_MANUAL_V, val, bJPEG_RESAMPLE_RATIO_MANUAL_V)

#define JPEG_RESAMPLE_RATIO_MANUAL_V_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_MANUAL_V, bJPEG_RESAMPLE_RATIO_MANUAL_V)

#define JPEG_RESAMPLE_RATIO_AUTO_H_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_AUTO_H, val, bJPEG_RESAMPLE_RATIO_AUTO_H)

#define JPEG_RESAMPLE_RATIO_AUTO_H_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_AUTO_H, bJPEG_RESAMPLE_RATIO_AUTO_H)

#define JPEG_RESAMPLE_RATIO_AUTO_V_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_AUTO_V, val, bJPEG_RESAMPLE_RATIO_AUTO_V)

#define JPEG_RESAMPLE_RATIO_AUTO_V_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_RESAMPLE_RATIO_AUTO_V, bJPEG_RESAMPLE_RATIO_AUTO_V)

#define JPEG_FRAME_BUFFER_ENDIAN_SET_BIG(reg, val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_BIG_ENDIAN, val, bJPEG_FRAME_BUFFER_BIG_ENDIAN)

#define JPEG_FRAME_BUFFER_ENDIAN_SET_LITTLE(reg) \
	REG_CLR_BIT(&(reg), bJPEG_FRAME_BUFFER_BIG_ENDIAN)

#define JPEG_FRAME_BUFFER_ENDIAN_GET(reg) \
	REG_GET_BIT(&(reg), bJPEG_FRAME_BUFFER_BIG_ENDIAN)

#define JPEG_BS_BUFFER_ENDIAN_SET_BIG(reg, val) \
	REG_SET_FIELD(&(reg), mJPEG_BS_BUFFER_BIG_ENDIAN, val, bJPEG_BS_BUFFER_BIG_ENDIAN)

#define JPEG_BS_BUFFER_ENDIAN_SET_LITTLE(reg) \
	REG_CLR_BIT(&(reg),bJPEG_BS_BUFFER_BIG_ENDIAN)

#define JPEG_BS_BUFFER_ENDIAN_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_BS_BUFFER_BIG_ENDIAN)

#define JPEG_CPU0_INT_SET_ENABLE(reg) \
	REG_SET_BIT(&(reg), bJPEG_CPU0_INT_ENABLE)

/* begin cpu bit enable/disable    */
#define JPEG_DEC_PIC_FINISH               (0)
#define JPEG_DEC_SLICE_FINISH             (1)
#define JPEG_DEC_SOF_FINISH               (2)
#define JPEG_DEC_FRAME_BUFF_ERROR         (3)
#define JPEG_CLIP_ERROR                   (4)
#define JPEG_BS_BUF_ERROR                 (5)
#define JPEG_DEC_ERROR                    (6)
#define JPEG_LINE_CNT_GATE                (7)
#define JPEG_BS_BUFFER_AMPTY              (8)
#define JPEG_BS_BUFFER_ALMOST_AMPTY       (9)
#define JPEG_CPU_INT_SET_ENABLE(reg, val) \
	REG_SET_BIT(&(reg), val)
#define JPEG_CPU_INT_SET_DISABLE(reg, val) \
	REG_CLR_BIT(&(reg), val)
/* end  */

#define bJPEG_INT_SOF_FINISH_SLICE			(0)
#define mJPEG_INT_SOF_FINISH_SLICE			(0x7 << bJPEG_INT_SOF_FINISH_SLICE)

#define JPEG_CPU_INT_SET_SOF_FINISH_SLICE_ENABLE(reg, val) \
	REG_SET_FIELD(&(reg), mJPEG_INT_SOF_FINISH_SLICE, val, bJPEG_INT_SOF_FINISH_SLICE)

#define bJPEG_INT_BUFFER_EMPTY				(8)
#define mJPEG_INT_BUFFER_EMPTY				(0x3 << bJPEG_INT_BUFFER_EMPTY)

#define JPEG_CPU_INT_SET_BUFFER_EMPTY_ENABLE(reg, val) \
	REG_SET_FIELD(&(reg), mJPEG_INT_BUFFER_EMPTY, val, bJPEG_INT_BUFFER_EMPTY)

#define JPEG_CPU0_INT_SET_DISENABLE(reg) \
	REG_CLR_BIT(&(reg), bJPEG_CPU0_INT_ENABLE)

#define JPEG_CPU0_INT_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_CPU0_INT_ENABLE)

#define JPEG_CPU1_INT_SET_ENABLE(reg) \
	REG_SET_BIT(&(reg), bJPEG_CPU1_INT_ENABLE)

#define JPEG_CPU1_INT_SET_DISENABLE(reg) \
	REG_CLR_BIT(&(reg), bJPEG_CPU1_INT_ENABLE)

#define JPEG_CPU1_INT_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_CPU1_INT_ENABLE)

#define JPEG_CPU2_INT_SET_ENABLE(reg) \
	REG_SET_BIT(&(reg), bJPEG_CPU2_INT_ENABLE)

#define JPEG_CPU2_INT_SET_DISENABLE(reg) \
	REG_CLR_BIT(&(reg), bJPEG_CPU2_INT_ENABLE)

#define JPEG_CPU2_INT_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_CPU2_INT_ENABLE)

#define JPEG_CPU3_INT_SET_ENABLE(reg) \
	REG_SET_BIT(&(reg), bJPEG_CPU3_INT_ENABLE)

#define JPEG_CPU3_INT_SET_DISENABLE(reg) \
	REG_CLR_BIT(&(reg), bJPEG_CPU3_INT_ENABLE)

#define JPEG_CPU3_INT_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_CPU3_INT_ENABLE)


//rBS_BUFFER_START_ADDR
#define bJPEG_BS_BUFFER_START_ADDR      (0)

#define mJPEG_BS_BUFFER_START_ADDR      (0xFFFFFFFF<<bJPEG_BS_BUFFER_START_ADDR)

#define JPEG_BS_BUFFER_START_ADDR_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_BS_BUFFER_START_ADDR, val, bJPEG_BS_BUFFER_START_ADDR)

#define JPEG_BS_BUFFER_START_ADDR_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_BS_BUFFER_START_ADDR, bJPEG_BS_BUFFER_START_ADDR)


//rBS_BUFFER_SIZE
#define bJPEG_BS_BUFFER_SIZE            (0)

#define mJPEG_BS_BUFFER_SIZE            (0xFFFFFFFF<<bJPEG_BS_BUFFER_SIZE)

#define JPEG_BS_BUFFER_SIZE_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_BS_BUFFER_SIZE, val, bJPEG_BS_BUFFER_SIZE)

#define JPEG_BS_BUFFER_SIZE_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_BS_BUFFER_SIZE, bJPEG_BS_BUFFER_SIZE)


//rBS_BUFFER_ALMOST_EMPTY_TH
#define bJPEG_BS_BUFFER_ALMOST_EMPTY_TH (0)

#define mJPEG_BS_BUFFER_ALMOST_EMPTY_TH (0xFFFFFFFF<<bJPEG_BS_BUFFER_ALMOST_EMPTY_TH)

#define JPEG_BS_BUFFER_ALMOST_EMPTY_TH_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_BS_BUFFER_ALMOST_EMPTY_TH, val, bJPEG_BS_BUFFER_ALMOST_EMPTY_TH)

#define JPEG_BS_BUFFER_ALMOST_EMPTY_TH_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_BS_BUFFER_ALMOST_EMPTY_TH, bJPEG_BS_BUFFER_ALMOST_EMPTY_TH)


//rBS_BUFFER_WR_PTR
#define bJPEG_BS_BUFFER_WR_PTR (0)

#define mJPEG_BS_BUFFER_WR_PTR (0xFFFFFFFF<<bJPEG_BS_BUFFER_WR_PTR)

#define JPEG_BS_BUFFER_WR_PTR_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_BS_BUFFER_WR_PTR, val, bJPEG_BS_BUFFER_WR_PTR)

#define JPEG_BS_BUFFER_WR_PTR_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_BS_BUFFER_WR_PTR, bJPEG_BS_BUFFER_WR_PTR)


//rBS_BUFFER_RD_PTR
#define bJPEG_BS_BUFFER_RD_PTR (0)

#define mJPEG_BS_BUFFER_RD_PTR (0xFFFFFFFF<<bJPEG_BS_BUFFER_RD_PTR)

#define JPEG_BS_BUFFER_RD_PTR_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_BS_BUFFER_RD_PTR, val, bJPEG_BS_BUFFER_RD_PTR)

#define JPEG_BS_BUFFER_RD_PTR_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_BS_BUFFER_RD_PTR, bJPEG_BS_BUFFER_RD_PTR)


//rFRAME_BUFFER_Y_BASE_ADDR
#define bJPEG_FRAME_BUFFER_Y_BASE_ADDR  (0)

#define mJPEG_FRAME_BUFFER_Y_BASE_ADDR  (0xFFFFFFFF<<bJPEG_FRAME_BUFFER_Y_BASE_ADDR)

#define JPEG_FRAME_BUFFER_Y_BASE_ADDR_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_Y_BASE_ADDR, val, bJPEG_FRAME_BUFFER_Y_BASE_ADDR)

#define JPEG_FRAME_BUFFER_Y_BASE_ADDR_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_BUFFER_Y_BASE_ADDR, bJPEG_FRAME_BUFFER_Y_BASE_ADDR)


//rFRAME_BUFFER_CB_BASE_ADDR
#define bJPEG_FRAME_BUFFER_CB_BASE_ADDR  (0)

#define mJPEG_FRAME_BUFFER_CB_BASE_ADDR  (0xFFFFFFFF<<bJPEG_FRAME_BUFFER_CB_BASE_ADDR)

#define JPEG_FRAME_BUFFER_CB_BASE_ADDR_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_CB_BASE_ADDR, val, bJPEG_FRAME_BUFFER_CB_BASE_ADDR)

#define JPEG_FRAME_BUFFER_CB_BASE_ADDR_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_BUFFER_CB_BASE_ADDR, bJPEG_FRAME_BUFFER_CB_BASE_ADDR)


//rFRAME_BUFFER_CR_BASE_ADDR
#define bJPEG_FRAME_BUFFER_CR_BASE_ADDR  (0)

#define mJPEG_FRAME_BUFFER_CR_BASE_ADDR  (0xFFFFFFFF<<bJPEG_FRAME_BUFFER_CR_BASE_ADDR)

#define JPEG_FRAME_BUFFER_CR_BASE_ADDR_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_CR_BASE_ADDR, val, bJPEG_FRAME_BUFFER_CR_BASE_ADDR)

#define JPEG_FRAME_BUFFER_CR_BASE_ADDR_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_BUFFER_CR_BASE_ADDR, bJPEG_FRAME_BUFFER_CR_BASE_ADDR)


//rFRAME_BUFFER_STRIDE
#define bJPEG_FRAME_BUFFER_STRIDE  (0)

#define mJPEG_FRAME_BUFFER_STRIDE  (0xFFFF<<bJPEG_FRAME_BUFFER_STRIDE)

#define JPEG_FRAME_BUFFER_STRIDE_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_STRIDE, val, bJPEG_FRAME_BUFFER_STRIDE)

#define JPEG_FRAME_BUFFER_STRIDE_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_BUFFER_STRIDE, bJPEG_FRAME_BUFFER_STRIDE)


//rFRAME_BUFFER_MAX_PIC_SIZE
#define bJPEG_FRAME_BUFFER_MAX_PIC_WIDTH    (0)
#define bJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT   (16)

#define mJPEG_FRAME_BUFFER_MAX_PIC_WIDTH    (0xFFFF<<bJPEG_FRAME_BUFFER_MAX_PIC_WIDTH)
#define mJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT   (0xFFFF<<bJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT)

#define JPEG_FRAME_BUFFER_MAX_PIC_WIDTH_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_MAX_PIC_WIDTH, val, bJPEG_FRAME_BUFFER_MAX_PIC_WIDTH)

#define JPEG_FRAME_BUFFER_MAX_PIC_WIDTH_GET(reg,val) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_BUFFER_MAX_PIC_WIDTH, val, bJPEG_FRAME_BUFFER_MAX_PIC_WIDTH)

#define JPEG_FRAME_BUFFER_MAX_PIC_HEIGHT_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT, val, bJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT)

#define JPEG_FRAME_BUFFER_MAX_PIC_HEIGHT_GET(reg,val) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT, val, bJPEG_FRAME_BUFFER_MAX_PIC_HEIGHT)


//rCLIP_UPPER_LEFT_COORDINATE
#define bJPEG_CLIP_UPPER_LEFT_X     (0)
#define bJPEG_CLIP_UPPER_LEFT_Y     (16)

#define mJPEG_CLIP_UPPER_LEFT_X     (0xFFFF<<bJPEG_CLIP_UPPER_LEFT_X)
#define mJPEG_CLIP_UPPER_LEFT_Y     (0xFFFF<<bJPEG_CLIP_UPPER_LEFT_Y)

#define JPEG_CLIP_UPPER_LEFT_X_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_CLIP_UPPER_LEFT_X, val, bJPEG_CLIP_UPPER_LEFT_X)

#define JPEG_CLIP_UPPER_LEFT_X_GET(reg,val) \
	REG_GET_FIELD(&(reg), mJPEG_CLIP_UPPER_LEFT_X, val, bJPEG_CLIP_UPPER_LEFT_X)

#define JPEG_CLIP_UPPER_LEFT_Y_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_CLIP_UPPER_LEFT_Y, val, bJPEG_CLIP_UPPER_LEFT_Y)

#define JPEG_CLIP_UPPER_LEFT_Y_GET(reg,val) \
	REG_GET_FIELD(&(reg), mJPEG_CLIP_UPPER_LEFT_Y, val, bJPEG_CLIP_UPPER_LEFT_Y)


//rCLIP_LOWER_RIGHT_COORDINATE
#define bJPEG_CLIP_LOWER_RIGHT_X     (0)
#define bJPEG_CLIP_LOWER_RIGHT_Y     (16)

#define mJPEG_CLIP_LOWER_RIGHT_X     (0xFFFF<<bJPEG_CLIP_LOWER_RIGHT_X)
#define mJPEG_CLIP_LOWER_RIGHT_Y     (0xFFFF<<bJPEG_CLIP_LOWER_RIGHT_Y)

#define JPEG_CLIP_LOWER_RIGHT_X_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_CLIP_LOWER_RIGHT_X, val, bJPEG_CLIP_LOWER_RIGHT_X)

#define JPEG_CLIP_LOWER_RIGHT_X_GET(reg,val) \
	REG_GET_FIELD(&(reg), mJPEG_CLIP_LOWER_RIGHT_X, val, bJPEG_CLIP_LOWER_RIGHT_X)

#define JPEG_CLIP_LOWER_RIGHT_Y_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_CLIP_LOWER_RIGHT_Y, val, bJPEG_CLIP_LOWER_RIGHT_Y)

#define JPEG_CLIP_LOWER_RIGHT_Y_GET(reg,val) \
	REG_GET_FIELD(&(reg), mJPEG_CLIP_LOWER_RIGHT_Y, val, bJPEG_CLIP_LOWER_RIGHT_Y)

//rDECODE_DEAD_OVER_TIME_GATE
#define bJPEG_DECODE_DEAD_OVER_TIME_GATE  (0)

#define mJPEG_DECODE_DEAD_OVER_TIME_GATE  (0xFFFFFFFF<<bJPEG_DECODE_DEAD_OVER_TIME_GATE)

#define JPEG_DECODE_DEAD_OVER_TIME_GATE_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_DECODE_DEAD_OVER_TIME_GATE, val, bJPEG_DECODE_DEAD_OVER_TIME_GATE)

#define JPEG_DECODE_DEAD_OVER_TIME_GATE_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_DECODE_DEAD_OVER_TIME_GATE, bJPEG_DECODE_DEAD_OVER_TIME_GATE)

//rPIC_INFO
#define bJPEG_FRAME_FORMAT              (0)
#define bJPEG_PROFILE                   (3)
#define bJPEG_SLICE_IDCT_INFO           (4)
#define bJPEG_SLICE_SS                  (8)
#define bJPEG_SLICE_SE                  (16)

#define mJPEG_FRAME_FORMAT              (0x7<<bJPEG_FRAME_FORMAT)
#define mJPEG_PROFILE                   (0x1<<bJPEG_PROFILE)
#define mJPEG_SLICE_IDCT_INFO           (0x3<<bJPEG_SLICE_IDCT_INFO)
#define mJPEG_SLICE_SS                  (0x3F<<bJPEG_SLICE_SS)
#define mJPEG_SLICE_SE                  (0x3F<<bJPEG_SLICE_SE)

#define JPEG_PIC_FRAME_FORMAT_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_FRAME_FORMAT, bJPEG_FRAME_FORMAT)

#define JPEG_PIC_PROFILE_GET(reg) \
	REG_GET_BIT(&(reg), bJPEG_PROFILE)

#define JPEG_PIC_SLICE_IDCT_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_SLICE_IDCT_INFO, bJPEG_SLICE_IDCT_INFO)

#define JPEG_PIC_SLICE_SS_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_SLICE_SS, bJPEG_SLICE_SS)

#define JPEG_PIC_SLICE_SE_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_SLICE_SE, bJPEG_SLICE_SE)


//rPIC_ORIGION_SIZE_INFO
#define bJPEG_ORIGION_WIDTH             (0)
#define bJPEG_ORIGION_HEIGHT            (16)

#define mJPEG_ORIGION_WIDTH             (0xFFFF<<bJPEG_ORIGION_WIDTH)
#define mJPEG_ORIGION_HEIGHT            (0xFFFF<<bJPEG_ORIGION_HEIGHT)

#define JPEG_ORIGION_WIDTH_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_ORIGION_WIDTH, bJPEG_ORIGION_WIDTH)

#define JPEG_ORIGION_HEIGHT_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_ORIGION_HEIGHT, bJPEG_ORIGION_HEIGHT)


//rPIC_WRITE_BACK_SIZE_INFO
#define bJPEG_WRITE_BACK_WIDTH             (0)
#define bJPEG_WRITE_BACK_HEIGHT            (16)

#define mJPEG_WRITE_BACK_WIDTH             (0xFFFF<<bJPEG_WRITE_BACK_WIDTH)
#define mJPEG_WRITE_BACK_HEIGHT            (0xFFFF<<bJPEG_WRITE_BACK_HEIGHT)

#define JPEG_WRITE_BACK_WIDTH_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_WRITE_BACK_WIDTH, bJPEG_WRITE_BACK_WIDTH)

#define JPEG_WRITE_BACK_HEIGHT_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_WRITE_BACK_HEIGHT, bJPEG_WRITE_BACK_HEIGHT)


//rPIC_DISPLAY_SIZE_INFO
#define bJPEG_DISPLAY_WIDTH             (0)
#define bJPEG_DISPLAY_HEIGHT            (16)

#define mJPEG_DISPLAY_WIDTH             (0xFFFF<<bJPEG_DISPLAY_WIDTH)
#define mJPEG_DISPLAY_HEIGHT            (0xFFFF<<bJPEG_DISPLAY_HEIGHT)

#define JPEG_DISPLAY_WIDTH_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_DISPLAY_WIDTH, bJPEG_DISPLAY_WIDTH)

#define JPEG_DISPLAY_HEIGHT_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_DISPLAY_HEIGHT, bJPEG_DISPLAY_HEIGHT)


//rPIC_DISPLAY_COORDINATE_INFO
#define bJPEG_DISPLAY_COORDINATE_X      (0)
#define bJPEG_DISPLAY_COORDINATE_Y      (16)

#define mJPEG_DISPLAY_COORDINATE_X      (0xFFFF<<bJPEG_DISPLAY_COORDINATE_X)
#define mJPEG_DISPLAY_COORDINATE_Y      (0xFFFF<<bJPEG_DISPLAY_COORDINATE_Y)

#define JPEG_DISPLAY_COORDINATE_X_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_DISPLAY_COORDINATE_X, bJPEG_DISPLAY_COORDINATE_X)

#define JPEG_DISPLAY_COORDINATE_Y_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_DISPLAY_COORDINATE_Y, bJPEG_DISPLAY_COORDINATE_Y)


//rINT_STATUS
#define bJPEG_INT_STATUS                    (0)
#define bJPEG_DECODE_PIC_FINISH             (0)
#define bJPEG_DECODE_SLICE_FINISH           (1)
#define bJPEG_DECODE_SOF_FINISH             (2)
#define bJPEG_FRAME_BUFFER_ERROR            (3)
#define bJPEG_CLIP_ERROR                  	(4)
#define bJPEG_BS_BUF_ERROR                  (5)
#define bJPEG_DECODE_ERROR					(6)
#define bJPEG_LINE_CNT					    (7)
#define bJPEG_BS_BUFFER_ALMOST_AMPTY        (8)
#define bJPEG_BUFFER_AMPTY                  (9)

#define mJPEG_DECODE_PIC_FINISH             (0x1<<bJPEG_DECODE_PIC_FINISH)
#define mJPEG_DECODE_SLICE_FINISH           (0x1<<bJPEG_DECODE_SLICE_FINISH)
#define mJPEG_DECODE_SOF_FINISH             (0x1<<bJPEG_DECODE_SOF_FINISH)
#define mJPEG_FRAME_BUFFER_ERROR            (0x1<<bJPEG_FRAME_BUFFER_ERROR)
#define mJPEG_CLIP_ERROR					(0x1<<bJPEG_CLIP_ERROR)
#define mJPEG_BS_BUF_ERROR                  (0x1<<bJPEG_BS_BUF_ERROR)
#define mJPEG_DECODE_ERROR                  (0x1<<bJPEG_DECODE_ERROR)
#define mJPEG_LINE_CNT						(0x1<<bJPEG_LINE_CNT)
#define mJPEG_BS_BUFFER_ALMOST_AMPTY        (0x1<<bJPEG_BS_BUFFER_ALMOST_AMPTY)
#define mJPEG_BUFFER_AMPTY                  (0x1<<bJPEG_BUFFER_AMPTY)
#define mJPEG_INT_STATUS                    (0xFFFFFFFF<<bJPEG_INT_STATUS)

#define JPEGDEC_REG_SET_BIT(reg,bit) (*reg = (1 << bit))

#define JPEG_INT_STATUS_SET(reg,val) \
	REG_SET_FIELD(&(reg), mJPEG_INT_STATUS, val, bJPEG_INT_STATUS)

#define JPEG_INT_STATUS_GET(reg) \
	REG_GET_FIELD(&(reg), mJPEG_INT_STATUS, bJPEG_INT_STATUS)

#define JPEG_INT_STATUS_DECODE_PIC_FINISH_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_DECODE_PIC_FINISH)

#define JPEG_INT_STATUS_DECODE_PIC_FINISH_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_DECODE_PIC_FINISH)

#define JPEG_INT_STATUS_DECODE_PIC_FINISH_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODE_PIC_FINISH)

#define JPEG_INT_STATUS_DECODE_SLICE_FINISH_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_DECODE_SLICE_FINISH)

#define JPEG_INT_STATUS_DECODE_SLICE_FINISH_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_DECODE_SLICE_FINISH)

#define JPEG_INT_STATUS_DECODE_SLICE_FINISH_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODE_SLICE_FINISH)

#define JPEG_INT_STATUS_DECODE_SOF_FINISH_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_DECODE_SOF_FINISH)

#define JPEG_INT_STATUS_DECODE_SOF_FINISH_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_DECODE_SOF_FINISH)

#define JPEG_INT_STATUS_DECODE_SOF_FINISH_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODE_SOF_FINISH)

#define JPEG_INT_STATUS_FRAME_BUFFER_ERROR_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_FRAME_BUFFER_ERROR)

#define JPEG_INT_STATUS_FRAME_BUFFER_ERROR_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_FRAME_BUFFER_ERROR)

#define JPEG_INT_STATUS_FRAME_BUFFER_ERROR_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_FRAME_BUFFER_ERROR)

#define JPEG_INT_CLIP_ERROR_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_CLIP_ERROR)

#define JPEG_INT_CLIP_ERROR_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_CLIP_ERROR)

#define JPEG_INT_CLIP_ERROR_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_CLIP_ERROR)

#define JPEG_INT_STATUS_BS_BUF_ERROR_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_BS_BUF_ERROR)

#define JPEG_INT_STATUS_BS_BUF_ERROR_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_BS_BUF_ERROR)

#define JPEG_INT_STATUS_BS_BUF_ERROR_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_BS_BUF_ERROR)

#define JPEG_INT_STATUS_DECODE_ERROR_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_DECODE_ERROR)

#define JPEG_INT_STATUS_DECODE_ERROR_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_DECODE_ERROR)

#define JPEG_INT_STATUS_DECODE_ERROR_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_DECODE_ERROR)

#define JPEG_INT_STATUS_LINE_CNT_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_LINE_CNT)

#define JPEG_INT_STATUS_LINE_CNT_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_LINE_CNT)

#define JPEG_INT_STATUS_LINE_CNT_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_LINE_CNT)

#define JPEG_INT_STATUS_BS_BUFFER_ALMOST_AMPTY_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_BS_BUFFER_ALMOST_AMPTY)

#define JPEG_INT_STATUS_BS_BUFFER_ALMOST_AMPTY_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_BS_BUFFER_ALMOST_AMPTY)

#define JPEG_INT_STATUS_BS_BUFFER_ALMOST_AMPTY_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_BS_BUFFER_ALMOST_AMPTY)

#define JPEG_INT_STATUS_BS_BUFFER_AMPTY_SET(reg) \
	REG_CLR_BIT(&(reg),bJPEG_BUFFER_AMPTY)

#define JPEG_INT_STATUS_BS_BUFFER_AMPTY_CLR(reg) \
	JPEGDEC_REG_SET_BIT(&(reg),bJPEG_BUFFER_AMPTY)

#define JPEG_INT_STATUS_BS_BUFFER_AMPTY_GET(reg) \
	REG_GET_BIT(&(reg),bJPEG_BUFFER_AMPTY)


#endif

