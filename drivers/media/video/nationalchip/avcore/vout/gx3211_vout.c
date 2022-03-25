#include "porting.h"
#include "clock_hal.h"
#include "hdmi_hal.h"
#include "vpu_hal.h"
#include "vout_hal.h"
#include "vout_module.h"
#include "gx3211_vout.h"
#include "gx3211_vout_reg.h"
#include "secure_hal.h"
#include "enhance.h"

extern int gx3211_osd_enable(int enable);
extern int gx3211_spp_enable(int enable);
extern int gx3211_vpp_enable(int enable);
static int gx3211_vout_set_resolution(struct gxav_videoout *vout, GxVideoOutProperty_Resolution *resolution);
static int gx3211_vout_set_macrovision(struct gxav_videoout* vout, GxVideoOutProperty_Macrovision *macrovision);
static int gx3211_vout_set_dcs(struct gxav_videoout* vout, GxVideoOutProperty_DCS *dcs);
static void gx3211_vout_init_interface(void);
static void gx3211_vout_init_resolution(void);
static SvpuSurfaceInfo svpu_fb;

#define IF_FAIL_IRQ_ENABLE(ret) \
	if(ret < 0) {\
		gx_interrupt_enable();\
	}

static volatile struct gx3211_videoout_reg* gx3211_vout0_reg;
static volatile struct gx3211_videoout_reg* gx3211_vout1_reg;
struct gxav_module_ops gx3211_videoout_module;

#if 0
#define PRINT_REGISTER() \
	do { \
		gx_printf("\nPrint at %s:%d:\n", __func__, __LINE__);
		gx_printf("\t[0x%x] 0x%x\n", &gx3211_vout1_reg->tv_adjust, gx3211_vout1_reg->tv_adjust);
		gx_printf("\t[0x%x] 0x%x\n", &gx3211_vout1_reg->colr_tran, gx3211_vout1_reg->colr_tran);
	}
#endif

static struct vout_info gx3211_voutinfo;
static struct reg_iface gx3211_valid_iface[] = {
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_RCA1 | GXAV_VOUT_SVIDEO | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_RCA1  ] = { GXAV_VOUT_RCA1,    VPU_DAC_0 },
		.ifaces_dac[IFACE_ID_SVIDEO] = { GXAV_VOUT_SVIDEO,  VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_RCA1 | GXAV_VOUT_YUV | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x5,
		.vpu_dac_mode                = 0x3,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x1,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_RCA1  ] = { GXAV_VOUT_RCA1, VPU_DAC_0 },
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV, VPU_DAC_1 | VPU_DAC_2 | VPU_DAC_3 },
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_YUV | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x1,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x1,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA, VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV, VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_SCART | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x2,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x2,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_SCART ] = { GXAV_VOUT_SCART,   VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 | VPU_DAC_3 },
	},
	{
		.select                      = GXAV_VOUT_YUV,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x3,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x3,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV,     VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_RCA,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x3,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_RCA]    = { GXAV_VOUT_RCA, VPU_DAC_3 },
	},
	{
		.select                      = GXAV_VOUT_HDMI,
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x1,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_3 },
	},
	{
		.select                      = GXAV_VOUT_SVIDEO | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_SVIDEO] = { GXAV_VOUT_SVIDEO,  VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_YUV,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x3,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV,     VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_YUV | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x3,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x3,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV,     VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_SCART | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x2,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x4,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_SCART ] = { GXAV_VOUT_SCART,   VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 | VPU_DAC_3 },
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_RCA1 | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_RCA1  ] = { GXAV_VOUT_RCA1,    VPU_DAC_0 },
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_SVIDEO | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_SVIDEO] = { GXAV_VOUT_SVIDEO,  VPU_DAC_1 | VPU_DAC_2 },
	},
};

#define CHECK_VOUT_INTERFACE(viface) \
	do {\
		if(!viface) {\
			VIDEOOUT_PRINTF("can't get viface\n");\
			return -1;\
		} \
	}while(0)


struct scale_param {
	int scale0, scale1, scale2;
	int shift;
	int black, blank;
};

static struct scale_param normal_scale[] = {
	{ .scale0 =   0, .scale1 =   0, .scale2 =   0, .shift =  0, .black =  0, .blank =  0 },
	{ .scale0 = 160, .scale1 = 143, .scale2 = 203, .shift = 63, .black = 63, .blank = 63 },//	GXAV_VOUT_PAL = 1
	{ .scale0 = 151, .scale1 = 134, .scale2 = 190, .shift = 71, .black = 71, .blank = 60 },//	GXAV_VOUT_PAL_M
	{ .scale0 = 160, .scale1 = 143, .scale2 = 203, .shift = 63, .black = 63, .blank = 63 },//	GXAV_VOUT_PAL_N
	{ .scale0 = 160, .scale1 = 143, .scale2 = 203, .shift = 63, .black = 63, .blank = 63 },//	GXAV_VOUT_PAL_NC
	{ .scale0 = 151, .scale1 = 134, .scale2 = 190, .shift = 71, .black = 71, .blank = 60 },//	GXAV_VOUT_NTSC_M
	{ .scale0 = 151, .scale1 = 134, .scale2 = 190, .shift = 71, .black = 71, .blank = 60 },//	GXAV_VOUT_NTSC_443
#if 0
	{ .scale0 = 160, .scale1 = 143, .scale2 = 203, .shift = 63 },
	{ .scale0 = 161, .scale1 = 136, .scale2 = 192, .shift = 71 },
	{ .scale0 = 161, .scale1 = 136, .scale2 = 192, .shift = 63 },
	{ .scale0 = 160, .scale1 = 164, .scale2 = 233, .shift = 71 },
	{ .scale0 = 151, .scale1 = 134, .scale2 = 190, .shift = 71 },
	{ .scale0 = 151, .scale1 = 134, .scale2 = 190, .shift = 71 },
#endif
};

static struct scale_param scart_scale[] = {
	{ .scale0 = 160, .scale1 = 143, .scale2 = 203, .shift = 63, .black = 63, .blank = 63 },//	GXAV_VOUT_PAL = 1
	{ .scale0 = 151, .scale1 = 134, .scale2 = 190, .shift = 63, .black = 63, .blank = 63 },//	GXAV_VOUT_NTSC_M
};


static struct {
	GxMacrovisionMode  mode;
	GxMacrovisionParam p_param;
	GxMacrovisionParam n_param;
} mcv_default_params[] = {
	{
		.mode = GX_MACROVISION_MODE_TYPE0,/*<type 0:no protect process *//**<CNcomment:无保护处理*/
		.p_param = {
			.cpc = 0x00,
			.cps = {
				0x7D, 0xB2, 0x90, 0xB5, 0x66,
				0xBD, 0x09, 0xF3, 0x0C, 0x00,
				0x00, 0x0F, 0x6F, 0xD0, 0x20,
				0xFC, 0x0F
			},
		},
		.n_param = {
			.cpc = 0x00,
			.cps = {
				0x7D, 0xB2, 0x90, 0xB5, 0x66,
				0xBD, 0x09, 0xF3, 0x0C, 0x00,
				0x00, 0x0F, 0x6F, 0xD0, 0x20,
				0xFC, 0x0F
			},
		},
	},

	{
		.mode = GX_MACROVISION_MODE_TYPE1,/*<type 1:AGC (automatic gain control) process only *//**<CNcomment:仅自动增益控制 */
		.p_param = {
			.cpc = 0x63,
			.cps = {
				0xA5, 0x54, 0x25, 0xA2, 0x78,
				0xD3, 0x05, 0xF1, 0x0F, 0x51,
				0xF4, 0x7E, 0x6E, 0x40, 0x20,
				0x57, 0x05
			},
		},
		.n_param = {
			.cpc = 0x63,
			.cps = {
				0xDD, 0x32, 0x92, 0x31, 0x6C,
				0xBD, 0x09, 0xF3, 0x0C, 0x00,
				0x00, 0x0F, 0x6F, 0xD0, 0x20,
				0xFC, 0x0F
			},
		},
	},

	{
		.mode = GX_MACROVISION_MODE_TYPE2,/*<type 2:AGC + 2-line color stripe *//**<CNcomment:自动增益控制和两线色 度干扰*/
		.p_param = {
			.cpc = 0x63,
			.cps = {
				0xA5, 0x54, 0x25, 0xA2, 0x78,
				0xD3, 0x05, 0xF1, 0x0F, 0x51,
				0xF4, 0x7E, 0x6E, 0x40, 0x20,
				0x57, 0x05
			},
		},
		.n_param = {
			.cpc = 0xE3,
			.cps = {
				0xDD, 0x32, 0x92, 0x31, 0x6C,
				0xBD, 0x09, 0xF3, 0x0C, 0x00,
				0x00, 0x0F, 0x6F, 0xD0, 0x20,
				0xFC, 0x0F
			},
		},
	},

	{
		.mode = GX_MACROVISION_MODE_TYPE3,/*<type 3:AGC + aggressive 4-line color stripe *//**<CNcomment:自动增益和四线强色度干扰*/
		.p_param = {
			.cpc = 0x63,
			.cps = {
				0xA5, 0x54, 0x25, 0xA2, 0x78,
				0xD3, 0x05, 0xF1, 0x0F, 0x51,
				0xF4, 0x7E, 0x6E, 0x40, 0x20,
				0x57, 0x05
			},
		},
		.n_param = {
			.cpc = 0xE3,
			.cps = {
				0x7D, 0xB2, 0x90, 0xB5, 0x66,
				0xBD, 0x09, 0xF3, 0x0C, 0x00,
				0x00, 0x0F, 0x6F, 0xD0, 0x20,
				0xFC, 0x0F
			},
		},
	}
};

static GxMacrovisionParam* mcv_get_default_param(GxMacrovisionMode mode)
{
	int i;
	GxMacrovisionParam *param = NULL;

	for (i = 0; i < sizeof(mcv_default_params)/sizeof(mcv_default_params[0]); i++) {
		if (mcv_default_params[i].mode == mode) {
			if (IS_P_MODE(gx3211_voutinfo.src[ID_SVPU].mode))
				param = &mcv_default_params[i].p_param;
			else
				param = &mcv_default_params[i].n_param;
			break;
		}
	}

	return param;
}

static void macrovision_set_reg(GxMacrovisionParam *param)
{
	unsigned int temp;
	unsigned char aps_n17;

	if (param) {
		REG_CLR_BIT(&(gx3211_vout1_reg->aps_reg4), 12);
		/* set cpc */
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg0), (0xff<<0), param->cpc, 0);
		/* set cps */
		temp = (param->cps[0] & 0xf3) |(param->cps[15] & 0xc0) >> 4;
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg0), (0xff<< 8), temp, 8);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg0), (0xff<<16), param->cps[1],  16);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg0), (0xff<<24), param->cps[2],  24);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg1), (0xff<< 0), param->cps[3],   0);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg1), (0xff<< 8), param->cps[4],   8);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg1), (0xff<<16), param->cps[5],  16);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg1), (0xff<<24), param->cps[6],  24);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg2), (0xff<< 0), param->cps[7],   0);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg2), (0xff<< 8), param->cps[8],   8);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg2), (0xff<<16), param->cps[9],  16);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg2), (0xff<<24), param->cps[10], 24);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg3), (0xff<< 0), param->cps[11],  0);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg3), (0xff<< 8), param->cps[12],  8);

		temp = REG_GET_FIELD(&(gx3211_vout1_reg->tv_ctrl), 0x1f, 0);

		aps_n17 = (param->cps[13] & 0x70) >> 3 | (param->cps[14] & 0x08) >> 3;
		aps_n17 = aps_n17 + 1;

		if(aps_n17 > 15) aps_n17 = 15;

		temp = (aps_n17 << 3 & 0x70) ;
		temp |= 1 << 7 | (param->cps[13] & 0x0f);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg3), (0xff<<16), temp, 16);

		temp = (param->cps[14] & 0xf7) | (aps_n17 & 0x1) << 3;
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg3), (0xff<<24), temp, 24);

		temp = (param->cps[15] & 0x0f) | (param->cps[0] & 0x0c) << 4 | (param->cps[16] & 0x03) << 4 ;
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg4), (0xff<< 0), temp, 0);

		param->cps[16] &= 0xf;
		temp = (param->cps[16] & 0x0c) | (param->cps[15] & 0x30) >> 4;
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg4), (0xff<< 8), temp, 8);

		REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl0), (0xff<<24), 1, 24);

		if ((param->cps[13] & 0x80) == 0x00){
			REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl0), (0xff<<16), 75, 16);
		}
		else {
			REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl0), (0xff<<16), 67, 16);
		}

		REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl0), (0xff<< 0), 6, 0);

		REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl3), (0xff<< 0),27, 0);

		temp = REG_GET_FIELD(&(gx3211_vout1_reg->tv_ctrl), 0x1f, 0);


		if (temp == 4 || temp == 1 || temp == 5) { //NTSC PAL-M NTSC-443

			REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl2), (0xff<< 0),14, 0);

			REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl3), (0xff<<10),41,10);

			if (param->cpc & 0x20)
				REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 8), 57, 8);
			else
				REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 8), 71, 8);
		}
		else { //PAL PAL-N PAL-NC

			REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl2), (0xff<< 0),11, 0);

			REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl3), (0xff<<10),37,10);

			if (param->cpc & 0x20)
				REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 8), 52, 8);
			else
				REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 8), 63, 8);
		}
	}
}

