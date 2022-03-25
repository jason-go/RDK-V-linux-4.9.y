#ifndef __SIRIUS_GA_INTERNEL_H__
#define __SIRIUS_GA_INTERNEL_H__

#include "kernelcalls.h"
#include "sirius_vpu.h"
#include "sirius_ga_reg.h"

/*========================== GA HardWare Command ===============================*/

typedef struct _Scaler {
	unsigned int    Scaler_Coefficent_Addr;
	unsigned int    Scaler_Filter_Sign;
	unsigned int    Scaler_Filter_Lmt;
	unsigned int    Filer_Lmt_En;
	unsigned int    Scaler_Mode;
	unsigned int    Scaler_En;
} ScalerState;

typedef enum _BlendFun {
	/*
	 *cf = color factor for blend function
	 *af = alpha factor for blend function
	 *sc = source color
	 *sa = source alpha
	 *dc = destination color
	 *da = destination alpha
	 */
	GABF_ZERO = 0,     // cf : 0            af : 0
	GABF_ONE,          // cf : 1            af : 1
	GABF_SRCCOLOR,     // cf : sc           af : sa
	GABF_INVSRCCOLOR,  // cf : 1-sc         af : 1-sa
	GABF_SRCALPHA,     // cf : sa           af : sa
	GABF_INVSRCALPHA,  // cf : 1-sa         af : 1-sa
	GABF_DESTALPHA,    // cf : da           af : da
	GABF_INVDESTALPHA, // cf : 1-da         af : 1-da
	GABF_DESTCOLOR,    // cf : dc           af : da
	GABF_INVDESTCOLOR, // cf : 1-dc         af : 1-da
	GABF_SRCALPHASAT,  // cf : min(sa,1-da) af : 1
	GABF_TRUE,
}BlendFun;

typedef struct _blender {
	unsigned int    Src_Blend_Mode;
	BlendFun        Blend_Src_Fun;
	unsigned int    Blend_Src_Color;
	unsigned int    Dst_Blend_Mode;
	BlendFun        Blend_Dst_Fun;
	unsigned int    Blend_Dst_Color;
	unsigned int    Alpha_Out_En;
	unsigned int    Blend_Alpha_Out;
	unsigned int    Blend_En;
} BlenderState;

typedef enum _RopCode{
	GARC_CLEAR = 0,     // DSTÇåÁã
	GARC_COPY,          // SRC¿½±´µ½DST
	GARC_AND,           // SRC and DST
	GARC_AND_INVERTED,  // (not SRC) and DST
	GARC_AND_REVERSE,   // SRC and (not DST)
	GARC_OR,            // SRC or DST
	GARC_OR_INVERTED,   // (not SRC) and DST
	GARC_OR_REVERSE,    // SRC and (not DST)
	GARC_NOOP,          // ¿Õ²Ù×÷
	GARC_INVERT,        // not DST
	GARC_COPY_INVERTED, // not SRC
	GARC_NAND,          // (not SRC) or (not DST)
	GARC_NOR,           // (not SRC) and (not DST)
	GARC_XOR,           // SRC xor DST
	GARC_EQUIV,         // not(SRC xor DST)
	GARC_SET,           // DSTÖÃ1
	GARC_EXTEND         // À©Õ¹¹¦ÄÜ£º·Ç½»Ö¯×ª½»Ö¯¸ñÊ½
}RopCode;

typedef struct _rop {
	RopCode         Rop_Code;
	unsigned int    Rop_Copy_Mode;
	unsigned int    Rop_En;
} RopState;

typedef struct _modulator {
	unsigned int    Premultiply_En;
	unsigned int    Modulator_Src_Color;
	unsigned int    Modulator_Mode_SrcA;
	unsigned int    Modulator_Mode_SrcB;
	unsigned int    Modulator_Mode_SrcC;
	unsigned int    Modulator_Dst_Color;
	unsigned int    Modulator_Mode_DstA;
	unsigned int    Modulator_Mode_DstB;
	unsigned int    Modulator_Mode_DstC;
	unsigned int    Demultiply_En;
	unsigned int    Demultiply_Mode;
} ModulatorState;

typedef struct _rotater {
	unsigned int    Rotater_Rotate_Mode;
	unsigned int    Rotater_Flip_Mode;
	unsigned int    Rotater_En;
} RotaterState;

