#include "porting.h"
#include "clock_hal.h"
#include "hdmi_hal.h"
#include "vpu_hal.h"
#include "vout_hal.h"
#include "vout_module.h"
#include "gx3113c_vout.h"
#include "gx3113c_vout_reg.h"
#include "gx3113c_vpu_reg.h"

static struct interface_ops gx3113c_vpu_interface_ops;
static volatile struct gx3113c_videoout_reg* gx3113c_vout0_reg;

#define IF_FAIL_IRQ_ENABLE(ret) \
	if(ret < 0) {\
		gx_interrupt_enable();\
	}

static struct vout_info gx3113c_voutinfo;
static struct reg_iface gx3113c_valid_iface[] = {
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_SVIDEO,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
		.ifaces_dac[IFACE_ID_SVIDEO] = { GXAV_VOUT_SVIDEO,  VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                      = GXAV_VOUT_RCA | GXAV_VOUT_RCA1,
		.vpu_dac_case                = 0x1,
		.vpu_dac_mode                = 0x0,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
		.ifaces_dac[IFACE_ID_RCA1  ] = { GXAV_VOUT_RCA1,    VPU_DAC_1 },
	},
	{
		.select                      = GXAV_VOUT_YUV,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x3,
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
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
	},
	{
		.select                       = GXAV_VOUT_SVIDEO,
		.vpu_dac_case                = 0x0,
		.vpu_dac_mode                = 0x0,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
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

extern unsigned int gx3113cvpu_get_scan_line(void);
extern volatile Gx3113cVpuReg    *gx3113cvpu_reg;

static int gs_scale[8][4] = {
	{ 0, 0, 0 , 0 },
	{ 160, 143, 203, 63 },
	{ 161, 136, 192, 71 },
	{ 161, 136, 192, 63 },
	{ 160, 164, 233, 71 },
	{ 151, 134, 190, 71 },
	{ 151, 134, 190, 71 },
};

/////////////////////  CVBS PAL/NTSC  /////////////////////
static unsigned int gx3113c_dve_pal_bdghi[] = {
	0x00020000, 0x00000000, 0x0097D47E, 0x001AE677,
	0x00781e13, 0x00b0a610, 0x25d05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x69973F3F, 0x00013F5E, 0x00000000, 0x00000089,
	0x00000000, 0x00000000, 0x000000c2, 0x00000000,
	0x00002C22, 0x008004B0, 0x0003F2E0, 0x06400000,
	0x00000018, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xAAAAAAAA, 0x000000A9,
	0x00000000, 0x0001FFFF, 0x10002000, 0x40826083,
	0xA082B082, 0xD082F000, 0x0026135F, 0x00002093,
	0x028506BF, 0x00001096, 0x0000002C, 0x00025625,
	0x00004AC4, 0x0000A935, 0x000A726E, 0x0009BA70,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08A8F20D,
	0x08A8F20D, 0x08A8F20D, 0x1705C170, 0x00000180
};

static unsigned int gx3113c_dve_pal_m[] = {
	0x00020001, 0x00000000, 0x0090d47e, 0x0017a65d,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x0004b800, 0x00000000,
	0x6b953f3f, 0x00003f37, 0x00000000, 0x00000089,
	0x00000000, 0x00000000, 0x000000c2, 0x00000000,
	0x00002C22, 0x008004B0, 0x0003D2D8, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x001FA359, 0x00002093,
	0x028506B3, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x00007901, 0x0008C208, 0x0008320B,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x00000187
};

static unsigned int gx3113c_dve_pal_n[] = {
	0x00020002, 0x00000000, 0x0097d47e, 0x001AE677,
	0x00781e13, 0x00b0a610, 0x25d05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x69973C47, 0x00003F37, 0x00000000, 0x00000089,
	0x00000000, 0x00000000, 0x000000c2, 0x00000000,
	0x00002C22, 0x008004b0, 0x0003F2E0, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0026135F, 0x00002093,
	0x028506BF, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x0000A935, 0x000A726E, 0x0009BA70,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x00000187
};

static unsigned int gx3113c_dve_pal_nc[] = {
	0x00020003, 0x00000000, 0x0097da7e, 0x001AE677,
	0x00781e13, 0x00b0a610, 0x25d05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x69973F3F, 0x00003F37, 0x00000000, 0x00000089,
	0x00000000, 0x00000000, 0x000000c2, 0x00000000,
	0x00002C22, 0x008004b0, 0x0003F2E0, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0026135F, 0x00002093,
	0x028506BF, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x0000A935, 0x000A726E, 0x0009BA70,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x00000187
};

static unsigned int gx3113c_dve_ntsc_m[] = {
#if  0
	0x00010001, 0x00000000, 0x0090d47e, 0x0017a65d,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x0004b800, 0x00000000,
	0x6b953f3f, 0x00003f37, 0x00000000, 0x00000089,
	0x00000000, 0x00000000, 0x000000c2, 0x00000000,
	0x00002C22, 0x008004B0, 0x0003D2D8, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x001FA359, 0x00002093,
	0x028506B3, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x00007901, 0x0008C208, 0x0008320B,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x00000187

#else
		0x00020004, 0x00000000, 0x0090d47e, 0x0017a65d,//0x001A2671,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x0004b800, 0x00000000,
	0x64803c47, 0x00003f37, 0x00000000, 0x0000008e,
	0x00000000, 0x00000000, 0x000000c6, 0x00000000,
	0x00002C22, 0x008004B0, 0x0003D2D8, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x001FA359, 0x00002093,
	0x028506B3, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x00007901, 0x0008C208, 0x0008320B,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x00000187
#endif
};

static unsigned int gx3113c_dve_ntsc_443[] = {
	0x00020005, 0x00000000, 0x0090c67e, 0x001A2671,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x0004b800, 0x00000000,
	0x64803c47, 0x00003f37, 0x00000000, 0x0000008e,
	0x00000000, 0x00000000, 0x000000c6, 0x00000000,
	0x00002C22, 0x008004B0, 0x0003D2D8, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x001FA359, 0x00002093,
	0x028506B3, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x00007901, 0x0008C208, 0x0008320B,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x00000187
};


/////////////////////  YPbPr/////////////////////
static unsigned int gx3113c_dve_ycbcr_480i[] = {
	0x14020004, 0x00000000, 0x0097D47E, 0x0015c64f,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x00003f37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00002F22, 0x008004B0, 0x0003D2D8, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x001FA359, 0x00002093,
	0x028506B3, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x00007901, 0x0008C208, 0x0008320B,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3113c_dve_ycbcr_576i[] = {
	0x14020000, 0x00000000, 0x0097D47E, 0x00182661,//0x001BE67F
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x00003f37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00002F22, 0x008004B0, 0x0003F2E0, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0026135f, 0x00002093,
	0x028506BF, 0x00001096, 0x0000002c, 0x00025625,
	0x00004AC4, 0x0000a935, 0x000a726e, 0x0009ba70,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0xC00001c7
};

static unsigned int gx3113c_dve_ypbpr_480p[] = {
	0x14020008, 0x00000000, 0x0097D43F, 0x00162651,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x00003f37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00003810, 0x008004B0, 0x0207e5b0, 0x06400000,//0x0081f2d8
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x001fa359, 0x00002093,
	0x028506b3, 0x00001096, 0x04000024, 0x00025625,
	0x00004AC4, 0x00011A02, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3113c_dve_ypbpr_576p[] = {
	0x14020009, 0x00000000, 0x0097D43F, 0x00188666,//0x001B867C
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x00003f37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x0000341b, 0x008004B0, 0x0207e5C0, 0x06400000,//0x0081f2e0
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0026135f, 0x00002093,
	0x028506bf, 0x00001096, 0x04000024, 0x00025625,
	0x00004AC4, 0x00014267, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static void gx3113c_vout_init_interface(void)
{
	gx3113c_voutinfo.interfaces[0].iface = GXAV_VOUT_RCA;
	gx3113c_voutinfo.interfaces[1].iface = GXAV_VOUT_RCA1;
	gx3113c_voutinfo.interfaces[2].iface = GXAV_VOUT_YUV;
	gx3113c_voutinfo.interfaces[3].iface = GXAV_VOUT_SCART;
	gx3113c_voutinfo.interfaces[4].iface = GXAV_VOUT_SVIDEO;

	gx3113c_voutinfo.interfaces[0].ops =
	gx3113c_voutinfo.interfaces[1].ops =
	gx3113c_voutinfo.interfaces[2].ops =
	gx3113c_voutinfo.interfaces[3].ops =
	gx3113c_voutinfo.interfaces[4].ops = &gx3113c_vpu_interface_ops;
}

static void gx3113c_vout_probe_vpu_mode(void)
{
	unsigned int  val  = REG_GET_VAL(&gx3113c_vout0_reg->tv_ctrl);
	unsigned char mode = 0, reg_mode = (val >> bTV_CTRL_TV_FORMAT) & 0xf;
	unsigned int dacpoweron = gxav_vpu_DACPower(VPU_DAC_ALL);

	if (dacpoweron != 0) {
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
	gx3113c_voutinfo.src[ID_VPU].mode = mode;
	gx3113c_voutinfo.src[ID_VPU].loader_inited = (mode != 0);
}

static void gx3113c_vout_init_resolution(void)
{
	int i;

	gx3113c_vout_probe_vpu_mode();
	for (i = 0; i< GX_ARRAY_SIZE(gx3113c_voutinfo.interfaces); i++) {
		gx3113c_voutinfo.interfaces[i].brightness = 50;
		gx3113c_voutinfo.interfaces[i].contrast   = 50;
		gx3113c_voutinfo.interfaces[i].saturation = 50;
	}
}

static struct reg_iface* gx3113c_vout_get_riface(int select)
{
	int i;

	for (i = 0; i< GX_ARRAY_SIZE(gx3113c_valid_iface); i++) {
		if (select == gx3113c_valid_iface[i].select)
			break;
	}
	if (i == GX_ARRAY_SIZE(gx3113c_valid_iface)) {
		VIDEOOUT_PRINTF("select is invalid!\n");
		return NULL;
	}

	return &gx3113c_valid_iface[i];
}

static struct vout_interface* gx3113c_get_vout_interface(GxVideoOutProperty_Interface iface)
{
	int i;

	for (i = 0; i< GX_ARRAY_SIZE(gx3113c_voutinfo.interfaces); i++) {
		if (iface == gx3113c_voutinfo.interfaces[i].iface)
			return &gx3113c_voutinfo.interfaces[i];
	}

	return NULL;
}

static volatile struct gx3113c_videoout_reg * gx3113c_get_vout_regs(GxVideoOutProperty_Interface iface)
{
	volatile struct gx3113c_videoout_reg *reg = NULL;

	switch (iface) {
	case GXAV_VOUT_RCA:
	case GXAV_VOUT_RCA1:
	case GXAV_VOUT_SCART:
	case GXAV_VOUT_SVIDEO:
	case GXAV_VOUT_YUV:
		reg = gx3113c_vout0_reg;
		break;
	default:
		break;
	}

	return reg;
}

int gx3113c_vout_get_dvemode(struct gxav_videoout *videoout, struct vout_dvemode *dvemode)
{
	int id = dvemode->id;

	if (id != ID_VPU)
		return -1;

	dvemode->mode = gx3113c_voutinfo.src[id].mode;

	return 0;
}

static void gx3113c_videoout_mode_config(volatile struct gx3113c_videoout_reg *reg, unsigned int *data, int mode)
{
	REG_SET_VAL((volatile unsigned int *)(reg), data[0]);

	if(mode == GXAV_VOUT_PAL) {
		* (volatile unsigned int *)((char *)reg + 0xa4) = 0x00000001;
		* (volatile unsigned int *)((char *)reg + 0xa8) = 0x10002000;
		* (volatile unsigned int *)((char *)reg + 0xac) = 0x40006081;
		* (volatile unsigned int *)((char *)reg + 0xb0) = 0xa000b001;
		* (volatile unsigned int *)((char *)reg + 0xb4) = 0xd003f003;
		* (volatile unsigned int *)((char *)reg + 0x10) = 0x00701a16;
		* (volatile unsigned int *)((char *)reg + 0x14) = 0x2060b809;
		* (volatile unsigned int *)((char *)reg + 0x18) = 0x26585546;
		* (volatile unsigned int *)((char *)reg + 0x1c) = 0x2184897a;
		* (volatile unsigned int *)((char *)reg + 0x24) = 0x2338706b;
		* (volatile unsigned int *)((char *)reg + 0x34) = 0x00003f61;
	}
}

static int gx3113c_videoout_pal_config(volatile struct gx3113c_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {
	case GXAV_VOUT_PAL:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_pal_bdghi, mode);
		break;
	case GXAV_VOUT_PAL_N:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_pal_n, mode);
		break;
	case GXAV_VOUT_PAL_NC:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_pal_nc, mode);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in pal\n");
		return -1;
	}

	return 0;
}

static int gx3113c_videoout_ntsc_config(volatile struct gx3113c_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {
	case GXAV_VOUT_NTSC_M:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_ntsc_m, mode);
		break;
	case GXAV_VOUT_NTSC_443:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_ntsc_443, mode);
		break;
	case GXAV_VOUT_PAL_M:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_pal_m, mode);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in ntsc\n");
		return -1;
	}

