#include "gx3201_vout.h"
#include "gx3201_vout_reg.h"
#include "porting.h"
#include "clock_hal.h"
#include "vpu_hal.h"
#include "vout_hal.h"
#include "hdmi_hal.h"
#include "vout_module.h"

volatile struct gx3201_videoout_reg* gx3201_vout0_reg;
volatile struct gx3201_videoout_reg* gx3201_vout1_reg;

static struct vout_info gx3201_voutinfo;

static struct reg_iface gx3201_valid_iface[] = {
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_RCA1 | GXAV_VOUT_SVIDEO | GXAV_VOUT_HDMI,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
		.ifaces_dac[IFACE_ID_RCA1  ] = { GXAV_VOUT_RCA1,    VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_SVIDEO] = { GXAV_VOUT_SVIDEO,  VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_YUV | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0xb,
		.vpu_dac_mode                = 0x5,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA, VPU_DAC_3 },
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV, VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_SCART | GXAV_VOUT_HDMI,
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
		.select                       = GXAV_VOUT_YUV,
		.vpu_dac_case                = 0xb,
		.vpu_dac_mode                = 0x5,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV,     VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                       = GXAV_VOUT_HDMI,
	},
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_HDMI,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
	},
	{
		.select                       = GXAV_VOUT_SVIDEO | GXAV_VOUT_HDMI,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_SVIDEO] = { GXAV_VOUT_SVIDEO,  VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_YUV,
		.vpu_dac_case                = 0xb,
		.vpu_dac_mode                = 0x5,
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
		.select                       = GXAV_VOUT_YUV | GXAV_VOUT_HDMI,
		.vpu_dac_case                = 0xb,
		.vpu_dac_mode                = 0x5,
		.dac_src0                    = VPU_DAC_SRC_VPU,
		.dac_src1                    = VPU_DAC_SRC_VPU,
		.dac_src2                    = VPU_DAC_SRC_VPU,
		.dac_src3                    = VPU_DAC_SRC_VPU,
		.ifaces_dac[IFACE_ID_YUV   ] = { GXAV_VOUT_YUV,     VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 },
	},
	{
		.select                       = GXAV_VOUT_SCART | GXAV_VOUT_HDMI,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x2,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_SCART ] = { GXAV_VOUT_SCART,   VPU_DAC_0 | VPU_DAC_1 | VPU_DAC_2 | VPU_DAC_3 },
	},
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_RCA1 | GXAV_VOUT_HDMI,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
		.ifaces_dac[IFACE_ID_RCA1  ] = { GXAV_VOUT_RCA1,    VPU_DAC_3 },
	},
	{
		.select                       = GXAV_VOUT_RCA | GXAV_VOUT_SVIDEO | GXAV_VOUT_HDMI,
		.svpu_dac_case               = 0x0,
		.svpu_dac_mode               = 0x0,
		.dac_src0                    = VPU_DAC_SRC_SVPU,
		.dac_src1                    = VPU_DAC_SRC_SVPU,
		.dac_src2                    = VPU_DAC_SRC_SVPU,
		.dac_src3                    = VPU_DAC_SRC_SVPU,
		.ifaces_dac[IFACE_ID_RCA   ] = { GXAV_VOUT_RCA,     VPU_DAC_0 },
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
static int c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11;

static void register_vout_interface(void);
extern unsigned int gx3201vpu_get_scan_line(void);
extern void Gx3201_disable_svpu(void);

static int scale[8][4] = {
	{ 0, 0, 0 , 0 },
	{ 160, 143, 203, 63 },
	{ 161, 136, 192, 71 },
	{ 161, 136, 192, 63 },
	{ 160, 164, 233, 71 },
	{ 151, 134, 190, 71 },
	{ 151, 134, 190, 71 },
};

/////////////////////  CVBS PAL/NTSC  /////////////////////
static unsigned int gx3201_dve_pal_bdghi[] = {
	0x00010000, 0x00000000, 0x0097D47E, 0x001AE677,
	0x00681a11, 0x00f0a215, 0x26404546, 0x1f639102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6a963f3f, 0x00013F5E, 0x00000000, 0x00000093,
	0x00000000, 0x00000000, 0x000000d0, 0x00000000,
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

static unsigned int gx3201_dve_pal_m[] = {
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
};

static unsigned int gx3201_dve_pal_n[] = {
	0x00010002, 0x00000000, 0x0097d47e, 0x001AE677,
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

static unsigned int gx3201_dve_pal_nc[] = {
	0x00010003, 0x00000000, 0x0097da7e, 0x001AE677,
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

static unsigned int gx3201_dve_ntsc_m[] = {
	0x00010004, 0x00000000, 0x0090d47e, 0x0017a65d,//0x001A2671,
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

static unsigned int gx3201_dve_ntsc_443[] = {
	0x00010005, 0x00000000, 0x0090c67e, 0x001A2671,
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


/////////////////////  YPbPr/HDMI  /////////////////////
static unsigned int gx3201_dve_ycbcr_480i[] = {
	0x14010004, 0x00000000, 0x0097D47E, 0x0015c64f,
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

static unsigned int gx3201_dve_ycbcr_576i[] = {
	0x14010000, 0x00000000, 0x0097D47E, 0x00182661,//0x001BE67F
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
	0x00004AC4, 0x0000b136, 0x000a726e, 0x0009ba70,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0xC00001c7
};

static unsigned int gx3201_dve_ypbpr_480p[] = {
	0x14010008, 0x00000000, 0x0097D43F, 0x00162651,
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

static unsigned int gx3201_dve_ypbpr_576p[] = {
	0x14010009, 0x00000000, 0x0097D43F, 0x002186ac,//188666,//0x001B867C
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
	//0x00004AC4, 0x0000624b, 0x00000000, 0x00000000,
	0x00004AC4, 0x00001241, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};


static unsigned int gx3201_dve_ypbpr_720p_50hz[] = {
	0x1405000D, 0x00000000, 0x0097D40A, 0x0039ebcf,//42cc16,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x01803F37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00003632, 0x00a584B0, 0x0207e671, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x002E87BB, 0x00002093,
	0x04E9EF77, 0x00001096, 0x04000024, 0x00025625,
	//0x00004AC4, 0x0000C2E7, 0x00000000, 0x00000000,
	0x00004AC4, 0x0000fe2d, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3201_dve_ypbpr_720p_60hz[] = {
	0x1405000D, 0x00000000, 0x0097D40A, 0x0039ebcf,//42cc16,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x01803F37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00003632, 0x00a584B0, 0x0207e62C, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x002E8671, 0x00002093,
	0x04E9ECE3, 0x00001096, 0x04000024, 0x00025625,
	//0x00004AC4, 0x0000C2E7, 0x00000000, 0x00000000,
	0x00004AC4, 0x0000fe2d, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3201_dve_ypbpr_1080i_50hz[] = {
	0x14050006, 0x00000000, 0x0097D40A, 0x0028f047,//2e1072,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x01803F37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00003632, 0x00aC04B0, 0x0207e83E, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0045Aa4f, 0x00002093,
	0x0AF5F49F, 0x00001096, 0x04000024, 0x00025625,
	0x00004AC4, 0x0000b231, 0x00124c64, 0x00119464,
	//0x00004AC4, 0x00009A2E, 0x00123461, 0x00119464,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3201_dve_ypbpr_1080i_60hz[] = {
	0x14050006, 0x00000000, 0x0097D40A, 0x0028f047,//2e1072,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x01803F37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00003632, 0x00aC04B0, 0x0207e83E, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0045A897, 0x00002093,
	0x0AF5F12F, 0x00001096, 0x04000024, 0x00025625,
	//0x00004AC4, 0x00009A2E, 0x00123461, 0x00119464,
	0x00004AC4, 0x0000b231, 0x00124c64, 0x00119464,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3201_dve_ypbpr_1080p_50hz[] = {
	0x1405000E, 0x00000000, 0x0097D40A, 0x00146823,//190848,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x01803F37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00001914, 0x008B04B0, 0x0081F436, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0045F527, 0x00002093,
	0x02C58a4f, 0x00001096, 0x04000024, 0x00025625,
	//0x00004AC4, 0x0001445F, 0x00000000, 0x00000000,
	0x00004AC4, 0x00015c62, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

static unsigned int gx3201_dve_ypbpr_1080p_60hz[] = {
	0x1405000E, 0x00000000, 0x0097D40A, 0x00146823,//190848,
	0x00781E13, 0x00B0A610, 0x25D05940, 0x1EC39102,
	0x20885218, 0x2010F094, 0x00050000, 0x00000000,
	0x6B953F3F, 0x01803F37, 0x00000000, 0x0000009d,
	0x00000000, 0x00000000, 0x0000009d, 0x00000000,
	0x00001914, 0x008B04B0, 0x0081F436, 0x06400000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0xaaaaaaaa, 0x000000A9,
	0x00000000, 0x0001FFFE, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x0045F44B, 0x00002093,
	0x02C58897, 0x00001096, 0x04000024, 0x00025625,
	//0x00004AC4, 0x0001445F, 0x00000000, 0x00000000,
	0x00004AC4, 0x00015c62, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x10002083, 0x40866082,
	0xA000B083, 0xD081F086, 0x10040100, 0x08a8f20d,
	0x08a8f20d, 0x08a8f20d, 0x1705c170, 0x000001c7
};

//////////////////////// TO CO. ////////////////////////

/* Functions ---------------------------------------------------------- */
void gx3201_videoout_out1_enable(unsigned int enable)
{
	if (enable)
		*(volatile int *)(&gx3201_vout1_reg->tv_ctrl) &= ~(1<<4);
	else
		*(volatile int *)(&gx3201_vout1_reg->tv_ctrl) |= (1<<4);
}

static void gx3201_videoout_mode_config(volatile struct gx3201_videoout_reg *reg, unsigned int *data)
{
	unsigned char* reg_address = (unsigned char *)reg;
	unsigned int i = 0,j = 0;

	for (i = 0,j = 0; i <= 0xec; i += 4,j++)
	{
		REG_SET_VAL((volatile unsigned int *)(reg_address + i), data[j]);
	}
}

static int gx3201_videoout_pal_config(volatile struct gx3201_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {

	case GXAV_VOUT_PAL:
		gx3201_videoout_mode_config(reg, gx3201_dve_pal_bdghi);
		break;
	case GXAV_VOUT_PAL_N:
		gx3201_videoout_mode_config(reg, gx3201_dve_pal_n);
		break;
	case GXAV_VOUT_PAL_NC:
		gx3201_videoout_mode_config(reg, gx3201_dve_pal_nc);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in pal\n");
		return -1;
	}

	return 0;
}

static int gx3201_videoout_ntsc_config(volatile struct gx3201_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {

	case GXAV_VOUT_NTSC_M:
		gx3201_videoout_mode_config(reg, gx3201_dve_ntsc_m);
		break;
	case GXAV_VOUT_NTSC_443:
		gx3201_videoout_mode_config(reg, gx3201_dve_ntsc_443);
		break;
	case GXAV_VOUT_PAL_M:
		gx3201_videoout_mode_config(reg, gx3201_dve_pal_m);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in ntsc\n");
		return -1;
	}

	return 0;
}

static int gx3201_videoout_yuv_config(volatile struct gx3201_videoout_reg *reg, GxVideoOutProperty_Mode mode)
{
	switch(mode) {

	case GXAV_VOUT_480I:
		gx3201_videoout_mode_config(reg, gx3201_dve_ycbcr_480i);
		break;
	case GXAV_VOUT_576I:
		gx3201_videoout_mode_config(reg, gx3201_dve_ycbcr_576i);
		break;
	case GXAV_VOUT_480P:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_480p);
		break;
	case GXAV_VOUT_576P:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_576p);
		break;
	case GXAV_VOUT_720P_50HZ:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_720p_50hz);
		break;
	case GXAV_VOUT_720P_60HZ:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_720p_60hz);
		break;
	case GXAV_VOUT_1080I_50HZ:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_1080i_50hz);
		break;
	case GXAV_VOUT_1080I_60HZ:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_1080i_60hz);
		break;
	case GXAV_VOUT_1080P_50HZ:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_1080p_50hz);
		break;
	case GXAV_VOUT_1080P_60HZ:
		gx3201_videoout_mode_config(reg, gx3201_dve_ypbpr_1080p_60hz);
		break;
	default:
		VIDEOOUT_PRINTF("unsupport dve in yuv\n");
		return -1;
	}

	return 0;
}

static struct reg_iface* gx3201_vout_get_riface(int select)
{
	int i;

	for (i = 0; i< GX_ARRAY_SIZE(gx3201_valid_iface); i++) {
		if (select == gx3201_valid_iface[i].select)
			break;
	}
	if (i == GX_ARRAY_SIZE(gx3201_valid_iface)) {
		VIDEOOUT_PRINTF("select is invalid!\n");
		return NULL;
	}

	return &gx3201_valid_iface[i];
}

static struct vout_interface* gx3201_get_vout_interface(GxVideoOutProperty_Interface iface)
{
	int i;

	for (i = 0; i< GX_ARRAY_SIZE(gx3201_voutinfo.interfaces); i++) {
		if (iface == gx3201_voutinfo.interfaces[i].iface)
			return &gx3201_voutinfo.interfaces[i];
	}

	return NULL;
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

static int vout_get_src_by_interface(struct gxav_videoout *videoout, GxVideoOutProperty_Interface iface)
{
	int ret;
	int dac, src = 0;
	struct reg_iface* riface = gx3201_vout_get_riface(videoout->select);

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

static volatile struct gx3201_videoout_reg* vout_get_reg_by_interface(struct gxav_videoout *videoout, GxVideoOutProperty_Interface iface)
{
	int src;
	volatile struct gx3201_videoout_reg *reg = NULL;

	src = vout_get_src_by_interface(videoout, iface);
	if (src == ID_VPU)
		reg = gx3201_vout0_reg;
	else
		reg = gx3201_vout1_reg;

	return reg;
}

/*
 *  config dac full case and mode, etc
 *  the tv_ctrl register may be change when config the resolution array
 */
static void gx3201_vout_config_dac_mode_case(struct vout_interface *viface)
{
	struct vout_interface* viface_yuv;
	volatile struct gx3201_videoout_reg *reg;
	GxVideoOutProperty_Mode mode;
	struct reg_iface* riface = gx3201_vout_get_riface(viface->vout->select);

	if (GX3201_HAVE_VPU_SRC(viface->vout->select)) {
		/* set dac mode generic */
		DVE_SET_DAC_OUT_MODE(gx3201_vout0_reg, riface->vpu_dac_mode);

		/* set dac full case */
		DVE_SET_DAC_FULLCASE_CHANGE(gx3201_vout0_reg, riface->vpu_dac_case);
	}
	if (GX3201_HAVE_SVPU_SRC(viface->vout->select)) {
		/* set dac mode generic */
		DVE_SET_DAC_OUT_MODE(gx3201_vout1_reg, riface->svpu_dac_mode);

		/* set dac full case */
		DVE_SET_DAC_FULLCASE_CHANGE(gx3201_vout1_reg, riface->svpu_dac_case);
	}

	if (viface->vout->select & GXAV_VOUT_YUV) {
		reg = vout_get_reg_by_interface(viface->vout, GXAV_VOUT_YUV);
		viface_yuv = gx3201_get_vout_interface(GXAV_VOUT_YUV);
		mode = viface_yuv->ops->get_mode(viface_yuv);

		switch (mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
			//Y_Cb_Cr_CVBS : YCBCR_480I、YCBCR_576I
			DVE_SET_DAC_OUT_MODE(reg, (Y_Cb_Cr_CVBS));
			break;
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
			//Y_Pb_Pr_Y : YPBPR_480P、YPBPR_576P
			DVE_SET_DAC_OUT_MODE(reg, (Y_Pb_Pr_Y));
			break;
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			//HDTV_R_G_B_R :  1080I、720P、1080P
			DVE_SET_DAC_OUT_MODE(reg, (HDTV_R_G_B_R));
			break;
		default:
			break;
		}
	}

}

static int gx3201_vout_set_dve(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	volatile struct gx3201_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);

	if(!reg)
		return -1;

	switch(mode)
	{
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
		gx3201_videoout_pal_config(reg, mode);
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
		gx3201_videoout_ntsc_config(reg, mode);
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
		gx3201_videoout_yuv_config(reg, mode);
		break;
	default:
		VIDEOOUT_PRINTF("un-supported.\n");
		return -1;
	}

	if (viface->iface == GXAV_VOUT_SCART) {
		gxav_clock_multiplex_pinsel(PIN_TYPE_VIDEOOUT_SCART);

		switch (mode) {
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

	int id = vout_get_src_by_interface(viface->vout, viface->iface);
	gx3201_voutinfo.src[id].mode = mode;
	viface->mode = mode;
	gx3201_vout_config_dac_mode_case(viface);

	{
		clock_params params;
		params.vout.src  = id;
		params.vout.mode = mode;
		gxav_clock_setclock(MODULE_TYPE_VOUT, &params);
	}

	//VPU hot-reset
	gxav_clock_hot_rst_set(MODULE_TYPE_VPU);
	gxav_clock_hot_rst_clr(MODULE_TYPE_VPU);

	return 0;
}

static int gx3201_videoout_ioremap(void)
{
	unsigned char *vout0_mapped_addr = NULL;
	unsigned char *vout1_mapped_addr = NULL;

	// VOUT0
	if (0 == gx_request_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3201_videoout_reg))) {
		VIDEOOUT_PRINTF("request_mem_region failed");
		return -1;
	}
	vout0_mapped_addr = gx_ioremap(TV_VOUT0_BASE_ADDR, sizeof(struct gx3201_videoout_reg));
	if (!vout0_mapped_addr) {
		VIDEOOUT_PRINTF("ioremap failed.\n");
		goto err_vout0_ioremap;
	}

	// VOUT1
	if (0 == gx_request_mem_region(TV_VOUT1_BASE_ADDR, sizeof(struct gx3201_videoout_reg))) {
		VIDEOOUT_PRINTF("request_mem_region failed");
		goto err_vout1_request_mem;
	}
	vout1_mapped_addr = gx_ioremap(TV_VOUT1_BASE_ADDR, sizeof(struct gx3201_videoout_reg));
	if (!vout1_mapped_addr) {
		VIDEOOUT_PRINTF("ioremap failed.\n");
		goto err_vout1_ioremap;
	}

	gx3201_vout0_reg = (struct gx3201_videoout_reg *)vout0_mapped_addr;
	gx3201_vout1_reg = (struct gx3201_videoout_reg *)vout1_mapped_addr;
	VIDEOOUT_DBG("[gx3201_vout0_reg=%x][gx3201_vout1_reg=%x]\n", (int)gx3201_vout0_reg,(int)gx3201_vout1_reg);

	return 0;

err_vout0_ioremap:
	gx_release_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3201_videoout_reg));
err_vout1_request_mem:
	gx_iounmap(gx3201_vout0_reg);
	gx3201_vout0_reg= NULL;

	gx_release_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3201_videoout_reg));
err_vout1_ioremap:
	gx_iounmap(gx3201_vout0_reg);
	gx3201_vout0_reg= NULL;

	gx_release_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3201_videoout_reg));
	gx_release_mem_region(TV_VOUT1_BASE_ADDR, sizeof(struct gx3201_videoout_reg));

	return -1;
}