typedef struct _clut {
	unsigned int    Clut_Addr;
	unsigned int    Clut_En;
	unsigned int    Clut_Length;
	unsigned int    Clut_Type;
} ClutState;

typedef struct _picstate {
	unsigned int    Addr;
	unsigned int    Base_Width;
	unsigned int    Width;
	unsigned int    Height;
	unsigned int    Data_Type;
} PicState;

typedef struct _endianness {
	unsigned int    Endianness_Srca_Read_Mode;
	unsigned int    Endianness_Srcb_Read_Mode;
	unsigned int    Endianness_Write_Mode;
} EndiannessState;

typedef struct _colorkey {
	unsigned int    Src_Has_Alpa;
	unsigned int    ColorKey_Src_En;
	unsigned int    ColorKey_Src_Reg;
	unsigned int    Dst_Has_Alpa;
	unsigned int    ColorKey_Dst_En;
	unsigned int    ColorKey_Dst_Reg;
	unsigned int    ColorKey_Mode;
	unsigned int	DstColorKey_switch;//0:DFB;1:INV_DF
} ColorKeyState;

typedef struct _gamode {
	unsigned int    Ga_Op_Mode;
	unsigned int    Ga_Cmd_Mode;
	unsigned int    Ga_Cmd_Point;
	unsigned int    Copy_Flag;
	unsigned int    Inter_Mode;
	unsigned int    Dither_Mode;
} GaModeState;

typedef struct _GaCmd {
	unsigned int    Xor_En;
	unsigned int    Pave_En;
	unsigned int    Posterization_En;
	unsigned int    Binarization_En;
	unsigned int    GAPF_A8_EN;
	unsigned int    RGB888_EN;
	RopState        Rop;
	PicState        SrcA;
	PicState        SrcB;
	PicState        Dst;
	ClutState       Clut_A;
	ClutState       Clut_B;
	ScalerState     Scaler;
	GaModeState     GaMode;
	RotaterState    Rotater;
	BlenderState    Blender;
	ColorKeyState   ColorKey;
	ModulatorState  Modulator;
	EndiannessState Endianness;
}GaCmd;

#define GA_CMD_SIZE (80)
#define GA_CCT_SIZE (2*sizeof(unsigned int))
/*========================== GA HardWare Command ===============================*/

typedef struct _GaJob {
	unsigned int max_cmd_num;
	unsigned int cur_cmd_num;
	unsigned int fin_cmd_num;
	unsigned int ready;
	unsigned int jobmem_size;
	void        *cmd_start_addr;
	void        *cct_start_addr;
}GaJob;

#define GA_MAX_JOB_NUM  (128)
typedef struct _GaJobList {
	unsigned int  job_num;
	GaJob        *jobs[GA_MAX_JOB_NUM];
	unsigned int  zt_coefficient;
}GaJobList;

typedef struct _GA {
	GaJobList       joblist;
	gx_sem_id       job_fin_sem;
	gx_sem_id       job_lock;
	GxGAMode        mode;
	SiriusGaReg     *reg;
}GA;

typedef struct {
	unsigned int    addr;
	unsigned int    width;
	unsigned int    height;
	unsigned int    base_width;
	unsigned int    data_type;
	GxColorFormat   color_format;
	unsigned int    bpp;
    unsigned int    argb_flag;
	unsigned int	global_alpha;
	unsigned int	global_alpha_en;
	GxBlitModulator modulator;
	GxClutConvertTable clut_convert_table;
}GaChannelInfo;

typedef struct {
    GaChannelInfo       src_a;
    GaChannelInfo       src_b;
    GaChannelInfo       dst;
    GxAluMode           mode;
    GxBlitColorKeyInfo  colorkey_info;
    GxBlitAlphaOutInfo  alpha_out_info;
    int time_wait_en;
}GaBlitParam;

typedef struct {
	GaBlitParam basic;
}GaDfbBlitParam;

typedef struct {
	unsigned int    color;
	GaChannelInfo   dst;
    GxBlitColorKeyInfo  colorkey_info;
	int				 blend_en;
	int              draw_XOR_en;
	int              time_wait_en;
    GxAluMode           blend_mode;
}GaFillRectParam;

typedef struct {
	GaChannelInfo   src;
	GaChannelInfo   dst;
	int             stretch_en;
}GaZoomParam;