	return 0;
}

static int gx3113c_videoout_yuv_config(volatile struct gx3113c_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {
	case GXAV_VOUT_480I:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_ycbcr_480i, mode);
		break;
	case GXAV_VOUT_576I:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_ycbcr_576i, mode);
		break;
	case GXAV_VOUT_480P:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_ypbpr_480p, mode);
		break;
	case GXAV_VOUT_576P:
		gx3113c_videoout_mode_config(reg, gx3113c_dve_ypbpr_576p, mode);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in yuv\n");
		return -1;
	}

	return 0;
}

/*
 *  config dac full case and mode, etc
 *  the tv_ctrl register may be change when config the resolution array
 */
static void gx3113c_vout_config_dac_mode_case(struct vout_interface *viface)
{
	struct vout_interface* viface_yuv;
	volatile struct gx3113c_videoout_reg *reg;
	GxVideoOutProperty_Mode mode;
	struct reg_iface* riface = gx3113c_vout_get_riface(viface->vout->select);

	if (GX3113C_HAVE_VPU_SRC(viface->vout->select)) {
		/* set dac mode generic */
		DVE_SET_DAC_OUT_MODE(gx3113c_vout0_reg, riface->vpu_dac_mode);

		/* set dac full case */
		DVE_SET_DAC_FULLCASE_CHANGE(gx3113c_vout0_reg, riface->vpu_dac_case);
	}

	if (viface->vout->select & GXAV_VOUT_YUV) {
		reg = gx3113c_get_vout_regs(GXAV_VOUT_YUV);
		viface_yuv = gx3113c_get_vout_interface(GXAV_VOUT_YUV);
		mode = viface_yuv->ops->get_mode(viface_yuv);

		switch (mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
			//Y_Cb_Cr_CVBS : YCBCR_480I?YCBCR_576I
			DVE_SET_DAC_OUT_MODE(reg, (Y_Cb_Cr_CVBS));
			break;
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
			//Y_Pb_Pr_Y : YPBPR_480P?YPBPR_576P
			DVE_SET_DAC_OUT_MODE(reg, (Y_Pb_Pr_Y));
			break;
		default:
			break;
		}
	}

}