static int gx3201_videoout_iounmap(void)
{
	gx_iounmap(gx3201_vout0_reg);
	gx_iounmap(gx3201_vout1_reg);
	gx3201_vout0_reg = NULL;
	gx3201_vout1_reg = NULL;

	gx_release_mem_region(TV_VOUT0_BASE_ADDR, sizeof(struct gx3201_videoout_reg));
	gx_release_mem_region(TV_VOUT1_BASE_ADDR, sizeof(struct gx3201_videoout_reg));

	return 0;
}
static int gx3201_vout_init(void)
{
	if (gx3201_videoout_ioremap() < 0) {
		return -1;
	}

	register_vout_interface();
	VIDEOOUT_DBG("gx3201_vout_init\n");

	return 0;
}

static int gx3201_vout_uninit(void)
{
	gx3201_videoout_iounmap();

	VIDEOOUT_DBG("gx3201_vout_uninit\n");

	return 0;
}

static int gx3201_vout_open(struct gxav_videoout *videoout)
{
	int i;

	for (i = 0; i< GX_ARRAY_SIZE(gx3201_voutinfo.interfaces); i++) {
		gx3201_voutinfo.interfaces[i].brightness = 50;
		gx3201_voutinfo.interfaces[i].contrast   = 50;
		gx3201_voutinfo.interfaces[i].saturation = 50;
		gx3201_voutinfo.interfaces[i].mode = 0;
	}

	VIDEOOUT_DBG("gx3201_vout_open\n");

	return 0;
}

