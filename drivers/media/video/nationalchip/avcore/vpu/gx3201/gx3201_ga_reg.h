#ifndef __GX3201_GA_REG_H__
#define __GX3201_GA_REG_H__

#include "gxav_bitops.h"

#define GA_MPW  (0)
#define GA_NRE  (1)
#define GA_MOD_VERSION GA_NRE

typedef struct gx3201_ga_reg {
	unsigned int    rGA_EDITION;
	unsigned int    rCMD_BUFFER_ADDR;
	unsigned int    rCMD_BUFFER_LENGTH;
	unsigned int    rCMD_READ_ADDR;
	unsigned int    rGA_ENABLE;
	unsigned int    rCMD_ENDIANNESS;
	unsigned int    rCMD_GAIN;
	unsigned int    rGA_INTERRUPT;
	unsigned int    rGA_SCALER_HOLD_DATA;
	unsigned int    rGA_ZOOM_IN_COEFFICIENT;
	unsigned int    rGA_ZOOM_OUT_COEFFICIENT;
	unsigned int    rCMD_CHECK;
	unsigned int    rDITHER_DISABLE;
}Gx3201GaReg;

#define bGA_EN              (0)
#define mGA_EN              (0x1<<bGA_EN)

#define bGA_HOLD_DATA_EN    (0)
#define mGA_HOLD_DATA_EN    (0x1<<bGA_HOLD_DATA_EN)

#define bGA_INTERRUPT_EN    (0)
#define mGA_INTERRUPT_EN    (0x1<<bGA_INTERRUPT_EN)
#define bGA_INTERRUPT_TYPE	(8)
#define mGA_INTERRUPT_TYPE	(0x1<<bGA_INTERRUPT_TYPE)

#define bGA_CMD_GAIN_SIGNAL (16)
#define mGA_CMD_GAIN_SIGNAL (0x1<<bGA_CMD_GAIN)

#define bGA_INTERRUPT_CLR   (1)
#define mGA_INTERRUPT_CLR   (0x1<<bGA_INTERRUPT_CLR)

#define GA_CMD_NUM_MASK     (0xFFFF)
#define GA_CMD_NUM_OFFSET   (0)

#define GA_ENABLE(reg)\
    REG_SET_BIT(&(reg),bGA_EN)
#define GA_DISABLE(reg)\
    REG_CLR_BIT(&(reg),bGA_EN)

#define GA_SET_HOLD_DATA_EN(reg)\
    REG_SET_BIT(&(reg),bGA_HOLD_DATA_EN)

#define GA_SET_DITHER_DISABLE(reg,val)\
    REG_SET_VAL(&(reg), val)

/* interrupt */
#define GA_SET_INTERRUPT_ENABLE(reg)\
    REG_SET_BIT(&(reg), bGA_INTERRUPT_EN)
#define GA_SET_INTERRUPT_TYPE(reg)\
    REG_SET_BIT(&(reg), bGA_INTERRUPT_TYPE)/*mult cmds and ond interrupt*/
#define GA_SET_INTERRUPT_DISABLE(reg)\
    REG_CLR_BIT(&(reg), bGA_INTERRUPT_EN)
#define GA_SET_INTERRUPT_CLR(reg)\
    REG_CLR_BIT(&(reg), bGA_INTERRUPT_CLR)

#define GA_CLEAN_INTERRUPT(reg)\
	do {\
		REG_CLR_BIT(&(reg), bGA_INTERRUPT_CLR);\
		REG_SET_BIT(&(reg), bGA_INTERRUPT_CLR);\
	}while(0)

#define GA_SET_CMD_BUF_ADDR(reg, val)\
    REG_SET_VAL(&(reg), val)

#define GA_SET_CMD_BUF_LENGTH(reg, val)\
    REG_SET_VAL(&(reg), (val))

#define GA_SET_CMD_ENDIAN(reg, val)\
    REG_SET_VAL(&(reg), val)

#define GA_SET_ZOOM_IN_COEFFICIENT(reg, val)\
	REG_SET_VAL(&(reg), val)
#define GA_SET_ZOOM_OUT_COEFFICIENT(reg, val)\
	REG_SET_VAL(&(reg), val)
#define GA_SET_CMD_NUM(reg, val)\
    REG_SET_FIELD(&(reg), GA_CMD_NUM_MASK, val, GA_CMD_NUM_OFFSET)

#define GA_CMD_GAIN_SET(reg)\
    REG_SET_BIT(&(reg), bGA_CMD_GAIN_SIGNAL)

#define GA_CMD_GAIN_CLEAR(reg)\
    REG_CLR_BIT(&(reg), bGA_CMD_GAIN_SIGNAL)

#define GA_SET_CMD_LITTLE_ENDIAN(reg)\
        GX_SET_VAL(&(reg),(0x977053))