static int gx3113c_vout_set_dve(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	volatile struct gx3113c_videoout_reg *reg = gx3113c_get_vout_regs(viface->iface);

	if(!reg)
		return -1;

	switch(mode)
	{
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
		gx3113c_videoout_pal_config(reg, mode);
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
		gx3113c_videoout_ntsc_config(reg, mode);
		break;
	case GXAV_VOUT_480I:
	case GXAV_VOUT_576I:
	case GXAV_VOUT_480P:
	case GXAV_VOUT_576P:
		gx3113c_videoout_yuv_config(reg, mode);
		break;
	default:
		VIDEOOUT_PRINTF("un-supported.\n");
		return -1;
	}

	if(viface->iface == GXAV_VOUT_SCART)	{
		gxav_clock_multiplex_pinsel(PIN_TYPE_VIDEOOUT_SCART);
		switch(mode)
		{
		case GXAV_VOUT_PAL:
			REG_SET_VAL(&(reg->tv_ctrl), 0x20000);
			break;
		case GXAV_VOUT_PAL_M:
			REG_SET_VAL(&(reg->tv_ctrl), 0x20001);
			break;
		case GXAV_VOUT_PAL_N:
			REG_SET_VAL(&(reg->tv_ctrl), 0x20002);
			break;
		case GXAV_VOUT_NTSC_M:
			REG_SET_VAL(&(reg->tv_ctrl), 0x20004);
			break;
		case GXAV_VOUT_NTSC_443:
			REG_SET_VAL(&(reg->tv_ctrl), 0x20005);
			break;
		default:
			VIDEOOUT_PRINTF("un-supported.\n");
			break;
		}
	}

	viface->mode = mode;
	gx3113c_voutinfo.src[ID_VPU].mode = mode;
	gx3113c_vout_config_dac_mode_case(viface);

	//clk set
	{
		clock_params params;
		params.vout.src  = ID_VPU;
		params.vout.mode = mode;
		gxav_clock_setclock(MODULE_TYPE_VOUT, &params);
	}

	//VPU hot-reset
	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	gxav_clock_hot_rst_clr(MODULE_TYPE_VPU);

	return 0;
}

