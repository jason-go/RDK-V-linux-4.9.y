//------------------------------------------------------------------------------
// File: RegDefine.h
//
// Copyright (c) 2006, Chips & Media.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef REGDEFINE_H_INCLUDED
#define REGDEFINE_H_INCLUDED

#define JPEG_DOWNSAMPLE_WORKAROUND
//------------------------------------------------------------------------------
// REGISTER BASE
//------------------------------------------------------------------------------
extern int 	BIT_BASE 	;
extern int 	GDMA_BASE	;
extern int 	MBC_BASE	;
extern int 	NC_Base_Addr;

//------------------------------------------------------------------------------
// HARDWARE REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_RUN                 (BIT_BASE + 0x000)
#define BIT_CODE_DOWN                (BIT_BASE + 0x004)
#define BIT_INT_REQ                  (BIT_BASE + 0x008)
#define BIT_INT_CLEAR                (BIT_BASE + 0x00C)
#define BIT_CODE_RESET               (BIT_BASE + 0x014)
#define BIT_CUR_PC                   (BIT_BASE + 0x018)
#define	BIT_CYCLE_INT_CNT			 (BIT_BASE + 0x028)
#define BIT_CODE_VERSION             (BIT_BASE + 0x050)
#define BIT_NDS_BUF_SIZE             (BIT_BASE + 0x058)
#define BIT_NDS_HEVC_EN_SIRIUS       (BIT_BASE + 0x05C)
#define BIT_CODE_PARAM0              (BIT_BASE + 0x070)
#define BIT_CODE_PARAM1              (BIT_BASE + 0x074)
#define BIT_CODE_PARAM2              (BIT_BASE + 0x078)
#define BIT_CODE_PARAM3              (BIT_BASE + 0x07C)
#define BIT_CODE_PARAM4              (BIT_BASE + 0x080)
#define BIT_CODE_PARAM5              (BIT_BASE + 0x084)
#define BIT_CODE_PARAM6              (BIT_BASE + 0x088)
#define BIT_CODE_PARAM7              (BIT_BASE + 0x08C)
#define BIT_CODE_PARAM8              (BIT_BASE + 0x090)

//------------------------------------------------------------------------------
// GLOBAL REGISTER
//------------------------------------------------------------------------------
#define BIT_CODE_BUF_ADDR            (BIT_BASE + 0x100)
#define BIT_WORK_BUF_ADDR            (BIT_BASE + 0x104)
#define BIT_PARA_BUF_ADDR            (BIT_BASE + 0x108)
#define BIT_BIT_STREAM_CTRL          (BIT_BASE + 0x10C)
#define BIT_FRAME_MEM_CTRL           (BIT_BASE + 0x110)
#define CMD_DEC_DISPLAY_REORDER      (BIT_BASE + 0x114)
#define	BIT_BIT_STREAM_PARAM         (BIT_BASE + 0x114)

#define BIT_RD_PTR_0                 (BIT_BASE + 0x120)
#define BIT_WR_PTR_0                 (BIT_BASE + 0x124)
#define BIT_RD_PTR_1                 (BIT_BASE + 0x128)
#define BIT_WR_PTR_1                 (BIT_BASE + 0x12C)
#define BIT_RD_PTR_2                 (BIT_BASE + 0x130)
#define BIT_WR_PTR_2                 (BIT_BASE + 0x134)
#define BIT_RD_PTR_3                 (BIT_BASE + 0x138)
#define BIT_WR_PTR_3                 (BIT_BASE + 0x13C)
#define BIT_AXI_SRAM_USE             (BIT_BASE + 0x140)

#define BIT_EXACT_RD_PTR			 (BIT_BASE + 0x144)
#define BIT_EXACT_RD_PTR_PREV		 (BIT_BASE + 0x148)

#define HOST_BB_POS_UPDATE
#define	BIT_FRM_DIS_FLG_0            (BIT_BASE + 0x150)
#define	BIT_FRM_DIS_FLG_1            (BIT_BASE + 0x154)
#define	BIT_FRM_DIS_FLG_2            (BIT_BASE + 0x158)
#define	BIT_FRM_DIS_FLG_3            (BIT_BASE + 0x15C)