typedef struct {
	unsigned int    flip_mode;
	unsigned int    rotate_mode;
	GaChannelInfo   src;
	GaChannelInfo   dst;
}GaTurnParam;

typedef struct {
	GaChannelInfo   src;
	void*           src_buf_U;
	void*           src_buf_V;
	GaChannelInfo   dst;
}GaCompletParam;

#define GA_PARAM_SET_CHANNEL_INFO(_channel, _surface, _rect)\
do {\
    typeof(_channel) *channel= &(_channel);\
    typeof(_surface) surface= (_surface);\
    typeof(_rect)    rect   = (_rect);\
    unsigned int bpp    = gx_color_get_bpp(surface->color_format);\
    unsigned int offset = (bpp * (surface->width * rect.y + rect.x)) >> 3;\
    channel->width      = rect.width;\
    channel->height     = rect.height;\
    channel->base_width = surface->width;\
    channel->data_type  = ga_data_type[surface->color_format];\
    channel->bpp        = bpp;\
    channel->addr       = (unsigned int)surface->buffer + offset;\
    if(surface->color_format == GX_COLOR_FMT_ARGB8888)\
        channel->argb_flag = 1;\
}while(0)

#define GA_CMD_SET_CLUT_CONVERT_INFO(_clut, _src)\
	do {\
		typeof(_clut)  *clut = &(_clut);\
		typeof(_src)    src  =  (_src);\
		if(src.clut_convert_table.table_len > 0 ) {\
			GxClutConvertTable *cct = &src.clut_convert_table;\
			clut->Clut_En      = 1;\
			clut->Clut_Addr    = gx_virt_to_phys((unsigned int)cct->table_addr);\
			clut->Clut_Type    = ga_data_type[cct->dst_format];\
			clut->Clut_Length  = gx_color_get_clut_length(cct->table_len);\
		}\
	}while(0)

#define GA_CMD_SET_CHANNEL_INFO(_channel, _param)\
	do {\
		typeof(_channel) *channel = &(_channel);\
		typeof(_param)    param   =  (_param);\
		channel->Addr       = gx_virt_to_phys(param.addr);\
		channel->Width      = param.width;\
		channel->Height     = param.height;\
		channel->Base_Width = param.base_width;\
		channel->Data_Type  = param.data_type;\
	}while(0)

#define GA_CMD_SET_COLORKEY_INFO(_dck, _sck)\
    do {\
        typeof(_dck) *dck = &(_dck);\
        typeof(_sck) *sck = &(_sck);\
        dck->ColorKey_Src_En    = sck->src_colorkey_en;\
		if (dck->ColorKey_Src_En)                    \
			dck->ColorKey_Src_Reg   = sck->src_colorkey;\
		dck->ColorKey_Dst_En    = sck->dst_colorkey_en;\
		if (dck->ColorKey_Dst_En)                    \
		    dck->ColorKey_Dst_Reg   = sck->dst_colorkey;\
        dck->ColorKey_Mode      = sck->mode;\
        dck->Src_Has_Alpa       = sck->src_colorkey_has_alpha;/* default: alpha is effective */\
        dck->Dst_Has_Alpa       = sck->dst_colorkey_has_alpha;\
        dck->DstColorKey_switch = 0;/*0:DFB;1:INV_DF*/\
    }while(0)

#define GA_CMD_SET_DATA_ENDIANNESS(_endian, wmode, srca_rmode, srcb_rmode)\
	do {\
		typeof(_endian) *endian = &(_endian);\
		endian->Endianness_Write_Mode    = wmode;\
		endian->Endianness_Srca_Read_Mode= srca_rmode;\
		endian->Endianness_Srcb_Read_Mode= srcb_rmode;\
	}while(0)

#define IF_TRUE_RETURN(p)\
	if((p)) {\
		gx_printf("%s:%d Warning: "#p" failed.\n", __func__, __LINE__);\
		return;\
	}

#define IF_TRUE_RETURN_WITH_VAL(p, ret)\
	if((p)) {\
		gx_printf("%s:%d Warning: "#p" failed.\n", __func__, __LINE__);\
		return (ret);\
	}

#define GA_PRINTF(fmt, args...) \
	do {\
		gx_printf("\n[GA][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)

#endif