static int gx3113c_videoout_ioremap(void)
{
	unsigned char *vout0_mapped_addr = NULL;

	// VOUT0
	if (0 == gx_request_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3113c_videoout_reg))) {
		VIDEOOUT_PRINTF("request_mem_region failed");
		return -1;
	}
	vout0_mapped_addr = gx_ioremap(TV_VOUT0_BASE_ADDR, sizeof(struct gx3113c_videoout_reg));
	if (!vout0_mapped_addr) {
		VIDEOOUT_PRINTF("ioremap failed.\n");
		goto err_vout0_ioremap;
	}

	gx3113c_vout0_reg = (struct gx3113c_videoout_reg *)vout0_mapped_addr;

	return 0;

err_vout0_ioremap:
	gx_release_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3113c_videoout_reg));
	gx3113c_vout0_reg= NULL;

	return -1;
}

static int gx3113c_videoout_iounmap(void)
{
	gx_iounmap(gx3113c_vout0_reg);
	gx3113c_vout0_reg = NULL;

	gx_release_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3113c_videoout_reg));

	return 0;
}

static int gx3113c_vout_init(void)
{
	if (gx3113c_videoout_ioremap() < 0) {
		return -1;
	}

	memset(&gx3113c_voutinfo, 0, sizeof(gx3113c_voutinfo));
	gx3113c_vout_init_interface();
	gx3113c_vout_init_resolution();
	VIDEOOUT_DBG("gx3113c_vout_init\n");

	return 0;
}