#define BIT_BUSY_FLAG                (BIT_BASE + 0x160)
#define BIT_RUN_COMMAND              (BIT_BASE + 0x164)
#define BIT_RUN_INDEX                (BIT_BASE + 0x168)
#define BIT_RUN_COD_STD              (BIT_BASE + 0x16C)
#define BIT_INT_ENABLE               (BIT_BASE + 0x170)
#define BIT_INT_REASON               (BIT_BASE + 0x174)
#define BIT_RUN_AUX_STD              (BIT_BASE + 0x178)

#define BIT_CMD_0                    (BIT_BASE + 0x1E0)
#define BIT_CMD_1                    (BIT_BASE + 0x1E4)

#define BIT_MSG_0                   (BIT_BASE + 0x1F4)
#define BIT_MSG_1                   (BIT_BASE + 0x1F8)
#define BIT_MSG_2                   (BIT_BASE + 0x1FC)

#define MBC_BUSY                	(MBC_BASE + 0x044)
#define MBC_TASK_STATUS             (MBC_BASE + 0x060)

//------------------------------------------------------------------------------
// [DEC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_SEQ_BB_START         (BIT_BASE + 0x180)
#define CMD_DEC_SEQ_BB_SIZE          (BIT_BASE + 0x184)
#define CMD_DEC_SEQ_OPTION           (BIT_BASE + 0x188)
#define CMD_DEC_SEQ_SRC_SIZE        (BIT_BASE + 0x18C)
#define CMD_DEC_SEQ_SAM_XY		    (BIT_BASE + 0x190)
#define CMD_DEC_SEQ_JPG_THUMB_EN    (BIT_BASE + 0x194)
#define CMD_DEC_SEQ_MP4_ASP_CLASS   (BIT_BASE + 0x194)
#define CMD_DEC_SEQ_X264_MV_EN		(BIT_BASE + 0x194)
#define	CMD_DEC_SEQ_CLIP_MODE		(BIT_BASE + 0x198)
#define	CMD_DEC_SEQ_CLIP_FROM		(BIT_BASE + 0x19C)
#define	CMD_DEC_SEQ_CLIP_TO			(BIT_BASE + 0x1A0)
#define	CMD_DEC_SEQ_CLIP_CNT		(BIT_BASE + 0x1A4)

// For MPEG2 and JPEG only
#define	CMD_DEC_SEQ_USER_DATA_BASE_ADDR		(BIT_BASE + 0x1A8)
#define	CMD_DEC_SEQ_USER_DATA_BUF_SIZE		(BIT_BASE + 0x1AC)

#define CMD_DEC_SEQ_INIT_ESCAPE		(BIT_BASE + 0x114)

#define RET_DEC_SEQ_ASPECT			(BIT_BASE + 0x1B0)
#define RET_DEC_SEQ_BIT_RATE		(BIT_BASE + 0x1B4)
#define RET_DEC_SEQ_SUCCESS         (BIT_BASE + 0x1C0)
#define RET_DEC_SEQ_SRC_SIZE        (BIT_BASE + 0x1C4)
//#define RET_DEC_SEQ_SRC_F_RATE      (BIT_BASE + 0x1C8)
#define RET_DEC_SEQ_FRAME_NEED      (BIT_BASE + 0x1CC)
#define RET_DEC_SEQ_FRAME_DELAY     (BIT_BASE + 0x1D0)
#define RET_DEC_SEQ_INFO            (BIT_BASE + 0x1D4)
#define RET_DEC_SEQ_CROP_LEFT_RIGHT (BIT_BASE + 0x1D8)
#define RET_DEC_SEQ_CROP_TOP_BOTTOM (BIT_BASE + 0x1DC)
#define RET_DEC_SEQ_HEADER_REPORT   (BIT_BASE + 0x1E4)
#define	RET_DEC_SEQ_FRAME_FORMAT	(BIT_BASE + 0x1E4)
#define RET_DEC_SEQ_JPG_THUMB_IND	(BIT_BASE + 0x1E8)
#define RET_DEC_SEQ_FRATE_NR		(BIT_BASE + 0x1EC)
#define RET_DEC_SEQ_FRATE_DR		(BIT_BASE + 0x1F0)
#define	RET_DEC_SEQ_SIZE_AIGN		(BIT_BASE + 0x130)//used the not used register addr from BIT_RD_PTR_2