static struct vout_interface* gx3211_get_vout_interface(GxVideoOutProperty_Interface iface)
{
	int i;

	for (i = 0; i< GX_ARRAY_SIZE(gx3211_voutinfo.interfaces); i++) {
		if (iface == gx3211_voutinfo.interfaces[i].iface)
			return &gx3211_voutinfo.interfaces[i];
	}

	return NULL;
}

static int get_scale_param(GxVideoOutProperty_Mode mode, struct scale_param *param)
{
	struct vout_interface* viface = NULL;

	if (gx3211_voutinfo.scart_config.enable == 1) {
		viface = gx3211_get_vout_interface(GXAV_VOUT_SCART);
		if (IS_P_MODE(viface->mode))
			*param = scart_scale[0];
		else
			*param = scart_scale[1];
	} else {
		*param = normal_scale[mode];
	}

	return 0;
}

static struct reg_iface* gx3211_vout_get_riface(int select)
{
	int i = 0, j = 0;

	for (i = 0; i< GX_ARRAY_SIZE(gx3211_valid_iface); i++) {
		if (select == gx3211_valid_iface[i].select)
			break;
	}
	if (i == GX_ARRAY_SIZE(gx3211_valid_iface)) {
		VIDEOOUT_PRINTF("select is invalid!\n");
		return NULL;
	}

	if (CHIP_IS_GX6605S || CHIP_IS_TAURUS) {
		for (j = 0; j < IFACE_ID_MAX; j++)
			gx3211_valid_iface[i].ifaces_dac[j].dac = VPU_DAC_0;
	}

	return &gx3211_valid_iface[i];
}


static int vout_get_interface_id(GxVideoOutProperty_Interface iface)
{
	int id = -1;

	switch (iface) {
	case GXAV_VOUT_RCA:
		id = IFACE_ID_RCA;
		break;
	case GXAV_VOUT_RCA1:
		id = IFACE_ID_RCA1;
		break;
	case GXAV_VOUT_YUV:
		id = IFACE_ID_YUV;
		break;
	case GXAV_VOUT_SCART:
		id = IFACE_ID_SCART;
		break;
	case GXAV_VOUT_SVIDEO:
		id = IFACE_ID_SVIDEO;
		break;
	case GXAV_VOUT_HDMI:
		id = IFACE_ID_HDMI;
		break;
	default:
		break;
	}

	return id;
}

static int vout_get_src_by_interface(struct gxav_videoout *vout, GxVideoOutProperty_Interface iface)
{
	int ret;
	int dac, src = 0;
	struct reg_iface* riface = gx3211_vout_get_riface(vout->select);

	if (iface == GXAV_VOUT_HDMI || iface == GXAV_VOUT_YUV)
		ret = ID_VPU;
	else {
		dac = riface->ifaces_dac[vout_get_interface_id(iface)].dac;
		if (dac & VPU_DAC_0)
			src = riface->dac_src0;
		if (dac & VPU_DAC_1)
			src = riface->dac_src1;
		if (dac & VPU_DAC_2)
			src = riface->dac_src2;
		if (dac & VPU_DAC_3)
			src = riface->dac_src3;
		ret = (src == VPU_DAC_SRC_VPU ? ID_VPU : ID_SVPU);
	}

	return ret;
}

static volatile struct gx3211_videoout_reg* vout_get_reg_by_interface(struct gxav_videoout *vout, GxVideoOutProperty_Interface iface)
{
	int src;
	volatile struct gx3211_videoout_reg *reg = NULL;

	src = vout_get_src_by_interface(vout, iface);
	if (src == ID_VPU)
		reg = gx3211_vout0_reg;
	else
		reg = gx3211_vout1_reg;

	return reg;
}

static int vout_use_src(struct gxav_videoout *vout, int src)
{
	int i, max;
	int interfaces[] = {
		GXAV_VOUT_RCA,
		GXAV_VOUT_RCA1,
		GXAV_VOUT_YUV,
		GXAV_VOUT_SCART,
		GXAV_VOUT_SVIDEO,
		GXAV_VOUT_HDMI,
	};

	max = sizeof(interfaces)/sizeof(interfaces[i]);
	for (i = 0; i < max; i++) {
		if (vout->select & interfaces[i]) {
			if (src == vout_get_src_by_interface(vout, interfaces[i]))
				break;
		}
	}

	return i != max;
}

static int gx3211_vout_get_dvemode(struct gxav_videoout *vout, struct vout_dvemode *dvemode)
{
	int id = dvemode->id;

	if (id != ID_VPU && id != ID_SVPU)
		return -1;

	dvemode->mode = gx3211_voutinfo.src[id].mode;

	return 0;
}

static int gx3211_vout_pal_config(volatile struct gx3211_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch (mode) {

	case GXAV_VOUT_PAL:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|0, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(&(reg->tv_ctrl), 31);
		break;
	case GXAV_VOUT_PAL_N:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|2, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(&(reg->tv_ctrl), 31);
		break;
	case GXAV_VOUT_PAL_NC:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|3, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(&(reg->tv_ctrl), 31);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in pal\n");
		return -1;
	}

	return 0;
}

static int gx3211_vout_ntsc_config(volatile struct gx3211_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {

	case GXAV_VOUT_NTSC_M:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|4, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(&(reg->tv_ctrl), 31);
		break;
	case GXAV_VOUT_NTSC_443:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|5, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(&(reg->tv_ctrl), 31);
		break;
	case GXAV_VOUT_PAL_M:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|1, bTV_CTRL_TV_FORMAT);
		REG_SET_BIT(&(reg->tv_ctrl), 31);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in ntsc\n");
		return -1;
	}

	return 0;
}

static int gx3211_vout_yuv_config(volatile struct gx3211_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {
	case GXAV_VOUT_480I:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|4, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_576I:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|0, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_480P:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|8, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_576P:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|9, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_720P_50HZ:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (1<<bTV_CTRL_FRAME_FRE)|13, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_720P_60HZ:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|13, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_1080I_50HZ:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (1<<bTV_CTRL_FRAME_FRE)|6, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_1080I_60HZ:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|6, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_1080P_50HZ:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (1<<bTV_CTRL_FRAME_FRE)|14, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	case GXAV_VOUT_1080P_60HZ:
		REG_SET_FIELD(&(reg->tv_ctrl), 0xff<<bTV_CTRL_TV_FORMAT, (0<<bTV_CTRL_FRAME_FRE)|14, bTV_CTRL_TV_FORMAT);
		REG_SET_FIELD(&(reg->tv_ctrl), 0x7<<bTV_CTRL_DAC_OUT_MODE, 3, bTV_CTRL_DAC_OUT_MODE);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in yuv\n");
		return -1;
	}

	REG_SET_BIT(&(reg->tv_ctrl), 31);

	return 0;
}

/*
 *  config dac full case and mode, etc
 *  the tv_ctrl register may be change when config the resolution array
 */
static void gx3211_vout_config_dac_mode_case(struct vout_interface *viface)
{
	struct vout_interface* viface_yuv;
	volatile struct gx3211_videoout_reg *reg;
	GxVideoOutProperty_Mode mode;
	struct reg_iface* riface = gx3211_vout_get_riface(viface->vout->select);
	int src = vout_get_src_by_interface(viface->vout, viface->iface);

	if (src == ID_VPU) {
		DVE_SET_DAC_OUT_MODE(gx3211_vout0_reg, riface->vpu_dac_mode);
#if 0
		if (CHIP_IS_SIRIUS || CHIP_IS_TAURUS)
			DVE_SET_DAC_FULLCASE_CHANGE(gx3211_vout0_reg, 6);
		else
			DVE_SET_DAC_FULLCASE_CHANGE(gx3211_vout0_reg, riface->vpu_dac_case);
#endif
	}

	if (src == ID_SVPU) {
		DVE_SET_DAC_OUT_MODE(gx3211_vout1_reg, riface->svpu_dac_mode);
#if 0
		DVE_SET_DAC_FULLCASE_CHANGE(gx3211_vout1_reg, riface->svpu_dac_case);
#endif
	}

	if (viface->vout->select & GXAV_VOUT_YUV) {
		reg = vout_get_reg_by_interface(viface->vout, GXAV_VOUT_YUV);
		viface_yuv = gx3211_get_vout_interface(GXAV_VOUT_YUV);
		mode = viface_yuv->mode;

		switch (mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
			//Y_Cb_Cr_CVBS : YCBCR_480I、YCBCR_576I
			DVE_SET_DAC_OUT_MODE(reg, (Y_Cb_Cr_CVBS));
			break;
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			//Y_Pb_Pr_Y : YPBPR_480P、YPBPR_576P
			DVE_SET_DAC_OUT_MODE(reg, (Y_Pb_Pr_Y));
			break;
		default:
			break;
		}
	}
}

static GxVideoOutProperty_Mode gx3211_iface_get_mode(struct vout_interface *viface);
static int gx3211_vout_config_vout_hvsync(void)
{
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, GXAV_VOUT_HDMI);
	GxVideoOutProperty_Mode mode = gx3211_iface_get_mode(viface);

	volatile unsigned int *hvsync   = &reg->scart_ctrl;  //0xa4804070
	volatile unsigned int *baseline = &reg->bt656_ctrl1; //0xa4804098

	switch(mode)
	{
	case GXAV_VOUT_480I:
	case GXAV_VOUT_576I:
		*hvsync &= ~(3 << 3);
		break;
	case GXAV_VOUT_480P:
		*hvsync &= ~(3 << 3);
		break;
	case GXAV_VOUT_576P:
		*hvsync &= ~(3 << 3);
		*baseline = 0x0001626b;
		break;
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		*hvsync |= 3 << 3;
		*baseline = 0x0000cae8;
		break;
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ:
		*hvsync |= (3 << 3);
		break;
	default:
		VIDEOOUT_PRINTF("un-supported.\n");
		return -1;
	}

	return 0;
}

static int gx3211_fullrange_enable(int flag)
{
	if (CHIP_IS_CYGNUS) {
		int y_value = 0, cb_value = 0, cr_value = 0;
		{
			DVE_GET_YUV_TRAN_Y_SCALE0(gx3211_vout0_reg, y_value);
			DVE_GET_YUV_TRAN_U_SCALE1(gx3211_vout0_reg, cb_value);
			DVE_GET_YUV_TRAN_V_SCALE2(gx3211_vout0_reg, cr_value);
			if(flag) {
				REG_SET_BIT(&(gx3211_vout0_reg->digital_ctrl), 16);
				y_value = y_value * 220 / 256;
				cb_value = cb_value * 224 / 256;
				cr_value = cr_value * 224 / 256;
				DVE_SET_YUV_TRAN_Y_SCALE0(gx3211_vout0_reg, y_value);
				DVE_SET_YUV_TRAN_U_SCALE1(gx3211_vout0_reg, cb_value);
				DVE_SET_YUV_TRAN_V_SCALE2(gx3211_vout0_reg, cr_value);
			} else {
				REG_CLR_BIT(&(gx3211_vout0_reg->digital_ctrl), 16);
				y_value = y_value * 256 / 220;
				cb_value = cb_value * 256 / 224;
				cr_value = cr_value * 256 / 224;
				DVE_SET_YUV_TRAN_Y_SCALE0(gx3211_vout0_reg, y_value);
				DVE_SET_YUV_TRAN_U_SCALE1(gx3211_vout0_reg, cb_value);
				DVE_SET_YUV_TRAN_V_SCALE2(gx3211_vout0_reg, cr_value);
			}
		}
	}
	return 0;
}