static int gx3113c_vout_uninit(void)
{
	gx3113c_videoout_iounmap();

	VIDEOOUT_DBG("gx3113c_vout_uninit\n");

	return 0;
}

static int gx3113c_vout_open(struct gxav_videoout *videoout)
{
	VIDEOOUT_DBG("gx3113c_vout_open\n");
	return 0;
}

static int gx3113c_vout_close(struct gxav_videoout *videoout)
{
	return 0;
}

static void gx3113c_vout_insert_scan_chance(int mode)
{
	while(gx3113cvpu_get_scan_line()!=0);
	switch (mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_576I:
		while(gx3113cvpu_get_scan_line()<270);
		break;
	case GXAV_VOUT_576P:
		while(gx3113cvpu_get_scan_line()<560);
		break;
	case GXAV_VOUT_480P:
		while(gx3113cvpu_get_scan_line()<470);
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
		while(gx3113cvpu_get_scan_line()<230);
		break;
	default:
		break;
	}
}

static int gx3113c_vout_config(struct gxav_videoout *videoout, GxVideoOutProperty_OutputConfig *config)
{
	return 0;
}


static int gx3113c_vout_patch_interface_p_active_off(GxVideoOutProperty_Mode mode)
{
	int ret = 0;

#define TV_ACTIVE_OFFSET (6)

	unsigned int tv_active_off = 0;

	tv_active_off = REG_GET_FIELD(&(gx3113c_vout0_reg->tv_active),
			mTV_ACTIVE_OFF,
			bTV_ACTIVE_OFF);
	switch(mode)
	{
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		DVE_SET_ACTIVE_OFF(gx3113c_vout0_reg, tv_active_off + TV_ACTIVE_OFFSET);
		break;
	default:
		break;
	}

	return ret;
}

static void gx3113c_after_set_mode(struct vout_interface *viface)
{
	unsigned int b, c, s;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);
	viface->ops->set_brightness(viface, b);
	viface->ops->set_saturation(viface, c);
	viface->ops->set_saturation(viface, s);
}

static int gx3113c_vout_set_resolution(struct gxav_videoout *videoout, GxVideoOutProperty_Resolution *resolution)
{
	unsigned int last_mode = 0;
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(resolution->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, resolution->iface);
	viface = gx3113c_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(viface);

	last_mode = viface->ops->get_mode(viface);
	if (last_mode == resolution->mode) {
		if (gx3113c_voutinfo.src[ID_VPU].loader_inited) {
			gx3113c_voutinfo.src[ID_VPU].loader_inited = 0;
			gxav_vpu_SetVoutIsready(1);
		}
		return 0;
	}

	gxav_vpu_SetVoutIsready(0);
	viface->ops->set_mode(viface, resolution->mode);
	gx3113c_after_set_mode(viface);
	gxav_vpu_SetVoutIsready(1);

	return 0;
}

int gx3113c_videoout_off(struct gxav_videoout *videoout)
{
	gxav_vpu_SetVoutIsready(0);

	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	return 0;
}

int gx3113c_vout_power_off(struct gxav_videoout *videoout, int selection)
{
	int i;
	struct reg_iface* riface = gx3113c_vout_get_riface(selection);

	if (!riface)
		return -1;
	GX3113C_CHECK_VOUT_IFACE(videoout->select, selection);

	for (i = 0; i < IFACE_ID_MAX; i++) {
		if (riface->ifaces_dac[i].iface & selection)
			gxav_vpu_DACEnable(riface->ifaces_dac[i].dac, 0);
	}


	return 0;
}