//------------------------------------------------------------------------------
// [DEC PIC RUN] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_PIC_ROT_MODE			(BIT_BASE + 0x180)
#define CMD_DEC_PIC_ROT_INDEX			(BIT_BASE + 0x184)
#define CMD_DEC_PIC_ROT_STRIDE			(BIT_BASE + 0x190)
#define CMD_DEC_PIC_OPTION				(BIT_BASE + 0x194)
#define	CMD_DEC_PIC_SKIP_NUM			(BIT_BASE + 0x198)
#define CMD_DEC_PIC_PARA_BASE_ADDR		(BIT_BASE + 0x1A4)
#define CMD_DEC_PIC_USER_DATA_BASE_ADDR	(BIT_BASE + 0x1A8)
#define CMD_DEC_PIC_USER_DATA_BUF_SIZE	(BIT_BASE + 0x1AC)
#define CMD_DEC_PIC_FILT_PARA			(BIT_BASE + 0x1B4)

#define RET_DEC_PIC_POC					(BIT_BASE + 0x1B8)
#define RET_DEC_PIC_SIZE				(BIT_BASE + 0x1BC)
#define RET_DEC_PIC_FRAME_NUM			(BIT_BASE + 0x1C0)
#define RET_DEC_PIC_FRAME_IDX			(BIT_BASE + 0x1C4)
#define RET_DEC_PIC_ERR_MB				(BIT_BASE + 0x1C8)
#define RET_DEC_PIC_TYPE				(BIT_BASE + 0x1CC)
#define RET_DEC_PIC_POST				(BIT_BASE + 0x1D0)
#define RET_DEC_PIC_INFO				(BIT_BASE + 0x1D4)
#define RET_DEC_PIC_SUCCESS				(BIT_BASE + 0x1D8)
#define RET_DEC_PIC_CUR_IDX				(BIT_BASE + 0x1DC)
#define	RET_DEC_PIC_CROP_LEFT_RIGHT		(BIT_BASE + 0x1E0)
#define RET_DEC_PIC_CROP_TOP_BOTTOM		(BIT_BASE + 0x1E4)
#define	RET_DEC_PIC_ASPECT				(BIT_BASE + 0x1E8)
#define	RET_DEC_PIC_FRATE_NR			(BIT_BASE + 0x1EC)
#define	RET_DEC_PIC_FRATE_DR			(BIT_BASE + 0x1F0)

//------------------------------------------------------------------------------
// [SET FRAME BUF] COMMAND
//------------------------------------------------------------------------------
#define CMD_SET_FRAME_BUF_NUM			(BIT_BASE + 0x180)
#define CMD_SET_FRAME_BUF_STRIDE		(BIT_BASE + 0x184)
#define CMD_SET_FRAME_AXI_BIT_ADDR		(BIT_BASE + 0x190)
#define CMD_SET_FRAME_AXI_IPACDC_ADDR	(BIT_BASE + 0x194)
#define CMD_SET_FRAME_AXI_DBKY_ADDR		(BIT_BASE + 0x198)
#define CMD_SET_FRAME_AXI_DBKC_ADDR		(BIT_BASE + 0x19C)
#define CMD_SET_FRAME_AXI_OVL_ADDR		(BIT_BASE + 0x1A0)
#define CMD_SET_FRAME_AXI_BTP_ADDR		(BIT_BASE + 0x1A4)
#ifdef RPR_SUPPORT
#define CMD_SET_FRAME_MAX_DEC_SIZE		(BIT_BASE + 0x1B0)
#endif // RPR_SUPPORT

//------------------------------------------------------------------------------
// [DEC SEQ INIT] COMMAND
//------------------------------------------------------------------------------
#define CMD_BITBUF_FLUSH_BB_START			(BIT_BASE + 0x180)