static int gx3201_vout_close(struct gxav_videoout *videoout)
{
	return 0;
}

static void gx3201_vout_insert_scan_chance(int mode)
{
	while(gx3201vpu_get_scan_line()!=0);
	switch (mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_576I:
		while(gx3201vpu_get_scan_line()<270);
		break;
	case GXAV_VOUT_576P:
		while(gx3201vpu_get_scan_line()<560);
		break;
	case GXAV_VOUT_480P:
		while(gx3201vpu_get_scan_line()<470);
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
		while(gx3201vpu_get_scan_line()<230);
		break;
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		while(gx3201vpu_get_scan_line()<710);
		break;
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
		while(gx3201vpu_get_scan_line()<530);
		break;
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ:
		while(gx3201vpu_get_scan_line()<1060);
		break;
	default:
		break;
	}
}

static int gx3201_vout_config(struct gxav_videoout *videoout, GxVideoOutProperty_OutputConfig *config)
{
	return 0;
}


static int gx3201_vout_power_off(struct gxav_videoout *videoout, int selection)
{
	int i;
	struct reg_iface* riface = gx3201_vout_get_riface(selection);

	if (!riface)
		return -1;
	GX3201_CHECK_VOUT_IFACE(videoout->select, selection);

	for (i = 0; i < IFACE_ID_MAX; i++) {
		if (riface->ifaces_dac[i].iface & selection)
			gxav_vpu_DACEnable(riface->ifaces_dac[i].dac, 0);
	}


	if (selection & GXAV_VOUT_HDMI) {
		unsigned char v = gxav_hdmi_read(0);
		v = 0x40 | (v & 0xf);
		gxav_hdmi_write(0, v);
	}

	return 0;
}