int gx3113c_vout_power_on(struct gxav_videoout *videoout, int selection)
{
	int i;
	struct reg_iface* riface = gx3113c_vout_get_riface(selection);

	if (!riface)
		return -1;
	GX3113C_CHECK_VOUT_IFACE(videoout->select, selection);

	for (i = 0; i < IFACE_ID_MAX; i++) {
		if (riface->ifaces_dac[i].iface & selection)
			gxav_vpu_DACEnable(riface->ifaces_dac[i].dac, 1);
	}


	return 0;
}

static int gx3113c_vout_set_macrovision(struct gxav_videoout* vout, GxVideoOutProperty_Macrovision *macrovision)
{
	volatile struct gx3113c_videoout_reg *reg = gx3113c_get_vout_regs(GXAV_VOUT_RCA);

	if (macrovision->mode == GX_MACROVISION_MODE_TYPE0) {
		REG_SET_BIT(((char*)reg + 0xdc), 12);
	} else {
		REG_CLR_BIT(((char*)reg + 0xdc), 12);
		REG_SET_FIELD(((char*)reg + 0xcc), 0xff, 0, 0);
	}

	return 0;
}

int gx3113c_vout_set_default(struct gxav_videoout *videoout, GxVideoOutProperty_OutDefault *outdefault)
{
	volatile struct gx3113c_videoout_reg *reg = gx3113c_get_vout_regs(outdefault->iface);

	if (!reg)
		return -1;

	if (outdefault->enable)
		REG_SET_BIT(&(reg->digital_ctl), 3);
	else
		REG_CLR_BIT(&(reg->digital_ctl), 3);

	return 0;
}

static int gx3113c_vout_set_interface(struct gxav_videoout *videoout, GxVideoOutProperty_OutputSelect *output)
{
#define VIFACE_ASSIGN_VOUT(iface)\
	do {\
		if (output->selection & iface) {\
			viface = gx3113c_get_vout_interface(iface);\
			viface->vout = videoout;\
		}\
	} while (0)
	struct vout_interface* viface = NULL;
	struct reg_iface* riface = gx3113c_vout_get_riface(output->selection);

	if (!riface)
		return -1;

	VIFACE_ASSIGN_VOUT(GXAV_VOUT_RCA);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_RCA1);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_YUV);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_SCART);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_SVIDEO);

	if (videoout->select == output->selection)
		return 0;

	videoout->select = 0;
	gxav_vpu_DACEnable(VPU_DAC_ALL, 0);

	videoout->select = output->selection;
	gx3113c_vout_power_on(videoout, output->selection);

	gxav_vpu_DACSource(VPU_DAC_0, riface->dac_src0);
	gxav_vpu_DACSource(VPU_DAC_1, riface->dac_src1);
	gxav_vpu_DACSource(VPU_DAC_2, riface->dac_src2);
	gxav_vpu_DACSource(VPU_DAC_3, riface->dac_src3);

	return 0;
}


static int gx3113c_vout_set_brightness(struct gxav_videoout *videoout, GxVideoOutProperty_Brightness *brightness)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(brightness->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, brightness->iface);
	viface = gx3113c_get_vout_interface(brightness->iface);
	CHECK_VOUT_INTERFACE(viface);

	viface->ops->set_brightness(viface, brightness->value);

	return 0;
}

static int gx3113c_vout_set_saturation(struct gxav_videoout *videoout, GxVideoOutProperty_Saturation *saturation)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(saturation->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, saturation->iface);
	viface = gx3113c_get_vout_interface(saturation->iface);
	CHECK_VOUT_INTERFACE(viface);

	viface->ops->set_saturation(viface, saturation->value);

	return 0;
}

static int gx3113c_vout_set_contrast(struct gxav_videoout *videoout, GxVideoOutProperty_Contrast *contrast)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(contrast->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, contrast->iface);
	viface = gx3113c_get_vout_interface(contrast->iface);
	CHECK_VOUT_INTERFACE(viface);

	viface->ops->set_contrast(viface, contrast->value);

	return 0;
}