static int gx3211_vout_set_dve(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	struct gxav_videoout *vout = viface->vout;
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(vout, viface->iface);

	if(!reg)
		return -1;

	if (viface->iface == GXAV_VOUT_HDMI) {
		switch (mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			gx3211_vout_yuv_config(reg, mode);
			break;
		default:
			VIDEOOUT_PRINTF("un-supported.\n");
			return -1;
		}
	}
	else {
		switch(mode)
		{
		case GXAV_VOUT_PAL:
		case GXAV_VOUT_PAL_N:
		case GXAV_VOUT_PAL_NC:
			gx3211_vout_pal_config(reg, mode);
			break;
		case GXAV_VOUT_PAL_M:
		case GXAV_VOUT_NTSC_M:
		case GXAV_VOUT_NTSC_443:
			gx3211_vout_ntsc_config(reg, mode);
			break;
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			gx3211_vout_yuv_config(reg, mode);
			break;
		default:
			VIDEOOUT_PRINTF("un-supported.\n");
			return -1;
		}
	}

	if(viface->iface == GXAV_VOUT_SCART)	{
		gxav_clock_multiplex_pinsel(PIN_TYPE_VIDEOOUT_SCART);
		switch(mode)
		{
		case GXAV_VOUT_PAL:
			REG_SET_FIELD(&(reg->tv_ctrl), 0xfffff, 0x20000, 0);
			break;
		case GXAV_VOUT_PAL_M:
			REG_SET_FIELD(&(reg->tv_ctrl), 0xfffff, 0x20001, 0);
			break;
		case GXAV_VOUT_PAL_N:
			REG_SET_FIELD(&(reg->tv_ctrl), 0xfffff, 0x20002, 0);
			break;
		case GXAV_VOUT_NTSC_M:
			REG_SET_FIELD(&(reg->tv_ctrl), 0xfffff, 0x20004, 0);
			break;
		case GXAV_VOUT_NTSC_443:
			REG_SET_FIELD(&(reg->tv_ctrl), 0xfffff, 0x20005, 0);
			break;
		default:
			VIDEOOUT_PRINTF("un-supported.\n");
			break;
		}
	}

	int id = vout_get_src_by_interface(vout, viface->iface);
	gx3211_voutinfo.src[id].mode = mode;
	viface->mode = mode;
	gx3211_vout_config_dac_mode_case(viface);

	REG_CLR_BIT(&(gx3211_vout0_reg->tv_ctrl), 30);
	REG_CLR_BIT(&(gx3211_vout1_reg->tv_ctrl), 30);

	//clk set
	{
		clock_params params;
		params.vout.src  = id;
		params.vout.mode = mode;
		gxav_clock_setclock(MODULE_TYPE_VOUT, &params);
	}

	//VPU hot-reset
	gxav_vpu_reset();
	gx3211_vout_config_vout_hvsync();

	gx3211_vout_set_dcs(vout, &vout->dcs);
	gx3211_vout_set_macrovision(vout, &vout->macrovision);

	return 0;
}

static unsigned int GX3211_VOUT0_BASE_ADDR = 0x04804000;
static unsigned int GX3211_VOUT1_BASE_ADDR = 0x04904000;
static int gx3211_vout_ioremap(void)
{
	unsigned char *vout0_mapped_addr = NULL;
	unsigned char *vout1_mapped_addr = NULL;

	// VOUT0
	// Sirius and Taurus VPU register include VOUT0
	if(CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		if (0 == gx_request_mem_region(GX3211_VOUT0_BASE_ADDR, sizeof(struct gx3211_videoout_reg))) {
			VIDEOOUT_PRINTF("request_mem_region failed");
			return -1;
		}
	}
	vout0_mapped_addr = gx_ioremap(GX3211_VOUT0_BASE_ADDR, sizeof(struct gx3211_videoout_reg));
	if (!vout0_mapped_addr) {
		VIDEOOUT_PRINTF("ioremap failed.\n");
		goto err_vout0_ioremap;
	}

	// VOUT1
	if (0 == gx_request_mem_region(GX3211_VOUT1_BASE_ADDR, sizeof(struct gx3211_videoout_reg))) {
		VIDEOOUT_PRINTF("request_mem_region failed");
		goto err_vout1_request_mem;
	}
	vout1_mapped_addr = gx_ioremap(GX3211_VOUT1_BASE_ADDR, sizeof(struct gx3211_videoout_reg));
	if (!vout1_mapped_addr) {
		VIDEOOUT_PRINTF("ioremap failed.\n");
		goto err_vout1_ioremap;
	}

	gx3211_vout0_reg = (struct gx3211_videoout_reg *)vout0_mapped_addr;
	gx3211_vout1_reg = (struct gx3211_videoout_reg *)vout1_mapped_addr;

	return 0;

err_vout0_ioremap:
	gx_release_mem_region(GX3211_VOUT0_BASE_ADDR, sizeof(struct gx3211_videoout_reg));
err_vout1_request_mem:
	gx_iounmap(gx3211_vout0_reg);
	gx3211_vout0_reg= NULL;

	gx_release_mem_region(GX3211_VOUT0_BASE_ADDR, sizeof(struct gx3211_videoout_reg));
err_vout1_ioremap:
	gx_iounmap(gx3211_vout0_reg);
	gx3211_vout0_reg= NULL;

	// Sirius and Taurus VPU register include VOUT0
	if(CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		gx_release_mem_region(GX3211_VOUT0_BASE_ADDR, sizeof(struct gx3211_videoout_reg));
	}
	gx_release_mem_region(GX3211_VOUT1_BASE_ADDR, sizeof(struct gx3211_videoout_reg));

	return -1;
}

static int gx3211_vout_iounmap(void)
{
	gx_iounmap(gx3211_vout0_reg);
	gx_iounmap(gx3211_vout1_reg);
	gx3211_vout0_reg = NULL;
	gx3211_vout1_reg = NULL;

	// Sirius and Taurus VPU register include VOUT0
	if(CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		gx_release_mem_region(GX3211_VOUT0_BASE_ADDR, sizeof(struct gx3211_videoout_reg));
	}
	gx_release_mem_region(GX3211_VOUT1_BASE_ADDR, sizeof(struct gx3211_videoout_reg));

	return 0;
}

static int gx3211_vout_init(void)
{
	if (gx3211_vout_ioremap() < 0) {
		return -1;
	}

	memset(&gx3211_voutinfo, 0, sizeof(gx3211_voutinfo));
	gx3211_vout_init_interface();
	gx3211_vout_init_resolution();

	REG_CLR_BIT(&(gx3211_vout0_reg->digital_ctrl), 1);
	REG_SET_BIT(&(gx3211_vout0_reg->gamma0_ctl0), 28);

	return 0;
}

static int gx3211_vout_uninit(void)
{
	gx3211_vout_iounmap();

	VIDEOOUT_DBG("gx3211_vout_uninit\n");

	return 0;
}

static int gx3211_vout_open(struct gxav_videoout *vout)
{
	vout->hdmi_delayms = 0;
	vout->dcs.mode = GX_DCS_MODE_DISABLE;
	vout->macrovision.mode = GX_MACROVISION_MODE_TYPE0;

	if (CHIP_IS_GEMINI || CHIP_IS_TAURUS) {
		vout->enhancement_enable = 1;
		REG_SET_VAL(&gx3211_vout0_reg->resv4[11], 0x2020);
		REG_SET_VAL(&gx3211_vout0_reg->resv3[2], 0x401);
	} else {
		vout->enhancement_enable = 0;
		REG_SET_VAL(&gx3211_vout0_reg->resv3[2], 0);
	}

	if (CHIP_IS_CYGNUS) {
		vout->enhancement_enable = 1;
		enhance_init();
	}

	return 0;
}

static int gx3211_vout_close(struct gxav_videoout *vout)
{
	return 0;
}

static void gx3211_vout_insert_scan_chance(int mode)
{
	while(gxav_vpu_get_scan_line()!=0);
	switch (mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_576I:
		while(gxav_vpu_get_scan_line()<1);
		break;
	case GXAV_VOUT_576P:
		while(gxav_vpu_get_scan_line()<1);
		break;
	case GXAV_VOUT_480P:
		while(gxav_vpu_get_scan_line()<1);
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
		while(gxav_vpu_get_scan_line()<1);
		break;
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		while(gxav_vpu_get_scan_line()<1);
		break;
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
		while(gxav_vpu_get_scan_line()<1);
		break;
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ:
		while(gxav_vpu_get_scan_line()<1);
		break;
	default:
		break;
	}
}

typedef enum {
	VOUT_ON,
	VOUT_OFF,
}VoutState;
static void vout_switch(VoutState state)
{
	static unsigned int reg_back;

	if(state == VOUT_OFF) {
		reg_back = REG_GET_VAL(&gx3211_vout1_reg->colr_tran);
		REG_SET_VAL(&gx3211_vout1_reg->colr_tran, 0);
	}
	else if(state == VOUT_ON) {
		gx_mdelay(60);
		REG_SET_VAL(&gx3211_vout1_reg->colr_tran, reg_back);
	}
}

static int gx3211_vout_svpu_power_on(struct gxav_videoout *vout, int selection, int on)
{
	int i;
	struct reg_iface* riface = gx3211_vout_get_riface(selection);

	if (!riface)
		return -1;
	GX3211_CHECK_VOUT_IFACE(vout->select, selection);

	for (i = 0; i < IFACE_ID_MAX; i++) {
		if (riface->ifaces_dac[i].iface & selection)
			gxav_vpu_DACEnable(riface->ifaces_dac[i].dac, on);
	}

	return 0;
}

static int gx3211_vout_power_off(struct gxav_videoout *vout, int selection)
{
	gx3211_vout_svpu_power_on(vout, selection, 0);

	if (selection & GXAV_VOUT_HDMI) {
		gxav_hdmi_enable(0);
	}

	return 0;
}

static int gx3211_vout_power_on(struct gxav_videoout *vout, int selection)
{
	gx3211_vout_svpu_power_on(vout, selection, 1);

	if (selection & GXAV_VOUT_HDMI)
		gxav_hdmi_enable(1);

	return 0;
}

static void svpu_stop(struct gxav_videoout* vout)
{
	gx3211_vout_svpu_power_on(vout, vout->select, 0);
	gxav_svpu_stop();
}

static void svpu_config(struct gxav_videoout* vout, int src_mode, int dst_mode)
{
	if (vout_use_src(vout, ID_SVPU) == 1) {
		struct vout_dvemode vpu_mode, svpu_mode;

		if (src_mode == -1) {
			vpu_mode.id = ID_VPU;
			gx3211_vout_get_dvemode(vout, &vpu_mode);
		} else {
			vpu_mode.mode = src_mode;
		}

		if (dst_mode == -1) {
			svpu_mode.id = ID_SVPU;
			gx3211_vout_get_dvemode(vout, &svpu_mode);
		} else {
			svpu_mode.mode = dst_mode;
		}

		if(IS_VPU_MODE_VALID(vpu_mode.mode) &&
				IS_SVPU_MODE_VALID(svpu_mode.mode) &&
				svpu_fb.buf_addrs[0] != 0 &&
				svpu_fb.buf_addrs[1] != 0 &&
				svpu_fb.buf_addrs[2] != 0) {
			if (gxav_svpu_config(vpu_mode.mode, svpu_mode.mode, &svpu_fb) == 0) {
				//patch for svpu hw bug
				unsigned int val;
				gx_mdelay(2);
				val = REG_GET_VAL(&gx3211_vout1_reg->tv_ctrl);
				REG_SET_VAL(&gx3211_vout1_reg->tv_ctrl, val | 0xf);
				REG_SET_VAL(&gx3211_vout1_reg->tv_ctrl, val);
				vout->svpu_configed = 1;
			}
		}
	}
}

static void svpu_run(struct gxav_videoout* vout)
{
	if (vout->svpu_configed)  {
		vout_switch(VOUT_OFF);
		if (vout->svpu_delayms)
			gx_mdelay(vout->svpu_delayms);
		gxav_svpu_run();
		vout_switch(VOUT_ON);
		gx3211_vout_svpu_power_on(vout, vout->select, 1);
	}
}


static int gx3211_vout_config(struct gxav_videoout *vout, GxVideoOutProperty_OutputConfig *config)
{
	vout->svpu_delayms = config->svpu_delayms;
	vout->hdmi_delayms = config->hdmi_delayms;

	if (config->svpu_enable == 0) {
		svpu_stop(vout);
		svpu_fb.buf_addrs[0] = (int)NULL;
		svpu_fb.buf_addrs[1] = (int)NULL;
		svpu_fb.buf_addrs[2] = (int)NULL;
	} else {
		if (!(config->svpu_buf[0] && config->svpu_buf[1] && config->svpu_buf[2])){
			VIDEOOUT_PRINTF("param is invalid!\n");
			return -1;
		}

		svpu_fb.buf_addrs[0] = (int)config->svpu_buf[0];
		svpu_fb.buf_addrs[1] = (int)config->svpu_buf[1];
		svpu_fb.buf_addrs[2] = (int)config->svpu_buf[2];
		REG_SET_FIELD(&(gx3211_vout1_reg->line_pixel_num), 0x7<<29, 0x7, 29);
	}

	if (config->vout1_auto.enable)
		vout->vout1_auto = config->vout1_auto;

	if (config->hdcp_enable) {
		vout->hdcp_enable = config->hdcp_enable;
		if (vout->hdcp_enable)
			gxav_hdmi_hdcp_enable_auth();
	}

	return 0;
}

static void set_blank_and_black(int blank, int black)
{
	int otp_ret = 0;
	int otp_key = 0, tmp;

	if (CHIP_IS_GX3211) {
		otp_ret = gxav_secure_otp_read(0x312);
		otp_key = otp_ret!=-1 ? (otp_ret>>4)&0x1 : 0;
		if (otp_key == 0) {
			//exchange blank and black
			tmp   = blank;
			blank = black;
			black = tmp;
		}
	}

	DVE_SET_RANGE_BLANK_AMP(gx3211_vout1_reg, blank);
	DVE_SET_RANGE_BLACK_AMP(gx3211_vout1_reg, black);
}