//------------------------------------------------------------------------------
// [DEC_PARA_SET] COMMAND
//------------------------------------------------------------------------------
#define CMD_DEC_PARA_SET_TYPE        (BIT_BASE + 0x180)
#define CMD_DEC_PARA_SET_SIZE        (BIT_BASE + 0x184)


//------------------------------------------------------------------------------
// [SET PIC INFO] COMMAND
//------------------------------------------------------------------------------
#define GDI_PRI_RD_PRIO_L   		(GDMA_BASE + 0x000)
#define GDI_PRI_RD_PRIO_H   		(GDMA_BASE + 0x004)
#define GDI_PRI_WR_PRIO_L   		(GDMA_BASE + 0x008)
#define GDI_PRI_WR_PRIO_H   		(GDMA_BASE + 0x00c)
#define GDI_PRI_RD_LOCK_CNT 		(GDMA_BASE + 0x010)
#define GDI_PRI_WR_LOCK_CNT 		(GDMA_BASE + 0x014)
#define GDI_SEC_RD_PRIO_L   		(GDMA_BASE + 0x018)
#define GDI_SEC_RD_PRIO_H   		(GDMA_BASE + 0x01c)
#define GDI_SEC_WR_PRIO_L   		(GDMA_BASE + 0x020)
#define GDI_SEC_WR_PRIO_H   		(GDMA_BASE + 0x024)
#define GDI_SEC_RD_LOCK_CNT 		(GDMA_BASE + 0x028)
#define GDI_SEC_WR_LOCK_CNT 		(GDMA_BASE + 0x02c)
#define GDI_SEC_CLIENT_EN   		(GDMA_BASE + 0x030)

#define GDI_CONTROL					(GDMA_BASE + 0x034)
#define GDI_STATUS					(GDMA_BASE + 0x080)
#define	GDI_ADR_DEBUG_0				(GDMA_BASE + 0x084)
#define	GDI_ADR_DEBUG_1				(GDMA_BASE + 0x088)
#define	GDI_ADR_DEBUG_2				(GDMA_BASE + 0x08c)
#define	GDI_ADR_DEBUG_3				(GDMA_BASE + 0x090)
#define GDI_INFO_CONTROL			(GDMA_BASE + 0x400)
#define GDI_INFO_PIC_SIZE			(GDMA_BASE + 0x404)
#define GDI_INFO_BASE_Y				(GDMA_BASE + 0x408)
#define GDI_INFO_BASE_CB			(GDMA_BASE + 0x40C)
#define GDI_INFO_BASE_CR			(GDMA_BASE + 0x410)


#define BIT_GDI_PROTECT				(GDMA_BASE + 0x05C)
#define BIT_GDI_START_ADDR0         (GDMA_BASE + 0x060)
#define BIT_GDI_END_ADDR0           (GDMA_BASE + 0x064)
#define BIT_GDI_START_ADDR1         (GDMA_BASE + 0x068)
#define BIT_GDI_END_ADDR1           (GDMA_BASE + 0x06C)
#define BIT_GDI_START_ADDR2         (GDMA_BASE + 0x070)
#define BIT_GDI_END_ADDR2           (GDMA_BASE + 0x074)
#define BIT_GDI_START_ADDR3         (GDMA_BASE + 0x078)
#define BIT_GDI_END_ADDR3           (GDMA_BASE + 0x07C)

#define BIT_GDI_CLOSE				(GDMA_BASE + 0x0F0)
#define BIT_GDI_EMPTY				(GDMA_BASE + 0x0F4)


//------------------------------------------------------------------------------
// [FIRMWARE VERSION] COMMAND
// [32:16] project number =>
// [16:0]  version => xxxx.xxxx.xxxxxxxx
//------------------------------------------------------------------------------
#define RET_VER_NUM			(BIT_BASE + 0x1c0)

//------------------------------------------------------------------------------
// MIXER REGISTER ADDRESS
//------------------------------------------------------------------------------
#define MIX_INT                 (MIX_BASE + 0x044)

#define MIX_STRIDE_Y		    (MIX_BASE + 0x144)
#define MIX_STRIDE_CB	    	(MIX_BASE + 0x148)
#define MIX_STRIDE_CR   		(MIX_BASE + 0x14c)