static int gx3113c_vout_set_aspratio(struct gxav_videoout *videoout, GxVideoOutProperty_AspectRatio *aspect_ratio)
{
	GxVpuProperty_AspectRatio spec;
	int ret = -1;

	switch (aspect_ratio->spec) {
	case ASPECT_RATIO_PAN_SCAN:
		spec.AspectRatio = VP_PAN_SCAN;
		break;
	case ASPECT_RATIO_LETTER_BOX:
		spec.AspectRatio = VP_LETTER_BOX;
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
	GX3113C_DVE_CHECK_RET(ret);

	return 0;
}

static int gx3113c_vout_set_tvscreen(struct gxav_videoout *videoout, GxVideoOutProperty_TvScreen *TvScreen)
{
	GxVpuProperty_TvScreen Screen;
	int ret = -1;

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
	GX3113C_DVE_CHECK_RET(ret);

	return 0;
}

int gx3113c_vout_get_resolution(struct gxav_videoout *videoout, GxVideoOutProperty_Resolution *resolution)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(resolution->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, resolution->iface);
	viface = gx3113c_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(viface);

	resolution->mode = viface->ops->get_mode(viface);

	return 0;
}

int gx3113c_vout_get_brightness(struct gxav_videoout *videoout, GxVideoOutProperty_Brightness *brightness)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(brightness->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, brightness->iface);
	viface = gx3113c_get_vout_interface(brightness->iface);
	CHECK_VOUT_INTERFACE(viface);

	brightness->value = viface->ops->get_brightness(viface);

	return 0;
}

int gx3113c_vout_get_contrast(struct gxav_videoout *videoout, GxVideoOutProperty_Contrast *contrast)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(contrast->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, contrast->iface);
	viface = gx3113c_get_vout_interface(contrast->iface);
	CHECK_VOUT_INTERFACE(viface);

	contrast->value = viface->ops->get_contrast(viface);

	return 0;
}

int gx3113c_vout_get_saturation(struct gxav_videoout *videoout, GxVideoOutProperty_Saturation *saturation)
{
	struct vout_interface* viface = NULL;

	GX3113C_CHECK_IFACE(saturation->iface);
	GX3113C_CHECK_VOUT_IFACE(videoout->select, saturation->iface);
	viface = gx3113c_get_vout_interface(saturation->iface);
	CHECK_VOUT_INTERFACE(viface);

	saturation->value = viface->ops->get_saturation(viface);

	return 0;
}

static int gx3113c_vpu_iface_set_mode(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	unsigned int last_mode = 0;

	last_mode = viface->ops->get_mode(viface);

	gxav_vpu_SetVoutIsready(0);

	gx3113c_vout_set_dve(viface, mode);

	gx3113c_vout_patch_interface_p_active_off(mode);
	gxav_vpu_SetVoutIsready(1);

	return 0;
}

static int gx3113c_vpu_iface_set_brightness(struct vout_interface *viface, unsigned int value)
{
	unsigned int brightness_value = 0;
	unsigned int b, c;
	unsigned int scale0;
	unsigned int scale3 = 63;
	unsigned int luma_shift = 0;
	volatile struct gx3113c_videoout_reg *reg = gx3113c_get_vout_regs(viface->iface);
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->brightness = value;

	scale0 = gs_scale[mode][0];
	scale3 = gs_scale[mode][3];
	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);

	brightness_value = (scale0 *((c << 1) * (b << 1))) / (100 * 100);
	brightness_value = brightness_value > 224 ? 224 : brightness_value;
	brightness_value = brightness_value < 16 ? 16 : brightness_value;

	if(brightness_value > scale0){
		luma_shift = scale3 - 6;
	}
	else {
		luma_shift = scale3;
	}
	gx3113c_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_Y_SCALE0(reg, brightness_value);
	DVE_SET_ADJUST_Y_SHIFT(reg,luma_shift);

	return 0;
}

static int gx3113c_vpu_iface_set_saturation(struct vout_interface *viface, unsigned int value)
{
	unsigned int saturation_value;
	unsigned int c, s, scale;
	volatile struct gx3113c_videoout_reg *reg = gx3113c_get_vout_regs(viface->iface);
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->saturation = value;

	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	gx3113c_vout_insert_scan_chance(mode);

	scale = gs_scale[mode][1];
	saturation_value = (scale*((c << 1)*(s << 1))) / (100 * 100);
	saturation_value = saturation_value > 314 ? 314 : saturation_value;
	DVE_SET_YUV_TRAN_U_SCALE1(reg, saturation_value);

	scale = gs_scale[mode][2];
	saturation_value = (scale*((c << 1)*(s << 1))) / (100 * 100);
	saturation_value = saturation_value > 314 ? 314 : saturation_value;
	DVE_SET_YUV_TRAN_V_SCALE2(reg, saturation_value);

	return 0;
}