static int scart_config(struct vout_interface *viface, GxVideoOutProperty_ScartConfig *config)
{
	struct scale_param param = {0};
	GxVideoOutProperty_Mode mode = viface->mode;

	get_scale_param(mode, &param);
	if (config->enable && config->mode == SCART_RGB) {
		gxav_vpu_DACEnable(VPU_DAC_0|VPU_DAC_1|VPU_DAC_2|VPU_DAC_3, 1);
		set_blank_and_black(param.blank, param.black);
		DVE_SET_ADJUST_Y_SHIFT (gx3211_vout1_reg, param.shift);
		gxav_vpu_set_triming_offset(VPU_DAC_0|VPU_DAC_1|VPU_DAC_2, -1);
	}

	if (config->enable && config->mode == SCART_CVBS) {
		gxav_vpu_DACEnable(VPU_DAC_0|VPU_DAC_1|VPU_DAC_2, 0);
		if (IS_P_MODE(viface->mode)) {
			set_blank_and_black(param.blank, param.black);
			DVE_SET_ADJUST_Y_SHIFT (gx3211_vout1_reg, param.shift);
		} else {
			set_blank_and_black(param.blank, param.black);
			DVE_SET_ADJUST_Y_SHIFT (gx3211_vout1_reg, param.shift);
		}
	}

	return 0;
}

static void gx3211_after_set_mode(struct vout_interface *viface)
{
	int shift = 0, offset = 3;
	unsigned int b, c, s;

	scart_config(viface, &gx3211_voutinfo.scart_config);

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);
	viface->ops->set_brightness(viface, b);
	viface->ops->set_saturation(viface, c);
	viface->ops->set_saturation(viface, s);

	if (CHIP_IS_SIRIUS) {
		if (IS_P_MODE(viface->mode))
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0xd0338+offset);
		else
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0xb832c+offset);
	}

	if (CHIP_IS_TAURUS) {
		if (IS_P_MODE(viface->mode))
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0xa0320+offset);
		else
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0x80310+offset);
	}

	if (CHIP_IS_GX3211) {
		if (IS_P_MODE(viface->mode)) {
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0x000b832c+offset);
			REG_SET_VAL(&gx3211_vout1_reg->tv_hsync, 0x004c6a3e);
		} else {
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0x0009e31f+offset);
			REG_SET_VAL(&gx3211_vout1_reg->tv_hsync, 0x004c6d3e);
		}
	}

	if (CHIP_IS_GEMINI) {
		if (gx3211_voutinfo.scart_config.enable == 1 && gx3211_voutinfo.scart_config.mode == SCART_RGB) {
			if (IS_P_MODE(viface->mode))
				shift = 82;
			else
				shift = 76;
			REG_SET_VAL(&gx3211_vout1_reg->tv_active, ((shift<<13)|(shift+722))+offset);
		} else {
			if (IS_P_MODE(viface->mode))
				REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0xaa325+offset);
			else
				REG_SET_VAL(&gx3211_vout1_reg->tv_active, 0x8a315+offset);
		}

		/* 调整RGB频率响应参数 */
		REG_SET_VAL(&gx3211_vout1_reg->tv_fir_y0_2, 0x20402e01);
		REG_SET_VAL(&gx3211_vout1_reg->tv_fir_y3_5, 0x21b05830);
		REG_SET_VAL(&gx3211_vout1_reg->tv_fir_y6_8, 0x25392542);
		REG_SET_VAL(&gx3211_vout1_reg->tv_fir_y9  , 0x2524897a);
		REG_SET_VAL(&gx3211_vout1_reg->tv_fir_c3_5, 0x2338706b);
		/* 调整RGB非线性*/
		REG_SET_VAL(&gx3211_vout1_reg->dgdp0_enable   , 0x1       );
		REG_SET_VAL(&gx3211_vout1_reg->dgdp0_ctrl_reg1, 0x20004000);
		REG_SET_VAL(&gx3211_vout1_reg->dgdp0_ctrl_reg2, 0x60828000);
		REG_SET_VAL(&gx3211_vout1_reg->dgdp0_ctrl_reg3, 0xa000b000);
		REG_SET_VAL(&gx3211_vout1_reg->dgdp0_ctrl_reg4, 0xc000d000);
	}

	if (viface->mode == GXAV_VOUT_PAL_M) { // 这个制式比较特殊
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_range), (0xff << 24), 0x6c, 24);
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_range), (0xff << 16), 0x94, 16);
	}
}

static int gx3211_vout_patch_interface_p_active_off(GxVideoOutProperty_Mode mode)
{
	int ret = 0;

#define TV_ACTIVE_OFFSET (6)

	unsigned int tv_active_off = 0;

	tv_active_off = REG_GET_FIELD(&(gx3211_vout0_reg->tv_active),
			mTV_ACTIVE_OFF,
			bTV_ACTIVE_OFF);
	switch(mode)
	{
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		DVE_SET_ACTIVE_OFF(gx3211_vout0_reg, tv_active_off + TV_ACTIVE_OFFSET);
		break;
	default:
		break;
	}

	return ret;
}

static int is_same_svpu_buf(void)
{
	int same = 0;
	SvpuSurfaceInfo buf_info;

	buf_info.buf_addrs[0] = 0;
	gxav_svpu_get_buf(&buf_info);
	if (buf_info.buf_addrs[0] != 0 && svpu_fb.buf_addrs[0] != 0)
		same = (buf_info.buf_addrs[0] == svpu_fb.buf_addrs[0]);

	return same;
}

static int gx3211_vout_set_resolution_auto(struct gxav_videoout *vout, GxVideoOutProperty_Resolution *resolution)
{
	int id;
	int hdmi_muted = 0;
	struct vout_interface* vpu_viface = NULL;
	struct vout_interface* svpu_viface = NULL;
	int svpu_src_mode, svpu_dst_mode;

	GX3211_CHECK_IFACE(resolution->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, resolution->iface);
	vpu_viface = gx3211_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(vpu_viface);
	if ((vout->select & GXAV_VOUT_RCA) != 0)
		svpu_viface = gx3211_get_vout_interface(GXAV_VOUT_RCA);
	if ((vout->select & GXAV_VOUT_SCART) != 0)
		svpu_viface = gx3211_get_vout_interface(GXAV_VOUT_SCART);

	id = vout_get_src_by_interface(vout, resolution->iface);
	if (id != ID_VPU)
		return 0;

	svpu_src_mode = resolution->mode;
	svpu_dst_mode = IS_P_MODE(svpu_src_mode) ? vout->vout1_auto.pal : vout->vout1_auto.ntsc;

	VIDEOOUT_PRINTF("cur-mode = %d, dst-mode = %d\n", vpu_viface->mode, resolution->mode);

	if (vpu_viface->mode == resolution->mode) {
		id = vout_get_src_by_interface(vout, vpu_viface->iface);
		if (gx3211_voutinfo.src[id].loader_inited) {
			if (is_same_svpu_buf() == 0) {
				gx3211_voutinfo.src[id].loader_inited = 0;
				hdmi_muted = 1;
				gxav_hdmi_av_mute(1);
				svpu_stop(vout);
				svpu_config(vout, svpu_src_mode, svpu_dst_mode);
				svpu_run(vout);
				gxav_hdmi_av_mute(0);
			} else {
				gx3211_voutinfo.src[id].loader_inited = 0;
				vout->svpu_configed = 1;
				svpu_run(vout);
			}
		}
		if (vout->hdcp_enable) {
			gxav_hdmi_hdcp_enable_auth();
			gxav_hdmi_hdcp_start_auth(vpu_viface->mode);
		}
		gxav_vpu_SetVoutIsready(1);
	} else {
		hdmi_muted = 1;
		gxav_hdmi_av_mute(1);
		gxav_hdmi_hdcp_disable_auth();
		gxav_hdmi_powerdown(1);
		gxav_vpu_SetVoutIsready(0);
		svpu_stop(vout);

		vpu_viface->ops->set_mode(vpu_viface, resolution->mode);
		if (svpu_viface != NULL) {
			svpu_viface->ops->set_mode(svpu_viface, svpu_dst_mode);
			svpu_config(vout, svpu_src_mode, svpu_dst_mode);
		}
		gx3211_after_set_mode(vpu_viface);
		gx3211_after_set_mode(svpu_viface);
		gx3211_fullrange_enable(1);
		svpu_run(vout);
		gx_mdelay(60);
		gxav_vpu_SetVoutIsready(1);
		id = vout_get_src_by_interface(vout, vpu_viface->iface);
		gx3211_voutinfo.src[id].loader_inited = 0;
	}

	return 0;
}

static int gx3211_vout_set_resolution(struct gxav_videoout *vout, GxVideoOutProperty_Resolution *resolution)
{
	int id;
	int hdmi_muted = 0;
	int vpu_reseted = 0;
	unsigned int last_mode = 0;
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(resolution->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, resolution->iface);
	viface = gx3211_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("auto-enable = %d, interface = %d, mode = %d\n", vout->vout1_auto.enable, resolution->iface, resolution->mode);
	if (vout->vout1_auto.enable)
		return gx3211_vout_set_resolution_auto(vout, resolution);

	last_mode = viface->mode;
	id = vout_get_src_by_interface(vout, viface->iface);
	if (last_mode == resolution->mode) {
		if (gx3211_voutinfo.src[id].loader_inited) {
			if (is_same_svpu_buf() == 0) {
				gx3211_voutinfo.src[id].loader_inited = 0;
				hdmi_muted = 1;
				gxav_hdmi_av_mute(1);
				svpu_stop(vout);
				svpu_config(vout, -1, -1);
				svpu_run(vout);
				vpu_reseted = 1;
			} else {
				gx3211_voutinfo.src[id].loader_inited = 0;
				vout->svpu_configed = 1;
				svpu_run(vout);
			}
		}
		gxav_vpu_SetVoutIsready(1);
	} else {
		gxav_hdmi_av_mute(1);
		gxav_hdmi_hdcp_disable_auth();
		gxav_hdmi_powerdown(1);
		gxav_vpu_SetVoutIsready(0);
		svpu_stop(vout);
		viface->ops->set_mode(viface, resolution->mode);
		svpu_config(vout, -1, -1);
		svpu_run(vout);
		gx3211_after_set_mode(viface);
		gx3211_fullrange_enable(1);
		gxav_vpu_SetVoutIsready(1);
		hdmi_muted  = 1;
		vpu_reseted = 1;
		id = vout_get_src_by_interface(vout, viface->iface);
		gx3211_voutinfo.src[id].loader_inited = 0;
	}
	if (hdmi_muted)
		gxav_hdmi_av_mute(0);

	if (vpu_reseted && (vout->select & GXAV_VOUT_HDMI) && resolution->iface != GXAV_VOUT_HDMI) {
		struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
		if (vout->hdcp_enable) {
			CHECK_VOUT_INTERFACE(viface);
			gxav_hdmi_hdcp_enable_auth();
			gxav_hdmi_hdcp_start_auth(viface->mode);
		} else {
			gxav_hdmi_hdcp_disable_auth();
			gxav_hdmi_videoout_set(viface->mode);
		}
	}

	return 0;
}

static int gx3211_vout_cec_enable(struct gxav_videoout* vout, GxVideoOutProperty_HdmiCecEnable *param)
{
	int ret = -1;
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("cec enable\n");
	vout->cec_enable = param->enable;
	ret = gxav_hdmi_cec_enable(param->enable);

	return ret;
}

static int gx3211_vout_cec_send_cmd(struct gxav_videoout* vout, GxVideoOutProperty_HdmiCecCmd *param)
{
	int ret = -1;
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("cec send cmd\n");
	if (vout->cec_enable)
		ret = gxav_hdmi_cec_send_cmd(param);

	return ret;
}

static int gx3211_vout_cec_recv_cmd(struct gxav_videoout* vout, GxVideoOutProperty_HdmiCecCmd *param)
{
	int ret = -1;
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("cec recv cmd\n");
	if (vout->cec_enable)
		ret = gxav_hdmi_cec_recv_cmd(param);

	return ret;
}

static int gx3211_vout_hdcp_enable(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpEnable *auth)
{
	int ret = -1;
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("hdcp enable\n");
	vout->hdcp_enable = auth->enable;

	if (auth->enable == 1)
		ret = gxav_hdmi_hdcp_enable_auth();
	if (auth->enable == 0)
		ret = gxav_hdmi_hdcp_disable_auth();

	return ret;
}

static int gx3211_vout_hdcp_config(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpConfig *config)
{
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("hdcp config\n");
	return gxav_hdmi_hdcp_config(config);
}

static int gx3211_vout_hdcp_start(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpEnable *auth)
{
	int ret = -1;
	struct vout_interface* viface = gx3211_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	VIDEOOUT_PRINTF("hdcp start\n");
	if (vout->hdcp_enable == 1 && viface->mode) {
		gxav_hdmi_hdcp_enable_auth();
		ret = gxav_hdmi_hdcp_start_auth(viface->mode);
	}

	return ret;
}