#define GA_SEND_CMD_NUM_SIGNAL(reg) \
	do { \
		GA_CMD_GAIN_CLEAR(reg);         \
		GA_CMD_GAIN_SET(reg);           \
	}while(0)

#define GA_HOT_RESET()\
do {\
	extern volatile struct config_regs *gx3201_config_reg;\
	REG_SET_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset), 5);\
	gx_mdelay(1);\
	REG_CLR_BIT(&(gx3201_config_reg->cfg_mpeg_hot_reset), 5);\
}while(0)

#define SUBMIT_JOB_TO_GA(reg, job)\
	do {\
		/*GA_HOT_RESET();*/\
		GA_DISABLE              ( reg->rGA_ENABLE);\
		GA_SET_INTERRUPT_TYPE	( reg->rGA_INTERRUPT);\
		GA_SET_INTERRUPT_ENABLE ( reg->rGA_INTERRUPT);\
		GA_SET_CMD_BUF_ADDR     ( reg->rCMD_BUFFER_ADDR, gx_virt_to_phys((unsigned int)job->cmd_start_addr));\
		GA_SET_CMD_BUF_LENGTH   ( reg->rCMD_BUFFER_LENGTH, job->cur_cmd_num*GA_CMD_SIZE);\
		GA_SET_CMD_ENDIAN       ( reg->rCMD_ENDIANNESS, 0x977053&0xffffff);\
		GA_SET_ZOOM_IN_COEFFICIENT(reg->rGA_ZOOM_IN_COEFFICIENT, 0x00ff0100);\
		GA_SET_ZOOM_OUT_COEFFICIENT(reg->rGA_ZOOM_OUT_COEFFICIENT, 0x00ff0100);\
		GA_ENABLE               ( reg->rGA_ENABLE);\
		GA_SET_CMD_NUM          ( reg->rCMD_GAIN, job->cur_cmd_num);\
		GA_SEND_CMD_NUM_SIGNAL  ( reg->rCMD_GAIN);\
	}while(0)


#define REGION_VAL(val, mask, shift) \
	(((val) & (mask)) << (shift))