static int gx3201_vout_power_on(struct gxav_videoout *videoout, int selection)
{
	int i;
	struct reg_iface* riface = gx3201_vout_get_riface(selection);

	if (!riface)
		return -1;
	GX3201_CHECK_VOUT_IFACE(videoout->select, selection);

	for (i = 0; i < IFACE_ID_MAX; i++) {
		if (riface->ifaces_dac[i].iface & selection)
			gxav_vpu_DACEnable(riface->ifaces_dac[i].dac, 1);
	}

	if (selection & GXAV_VOUT_HDMI) {
		unsigned char v = gxav_hdmi_read(0);
		v = 0x80 | (v & 0xf);
		gxav_hdmi_write(0, v);
	}

	return 0;
}

static void gx3201_after_set_mode(struct vout_interface *viface)
{
	unsigned int b, c, s;

	if (viface->iface == GXAV_VOUT_YUV ||
			viface->iface == GXAV_VOUT_HDMI) {
		/*huanglei start
		  R = c0*Y + c1*CR + c2*CB + C3;
		  G = c4*Y + c5*CR + c6*CB + C7;
		  B = c8*Y + c9*CR + c10*CB + C11;
		  */

		c0  = (gxav_hdmi_read(0x18)<<8) | gxav_hdmi_read(0x19);
		c4  = (gxav_hdmi_read(0x20)<<8) | gxav_hdmi_read(0x21);
		c8  = (gxav_hdmi_read(0X28)<<8) | gxav_hdmi_read(0X29);
		c1  = (gxav_hdmi_read(0x1a)<<8) | gxav_hdmi_read(0x1b);
		c5  = (gxav_hdmi_read(0x22)<<8) | gxav_hdmi_read(0x23);
		c9  = (gxav_hdmi_read(0X2a)<<8) | gxav_hdmi_read(0x2b);
		c2  = (gxav_hdmi_read(0x1C)<<8) | gxav_hdmi_read(0x1d);
		c6  = (gxav_hdmi_read(0x24)<<8) | gxav_hdmi_read(0x25);
		c10 = (gxav_hdmi_read(0x2c)<<8) | gxav_hdmi_read(0x2d);
		c3	=	(gxav_hdmi_read(0x1e)<<8)	|	gxav_hdmi_read(0x1f);
		c7	=	(gxav_hdmi_read(0x26)<<8)	|	gxav_hdmi_read(0x27);
		c11	=	(gxav_hdmi_read(0x2e)<<8)	|	gxav_hdmi_read(0x2f);
	}

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);
	viface->ops->set_brightness(viface, b);
	viface->ops->set_saturation(viface, c);
	viface->ops->set_saturation(viface, s);
}

int gx3201_vout_patch_interface_p_active_off(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	int ret = 0;

#define TV_ACTIVE_OFFSET (6)

	unsigned int tv_active_off = 0;
	volatile struct gx3201_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);

	if (IS_NULL(reg))
		return -1;

	tv_active_off = REG_GET_FIELD(&(reg->tv_active),
			mTV_ACTIVE_OFF,
			bTV_ACTIVE_OFF);
	switch(mode)
	{
	case GXAV_VOUT_720P_50HZ:
	case GXAV_VOUT_720P_60HZ:
		DVE_SET_ACTIVE_OFF(reg, tv_active_off + TV_ACTIVE_OFFSET);
		break;
	default:
		break;
	}

	return ret;
}

static int gx3201_get_svpu_ctrl_mode(GxVideoOutProperty_Mode vpu_mode)
{
	int svpu_mode = 0;

	switch (vpu_mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
	case GXAV_VOUT_576I:
		svpu_mode = 0;
		break;
	case GXAV_VOUT_576P:
		svpu_mode = 1;
		break;
	case GXAV_VOUT_720P_50HZ:
		svpu_mode = 2;
		break;
	case GXAV_VOUT_1080I_50HZ:
		svpu_mode = 3;
		break;
	case GXAV_VOUT_1080P_50HZ:
		svpu_mode = 4;
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
	case GXAV_VOUT_480I:
		svpu_mode = 8;
		break;
	case GXAV_VOUT_480P:
		svpu_mode = 9;
		break;
	case GXAV_VOUT_720P_60HZ:
		svpu_mode = 10;
		break;
	case GXAV_VOUT_1080I_60HZ:
		svpu_mode = 11;
		break;
	case GXAV_VOUT_1080P_60HZ:
		svpu_mode = 12;
		break;
	default:
		break;
	}

	return svpu_mode;
}

static int gx3201_vout_set_resolution(struct gxav_videoout *videoout, GxVideoOutProperty_Resolution *resolution)
{
	unsigned int last_mode = 0;
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(resolution->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, resolution->iface);
	viface = gx3201_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(viface);

	last_mode = viface->ops->get_mode(viface);
	if (last_mode == resolution->mode)
		return 0;

	viface->ops->set_mode(viface, resolution->mode);

	gx3201_after_set_mode(viface);

	return 0;
}

static int gx3201_vout_set_interface(struct gxav_videoout *videoout, GxVideoOutProperty_OutputSelect *output)
{
#define VIFACE_ASSIGN_VOUT(iface)\
	do {\
		if (output->selection & iface) {\
			viface = gx3201_get_vout_interface(iface);\
			viface->vout = videoout;\
		}\
	} while (0)
	struct vout_interface* viface = NULL;
	struct reg_iface* riface = gx3201_vout_get_riface(output->selection);

	if (!riface)
		return -1;
	if (videoout->select == output->selection)
		return 0;

	videoout->select = 0;
	gxav_vpu_DACEnable(VPU_DAC_ALL, 0);

	videoout->select = output->selection;
	gx3201_vout_power_on(videoout, output->selection);

	gxav_vpu_DACSource(VPU_DAC_0, riface->dac_src0);
	gxav_vpu_DACSource(VPU_DAC_1, riface->dac_src1);
	gxav_vpu_DACSource(VPU_DAC_2, riface->dac_src2);
	gxav_vpu_DACSource(VPU_DAC_3, riface->dac_src3);

	VIFACE_ASSIGN_VOUT(GXAV_VOUT_RCA);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_RCA1);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_YUV);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_SCART);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_SVIDEO);
	VIFACE_ASSIGN_VOUT(GXAV_VOUT_HDMI);

	{
		/*
		 * config the vpu resoluton first,
		 * if not ,the svpu(RCA/RCA1/SCART/SVIDEO) will not work well
		 */
		GxVideoOutProperty_Resolution resolution;

		if (output->selection & GXAV_VOUT_YUV) {

			resolution.iface = GXAV_VOUT_YUV;
			resolution.mode = GXAV_VOUT_1080I_50HZ;
		} else if (output->selection & GXAV_VOUT_HDMI) {

			resolution.iface = GXAV_VOUT_HDMI;
			resolution.mode = GXAV_VOUT_1080I_50HZ;
		} else {
			return -1;
		}

		gx3201_vout_set_resolution(videoout, &resolution);
	}

	return 0;
}