static int gx3211_vout_get_hdmi_status(struct gxav_videoout* vout, GxVideoOutProperty_OutHdmiStatus *status)
{
	status->status = (0 == gxav_hdmi_detect_hotplug()) ? HDMI_PLUG_IN : HDMI_PLUG_OUT;

	return 0;
}

static int gx3211_vout_get_hdmi_version(struct gxav_videoout *vout, GxVideoOutProperty_HdmiVersion *version)
{
	gxav_hdmi_get_version(version);
	return 0;
}

static int gx3211_vout_get_edid_info(struct gxav_videoout* vout, GxVideoOutProperty_EdidInfo *edid_info)
{
	int ret;
	int len = 0;
	struct videoout_hdmi_edid edid = {0};

	if (0 == (ret = gxav_hdmi_read_edid(&edid))) {
		edid_info->tv_cap      = edid.tv_cap;
		edid_info->audio_codes = edid.audio_codes;
		if (edid.data_len) {
			len = (edid.data_len >= sizeof(edid_info->data) ? sizeof(edid_info->data) : edid.data_len);
			edid_info->data_len = edid.data_len;
			gx_memcpy(edid_info->data, edid.data, len);
		}
	}

	return ret;
}

static void dcs_init(int is_cvbs, int is_50hz)
{
	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (0 << 4 | is_cvbs << 3 |  is_50hz << 2 | 0));
	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (1 << 4 | is_cvbs << 3 |  is_50hz << 2 | 0));
}

typedef struct {
	GxVideoOutDCSMode mode;
	struct active_code {
		int dcs_l[2];
		int dcs_h[2];
	} enable, disable;
} DcsParam;

DcsParam dcs_params[] = {
	{
		.mode    = GX_DCS_MODE_A35,
		.enable  = {.dcs_l = {0xB4F54C16, 0xB20F9F12}, .dcs_h = {0x2542B0ED, 0xEEA3648D}},
		.disable = {.dcs_l = {0xFF201481, 0x5800CE31}, .dcs_h = {0x55892559, 0x581BE232}},
	},
	{
		.mode    = GX_DCS_MODE_A36,
		.enable  = {.dcs_l = {0xF1B9590E, 0x49420627}, .dcs_h = {0x7594A1ED, 0x5DA070EA}},
		.disable = {.dcs_l = {0x2BB86E0D, 0x751B7318}, .dcs_h = {0x6264EB14, 0x53010090}},
	},
};

static void dcs_set_reg(struct gxav_videoout* vout, struct active_code *code, int is_cvbs, int is_50hz)
{
	if(NULL == code)
		return;

	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (1<< 4 | is_cvbs << 3 | is_50hz << 2 | 0 ));

	REG_SET_VAL(&(gx3211_vout1_reg->dcs_cpreg_in_L), code->dcs_l[0]);
	REG_SET_VAL(&(gx3211_vout1_reg->dcs_cpreg_in_H), code->dcs_h[0]);

	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (1<< 4 | is_cvbs << 3 | is_50hz << 2 | 1));
	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (1<< 4 | is_cvbs << 3 | is_50hz << 2 | 0));

	REG_SET_VAL(&(gx3211_vout1_reg->dcs_cpreg_in_L), code->dcs_l[1]);
	REG_SET_VAL(&(gx3211_vout1_reg->dcs_cpreg_in_H), code->dcs_h[1]);

	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (1<< 4 | is_cvbs << 3 | is_50hz << 2 | 1));
	REG_SET_VAL(&(gx3211_vout1_reg->dcs_ctrl4), (1<< 4 | is_cvbs << 3 | is_50hz << 2 | 0));
}

static struct active_code* dcs_get_params(GxVideoOutDCSMode mode, int enable)
{
	int i;
	struct active_code *code = NULL;

	if (mode == GX_DCS_MODE_DISABLE)
		code = NULL;
	else {
		for (i = 0; i < sizeof(dcs_params)/sizeof(dcs_params[0]); i++) {
			if (dcs_params[i].mode == mode) {
				code = (enable ? &dcs_params[i].enable : &dcs_params[i].disable);
				break;
			}
		}
	}

	return code;
}

static int gx3211_vout_set_dcs(struct gxav_videoout* vout, GxVideoOutProperty_DCS *dcs)
{
	#define MODE_ENABLE  (1)
	#define MODE_DISABLE (0)
	struct active_code *code = NULL;
	GxVideoOutDCSMode cur_mode = vout->dcs.mode;
	GxVideoOutDCSMode dst_mode = dcs->mode;

	int is_cvbs = (vout->select & (GXAV_VOUT_RCA | GXAV_VOUT_RCA1));
	int is_50hz = IS_P_MODE(gx3211_voutinfo.src[ID_SVPU].mode);

	if (dst_mode == cur_mode) {
		;
	} else {
		if (cur_mode != GX_DCS_MODE_DISABLE) {
			code = dcs_get_params(cur_mode, MODE_DISABLE);
			dcs_set_reg(vout, code, is_cvbs, is_50hz);
		}
		if (dst_mode != GX_DCS_MODE_DISABLE) {
			dcs_init(is_cvbs, is_50hz);
			code = dcs_get_params(dst_mode, MODE_ENABLE);
			dcs_set_reg(vout, code, is_cvbs, is_50hz);
		}
	}
	vout->dcs = *dcs;

	return (0);
}

static int gx3211_vout_set_macrovision(struct gxav_videoout* vout, GxVideoOutProperty_Macrovision *macrovision)
{
	int otp_key = 0;
	unsigned int temp;

	GxMacrovisionParam *param  = NULL;

	otp_key = gxav_secure_get_macrovision_status();
	gx_memcpy(&vout->macrovision, macrovision, sizeof(GxVideoOutProperty_Macrovision));

#if 0 // Rovi 认证时, 如果Loader未配置 Triming 值, 这里需要打开.
	if (CHIP_IS_SIRIUS) {
		// 正式方案这个值是实际的triming值, 在Loader中配置. correct video dac gain
		*(volatile unsigned int *)(0x8a800130) = 0x20202020;
	}
#endif

	REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 0), 97, 0);

	temp = REG_GET_FIELD(&(gx3211_vout1_reg->tv_ctrl), 0x1f, 0);

	if (temp == 4 || temp == 1 || temp == 5) { //NTSC PAL-M NTSC-443
		//set choma sync burst on and choma burst off position
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_hsync), (0xff<< 8), 0x6d, 8);
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_hsync), (0xff<< 16), 0x4c, 16);
	} else {
		//set choma sync burst on and choma burst off position
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_hsync), (0xff<< 8), 0x6a, 8);
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_hsync), (0xff<< 16), 0x4c, 16);
	}

	if (otp_key == 0 || macrovision->mode == GX_MACROVISION_MODE_TYPE0) {
		REG_CLR_BIT(&(gx3211_vout1_reg->aps_reg4), 12);
		REG_SET_FIELD(&(gx3211_vout1_reg->aps_reg0), 0xff, 0, 0);

		REG_SET_FIELD(&(gx3211_vout1_reg->aps_ctrl2), (0xff<< 0),0, 0);

		if (temp == 4 || temp == 1 || temp == 5) { //NTSC PAL-M NTSC-443
			REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 8), 71, 8);
		}
		else { //PAL PAL-N PAL-NC
			REG_SET_FIELD(&(gx3211_vout1_reg->tv_adjust), (0xff<< 8), 63, 8);
		}
	} else {
		if (macrovision->mode == GX_MACROVISION_MODE_CUSTOM)
			param = &macrovision->custom_param;
		else
			param = mcv_get_default_param(macrovision->mode);
		macrovision_set_reg(param);
	}

	return 0;
}

static int gx3211_vout_set_default(struct gxav_videoout *vout, GxVideoOutProperty_OutDefault *outdefault)
{
	unsigned int temp;
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(vout, outdefault->iface);

	if (!reg)
		return -1;

	if (outdefault->enable)
		REG_SET_BIT(&(reg->digital_ctrl), 3);
	else
		REG_CLR_BIT(&(reg->digital_ctrl), 3);
	REG_SET_BIT(&(reg->digital_ctrl), 11);

	/* macrovision 开彩条认证时，需要配置如下时序 */
	temp = REG_GET_FIELD(&(gx3211_vout1_reg->tv_ctrl), 0x1f, 0);
	if (temp == 4 || temp == 1 || temp == 5) { //NTSC PAL-M NTSC-443
		//set line active on and active off position
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_active), (0x7ff<< 13), 80, 13);
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_active), (0x7ff<< 0), 80 + 712, 0);
	} else {
		//set line active on and active off position
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_active), (0x7ff<< 13), 104, 13);
		REG_SET_FIELD(&(gx3211_vout1_reg->tv_active), (0x7ff<< 0), 104 + 720, 0);
	}


	return 0;
}

static EnhanceID get_enhance_id_by_interface(GxVideoOutProperty_Interface interface)
{
	EnhanceID ret = 0;

	if (interface == GXAV_VOUT_YUV || interface == GXAV_VOUT_HDMI)
		ret = HD_ENHANCE;
	else
		ret = SD_ENHANCE;

	return ret;
}

static int gx3211_vout_set_brightness(struct gxav_videoout *vout, GxVideoOutProperty_Brightness *brightness)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(brightness->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, brightness->iface);
	viface = gx3211_get_vout_interface(brightness->iface);
	CHECK_VOUT_INTERFACE(viface);
	viface->brightness = brightness->value;

	if (vout->enhancement_enable == 1 && CHIP_IS_CYGNUS) {
		enhance_set_brightness(get_enhance_id_by_interface(brightness->iface), brightness->value);
	} else {
		viface->ops->set_brightness(viface, brightness->value);
	}

	return 0;
}

static int gx3211_vout_set_saturation(struct gxav_videoout *vout, GxVideoOutProperty_Saturation *saturation)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(saturation->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, saturation->iface);
	viface = gx3211_get_vout_interface(saturation->iface);
	CHECK_VOUT_INTERFACE(viface);
	viface->saturation = saturation->value;

	if (vout->enhancement_enable == 1 && CHIP_IS_CYGNUS) {
		enhance_set_saturation(get_enhance_id_by_interface(saturation->iface), saturation->value);
	} else {
		viface->ops->set_saturation(viface, saturation->value);
	}

	return 0;
}

static int gx3211_vout_set_contrast(struct gxav_videoout *vout, GxVideoOutProperty_Contrast *contrast)
{
	unsigned int temp, value;
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(contrast->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, contrast->iface);
	viface = gx3211_get_vout_interface(contrast->iface);
	CHECK_VOUT_INTERFACE(viface);
	viface->contrast = contrast->value;

	if (vout->enhancement_enable == 1 && CHIP_IS_CYGNUS) {
		enhance_set_contrast(get_enhance_id_by_interface(contrast->iface), contrast->value);
	} else {
		viface->ops->set_contrast(viface, contrast->value);
	}

	return 0;
}


static int gx3211_vout_set_sharpness(struct gxav_videoout *vout, GxVideoOutProperty_Sharpness *sharpness)
{
	unsigned dif_core = 5;//0~255
	unsigned tot_core = 5;//0~255
	unsigned tot_limit = 85;//0~255
	unsigned en = 1;
	unsigned pos_gain = 10;//0~63
	unsigned lvl_gain = 10;//0~63
	unsigned tot_gain = 0;//0~63
	unsigned gain_dly = 3;//0~7
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(sharpness->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, sharpness->iface);
	viface = gx3211_get_vout_interface(sharpness->iface);
	CHECK_VOUT_INTERFACE(viface);
	viface->sharpness = sharpness->value;

	if (vout->enhancement_enable == 1) {
		if (CHIP_IS_CYGNUS) {
			enhance_set_sharpness(get_enhance_id_by_interface(sharpness->iface), sharpness->value);
		} else {
			tot_gain = (sharpness->value/10)*4;
			REG_SET_VAL(&gx3211_vout0_reg->cb_hue_ctl, ((gain_dly<<24)|(tot_gain<<16)|(lvl_gain<<8)|pos_gain));
			REG_SET_VAL(&gx3211_vout0_reg->lvds_ctrl , ((en << 24)|(tot_limit<<16)|(tot_core<<8)|dif_core));
		}
	}

	return 0;
}

static int gx3211_vout_set_hue(struct gxav_videoout *vout, GxVideoOutProperty_Hue *hue)
{
	unsigned temp, value;
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(hue->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, hue->iface);
	viface = gx3211_get_vout_interface(hue->iface);
	CHECK_VOUT_INTERFACE(viface);
	viface->hue = hue->value;

	if (vout->enhancement_enable == 1) {
		if (CHIP_IS_CYGNUS) {
			enhance_set_hue(get_enhance_id_by_interface(hue->iface), hue->value);
		} else {
			temp = REG_GET_VAL(&gx3211_vout0_reg->resv4[4]);
			temp = temp & 0xffffff80;
			value = (hue->value>>1);
			if (hue->value < 50)
			  value = 64 + 25 - value;
			else
			  value = value - 25;
			REG_SET_VAL(&gx3211_vout0_reg->resv4[4], (temp|value));
		}
	}

	return 0;
}