static int gx3113c_vpu_iface_set_contrast(struct vout_interface *viface, unsigned int value)
{
	unsigned int brightness_value = 0;
	unsigned int saturation_value = 0;
	unsigned int b, c, s;
	unsigned int scale, scale3;
	unsigned int luma_shift = 0;
	volatile struct gx3113c_videoout_reg *reg = gx3113c_get_vout_regs(viface->iface);
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->contrast = value;

	scale3 = gs_scale[mode][3];
	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	scale = gs_scale[mode][0];
	brightness_value = (scale *((c << 1) * (b << 1))) / (100 * 100);
	brightness_value = brightness_value > 224 ? 224 : brightness_value;
	brightness_value = brightness_value < 16 ? 16 : brightness_value;

	if(brightness_value > scale){
		luma_shift = scale3 - 6;
	}
	else {
		luma_shift = scale3;
	}
	gx3113c_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_Y_SCALE0(reg, brightness_value);

	scale = gs_scale[mode][1];
	saturation_value = (scale*((c << 1) * (s << 1))) / (100 * 100);
	saturation_value = saturation_value > 314 ? 314 : saturation_value;
	DVE_SET_YUV_TRAN_U_SCALE1(reg, saturation_value);

	scale = gs_scale[mode][2];
	saturation_value = (scale*((c << 1) * (s << 1))) / (100 * 100);
	saturation_value = saturation_value > 314 ? 314 : saturation_value;
	DVE_SET_YUV_TRAN_V_SCALE2(reg, saturation_value);
	DVE_SET_ADJUST_Y_SHIFT(reg,luma_shift);

	return 0;
}


GxVideoOutProperty_Mode gx3113c_iface_get_mode(struct vout_interface *viface)
{
	return viface->mode;
}

unsigned int gx3113c_iface_get_brightness(struct vout_interface *viface)
{
	return viface->brightness;
}

unsigned int gx3113c_iface_get_saturation(struct vout_interface *viface)
{
	return viface->saturation;
}

unsigned int gx3113c_iface_get_contrast(struct vout_interface *viface)
{
	return viface->contrast;
}

static struct interface_ops gx3113c_vpu_interface_ops = {
	.set_mode       = gx3113c_vpu_iface_set_mode,
	.set_brightness = gx3113c_vpu_iface_set_brightness,
	.set_saturation = gx3113c_vpu_iface_set_saturation,
	.set_contrast   = gx3113c_vpu_iface_set_contrast,
	.get_mode       = gx3113c_iface_get_mode,
	.get_brightness = gx3113c_iface_get_brightness,
	.get_saturation = gx3113c_iface_get_saturation,
	.get_contrast   = gx3113c_iface_get_contrast,
};

static struct vout_ops gx3113c_vout_ops = {
	.init           = gx3113c_vout_init,
	.uninit         = gx3113c_vout_uninit,
	.open           = gx3113c_vout_open,
	.close          = gx3113c_vout_close,
	.config         = gx3113c_vout_config,
	.set_interface  = gx3113c_vout_set_interface,
	.set_resolution = gx3113c_vout_set_resolution,
	.set_brightness = gx3113c_vout_set_brightness,
	.set_saturation = gx3113c_vout_set_saturation,
	.set_contrast   = gx3113c_vout_set_contrast,
	.set_default    = gx3113c_vout_set_default,
	.set_aspratio   = gx3113c_vout_set_aspratio,
	.set_tvscreen   = gx3113c_vout_set_tvscreen,
	.get_resolution = gx3113c_vout_get_resolution,
	.get_dvemode    = gx3113c_vout_get_dvemode,
	.get_brightness = gx3113c_vout_get_brightness,
	.get_contrast   = gx3113c_vout_get_contrast,
	.get_saturation = gx3113c_vout_get_saturation,
	.set_PowerOff   = gx3113c_vout_power_off,
	.set_PowerOn    = gx3113c_vout_power_on,
	.set_macrovision= gx3113c_vout_set_macrovision,
};

struct gxav_module_ops gx3113c_videoout_module = {
	.module_type = GXAV_MOD_VIDEO_OUT,
	.count = 1,
	.irqs = {-1},
	.event_mask = 0xffffffff,
	.init = gx_videoout_init,
	.cleanup = gx_videoout_uninit,
	.open = gx_videoout_open,
	.close = gx_videoout_close,
	.set_property = gx_videoout_set_property,
	.get_property = gx_videoout_get_property,
	.priv = &gx3113c_vout_ops,
};