int gx3201_vout_set_default(struct gxav_videoout *videoout, GxVideoOutProperty_OutDefault *outdefault)
{
	volatile struct gx3201_videoout_reg *reg = vout_get_reg_by_interface(videoout, outdefault->iface);

	if (!reg)
		return -1;

	if (outdefault->enable)
		REG_SET_BIT(&(reg->lcd_ctrl), 7);
	else
		REG_CLR_BIT(&(reg->lcd_ctrl), 7);

	return 0;
}

static int gx3201_vout_set_brightness(struct gxav_videoout *videoout, GxVideoOutProperty_Brightness *brightness)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(brightness->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, brightness->iface);
	viface = gx3201_get_vout_interface(brightness->iface);
	CHECK_VOUT_INTERFACE(viface);

	viface->ops->set_brightness(viface, brightness->value);

	return 0;
}

static int gx3201_vout_set_saturation(struct gxav_videoout *videoout, GxVideoOutProperty_Saturation *saturation)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(saturation->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, saturation->iface);
	viface = gx3201_get_vout_interface(saturation->iface);
	CHECK_VOUT_INTERFACE(viface);

	viface->ops->set_saturation(viface, saturation->value);

	return 0;
}

static int gx3201_vout_set_contrast(struct gxav_videoout *videoout, GxVideoOutProperty_Contrast *contrast)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(contrast->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, contrast->iface);
	viface = gx3201_get_vout_interface(contrast->iface);
	CHECK_VOUT_INTERFACE(viface);

	viface->ops->set_contrast(viface, contrast->value);

	return 0;
}

static int gx3201_vout_set_aspratio(struct gxav_videoout *videoout, GxVideoOutProperty_AspectRatio *aspect_ratio)
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
	GX3201_DVE_CHECK_RET(ret);

	return 0;
}

static int gx3201_vout_set_tvscreen(struct gxav_videoout *videoout, GxVideoOutProperty_TvScreen *TvScreen)
{
	GxVpuProperty_TvScreen Screen;
	int ret = -1;

	if (TvScreen->screen == TV_SCREEN_4X3) {
		Screen.Screen= SCREEN_4X3;
	}
	else if (TvScreen->screen == TV_SCREEN_16X9) {
		Screen.Screen= SCREEN_16X9;
	}
	else {
		return -1;
	}

	ret = gxav_vpu_SetTvScreen(&Screen);
	GX3201_DVE_CHECK_RET(ret);

	return 0;
}

int gx3201_vout_get_resolution(struct gxav_videoout *videoout, GxVideoOutProperty_Resolution *resolution)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(resolution->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, resolution->iface);
	viface = gx3201_get_vout_interface(resolution->iface);
	CHECK_VOUT_INTERFACE(viface);

	resolution->mode = viface->ops->get_mode(viface);

	return 0;
}

static int gx3201_vout_get_dvemode(struct gxav_videoout *videoout, struct vout_dvemode *dvemode)
{
	int id = dvemode->id;

	if (id != ID_VPU && id != ID_SVPU)
		return -1;

	dvemode->mode = gx3201_voutinfo.src[id].mode;

	return 0;
}

int gx3201_vout_get_brightness(struct gxav_videoout *videoout, GxVideoOutProperty_Brightness *brightness)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(brightness->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, brightness->iface);
	viface = gx3201_get_vout_interface(brightness->iface);
	CHECK_VOUT_INTERFACE(viface);

	brightness->value = viface->ops->get_brightness(viface);

	return 0;
}

int gx3201_vout_get_contrast(struct gxav_videoout *videoout, GxVideoOutProperty_Contrast *contrast)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(contrast->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, contrast->iface);
	viface = gx3201_get_vout_interface(contrast->iface);
	CHECK_VOUT_INTERFACE(viface);

	contrast->value = viface->ops->get_contrast(viface);

	return 0;
}

int gx3201_vout_get_saturation(struct gxav_videoout *videoout, GxVideoOutProperty_Saturation *saturation)
{
	struct vout_interface* viface = NULL;

	GX3201_CHECK_IFACE(saturation->iface);
	GX3201_CHECK_VOUT_IFACE(videoout->select, saturation->iface);
	viface = gx3201_get_vout_interface(saturation->iface);
	CHECK_VOUT_INTERFACE(viface);

	saturation->value = viface->ops->get_saturation(viface);

	return 0;
}