static int gx3211_vout_set_aspratio(struct gxav_videoout *vout, GxVideoOutProperty_AspectRatio *aspect_ratio)
{
	int ret = -1;
	GxVpuProperty_AspectRatio spec;

	switch (aspect_ratio->spec) {
	case ASPECT_RATIO_PAN_SCAN:
		spec.AspectRatio = VP_PAN_SCAN;
		break;
	case ASPECT_RATIO_LETTER_BOX:
		spec.AspectRatio = VP_LETTER_BOX;
		break;
	case ASPECT_RATIO_COMBINED:
		spec.AspectRatio = VP_COMBINED;
		break;
	case ASPECT_RATIO_RAW_SIZE:
		spec.AspectRatio = VP_RAW_SIZE;
		break;
	case ASPECT_RATIO_RAW_RATIO:
		spec.AspectRatio = VP_RAW_RATIO;
		break;
	case ASPECT_RATIO_4X3_PULL:
		spec.AspectRatio = VP_4X3_PULL;
		break;
	case ASPECT_RATIO_4X3_CUT:
		spec.AspectRatio = VP_4X3_CUT;
		break;
	case ASPECT_RATIO_16X9_PULL:
		spec.AspectRatio = VP_16X9_PULL;
		break;
	case ASPECT_RATIO_16X9_CUT:
		spec.AspectRatio = VP_16X9_CUT;
		break;
	case ASPECT_RATIO_NOMAL:
		spec.AspectRatio = VP_UDEF;
		break;
	case ASPECT_RATIO_4X3:
		spec.AspectRatio = VP_4X3;
		break;
	case ASPECT_RATIO_16X9:
		spec.AspectRatio = VP_16X9;
		break;
	case ASPECT_RATIO_AUTO:
		spec.AspectRatio = VP_AUTO;
		break;

	default:
		return -1;
	}

	ret = gxav_vpu_SetAspectRatio(&spec);
	GX3211_DVE_CHECK_RET(ret);

	return 0;
}

static int gx3211_vout_set_tvscreen(struct gxav_videoout *vout, GxVideoOutProperty_TvScreen *TvScreen)
{
	int ret = -1;
	GxVpuProperty_TvScreen Screen;

	if(TvScreen->screen == TV_SCREEN_4X3) {
		Screen.Screen= SCREEN_4X3;
	}
	else if(TvScreen->screen == TV_SCREEN_16X9) {
		Screen.Screen= SCREEN_16X9;
	}
	else {
		return -1;
	}

	ret = gxav_vpu_SetTvScreen(&Screen);
	GX3211_DVE_CHECK_RET(ret);

	return 0;
}

int gx3211_vout_get_resolution(struct gxav_videoout *vout, GxVideoOutProperty_Resolution *resolution)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(resolution->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, resolution->iface);
	viface = gx3211_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(viface);

	resolution->mode = viface->mode;
	if (resolution->mode >= GXAV_VOUT_PAL && resolution->mode <= GXAV_VOUT_1080P_60HZ)
		return 0;

	return -1;
}

int gx3211_vout_get_brightness(struct gxav_videoout *vout, GxVideoOutProperty_Brightness *brightness)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(brightness->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, brightness->iface);
	viface = gx3211_get_vout_interface(brightness->iface);
	CHECK_VOUT_INTERFACE(viface);

	brightness->value = viface->ops->get_brightness(viface);

	return 0;
}

int gx3211_vout_get_contrast(struct gxav_videoout *vout, GxVideoOutProperty_Contrast *contrast)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(contrast->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, contrast->iface);
	viface = gx3211_get_vout_interface(contrast->iface);
	CHECK_VOUT_INTERFACE(viface);

	contrast->value = viface->ops->get_contrast(viface);

	return 0;
}

int gx3211_vout_get_saturation(struct gxav_videoout *vout, GxVideoOutProperty_Saturation *saturation)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(saturation->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, saturation->iface);
	viface = gx3211_get_vout_interface(saturation->iface);
	CHECK_VOUT_INTERFACE(viface);

	saturation->value = viface->ops->get_saturation(viface);

	return 0;
}

int gx3211_vout_get_sharpness(struct gxav_videoout *vout, GxVideoOutProperty_Sharpness *sharpness)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(sharpness->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, sharpness->iface);
	viface = gx3211_get_vout_interface(sharpness->iface);
	CHECK_VOUT_INTERFACE(viface);

	sharpness->value = viface->sharpness;

	return 0;
}

int gx3211_vout_get_hue(struct gxav_videoout *vout, GxVideoOutProperty_Hue *hue)
{
	struct vout_interface* viface = NULL;

	GX3211_CHECK_IFACE(hue->iface);
	GX3211_CHECK_VOUT_IFACE(vout->select, hue->iface);
	viface = gx3211_get_vout_interface(hue->iface);
	CHECK_VOUT_INTERFACE(viface);

	hue->value = viface->hue;

	return 0;
}

static int gx3211_vout_play_cc(struct gxav_videoout *vout, struct vout_ccinfo *ccinfo)
{
	char byte0, byte1;
	short cc_type, cc_data;
	unsigned short cc_line_top = 0, cc_line_bottom = 0;
	struct vout_dvemode svpu_mode;
	volatile struct gx3211_videoout_reg *reg = gx3211_vout1_reg;

	if (vout_use_src(vout, ID_SVPU) == 0)
		return -1;

	svpu_mode.id = ID_SVPU;
	gx3211_vout_get_dvemode(vout, &svpu_mode);
	cc_type = ccinfo->cc_type;
	cc_data = ccinfo->cc_data;

	switch(svpu_mode.mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
		if(cc_type==0x00 || cc_type==0x10) {//top
			cc_line_top = 21;
		}
		else if(cc_type==0x01 || cc_type==0x11) {
			cc_line_bottom = 284;
		}
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
		if(cc_type==0x00 || cc_type==0x10) {//top
			cc_line_top = 17;
		}
		else if(cc_type==0x01 || cc_type==0x11) {
			cc_line_bottom = 280;
		}
		break;
	default:
		return -1;
	}

	byte0 = (cc_data>>0)&0xff;
	byte1 = (cc_data>>8)&0xff;
	byte0 = (((byte0>>0)&0x1)<<7)|
		(((byte0>>1)&0x1)<<6)|
		(((byte0>>2)&0x1)<<5)|
		(((byte0>>3)&0x1)<<4)|
		(((byte0>>4)&0x1)<<3)|
		(((byte0>>5)&0x1)<<2)|
		(((byte0>>6)&0x1)<<1)|
		(((byte0>>7)&0x1)<<0);
	byte1 = (((byte1>>0)&0x1)<<7)|
		(((byte1>>1)&0x1)<<6)|
		(((byte1>>2)&0x1)<<5)|
		(((byte1>>3)&0x1)<<4)|
		(((byte1>>4)&0x1)<<3)|
		(((byte1>>5)&0x1)<<2)|
		(((byte1>>6)&0x1)<<1)|
		(((byte1>>7)&0x1)<<0);
	cc_data = (byte0<<8)|(byte1<<0);

	DVE_SET_CC_PHASE(reg, 0x19);
	if(cc_line_top) {
		DVE_SET_CC_LINE_TOP(reg, cc_line_top);
		DVE_SET_CC_DATA_TOP(reg, cc_data);
		DVE_SET_CC_TOP_EN(reg);
	}
	if(cc_line_bottom) {
		DVE_SET_CC_LINE_BOTTOM(reg, cc_line_bottom);
		DVE_SET_CC_DATA_BOTTOM(reg, cc_data);
		DVE_SET_CC_BOTTOM_EN(reg);
	}

	return 0;
}


static int gx3211_vout_set_interface(struct gxav_videoout *vout, GxVideoOutProperty_OutputSelect *output)
{
#define VIFACE_ASSIGN_VOUT(iface)\
	do {\
		if (output->selection & iface) {\
			viface = gx3211_get_vout_interface(iface);\
			viface->vout = vout;\
		}\
	} while (0)
	int id;
	struct vout_interface* viface = NULL;
	struct reg_iface* riface = gx3211_vout_get_riface(output->selection);

	if (!riface)
		return -1;

	VIFACE_ASSIGN_VOUT(GXAV_VOUT_RCA);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_RCA1);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_YUV);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_SCART);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_SVIDEO);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_HDMI);

	if (vout->select == output->selection)
		return 0;

	VIDEOOUT_PRINTF(" start, select=%d\n", output->selection);
	vout->select = 0;
	gxav_vpu_DACEnable(VPU_DAC_ALL, 0);

	vout->select = output->selection;
	gx3211_voutinfo.select = output->selection;
	gx3211_vout_power_on(vout, output->selection);

	if (CHIP_IS_GX6605S || CHIP_IS_TAURUS) {
		gxav_vpu_DACSource(VPU_DAC_0, VPU_DAC_SRC_SVPU);
	}
	else {
		gxav_vpu_DACSource(VPU_DAC_0, riface->dac_src0);
		gxav_vpu_DACSource(VPU_DAC_1, riface->dac_src1);
		gxav_vpu_DACSource(VPU_DAC_2, riface->dac_src2);
		gxav_vpu_DACSource(VPU_DAC_3, riface->dac_src3);
	}

	if (output->selection & GXAV_VOUT_HDMI) {
		if (gx3211_voutinfo.interfaces[IFACE_ID_HDMI].loader_inited == 0)
			gxav_hdmi_open(vout->hdmi_delayms);
		else {
			gxav_hdmi_ready(1);
			gx3211_voutinfo.interfaces[IFACE_ID_HDMI].loader_inited = 0;
		}
		gxav_hdmi_set_cgmsa_permission(gx3211_voutinfo.digital_copy_permission);
	}

	if (output->selection & GXAV_VOUT_RCA) {
		id = vout_get_src_by_interface(vout, GXAV_VOUT_RCA);
		if (id == ID_SVPU && gx3211_voutinfo.src[ID_SVPU].loader_inited)
			gx3211_voutinfo.interfaces[IFACE_ID_RCA].mode = gx3211_voutinfo.src[ID_SVPU].mode;
	}
	VIDEOOUT_PRINTF(" end\n");

	return 0;
}

static int gx3211_vpu_iface_set_mode(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	unsigned int last_mode = 0;

	last_mode = viface->mode;

	gx3211_vout_set_dve(viface, mode);

	if (vout_get_src_by_interface(viface->vout, viface->iface) == ID_VPU) {
		gx3211_vout_patch_interface_p_active_off(mode);
	}

	return 0;
}

static int gx3211_vpu_iface_set_brightness(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale0 = 160;
	unsigned int scale3 = 63;
	unsigned int blank = 63;
	unsigned int black = 63;
	unsigned int brightness_value = 0;
	unsigned int luma_shift = 0;
	unsigned int b, c;
	struct scale_param param = {0};
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);
	GxVideoOutProperty_Mode mode = viface->mode;

	VIDEOOUT_PRINTF("interface = %d, value = %d, cur-mode = %d\n", viface->iface, value, mode);
	if (vout_get_src_by_interface(viface->vout, viface->iface) == ID_SVPU) {
		get_scale_param(mode, &param);
		scale0 = param.scale0;
		scale3 = param.shift;
		blank  = param.blank;
		black  = param.black;
	}

	viface->brightness = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);

	brightness_value = (scale0 * ((c << 1) * (b << 1))) / (100 * 100);
	brightness_value = brightness_value >= 370 ? 370 : brightness_value;
	brightness_value = brightness_value <= 16  ? 16  : brightness_value;

	VIDEOOUT_PRINTF("brightness_val= %d, scale0 = %d, scale3 = %d\n", brightness_value, scale0, scale3);

	if (brightness_value > scale0)
		luma_shift = scale3 - 6;
	else
		luma_shift = scale3;
	gx3211_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_Y_SCALE0(reg, brightness_value);
	DVE_SET_ADJUST_Y_SHIFT(reg, luma_shift);
	set_blank_and_black(blank, black);

	return 0;
}

static int gx3211_vpu_iface_set_saturation(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale1 = 157, scale2 = 157;
	unsigned int saturation_value1, saturation_value2;
	unsigned int c, s;
	unsigned int blank = 63;
	unsigned int black = 63;
	struct scale_param param = {0};
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);
	GxVideoOutProperty_Mode mode = viface->mode;

	if (vout_get_src_by_interface(viface->vout, viface->iface) == ID_SVPU) {
		get_scale_param(mode, &param);
		scale1 = param.scale1;
		scale2 = param.scale2;
		blank  = param.blank;
		black  = param.black;
	}

	viface->saturation = value;

	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	saturation_value1 = (scale1 * ((c << 1)*(s << 1))) / (100 * 100);
	saturation_value1 = saturation_value1  >= 345 ? 345 : saturation_value1;
	saturation_value1 = saturation_value1 <= 16  ? 16  : saturation_value1;
	saturation_value2 = (scale2 * ((c << 1)*(s << 1))) / (100 * 100);
	saturation_value2 = saturation_value2  >= 345 ? 345 : saturation_value2;
	saturation_value2 = saturation_value2 <= 16  ? 16  : saturation_value2;

	gx3211_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_U_SCALE1(reg, saturation_value1);
	DVE_SET_YUV_TRAN_V_SCALE2(reg, saturation_value2);
	set_blank_and_black(blank, black);

	return 0;
}