#define MIX_ADDR_Y              (MIX_BASE + 0x138)
#define MIX_ADDR_CB             (MIX_BASE + 0x13C)
#define MIX_ADDR_CR             (MIX_BASE + 0x140)

#define MIX_RUN                 (MIX_BASE + 0x120)

#define DISP_TOTAL_SAMPLE       (DISP_MIX + 0x00C)
#define DISP_ACTIVE_SAMPLE      (DISP_MIX + 0x010)
#define DISP_HSYNC_START_END    (DISP_MIX + 0x014)
#define DISP_VSYNC_TOP_START    (DISP_MIX + 0x018)
#define DISP_VSYNC_TOP_END      (DISP_MIX + 0x01C)
#define DISP_VSYNC_BOT_START    (DISP_MIX + 0x020)
#define DISP_VSYNC_BOT_END      (DISP_MIX + 0x024)
#define DISP_ACTIVE_REGION_TOP  (DISP_MIX + 0x02C)
#define DISP_ACTIVE_REGION_BOT  (DISP_MIX + 0x030)


//------------------------------------------------------------------------------
// MIXER REGISTER ADDRESS
//------------------------------------------------------------------------------
#define MIX_MIX_INTRPT			(MIX_BASE + 0x0000)
#define MIX_SYNC_STATE			(MIX_BASE + 0x0004)
#define MIX_SYNC_CTRL			(MIX_BASE + 0x0008)
#define MIX_TOTAL_SAMPLE		(MIX_BASE + 0x000c)
#define MIX_ACTIVE_SAMPLE		(MIX_BASE + 0x0010)
#define MIX_HSYNC_START_END	    (MIX_BASE + 0x0014)
#define MIX_VSYNC_TOP_START	    (MIX_BASE + 0x0018)
#define MIX_VSYNC_TOP_END		(MIX_BASE + 0x001c)
#define MIX_VSYNC_BOT_START	    (MIX_BASE + 0x0020)
#define MIX_VSYNC_BOT_END		(MIX_BASE + 0x0024)
#define MIX_ACT_REGION_SAMPLE	(MIX_BASE + 0x0028)
#define MIX_ACT_REGION_TOP		(MIX_BASE + 0x002c)
#define MIX_ACT_REGION_BOT		(MIX_BASE + 0x0030)
#define MIX_TOP_START			(MIX_BASE + 0x0034)
#define MIX_BOT_START			(MIX_BASE + 0x0038)
#define MIX_LINE_INC			(MIX_BASE + 0x003c)
#define MIX_LATCH_PARAM_CTRL	(MIX_BASE + 0x0040)
#define MIX_INTERRUPT			(MIX_BASE + 0x0044)

#define MIX_LAYER_CTRL			(MIX_BASE + 0x0100)
#define MIX_LAYER_ORDER		    (MIX_BASE + 0x0104)
#define MIX_BIG_ENDIAN			(MIX_BASE + 0x0108)
#define MIX_L0_BG_COLOR		    (MIX_BASE + 0x0110)
#define MIX_L1_CTRL			    (MIX_BASE + 0x0120)
#define MIX_L1_LSIZE			(MIX_BASE + 0x0124)
#define MIX_L1_SSIZE			(MIX_BASE + 0x0128)
#define MIX_L1_LPOS			    (MIX_BASE + 0x012c)
#define MIX_L1_SPOS			    (MIX_BASE + 0x0130)
#define MIX_L1_BG_COLOR		    (MIX_BASE + 0x0134)
#define MIX_L1_Y_SADDR			(MIX_BASE + 0x0138)
#define MIX_L1_CB_SADDR		    (MIX_BASE + 0x013c)
#define MIX_L1_CR_SADDR		    (MIX_BASE + 0x0140)
#define MIX_L1_Y_STRIDE		    (MIX_BASE + 0x0144)
#define MIX_L1_CB_STRIDE		(MIX_BASE + 0x0148)
#define MIX_L1_CR_STRIDE		(MIX_BASE + 0x014c)

#endif // REGDEFINE_H_INCLUDED