static int gx3201_vout_play_cc(struct gxav_videoout *videoout, struct vout_ccinfo *ccinfo)
{
	char byte0, byte1;
	short cc_type, cc_data;
	unsigned short cc_line_top = 0, cc_line_bottom = 0;
	struct vout_dvemode svpu_mode;
	volatile struct gx3201_videoout_reg *reg = gx3201_vout1_reg;

	if (!GX3201_HAVE_SVPU_SRC(videoout->select))
		return -1;

	svpu_mode.id = ID_SVPU;
	gx3201_vout_get_dvemode(videoout, &svpu_mode);
	cc_type = ccinfo->cc_type;
	cc_data = ccinfo->cc_data;

	switch(svpu_mode.mode) {
	case GXAV_VOUT_PAL:
	case GXAV_VOUT_PAL_N:
	case GXAV_VOUT_PAL_NC:
		if (cc_type==0x00 || cc_type==0x10) {//top
			cc_line_top = 21;
		}
		else if (cc_type==0x01 || cc_type==0x11) {
			cc_line_bottom = 284;
		}
		break;
	case GXAV_VOUT_PAL_M:
	case GXAV_VOUT_NTSC_M:
	case GXAV_VOUT_NTSC_443:
		if (cc_type==0x00 || cc_type==0x10) {//top
			cc_line_top = 17;
		}
		else if (cc_type==0x01 || cc_type==0x11) {
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

	if (cc_line_top) {
		DVE_SET_CC_LINE_TOP(reg, cc_line_top);
		DVE_SET_CC_DATA_TOP(reg, cc_data);
		DVE_SET_CC_TOP_EN(reg);
		VIDEOOUT_PRINTF("videoout: cc: top, line = 0x%x, data = %x\n", cc_line_top, cc_data);
	}
	if (cc_line_bottom) {
		DVE_SET_CC_LINE_BOTTOM(reg, cc_line_bottom);
		DVE_SET_CC_DATA_BOTTOM(reg, cc_data);
		DVE_SET_CC_BOTTOM_EN(reg);
		VIDEOOUT_PRINTF("play cc: bottom, line = 0x%x, data = %x\n", cc_line_bottom, cc_data);
	}

	return 0;
}

static int gx3201_vout_enable_hdcp(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpEnable* auth)
{
	struct vout_interface* viface = gx3201_get_vout_interface(GXAV_VOUT_HDMI);
	CHECK_VOUT_INTERFACE(viface);

	vout->hdcp_enable = auth->enable;
	if (auth->enable == 1)
		return 0;//return gxav_hdmi_hdcp_enable_auth(viface->ops->get_mode(viface));
	else
		return gxav_hdmi_hdcp_disable_auth();
}

static int gx3201_vout_start_hdcp(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpEnable* auth)
{
	return 0;//gxav_hdmi_hdcp_start_auth();
}

static int gx3201_vout_hdmi_status(struct gxav_videoout* vout, GxVideoOutProperty_OutHdmiStatus *status)
{
	if (gxav_hdmi_detect_hotplug() == 1)
		status->status = HDMI_PLUG_IN;
	else
		status->status = HDMI_PLUG_OUT;
	return 0;
}

static int gx3201_vout_get_hdmi_version(struct gxav_videoout *vout, GxVideoOutProperty_HdmiVersion *version)
{
	gxav_hdmi_get_version(version);
	return 0;
}

static int gx3201_vout_edid_info(struct gxav_videoout* vout, GxVideoOutProperty_EdidInfo *edid_info)
{
	return gxav_hdmi_read_edid((struct videoout_hdmi_edid*)edid_info);
}

static int gx3201_vout_set_macrovision(struct gxav_videoout* vout, GxVideoOutProperty_Macrovision *macrovision)
{
	if (macrovision->mode == GX_MACROVISION_MODE_TYPE0) {
		REG_SET_BIT(0xa48040dc, 12);
		REG_SET_BIT(0xa49040dc, 12);
	}
	else {
		REG_CLR_BIT(0xa48040dc, 12);
		REG_CLR_BIT(0xa49040dc, 12);
		REG_SET_FIELD(0xa48040cc, 0xff, 0, 0);
		REG_SET_FIELD(0xa49040cc, 0xff, 0, 0);
	}

	return 0;
}

static int gx3201_vpu_iface_set_brightness(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale0 = 160;
	unsigned int scale3 = 63;
	unsigned int brightness_value = 0;
	unsigned int luma_shift = 0;
	unsigned int b, c;
	volatile struct gx3201_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	if (GX3201_IS_SVPU_SRC(viface->iface)) {
		scale0 = scale[mode][0];
		scale3 = scale[mode][3];
	}

	viface->brightness = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);

	brightness_value = (scale0 * ((c << 1) * (b << 1))) / (100 * 100);
	brightness_value = brightness_value >= 370 ? 370 : brightness_value;
	brightness_value = brightness_value <= 16  ? 16  : brightness_value;

	if(brightness_value > scale0){
		luma_shift = scale3 - 6;
	}
	else {
		luma_shift = scale3;
	}

	gx3201_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_Y_SCALE0(reg, brightness_value);
	DVE_SET_YUV_TRAN_Y_SCALE1(reg, 0);
	DVE_SET_YUV_TRAN_Y_SCALE2(reg, 0);
	DVE_SET_ADJUST_Y_SHIFT(reg,luma_shift);

	return 0;
}

static int gx3201_vpu_iface_set_saturation(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale1 = 157, scale2 = 157;
	unsigned int saturation_value1, saturation_value2;
	unsigned int c, s;
	volatile struct gx3201_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	if (GX3201_IS_SVPU_SRC(viface->iface)) {
		scale1 = scale[mode][1];
		scale2 = scale[mode][2];
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

	gx3201_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_U_SCALE1(reg, saturation_value1);
	DVE_SET_YUV_TRAN_V_SCALE2(reg, saturation_value2);

	return 0;
}

static int gx3201_vpu_iface_set_contrast(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale0 = 160, scale1 = 157, scale2 = 157;
	unsigned int scale3 = 63;
	unsigned int luma_shift = 0;
	unsigned int brightness_value = 0;
	unsigned int saturation_value1, saturation_value2;
	unsigned int b, c, s;
	volatile struct gx3201_videoout_reg *reg = vout_get_reg_by_interface(viface->vout, viface->iface);
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->contrast = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	if (GX3201_IS_SVPU_SRC(viface->iface)) {
		scale0 = scale[mode][0];
		scale1 = scale[mode][1];
		scale2 = scale[mode][2];
		scale3 = scale[mode][3];
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
	gx3201_vout_insert_scan_chance(mode);

	DVE_SET_YUV_TRAN_Y_SCALE0(reg, brightness_value);
	DVE_SET_YUV_TRAN_U_SCALE1(reg, saturation_value1);
	DVE_SET_YUV_TRAN_V_SCALE2(reg, saturation_value2);
	DVE_SET_ADJUST_Y_SHIFT(reg,luma_shift);

	return 0;
}

static int gx3201_iface_set_mode(struct vout_interface *viface, GxVideoOutProperty_Mode mode)
{
	unsigned int last_mode = 0;
	struct gxav_videoout* videoout = viface->vout;
	struct vout_interface *svpu_viface = NULL;
	int svpu_iface = 0;
	int svpu_ctrl_mode = 0;
	int vpu_mode = 0, svpu_mode = 0;
	struct vout_interface *viface_vpu = NULL;
	struct vout_interface *viface_svpu = NULL;

	if (GX3201_IS_SVPU_SRC(viface->iface)) {

		viface_svpu = viface;
		if (viface_svpu->vout->select & GXAV_VOUT_HDMI)
			viface_vpu = gx3201_get_vout_interface(GXAV_VOUT_HDMI);
		else
			viface_vpu = gx3201_get_vout_interface(GXAV_VOUT_YUV);
		CHECK_VOUT_INTERFACE(viface_vpu);

		vpu_mode = viface_vpu->ops->get_mode(viface_vpu);
		svpu_mode = mode;
	} else {

		viface_vpu = viface;
		if (viface->vout->select & GXAV_VOUT_RCA)
			svpu_iface = GXAV_VOUT_RCA;
		else if (viface->vout->select & GXAV_VOUT_RCA1)
			svpu_iface = GXAV_VOUT_RCA1;
		else if (viface->vout->select & GXAV_VOUT_SCART)
			svpu_iface = GXAV_VOUT_SCART;
		else if (viface->vout->select & GXAV_VOUT_SVIDEO)
			svpu_iface = GXAV_VOUT_SVIDEO;

		if (svpu_iface > 0) {
			viface_svpu = gx3201_get_vout_interface(svpu_iface);
			CHECK_VOUT_INTERFACE(viface_svpu);
			svpu_mode = viface_svpu->ops->get_mode(viface_svpu);
		}

		vpu_mode = mode;
	}
	last_mode = viface_vpu->ops->get_mode(viface_vpu);

	//在关VPU之前
	Gx3201_disable_svpu();

	gxav_vpu_SetVoutIsready(0);

	/* 设置hdmi黑屏, 否则在配置vpu mode时可能导致hdmi输出花屏 */
	gxav_hdmi_black_enable(1);

	gx3201_vout_insert_scan_chance(last_mode);

	gxav_hdmi_cold_reset_set();

	gx3201_vout_set_dve(viface_vpu, vpu_mode);

	gx3201_vout_patch_interface_p_active_off(viface_vpu, vpu_mode);

	if (svpu_mode > 0 && viface_svpu) {
		svpu_viface = gx3201_get_vout_interface(svpu_iface);
		//GxVideoOutProperty_Mode svpu_mode = IS_P_MODE(mode) ? auto_pal : auto_ntsc;
		//配置SVPU
		svpu_ctrl_mode = gx3201_get_svpu_ctrl_mode(vpu_mode);
		gx3201_vout_set_dve(viface_svpu, svpu_mode);
		Gx3201_enable_svpu(svpu_ctrl_mode);
	}

	VIDEOOUT_DBG("\n++++++++++++++++++hdmi start++++++++++++++++++++++++++++++++\n");

	gxav_hdmi_cold_reset_clr();

	gxav_hdmi_audioout_set(3);
	VIDEOOUT_DBG("+++++++++++resolution mode = %d \n", mode);
	gxav_hdmi_videoout_set(vpu_mode);
	gxav_hdmi_clock_set(vpu_mode);
	VIDEOOUT_DBG("\n+++++++++++++++++++hdmi end+++++++++++++++++++++++++++++++\n");

	VIDEOOUT_DBG("\n+++++++++++++++++++hdmi hdcp auth+++++++++++++++++++++++++++++++\n");
	if (videoout->hdcp_enable)
		;//gxav_hdmi_hdcp_enable_auth(vpu_mode);
	VIDEOOUT_DBG("\n+++++++++++++++++++hdmi hdcp auth+++++++++++++++++++++++++++++++\n");

	gxav_hdmi_black_enable(0);

	gxav_vpu_SetVoutIsready(1);

	return 0;
}

static int gx3201_hdmi_iface_set_brightness(struct vout_interface *viface, unsigned int value)
{
	unsigned int brightness_value_hdmi = 0;
	unsigned int tmp_c,tmp;
	unsigned int b, c, s;
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->brightness = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	brightness_value_hdmi = (GX3201_HDMI_BRIGHTNESS *((c << 1) * (b << 1))) / (100 * 100);

	gx3201_vout_insert_scan_chance(mode);

	//hdmi y
	tmp_c = gxav_hdmi_read(0xd3);
	tmp_c &= ~(0xf8);
	gxav_hdmi_write(0xd3,tmp_c);

	tmp= c0 * brightness_value_hdmi/256;
	if (tmp > 2836)
		tmp = 2836;
	gxav_hdmi_write(0x18, (tmp>> 8) & 0xff);
	gxav_hdmi_write(0x19, (tmp)&0xff);

	tmp = c4 * brightness_value_hdmi/256;
	if (tmp > 2836)
		tmp = 2836;
	gxav_hdmi_write(0x20, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x21, (tmp)&0xff);

	tmp = c8 * brightness_value_hdmi/256;
	if (tmp>2836)
		tmp = 2836;
	gxav_hdmi_write(0x28, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x29, (tmp)&0xff);

	return 0;
}

static int gx3201_hdmi_iface_set_saturation(struct vout_interface *viface, unsigned int value)
{
	unsigned int saturation_value = 0;
	unsigned int  tmp_c,tmp;
	unsigned int signal_flag;
	unsigned int c, s;
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->saturation = value;

	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	gx3201_vout_insert_scan_chance(mode);

	tmp_c = gxav_hdmi_read(0xd3);
	tmp_c &= ~(0xf8);
	gxav_hdmi_write(0xd3,tmp_c);

	saturation_value = (GX3201_HDMI_SATURATION_U*((c << 1)*(s << 1))) / (100 * 100);

	//hdmi cr
	tmp = c1;
	if (tmp >= 4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp= tmp * saturation_value / 256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x1a, (tmp>> 8) & 0xff);
	gxav_hdmi_write(0x1b, (tmp>> 0) & 0xff);

	tmp = c5;
	if (tmp >= 4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp= tmp * saturation_value / 256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x22, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x23, (tmp >> 0) & 0xff);

	tmp = c9;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0X2a, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x2b, (tmp >> 0) & 0xff);

	saturation_value = (GX3201_HDMI_SATURATION_V*((c << 1)*(s << 1))) / (100 * 100);

	//hdmi cb
	tmp = c2;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp * saturation_value / 256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x1C, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x1d, (tmp >> 0) & 0xff);

	tmp = c6;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x24, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x25, (tmp >> 0) & 0xff);

	tmp = c10;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x2c, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x2d, (tmp >> 0) & 0xff);

	tmp = c3;
	if (tmp>=512)
	{
		tmp += 10;
	}
	else
	{
		tmp -= 10;
	}
	if (tmp>=512)
	{
		tmp -= 512;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp	=	tmp*saturation_value/256;
	if (tmp>511)
		tmp = 511;
	tmp += signal_flag*512;
	gxav_hdmi_write(0x1e, (tmp>>8)&0xff);
	gxav_hdmi_write(0x1f, (tmp)&0xff);

	tmp = c7;

	if (tmp>=512)
	{
		tmp += 10;
	}
	else
	{
		tmp -= 10;
	}

	if (tmp>=512)
	{
		tmp -= 512;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp	=	tmp*saturation_value/256;
	if (tmp>511)
		tmp = 511;
	tmp += signal_flag*512;
	gxav_hdmi_write(0x26, (tmp>>8)&0xff);
	gxav_hdmi_write(0x27, (tmp)&0xff);

	tmp = c11 ;
	if (tmp>=512)
	{
		tmp += 10;
	}
	else
	{
		tmp -= 10;
	}
	if (tmp>=512)
	{
		tmp -= 512;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp	=	tmp*saturation_value/256;
	if (tmp>511)
		tmp = 511;
	tmp += signal_flag*512;
	gxav_hdmi_write(0x2e, (tmp>>8)&0xff);
	gxav_hdmi_write(0x2f, (tmp)&0xff);

	return 0;
}

static int gx3201_hdmi_iface_set_contrast(struct vout_interface *viface, unsigned int value)
{
	unsigned int scale0 = 160;
	unsigned int brightness_value = 0;
	unsigned int brightness_value_hdmi = 0;
	unsigned int saturation_value = 0;
	unsigned int tmp_c,tmp,signal_flag;
	unsigned int b, c, s;
	GxVideoOutProperty_Mode mode = viface->ops->get_mode(viface);

	viface->contrast = value;

	b = viface->ops->get_brightness(viface);
	c = viface->ops->get_contrast(viface);
	s = viface->ops->get_saturation(viface);

	brightness_value = (scale0 * ((c << 1) * (b << 1))) / (100 * 100);
	brightness_value = brightness_value >= 370 ? 370 : brightness_value;
	brightness_value = brightness_value <= 16  ? 16  : brightness_value;

	brightness_value_hdmi = (GX3201_HDMI_BRIGHTNESS *((c << 1) * (b << 1))) / (100 * 100);

	gx3201_vout_insert_scan_chance(mode);
	tmp_c = gxav_hdmi_read(0xd3);
	tmp_c &= ~(0xf8);
	gxav_hdmi_write(0xd3,tmp_c);

	//hdmi y
	tmp = c0*brightness_value_hdmi/256;
	if (tmp > 2836)
		tmp = 2836;
	gxav_hdmi_write(0x18, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x19, (tmp >> 0) & 0xff);

	tmp = c4*brightness_value_hdmi/256;
	if (tmp > 2836)
		tmp = 2836;
	gxav_hdmi_write(0x20, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x21, (tmp >> 0) & 0xff);

	tmp = c8*brightness_value_hdmi/256;
	if (tmp > 2836)
		tmp = 2836;
	gxav_hdmi_write(0x28, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x29, (tmp >> 0) & 0xff);

	saturation_value = (GX3201_HDMI_SATURATION_U*((c << 1)*(s << 1))) / (100 * 100);

	//hdmi cr
	tmp = c1;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x1a, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x1b, (tmp >> 0) & 0xff);

	tmp = c5;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x22, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x23, (tmp >> 0) & 0xff);

	tmp = c9;
	if (tmp >= 4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0X2a, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x2b, (tmp >> 0) & 0xff);

	saturation_value = (GX3201_HDMI_SATURATION_V*((c << 1)*(s << 1))) / (100 * 100);
	//hdmi cb
	tmp = c2;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp>4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x1C, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x1d, (tmp >> 0) & 0xff);

	tmp = c6;
	if (tmp>=4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp>4095)
		tmp = 4095;
	tmp += signal_flag*4096;
	gxav_hdmi_write(0x24, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x25, (tmp >> 0) & 0xff);

	tmp = c10;
	if (tmp >= 4096) {
		tmp -= 4096;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp = tmp*saturation_value/256;
	if (tmp > 4095)
		tmp = 4095;
	tmp += signal_flag * 4096;
	gxav_hdmi_write(0x2c, (tmp >> 8) & 0xff);
	gxav_hdmi_write(0x2d, (tmp >> 0) & 0xff);

	tmp = c3;
	if (tmp>=512)
	{
		tmp -= 512;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp	=	tmp*saturation_value/256;
	if (tmp>511)
		tmp = 511;
	tmp += signal_flag*512;
	gxav_hdmi_write(0x1e, (tmp>>8)&0xff);
	gxav_hdmi_write(0x1f, (tmp)&0xff);

	tmp = c7;
	if (tmp>=512)
	{
		tmp -= 512;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp	=	tmp*saturation_value/256;
	if (tmp>511)
		tmp = 511;
	tmp += signal_flag*512;
	gxav_hdmi_write(0x26, (tmp>>8)&0xff);
	gxav_hdmi_write(0x27, (tmp)&0xff);

	tmp = c11;
	if (tmp>=512)
	{
		tmp -= 512;
		signal_flag = 1;
	}
	else
		signal_flag = 0;
	tmp	=	tmp*saturation_value/256;
	if (tmp>511)
		tmp = 511;
	tmp += signal_flag*512;
	gxav_hdmi_write(0x2e, (tmp>>8)&0xff);
	gxav_hdmi_write(0x2f, (tmp)&0xff);

	return 0;
}

GxVideoOutProperty_Mode gx3201_iface_get_mode(struct vout_interface *viface)
{
	return viface->mode;
}
unsigned int gx3201_iface_get_brightness(struct vout_interface *viface)
{
	return viface->brightness;
}
unsigned int gx3201_iface_get_saturation(struct vout_interface *viface)
{
	return viface->saturation;
}
unsigned int gx3201_iface_get_contrast(struct vout_interface *viface)
{
	return viface->contrast;
}

static struct interface_ops gx3201_vpu_interface_ops = {
	.set_mode       = gx3201_iface_set_mode,
	.set_brightness = gx3201_vpu_iface_set_brightness,
	.set_saturation = gx3201_vpu_iface_set_saturation,
	.set_contrast   = gx3201_vpu_iface_set_contrast,
	.get_mode       = gx3201_iface_get_mode,
	.get_brightness = gx3201_iface_get_brightness,
	.get_saturation = gx3201_iface_get_saturation,
	.get_contrast   = gx3201_iface_get_contrast,
};

static struct interface_ops gx3201_hdmi_interface_ops = {
	.set_mode       = gx3201_iface_set_mode,
	.set_brightness = gx3201_hdmi_iface_set_brightness,
	.set_saturation = gx3201_hdmi_iface_set_saturation,
	.set_contrast   = gx3201_hdmi_iface_set_contrast,
	.get_mode       = gx3201_iface_get_mode,
	.get_brightness = gx3201_iface_get_brightness,
	.get_saturation = gx3201_iface_get_saturation,
	.get_contrast   = gx3201_iface_get_contrast,
};

static void register_vout_interface(void)
{
	gx3201_voutinfo.interfaces[0].iface = GXAV_VOUT_RCA;
	gx3201_voutinfo.interfaces[1].iface = GXAV_VOUT_RCA1;
	gx3201_voutinfo.interfaces[2].iface = GXAV_VOUT_YUV;
	gx3201_voutinfo.interfaces[3].iface = GXAV_VOUT_SCART;
	gx3201_voutinfo.interfaces[4].iface = GXAV_VOUT_SVIDEO;
	gx3201_voutinfo.interfaces[5].iface = GXAV_VOUT_HDMI;

	gx3201_voutinfo.interfaces[0].ops =
	gx3201_voutinfo.interfaces[1].ops =
	gx3201_voutinfo.interfaces[2].ops =
	gx3201_voutinfo.interfaces[3].ops =
	gx3201_voutinfo.interfaces[4].ops = &gx3201_vpu_interface_ops;
	gx3201_voutinfo.interfaces[5].ops = &gx3201_hdmi_interface_ops;
}


static struct vout_ops gx3201_vout_ops = {
	.init           = gx3201_vout_init,
	.uninit         = gx3201_vout_uninit,
	.open           = gx3201_vout_open,
	.close          = gx3201_vout_close,
	.config         = gx3201_vout_config,
	.set_interface  = gx3201_vout_set_interface,
	.set_resolution = gx3201_vout_set_resolution,
	.set_brightness = gx3201_vout_set_brightness,
	.set_saturation = gx3201_vout_set_saturation,
	.set_contrast   = gx3201_vout_set_contrast,
	.set_default    = gx3201_vout_set_default,
	.set_aspratio   = gx3201_vout_set_aspratio,
	.set_tvscreen   = gx3201_vout_set_tvscreen,
	.get_resolution = gx3201_vout_get_resolution,
	.get_dvemode    = gx3201_vout_get_dvemode,
	.get_brightness = gx3201_vout_get_brightness,
	.get_contrast   = gx3201_vout_get_contrast,
	.get_saturation = gx3201_vout_get_saturation,
	.play_cc        = gx3201_vout_play_cc,
	.set_PowerOff   = gx3201_vout_power_off,
	.set_PowerOn    = gx3201_vout_power_on,
	.hdcp_enable    = gx3201_vout_enable_hdcp,
	.hdcp_start     = gx3201_vout_start_hdcp,
	.get_hdmi_status= gx3201_vout_hdmi_status,
	.get_edid_info  = gx3201_vout_edid_info,
	.set_macrovision= gx3201_vout_set_macrovision,
	.get_hdmi_version = gx3201_vout_get_hdmi_version,
};

struct gxav_module_ops gx3201_videoout_module = {
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
	.priv = &gx3201_vout_ops,
};