static int gx3211_vpu_iface_set_contrast(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale0 = 160, scale1 = 157, scale2 = 157;
	unsigned int scale3 = 63;
	unsigned int blank = 63;
	unsigned int black = 63;
	unsigned int luma_shift = 0;
	unsigned int brightness_value = 0;
	unsigned int saturation_value1, saturation_value2;
	unsigned int b, c, s;
	struct scale_param param = {0};
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);
	GxVideoOutProperty_Mode mode = viface->mode;

	VIDEOOUT_PRINTF("interface = %d, value = %d\n", viface->iface, value);
	viface->contrast = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	if (vout_get_src_by_interface(viface->vout, viface->iface) == ID_SVPU) {
		get_scale_param(mode, &param);
		scale0 = param.scale0;
		scale1 = param.scale1;
		scale2 = param.scale2;
		scale3 = param.shift;
		blank  = param.blank;
		black  = param.black;
	}

	brightness_value = (scale0 * ((c << 1) * (b << 1))) / (100 * 100);
	brightness_value = brightness_value >= 370 ? 370 : brightness_value;
	brightness_value = brightness_value <= 16  ? 16  : brightness_value;

	saturation_value1 = (scale1 * ((c << 1)*(s << 1))) / (100 * 100);
	saturation_value1 = saturation_value1  >= 345 ? 345 : saturation_value1;
	saturation_value1 = saturation_value1 <= 16  ? 16  : saturation_value1;
	saturation_value2 = (scale2 * ((c << 1)*(s << 1))) / (100 * 100);
	saturation_value2 = saturation_value2  >= 345 ? 345 : saturation_value2;
	saturation_value2 = saturation_value2 <= 16  ? 16  : saturation_value2;

	if(brightness_value > scale0){
		luma_shift = scale3 - 6;
	}
	else {
		luma_shift = scale3;
	}
	gx3211_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_Y_SCALE0(reg, brightness_value);
	DVE_SET_YUV_TRAN_U_SCALE1(reg, saturation_value1);
	DVE_SET_YUV_TRAN_V_SCALE2(reg, saturation_value2);
	DVE_SET_ADJUST_Y_SHIFT(reg,luma_shift);
	set_blank_and_black(blank, black);

	return 0;
}

static int gx3211_hdmi_iface_set_mode(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	unsigned int last_mode = 0;
	struct gxav_videoout* vout = viface->vout;

	last_mode = viface->mode;

	if (vout_use_src(vout, ID_SVPU) == 1)
		svpu_stop(vout);

	/* 设置hdmi黑屏, 否则在配置vpu mode时可能导致hdmi输出花屏 */
	gxav_hdmi_black_enable(1);

	gx3211_vout_insert_scan_chance(last_mode);
	gx3211_vout_set_dve(viface, mode);
	gx3211_vout_patch_interface_p_active_off(mode);

	if (vout->hdcp_enable) {
		gxav_hdmi_hdcp_enable_auth();
		gxav_hdmi_hdcp_start_auth(mode);
	} else {
		gxav_hdmi_hdcp_disable_auth();
		gxav_hdmi_videoout_set(mode);
	}

	gxav_hdmi_black_enable(0);

	return 0;
}

static int gx3211_hdmi_iface_set_brightness(struct vout_interface *viface, unsigned int value)
{
	unsigned int b, c, s;
	unsigned int hdmi_value = 0;
	GxVideoOutProperty_Mode mode = viface->mode;
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);

	viface->brightness = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	gx3211_vout_insert_scan_chance(mode);

	if (gxav_hdmi_set_brightness(b, c, s) == 0) {
		b = c = s = 50;
	}

	hdmi_value = (256 *((c << 1) * (b << 1))) / (100 * 100);
	hdmi_value = hdmi_value  >= 511 ? 511 : hdmi_value;
	DVE_SET_HDMI_TRAN_Y_SCALE(reg, hdmi_value);

	return 0;
}

static int gx3211_hdmi_iface_set_saturation(struct vout_interface *viface, unsigned int value)
{
	unsigned int b, c, s;
	unsigned int hdmi_value = 0;
	GxVideoOutProperty_Mode mode = viface->mode;
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);

	viface->saturation = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	gx3211_vout_insert_scan_chance(mode);

	if (gxav_hdmi_set_saturation(b, c, s) == 0) {
		b = c = s = 50;
	}

	hdmi_value = (256*((c << 1)*(s << 1))) / (100 * 100);
	hdmi_value = hdmi_value  >= 290 ? 290 : hdmi_value;
	DVE_SET_HDMI_TRAN_CB_SCALE(reg, hdmi_value);
	DVE_SET_HDMI_TRAN_CR_SCALE(reg, hdmi_value);

	return 0;
}

static int gx3211_hdmi_iface_set_contrast(struct vout_interface *viface, unsigned int value)
{
	unsigned int b, c, s;
	unsigned int hdmi_value = 0;
	GxVideoOutProperty_Mode mode = viface->mode;
	volatile struct gx3211_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);

	viface->contrast = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	gx3211_vout_insert_scan_chance(mode);

	if (gxav_hdmi_set_contrast(b, c, s) == 0) {
		b = c = s = 50;
	}

	hdmi_value = (256 *((c << 1) * (b << 1))) / (100 * 100);
	hdmi_value = hdmi_value  >= 511 ? 511 : hdmi_value;
	DVE_SET_HDMI_TRAN_Y_SCALE(reg, hdmi_value);

	hdmi_value = (256*((c << 1) * (s << 1))) / (100 * 100);
	hdmi_value = hdmi_value  >= 290 ? 290 : hdmi_value;
	DVE_SET_HDMI_TRAN_CB_SCALE(reg, hdmi_value);
	DVE_SET_HDMI_TRAN_CR_SCALE(reg, hdmi_value);

	return 0;
}

static GxVideoOutProperty_Mode gx3211_iface_get_mode(struct vout_interface *viface)
{
	return viface->mode;
}

static unsigned int gx3211_iface_get_brightness(struct vout_interface *viface)
{
	return viface->brightness;
}

static unsigned int gx3211_iface_get_saturation(struct vout_interface *viface)
{
	return viface->saturation;
}

static unsigned int gx3211_iface_get_contrast(struct vout_interface *viface)
{
	return viface->contrast;
}

static struct interface_ops gx3211_vpu_interface_ops = {
	.set_mode       = gx3211_vpu_iface_set_mode,
	.set_brightness = gx3211_vpu_iface_set_brightness,
	.set_saturation = gx3211_vpu_iface_set_saturation,
	.set_contrast   = gx3211_vpu_iface_set_contrast,
	.get_mode       = gx3211_iface_get_mode,
	.get_brightness = gx3211_iface_get_brightness,
	.get_saturation = gx3211_iface_get_saturation,
	.get_contrast   = gx3211_iface_get_contrast,
};

static struct interface_ops gx3211_hdmi_interface_ops = {
	.set_mode       = gx3211_hdmi_iface_set_mode,
	.set_brightness = gx3211_hdmi_iface_set_brightness,
	.set_saturation = gx3211_hdmi_iface_set_saturation,
	.set_contrast   = gx3211_hdmi_iface_set_contrast,
	.get_mode       = gx3211_iface_get_mode,
	.get_brightness = gx3211_iface_get_brightness,
	.get_saturation = gx3211_iface_get_saturation,
	.get_contrast   = gx3211_iface_get_contrast,
};

static void gx3211_vout_init_interface(void)
{
	gx3211_voutinfo.interfaces[0].iface = GXAV_VOUT_RCA;
	gx3211_voutinfo.interfaces[1].iface = GXAV_VOUT_RCA1;
	gx3211_voutinfo.interfaces[2].iface = GXAV_VOUT_YUV;
	gx3211_voutinfo.interfaces[3].iface = GXAV_VOUT_SCART;
	gx3211_voutinfo.interfaces[4].iface = GXAV_VOUT_SVIDEO;
	gx3211_voutinfo.interfaces[5].iface = GXAV_VOUT_HDMI;

	gx3211_voutinfo.interfaces[0].ops =
	gx3211_voutinfo.interfaces[1].ops =
	gx3211_voutinfo.interfaces[2].ops =
	gx3211_voutinfo.interfaces[3].ops =
	gx3211_voutinfo.interfaces[4].ops = &gx3211_vpu_interface_ops;
	gx3211_voutinfo.interfaces[5].ops = &gx3211_hdmi_interface_ops;
}

static int get_poweron_dac_num(void)
{
	int num = 0;

	if (gxav_vpu_DACPower(VPU_DAC_ALL) == 0)
		num = 0;
	else {
		num += gxav_vpu_DACPower(VPU_DAC_0);
		num += gxav_vpu_DACPower(VPU_DAC_1);
		num += gxav_vpu_DACPower(VPU_DAC_2);
		num += gxav_vpu_DACPower(VPU_DAC_3);
	}

	return num;
}

static int gx3211_vout_probe_hdmi_mode(void)
{
	unsigned int val = REG_GET_VAL(&gx3211_vout0_reg->tv_dac);
	unsigned int mode = 0;

	if ((val & 0xff) == 0)
		mode = gxav_hdmi_videoout_get();

	gx3211_voutinfo.interfaces[IFACE_ID_HDMI].mode = mode;
	gx3211_voutinfo.interfaces[IFACE_ID_HDMI].loader_inited = (mode != 0);
	VIDEOOUT_PRINTF("flag-reg = 0x%x, hdmi-mode = %d\n", val, mode);

	return mode;
}

static int gx3211_vout_probe_yuv_mode(void)
{
	int mode = 0;
	int poweron_dac_num = 0;
	int val = REG_GET_VAL(&gx3211_vout0_reg->tv_ctrl);
	int reg = (val >> bTV_CTRL_TV_FORMAT) & 0xf;
	int fre = (val >> bTV_CTRL_FRAME_FRE) & 0x1;

	poweron_dac_num = get_poweron_dac_num();
	// yuv-port need 3 dac
	if (poweron_dac_num < 3) {
		mode = 0;
	} else {
		switch(reg) {
		case 4:
			mode = GXAV_VOUT_480I;
			break;
		case 0:
			mode = GXAV_VOUT_576I;
			break;
		case 8:
			mode = GXAV_VOUT_480P;
			break;
		case 9:
			mode = GXAV_VOUT_576P;
			break;
		case 13:
			mode = fre ? GXAV_VOUT_720P_50HZ : GXAV_VOUT_720P_60HZ;
			break;
		case 6:
			mode = fre ? GXAV_VOUT_1080I_50HZ : GXAV_VOUT_1080I_60HZ;
			break;
		case 14:
			mode = fre ? GXAV_VOUT_1080P_50HZ : GXAV_VOUT_1080P_60HZ;
			break;
		default:
			mode = 0;
			break;
		}
	}

	return mode;
}

static void gx3211_vout_probe_vpu_mode(void)
{
	unsigned char hdmi_mode = 0, yuv_mode = 0, mode = 0;

	yuv_mode  = gx3211_vout_probe_yuv_mode();
	hdmi_mode = gx3211_vout_probe_hdmi_mode();

	if (hdmi_mode == 0 || yuv_mode == 0)
		mode = (hdmi_mode ? hdmi_mode : yuv_mode);
	else if (hdmi_mode == yuv_mode)
		mode = yuv_mode;
	else
		mode = 0;

	gx3211_voutinfo.src[ID_VPU].mode = mode;
	gx3211_voutinfo.src[ID_VPU].loader_inited = (mode != 0);
}

static void gx3211_vout_probe_svpu_mode(void)
{
	unsigned int  val = REG_GET_VAL(&gx3211_vout1_reg->tv_ctrl);
	unsigned char mode = 0, reg_mode = (val >> bTV_CTRL_TV_FORMAT) & 0xf;
	unsigned int  dacpoweron = gxav_vpu_DACPower(VPU_DAC_ALL);

	VIDEOOUT_PRINTF("val = 0x%x, reg-mode = %d, dacpoweron = %d\n", val, reg_mode, dacpoweron);

	if (dacpoweron == 0) {
		mode = 0;
	} else {
		switch(reg_mode) {
		case 0:
			mode = GXAV_VOUT_PAL;
			break;
		case 2:
			mode = GXAV_VOUT_PAL_N;
			break;
		case 3:
			mode = GXAV_VOUT_PAL_NC;
			break;
		case 4:
			mode = GXAV_VOUT_NTSC_M;
			break;
		case 5:
			mode = GXAV_VOUT_NTSC_443;
			break;
		case 1:
			mode = GXAV_VOUT_PAL_M;
			break;
		default:
			mode = 0;
			break;
		}
	}

	gx3211_voutinfo.src[ID_SVPU].mode = mode;
	gx3211_voutinfo.src[ID_SVPU].loader_inited = (mode != 0);
	VIDEOOUT_PRINTF("svpu-mode = %d\n", mode);
}