#define GA_COMPRESS_CMD(addr, cmd)										\
{																		\
	unsigned int *reg = (unsigned int*)addr;                          \
	GX_CMD_SET_VAL(reg+0,                                             \
                    REGION_VAL(cmd->GaMode.Ga_Op_Mode, 0x1, 31)     | \
                    REGION_VAL(cmd->Scaler.Scaler_Mode,0x1, 30)     | \
                    REGION_VAL(cmd->GaMode.Ga_Cmd_Mode,0x1, 29)     | \
                    REGION_VAL(cmd->GaMode.Inter_Mode, 0x1, 28)     | \
                    REGION_VAL(cmd->Rotater.Rotater_En,0x1, 27)     | \
                    REGION_VAL(cmd->Scaler.Scaler_En,  0x1, 26)     | \
                    REGION_VAL(cmd->Modulator.Premultiply_En,0x1,25)| \
                    REGION_VAL(cmd->Modulator.Demultiply_En, 0x1,24)| \
                    REGION_VAL(cmd->Blender.Blend_En,0x1,23)        | \
                    REGION_VAL(cmd->Rop.Rop_En,0x1,22)              | \
                    REGION_VAL(cmd->Xor_En,0x1, 21)                 | \
                    REGION_VAL(cmd->ColorKey.ColorKey_Src_En, 0x1, 20)  | \
                    REGION_VAL(cmd->ColorKey.Src_Has_Alpa, 0x1, 19)     | \
                    REGION_VAL(cmd->ColorKey.ColorKey_Dst_En, 0x1, 18)  | \
                    REGION_VAL(cmd->ColorKey.Dst_Has_Alpa, 0x1, 17)     | \
                    REGION_VAL(cmd->Pave_En, 0x1, 16)                   | \
                    REGION_VAL(cmd->Clut_A.Clut_En, 0x1, 15)            | \
                    REGION_VAL(cmd->Clut_B.Clut_En, 0x1, 14)            | \
                    REGION_VAL(cmd->SrcA.Data_Type, 0x3f, 8)            | \
                    REGION_VAL(cmd->GaMode.Dither_Mode, 0x3, 6)         | \
					REGION_VAL(cmd->Dst.Data_Type, 0x3f, 0));            \
            GX_CMD_SET_VAL(reg+1, \
                    REGION_VAL(cmd->Rotater.Rotater_Rotate_Mode, 0x3, 30) | \
                    REGION_VAL(cmd->Modulator.Modulator_Mode_SrcA, 0x7, 27) | \
                    REGION_VAL(cmd->Modulator.Modulator_Mode_DstA, 0x7, 24) | \
                    REGION_VAL(cmd->Rotater.Rotater_Flip_Mode, 0x3, 22) | \
                    REGION_VAL(cmd->Modulator.Modulator_Mode_SrcB, 0x7, 19) | \
                    REGION_VAL(cmd->Modulator.Modulator_Mode_DstB, 0x7, 16) | \
                    REGION_VAL(cmd->Clut_A.Clut_Length, 0x3, 14) | \
                    REGION_VAL(cmd->Modulator.Modulator_Mode_SrcC, 0x7, 11) | \
                    REGION_VAL(cmd->Modulator.Modulator_Mode_DstC, 0x7, 8) | \
                    REGION_VAL(cmd->Clut_B.Clut_Length, 0x3, 6) | \
                    REGION_VAL(cmd->Clut_A.Clut_Type, 0x3f, 0));  \
            GX_CMD_SET_VAL(reg+2,\
                    REGION_VAL(cmd->Endianness.Endianness_Srca_Read_Mode, 0xffffff, 8) | \
                    REGION_VAL(cmd->Blender.Src_Blend_Mode, 0x3, 6)| \
                    REGION_VAL(cmd->Clut_B.Clut_Type, 0x3f, 0));\
            GX_CMD_SET_VAL(reg+3,\
                    REGION_VAL(cmd->Endianness.Endianness_Srcb_Read_Mode, 0xffffff, 8) | \
					REGION_VAL(cmd->Blender.Dst_Blend_Mode, 0x3, 6)| \
					REGION_VAL(cmd->SrcB.Data_Type, 0x3f, 0));\
            GX_CMD_SET_VAL(reg+4,\
                    REGION_VAL(cmd->Endianness.Endianness_Write_Mode, 0xffffff, 8) | \
                    REGION_VAL(cmd->GaMode.Ga_Cmd_Point, 0xff, 0));\
			GX_CMD_SET_VAL(reg+ 5, cmd->Blender.Blend_Src_Color|cmd->Modulator.Modulator_Src_Color); \
			GX_CMD_SET_VAL(reg+ 6, cmd->Blender.Blend_Dst_Color|cmd->Modulator.Modulator_Dst_Color); \
            GX_CMD_SET_VAL(reg+7,  cmd->Scaler.Scaler_Coefficent_Addr );\
            GX_CMD_SET_VAL(reg+8,  cmd->ColorKey.ColorKey_Src_Reg );\
            GX_CMD_SET_VAL(reg+9,  cmd->ColorKey.ColorKey_Dst_Reg );\
            GX_CMD_SET_VAL(reg+10, cmd->Clut_A.Clut_Addr );\
            GX_CMD_SET_VAL(reg+11, cmd->Clut_B.Clut_Addr );\
            GX_CMD_SET_VAL(reg+12, cmd->SrcA.Addr );\
            GX_CMD_SET_VAL(reg+13, cmd->SrcB.Addr );\
            GX_CMD_SET_VAL(reg+14, cmd->Dst.Addr );\
            GX_CMD_SET_VAL(reg+15,\
                    REGION_VAL(cmd->SrcA.Base_Width, 0x3fff, 18)| \
                    REGION_VAL(cmd->SrcA.Width, 0x3fff, 4)      | \
                    REGION_VAL(cmd->Blender.Blend_Src_Fun, 0xf, 0));\
            GX_CMD_SET_VAL(reg+16,\
                    REGION_VAL(cmd->SrcA.Height, 0x3fff, 18) | \
                    REGION_VAL(cmd->SrcB.Base_Width, 0x3fff, 4) | \
                    REGION_VAL(cmd->Blender.Blend_Dst_Fun, 0xf, 0));\
            GX_CMD_SET_VAL(reg+17,\
                    REGION_VAL(cmd->SrcB.Width, 0x3fff, 18) | \
                    REGION_VAL(cmd->SrcB.Height, 0x3fff, 4) | \
                    REGION_VAL(cmd->Scaler.Scaler_Filter_Sign, 0xf, 0));\
            GX_CMD_SET_VAL(reg+18,\
                    REGION_VAL(cmd->Dst.Base_Width, 0x3fff, 18) | \
                    REGION_VAL(cmd->Dst.Width, 0x3fff, 4) | \
                    REGION_VAL(cmd->Rop.Rop_Code, 0xf, 0));\
            GX_CMD_SET_VAL(reg+19,\
                    REGION_VAL(cmd->Dst.Height, 0x3fff, 18) | \
                    REGION_VAL(cmd->Modulator.Demultiply_Mode, 0x3, 16) | \
                    REGION_VAL(cmd->Blender.Blend_Alpha_Out, 0xff, 8)| \
                    REGION_VAL(cmd->Blender.Alpha_Out_En, 0x1, 7)| \
                    REGION_VAL(cmd->Rop.Rop_Copy_Mode, 0x3, 5)| \
                    REGION_VAL(cmd->ColorKey.ColorKey_Mode,0x3, 3)| \
                    REGION_VAL(cmd->ColorKey.DstColorKey_switch,0x1, 2)| \
					REGION_VAL(0, 0x3, 0));\
        }while(0)

#endif