static void gx3211_vout_init_resolution(void)
{
	int i;

	memset(&svpu_fb, 0, sizeof(svpu_fb));

	gx3211_vout_probe_vpu_mode();
	gx3211_vout_probe_svpu_mode();

	if (gx3211_voutinfo.src[ID_VPU].mode && gx3211_voutinfo.interfaces[IFACE_ID_HDMI].mode) {
		if (gx3211_voutinfo.src[ID_VPU].mode != gx3211_voutinfo.interfaces[IFACE_ID_HDMI].mode) {
			gx_printf("hdmi pre init error, will be reinit !\n");
			gx3211_voutinfo.interfaces[IFACE_ID_HDMI].mode = 0;;
		}
	}

	for (i = 0; i < GX_ARRAY_SIZE(gx3211_voutinfo.interfaces); i++) {
		gx3211_voutinfo.interfaces[i].brightness = 50;
		gx3211_voutinfo.interfaces[i].contrast   = 50;
		gx3211_voutinfo.interfaces[i].saturation = 50;
	}
}

static int gx3211_vout_cgms_enable(GxVideoOutProperty_CGMSEnable *property)
{
	volatile struct gx3211_videoout_reg *reg = gx3211_vout1_reg;

	//LINE_F1
	REG_SET_FIELD(&(reg->vbi_cgms_ctrl),  0x3FF<<10, 0x16, 10);
	//LINE_F2
	REG_SET_FIELD(&(reg->vbi_cgms_ctrl),  0x3FF<<0 , 0x16,  0);
	//LINE_F3
	REG_SET_FIELD(&(reg->vbi_cgms_ctrl2), 0x3FF<<10, 0x16, 10);
	//LINE_F4
	REG_SET_FIELD(&(reg->vbi_cgms_ctrl2), 0x3FF<<0 , 0x16,  0);

	if (property->enable)
		REG_SET_BIT(&(reg->vbi_cgms_ctrl), 20);
	else
		REG_CLR_BIT(&(reg->vbi_cgms_ctrl), 20);

	return 0;
}

// From SOC wanglx
static unsigned int cgms_data_generator(unsigned int CGMS_A_control, unsigned int APS_control, unsigned int ASB_control)
{
	unsigned int cgms_header = 48, cgms_payload = 0 , cgms_payload_bit13 = 0 ;
	unsigned int cgms_data = 0, crc_data = 0, cgms_crc_shift = 63;
	unsigned int cgms_crc_shift_bit0 = 1, cgms_crc_shift_bit5 = 1, cgms_crc_shift_bit4_delayed = 1;
	unsigned int cgms_crc_shift_bit5_delayed = 1, cgms_crc_shift_counter = 0;

	cgms_payload = cgms_header << 8 | CGMS_A_control << 6 | APS_control << 4 | ASB_control << 3;

	for(cgms_crc_shift_counter=0; cgms_crc_shift_counter<14; cgms_crc_shift_counter++) {
		cgms_payload_bit13 = (cgms_payload & 0x2000) >> 13;
		cgms_crc_shift_bit5 = (cgms_crc_shift & 0x20) >> 5;
		cgms_crc_shift_bit0 = (cgms_crc_shift & 0x1) >> 0;
		cgms_crc_shift_bit5_delayed = cgms_crc_shift_bit0 ^ cgms_payload_bit13;
		cgms_crc_shift_bit4_delayed = cgms_crc_shift_bit5_delayed ^ cgms_crc_shift_bit5;
		cgms_crc_shift = cgms_crc_shift_bit5_delayed << 5 | cgms_crc_shift_bit4_delayed << 4 | (cgms_crc_shift & 0x1e) >> 1;
		cgms_data = cgms_data << 1 | cgms_payload_bit13;
		cgms_payload = (cgms_payload & 0x1fff) << 1;
	}

	for(cgms_crc_shift_counter=0; cgms_crc_shift_counter<6; cgms_crc_shift_counter++) {
		cgms_crc_shift_bit0 = (cgms_crc_shift & 0x1) >> 0;
		cgms_crc_shift = (cgms_crc_shift & 0x3e) >> 1;
		cgms_data = cgms_data << 1 | cgms_crc_shift_bit0;
	}

	return (cgms_data);
}

static int gx3211_vout_cgms_config(GxVideoOutProperty_CGMSConfig *property)
{
	int ret = 0;
	unsigned int cgms_value = 0;
	volatile struct gx3211_videoout_reg *reg = NULL;

	reg = gx3211_vout1_reg;

	gx3211_voutinfo.analog_copy_permission  = property->CGMS_A_control;
	gx3211_voutinfo.digital_copy_permission = property->CGMS_A_control;

	gxav_hdmi_set_cgmsa_permission(gx3211_voutinfo.digital_copy_permission);
	//CRC value
	cgms_value = cgms_data_generator(property->CGMS_A_control,
					 property->ASP_control,
					 property->ASB_control);
	REG_SET_VAL(&(reg->vbi_cgms_data), cgms_value);

	return (ret);
}

static void vbi_wss_enable(unsigned int wss_enable, unsigned int wss_line1,unsigned int wss_line2)
{
	volatile struct gx3211_videoout_reg *reg =gx3211_vout1_reg;
	REG_SET_VAL(&(reg->vbi_wss_ctrl), wss_enable << 20 | wss_line1 << 10 | wss_line2);
}

//***************************************************************************************
// wss_data = b0 << 13 | b1 << 12 | b2 << 11 | b3  << 10 | b4  << 9 | b5  << 8 | b6 << 7
//          | b7 <<  6 | b8 <<  5 | b9 <<  4 | b10 <<  3 | b11 << 2 | b12 << 1 | b13 
//---------------------------------------------------------------------------------------
//  Group A (Aspect Ratio) Data Bit Assignments and Usage
//---------------------------------------------------------------------------------------
// b0,b1,b2,b3  | Aspect Ratio | Fromat     | Position On 4:3 Display | Active Lines |
//  0  0  0  1  |    4:3       | full format|            --           |      576     |
//  1  0  0  0  |   14:9       | letterbox  |          center         |      504     |
//  0  1  0  0  |   14:9       | letterbox  |          top            |      504     |
//  1  1  0  1  |   16:9       | letterbox  |          center         |      430     |
//  0  0  1  0  |   16:9       | letterbox  |          top            |      430     |
//  1  0  1  1  |  >16:9       | letterbox  |          center         |      ---     |
//  0  1  1  1  |   14:9       | full format|          center         |      576     |
//  1  1  1  0  |   16:9       | full format|             --          |      576     |
//---------------------------------------------------------------------------------------
//  b4:mode
//      0  camera mode
//      1  film mode
//---------------------------------------------------------------------------------------
//  b5:color encoding
//      0  nomal PAL
//      1  Motion Adaptive ColorPlus
//---------------------------------------------------------------------------------------
//  b6: helper signals
//      0  not present
//      1  present
//---------------------------------------------------------------------------------------
//  b7: must set to "0", since reserved
//---------------------------------------------------------------------------------------
//  b8: teletext subtitles
//      0  no
//      1  yes
//---------------------------------------------------------------------------------------
//  b9,b10: open subtitles
//      00 no
//      01 outside active picture
//      10 inside active picture
//      11 reserved
//---------------------------------------------------------------------------------------
//  b11:surround sound
//      0  no
//      1  yes
//---------------------------------------------------------------------------------------
//  b12:copyright
//      0  no copyright asserted or unknown
//      1  copyright asserted
//---------------------------------------------------------------------------------------
//  b13:copy protection
//      0  copying not restricted
//      1  copying restricted
//---------------------------------------------------------------------------------------
//***************************************************************************************

static int gx3211_vout_wss_config(GxVideoOutProperty_WSSConfig *property)
{
	unsigned int wss_data = 0x0400;
	volatile struct gx3211_videoout_reg *reg = gx3211_vout1_reg;;

	wss_data |= property->WSS_control;
	REG_SET_VAL(&(reg->vbi_wss_data), wss_data);

	return 0;
}

static int gx3211_vout_wss_enable(GxVideoOutProperty_WSSEnable *property)
{
	if (property->enable)
		vbi_wss_enable(1, 22, 22);
	else
		vbi_wss_enable(0, 22, 22);

	return 0;
}

static int gx3211_vout_white_screen(int enable)
{
	volatile struct gx3211_videoout_reg *reg = gx3211_vout0_reg;

	if (enable) {
		REG_SET_BIT(&(reg->digital_ctrl), 3);
		REG_SET_BIT(&(reg->digital_ctrl), 7);
		REG_CLR_BIT(&(reg->digital_ctrl), 11);
	} else {
		REG_CLR_BIT(&(reg->digital_ctrl), 3);
		REG_CLR_BIT(&(reg->digital_ctrl), 7);
	}

	return 0;
}

int gx3211_scart_config(struct gxav_videoout* vout, GxVideoOutProperty_ScartConfig *config)
{
	struct vout_interface *viface = NULL;

	GX3211_CHECK_VOUT_IFACE(vout->select, GXAV_VOUT_SCART);

	if (vout && config) {
		gx3211_voutinfo.scart_config = *config;
		viface = gx3211_get_vout_interface(GXAV_VOUT_SCART);
		gx3211_after_set_mode(viface);
	}

	return 0;
}

static struct vout_ops gx3211_vout_ops = {
	.init             = gx3211_vout_init,
	.uninit           = gx3211_vout_uninit,
	.open             = gx3211_vout_open,
	.close            = gx3211_vout_close,
	.config           = gx3211_vout_config,
	.set_interface    = gx3211_vout_set_interface,
	.set_resolution   = gx3211_vout_set_resolution,
	.set_brightness   = gx3211_vout_set_brightness,
	.set_saturation   = gx3211_vout_set_saturation,
	.set_contrast     = gx3211_vout_set_contrast,
	.set_sharpness    = gx3211_vout_set_sharpness,
	.set_hue          = gx3211_vout_set_hue,
	.set_default      = gx3211_vout_set_default,
	.set_aspratio     = gx3211_vout_set_aspratio,
	.set_tvscreen     = gx3211_vout_set_tvscreen,
	.get_resolution   = gx3211_vout_get_resolution,
	.get_dvemode      = gx3211_vout_get_dvemode,
	.get_brightness   = gx3211_vout_get_brightness,
	.get_contrast     = gx3211_vout_get_contrast,
	.get_saturation   = gx3211_vout_get_saturation,
	.get_sharpness    = gx3211_vout_get_sharpness,
	.get_hue          = gx3211_vout_get_hue,
	.play_cc          = gx3211_vout_play_cc,
	.set_PowerOff     = gx3211_vout_power_off,
	.set_PowerOn      = gx3211_vout_power_on,
	.hdcp_enable      = gx3211_vout_hdcp_enable,
	.hdcp_config      = gx3211_vout_hdcp_config,
	.hdcp_start       = gx3211_vout_hdcp_start,
	.get_hdmi_status  = gx3211_vout_get_hdmi_status,
	.get_edid_info    = gx3211_vout_get_edid_info,
	.set_macrovision  = gx3211_vout_set_macrovision,
	.set_dcs          = gx3211_vout_set_dcs,
	.get_hdmi_version = gx3211_vout_get_hdmi_version,
	.set_cgms_enable  = gx3211_vout_cgms_enable,
	.set_cgms_config  = gx3211_vout_cgms_config,
	.white_screen     = gx3211_vout_white_screen,
	.scart_config     = gx3211_scart_config,
	.cec_enable       = gx3211_vout_cec_enable,
	.cec_send_cmd     = gx3211_vout_cec_send_cmd,
	.cec_recv_cmd     = gx3211_vout_cec_recv_cmd,
	.set_vbi_wss_enable = gx3211_vout_wss_enable,
	.set_vbi_wss_config = gx3211_vout_wss_config,
};

static int gx3211_videoout_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	GX3211_VOUT0_BASE_ADDR = res->regs[0];
	GX3211_VOUT1_BASE_ADDR = res->regs[1];

	return 0;
}

struct gxav_module_ops gx3211_videoout_module = {
	.module_type = GXAV_MOD_VIDEO_OUT,
	.count = 1,
	.irqs = {-1},
	.irq_names = {NULL},
	.event_mask = 0xffffffff,
	.setup = gx3211_videoout_setup,
	.init = gx_videoout_init,
	.cleanup = gx_videoout_uninit,
	.open = gx_videoout_open,
	.close = gx_videoout_close,
	.set_property = gx_videoout_set_property,
	.get_property = gx_videoout_get_property,
	.priv = &gx3211_vout_ops,
};

