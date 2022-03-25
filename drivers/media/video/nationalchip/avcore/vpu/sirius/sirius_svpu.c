#include "sirius_svpu.h"
#include "sirius_vpu.h"
#include "firewall.h"
#include "clock_hal.h"
#include "kernelcalls.h"
#include "gxav_common.h"

static volatile SiriusSvpuReg *siriussvpu_reg = NULL;

static struct{
#define MODE_480P_TO_PAL       (0)
#define MODE_576I_TO_PAL       (1)
#define MODE_576P_TO_PAL       (2)
#define MODE_720P50HZ_TO_PAL   (3)
#define MODE_720P60HZ_TO_PAL   (4)
#define MODE_1080I50HZ_TO_PAL  (5)
#define MODE_1080I60HZ_TO_PAL  (6)
#define MODE_1080P50HZ_TO_PAL  (7)
#define MODE_1080P60HZ_TO_PAL  (8)

#define MODE_480I_TO_NTSC      (9)
#define MODE_480P_TO_NTSC      (10)
#define MODE_576I_TO_NTSC      (11)
#define MODE_576P_TO_NTSC      (12)
#define MODE_720P50HZ_TO_NTSC  (13)
#define MODE_720P60HZ_TO_NTSC  (14)
#define MODE_1080I50HZ_TO_NTSC (15)
#define MODE_1080I60HZ_TO_NTSC (16)
#define MODE_1080P50HZ_TO_NTSC (17)
#define MODE_1080P60HZ_TO_NTSC (18)
	int cap_left;
	int cap_right;
	int cap_top;
	int cap_bottom;
	int cap_interlace;

	int spp_left;
	int spp_right;
	int spp_top;
	int spp_bottom;

	int cap_h_zoom_correct;
	int cap_v_zoom_correct;
	int cap_delay;
} svpuPhase[19] = {
	/* 480p->pal */
	{ 0,  719, 0,  479, 0, 0, 719, 0, 575, 0, 0, 0x19,},
	/* 576i->pal */
	{ 0,  719, 0,  575, 1, 0, 719, 0, 575, 0, 0, 0x19,},
	/* 576p->pal */
	{ 0,  719, 0,  575, 0, 0, 719, 0, 575, 0, 0, 0x19,},
	/* 720p50hz->pal */
	{ 0, 1279, 0,  719, 0, 0, 719, 0, 575, 0, 0, 0x11,},
	/* 720p60hz->pal */
	{ 0, 1279, 0,  719, 0, 0, 719, 0, 575, 0, 0, 0x11,},
	/* 1080i50hz->pal */
	{ 0, 1919, 0, 1079, 1, 0, 719, 0, 575, 0, 0, 0x08,},
	/* 1080i60hz->pal */
	{ 0, 1919, 0, 1079, 1, 0, 719, 0, 575, 0, 0, 0x08,},
	/* 1080p50hz->pal */
	{ 0, 1919, 0, 1079, 0, 0, 719, 0, 575, 0, 0, 0x08,},
	/* 1080p60hz->pal */
	{ 0, 1919, 0, 1079, 0, 0, 719, 0, 575, 0, 0, 0x08,},

	/* 480i->ntsc */
	{ 0,  719, 0,  479, 1, 0, 719, 0, 479, 0, 0, 0x1a,},
	/* 480p->ntsc */
	{ 0,  719, 0,  479, 0, 0, 719, 0, 479, 0, 0, 0x1a,},
	/* 576i->ntsc */
	{ 0,  719, 0,  575, 1, 0, 719, 0, 479, 0, 0, 0x19,},
	/* 576p->ntsc */
	{ 0,  719, 0,  575, 0, 0, 719, 0, 479, 0, 0, 0x19,},
	/* 720p50hz->ntsc */
	{ 0, 1279, 0,  719, 0, 0, 719, 0, 479, 0, 0, 0x13,},
	/* 720p60hz->ntsc */
	{ 0, 1279, 0,  719, 0, 0, 719, 2, 481, 0, 0, 0x13,},
	/* 1080i50hz->ntsc */
	{ 0, 1919, 0, 1079, 1, 0, 719, 2, 481, 0, 0, 0x0b,},
	/* 1080i60hz->ntsc */
	{ 0, 1919, 0, 1079, 1, 0, 719, 2, 481, 0, 0, 0x0b,},
	/* 1080p50hz->ntsc */
	{ 0, 1919, 0, 1079, 0, 0, 719, 2, 481, 0, 0, 0x0b,},
	/* 1080p60hz->ntsc */
	{ 0, 1919, 0, 1079, 0, 0, 719, 2, 481, 0, 0, 0x0b,},

};

static unsigned int sirius_svpu_filter_table[OSD_FLICKER_TABLE_LEN] =
{
	0x00808000, 0x26b52803, 0x27b32903, 0x26b32a03,
	0x25b32b03, 0x23b32d03, 0x22b32e03, 0x22b22f03,
	0x21b23003, 0x20b23103, 0x1fb23203, 0x1db23403,
	0x1db13503, 0x1cb13603, 0x1bb13703, 0x1ab03903,
	0x19b03a03, 0x19af3b03, 0x17af3d03, 0x17ae3e03,
	0x16ae3f03, 0x15ad4103, 0x14ad4203, 0x14ac4303,
	0x12ac4503, 0x12ab4603, 0x11aa4803, 0x10aa4903,
	0x10a94a03, 0x0fa84c03, 0x0fa74d03, 0x0ea64f03,
	0x0da65003, 0x0ca55203, 0x0ca45303, 0x0ca35403,
	0x0ba25603, 0x0ba15703, 0x0aa05903, 0x0a9f5a03,
	0x099e5c03, 0x099d5d03, 0x089c5f03, 0x089b6003,
	0x069a6202, 0x06996302, 0x05986502, 0x05976602,
	0x04966802, 0x04956902, 0x05936a02, 0x04926c02,
	0x04916d02, 0x03906f02, 0x038f7002, 0x038d7202,
	0x028c7301, 0x018b7501, 0x02897601, 0x01887801,
	0x01877901, 0x02857a01, 0x01847c01, 0x00837d00,
};

static struct{
	SVPU_MODE mode;
	unsigned int xMargin;
	unsigned int yMargin;
	unsigned int xShift;
	unsigned int yShift;
} svpuConfigPara = {0};

//VPU filter table is as same as SVPU filter table in Sirius and Taurus.
extern unsigned int sirius_osd_filter_table[OSD_FLICKER_TABLE_LEN];

extern int sirius_vpu_reset(void);

static int sirius_set_filter(GxVideoOutProperty_Mode mode)
{
	int i, table_len;

	switch(mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
			table_len = sizeof(sirius_osd_filter_table)/sizeof(sirius_osd_filter_table[0]);
			for (i = 0; i < table_len; i++) {
				SVPU_SET_FLIKER_FLITER(siriussvpu_reg->rSCE_PHASE_PARA[i], sirius_osd_filter_table[i]);
			}
			REG_SET_VAL(&(siriussvpu_reg->rSCE_ZOOM_COEF), 0x80990080);
			REG_SET_VAL(&(siriussvpu_reg->rSCE_H_PHASE0), 0x00ff0100);
			REG_SET_VAL(&(siriussvpu_reg->rSCE_V_PHASE0), 0x00ff0100);
			break;
		default:
			table_len = sizeof(sirius_svpu_filter_table)/sizeof(sirius_svpu_filter_table[0]);
			for (i = 0; i < table_len; i++) {
				SVPU_SET_FLIKER_FLITER(siriussvpu_reg->rSCE_PHASE_PARA[i], sirius_svpu_filter_table[i]);
			}
			REG_SET_VAL(&(siriussvpu_reg->rSCE_ZOOM_COEF), 0x00114040);
			REG_SET_VAL(&(siriussvpu_reg->rSCE_H_PHASE0), 3|40<<8|181<<16|38<<24);
			REG_SET_VAL(&(siriussvpu_reg->rSCE_V_PHASE0), 3|40<<8|181<<16|38<<24);
			break;
}
return 0;
}

static int sirius_svpu_iounmap(void)
{
if(siriussvpu_reg) {
gx_iounmap(siriussvpu_reg);
gx_release_mem_region(SVPU_REG_BASE_ADDR, sizeof(SiriusSvpuReg));
siriussvpu_reg = NULL;
}
return 0;
}

static int sirius_svpu_ioremap(void)
{
if(!gx_request_mem_region(SVPU_REG_BASE_ADDR, sizeof(SiriusSvpuReg))) {
gx_printf("request_mem_region failed");
goto SVPU_IOMAP_ERROR;
}

siriussvpu_reg = gx_ioremap(SVPU_REG_BASE_ADDR, sizeof(SiriusSvpuReg));
if(!siriussvpu_reg) {
goto SVPU_IOMAP_ERROR;
}
return 0;

SVPU_IOMAP_ERROR:
sirius_svpu_iounmap();
return -1;
}

#define IS_PAL_MODE(mode)\
((mode)==GXAV_VOUT_PAL || (mode)==GXAV_VOUT_PAL_N || (mode)==GXAV_VOUT_PAL_NC)
#define IS_NTSC_MODE(mode)\
((mode)==GXAV_VOUT_PAL_M || (mode)==GXAV_VOUT_NTSC_M || (mode)==GXAV_VOUT_NTSC_443)
static int sirius_svpu_get_capindex(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1)
{
int capindex;

switch(mode_vout0)
{
case GXAV_VOUT_PAL_M:
case GXAV_VOUT_NTSC_M:
case GXAV_VOUT_NTSC_443:
case GXAV_VOUT_480I:
	capindex = IS_PAL_MODE(mode_vout1) ? -1 : MODE_480I_TO_NTSC;
	break;
case GXAV_VOUT_PAL:
case GXAV_VOUT_PAL_N:
case GXAV_VOUT_PAL_NC:
case GXAV_VOUT_576I:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_576I_TO_PAL : MODE_576I_TO_NTSC;
	break;
case GXAV_VOUT_480P:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_480P_TO_PAL : MODE_480P_TO_NTSC;
	break;
case GXAV_VOUT_576P:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_576P_TO_PAL : MODE_576P_TO_NTSC;
	break;
case GXAV_VOUT_720P_50HZ:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_720P50HZ_TO_PAL : MODE_720P50HZ_TO_NTSC;
	break;
case GXAV_VOUT_720P_60HZ:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_720P60HZ_TO_PAL : MODE_720P60HZ_TO_NTSC;
	break;
case GXAV_VOUT_1080P_50HZ:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_1080P50HZ_TO_PAL : MODE_1080P50HZ_TO_NTSC;
	break;
case GXAV_VOUT_1080P_60HZ:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_1080P60HZ_TO_PAL : MODE_1080P60HZ_TO_NTSC;
	break;
case GXAV_VOUT_1080I_50HZ:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_1080I50HZ_TO_PAL : MODE_1080I50HZ_TO_NTSC;
	break;
case GXAV_VOUT_1080I_60HZ:
	capindex = IS_PAL_MODE(mode_vout1) ? MODE_1080I60HZ_TO_PAL : MODE_1080I60HZ_TO_NTSC;
	break;
default:
	capindex = -1;
	break;
}

if (capindex < 0)
	gx_printf("\nvpu_mode = %3d:svpu_mode = %3d, is not support!", mode_vout0, mode_vout1);

return capindex;
}

int sirius_svpu_get_sce_height(void)
{
int height = 0;

if(!siriussvpu_reg) {
	return -1;
}

height = SVPU_GET_SCE_ZOOM_HEIGHT(siriussvpu_reg->rSCE_ZOOM1);

return (height);
}

int sirius_svpu_get_sce_length(void)
{
int width = 0;

if(!siriussvpu_reg) {
	return -1;
}

width = SVPU_GET_SCE_ZOOM_LENGTH(siriussvpu_reg->rSCE_ZOOM1);

return (width);
}

int sirius_svpu_config_buf(SvpuSurfaceInfo *buf_info)
{
int i;
unsigned buf_block_size = SVPU_SURFACE_WIDTH*SVPU_SURFACE_HEIGHT;

if(siriussvpu_reg && buf_info) {
	// 4:2:2 模式 v_addr 不用配置, 保护大小为 720*576
	unsigned buf_base_addr = gx_virt_to_phys(buf_info->buf_addrs[0]);
	gxav_firewall_register_buffer(GXAV_BUFFER_VPU_SVPU, buf_base_addr, VOUT_BUF_SIZE/3*2);
	for(i = 0; i < SVPU_SURFACE_NUM; i ++) {
		buf_base_addr = gx_virt_to_phys(buf_info->buf_addrs[i]);
		REG_SET_VAL(&siriussvpu_reg->rBUF[i].y_topfield_addr,    buf_base_addr+buf_block_size*0);
		REG_SET_VAL(&siriussvpu_reg->rBUF[i].y_bottomfield_addr, buf_base_addr+buf_block_size*0+SVPU_SURFACE_WIDTH);
		REG_SET_VAL(&siriussvpu_reg->rBUF[i].u_topfield_addr,    buf_base_addr+buf_block_size*1);
		REG_SET_VAL(&siriussvpu_reg->rBUF[i].u_bottomfield_addr, buf_base_addr+buf_block_size*1+SVPU_SURFACE_WIDTH);
	}
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
}

return 0;
}

int sirius_svpu_get_buf(SvpuSurfaceInfo *buf_info)
{
int i;

if (buf_info) {
	for(i = 0; i < SVPU_SURFACE_NUM; i ++) {
		buf_info->buf_addrs[i] = gx_phys_to_virt(siriussvpu_reg->rBUF[i].y_topfield_addr);
	}
}

return 0;
}
static int sirius_svpu_get_reqblock(int width, int bpp)
{
int ret = 32;
int min = 32, max = 896;
int step = 8, align = 128, times = 4;
int line_byte = ((width*bpp)>>3);
int request   = (line_byte/times)/align*align;

if(request < min)
	ret = min;
else {
	while(request<max && (line_byte%request && line_byte%request<min)) {
		request += step;
	}
	if(request>=max || (line_byte%request && line_byte%request<min)) {
		ret = min;
		gx_printf("\n%s:%d, get svpu reqblock failed!\n", __func__, __LINE__);
	}
	else
		ret = request;
}

return ret;
}

int sirius_svpu_init(void)
{
int i, table_len;

	sirius_svpu_ioremap();

	SVPU_CAP_DISABLE(siriussvpu_reg->rCAP_CTRL);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	SVPU_SET_CAP_YUVMODE(siriussvpu_reg->rCAP_CTRL, SVPU_YUV_422);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

	table_len = sizeof(sirius_osd_filter_table)/sizeof(sirius_osd_filter_table[0]);
	for (i = 0; i < table_len; i++) {
		SVPU_SET_FLIKER_FLITER(siriussvpu_reg->rSCE_PHASE_PARA[i], sirius_osd_filter_table[i]);
	}

	return 0;
}

int sirius_svpu_cleanup(void)
{
	if(!siriussvpu_reg)
		return -1;

	sirius_svpu_stop();
	sirius_svpu_iounmap();
	return 0;
}

static unsigned int sirius_SvpuSourceWidth(unsigned int SourceLeft, unsigned int SourceRight)
{
	unsigned int SourceWidth = 0;

	SourceWidth = SourceRight - SourceLeft + 1;

	return SourceWidth;
}

static unsigned int sirius_SvpuSourceHeight(unsigned int SourceTop, unsigned int SourceBottom)
{
	unsigned int SourceHeight = 0;

	SourceHeight = SourceBottom - SourceTop + 1;

	return SourceHeight;
}

static unsigned int sirius_SvpuDestnationWidth(unsigned int DestnationLeft, unsigned int DestnationRight)
{
	unsigned int DestnationWidth = 0;

	DestnationWidth = DestnationRight - DestnationLeft + 1;

	return DestnationWidth;
}

static unsigned int sirius_SvpuDestnationHeight(unsigned int DestnationTop, unsigned int DestnationBottom)
{
	unsigned int DestnationHeight = 0;

	DestnationHeight = DestnationBottom - DestnationTop + 1;

	return DestnationHeight;
}


static unsigned int sirius_SvpuDestnationXMargin(unsigned int XCorrect)
{
	unsigned int XMargin = 0;

	//XCorrect value is the dispaly margin of horizontal, value is change from 0 to 36
	XMargin = (XCorrect >> 2) << 2; //horizontal margin must be four times pixel

	return  XMargin;
}

static unsigned int sirius_SvpuDestnationYMargin(unsigned int YCorrect)
{
	unsigned int YMargin = 0;

	//vertical margin must be even data
	YMargin = (YCorrect >> 1) << 1;

	return  YMargin;
}

static unsigned int sirius_SvpuZoomWidth(unsigned int XCorrect,
					 unsigned int ZoomLeft,
					 unsigned int ZoomRight)
{
	unsigned int ZoomWidth = 0, DestnationWidth = 0, XMargin = 0;

	XMargin = sirius_SvpuDestnationXMargin(XCorrect);

	DestnationWidth = sirius_SvpuDestnationWidth(ZoomLeft,ZoomRight);

	ZoomWidth = DestnationWidth - XMargin * 2;

	return ZoomWidth;
}

static unsigned int sirius_SvpuZoomHeight(unsigned int YCorrect,
					  unsigned int ZoomTop,
					  unsigned int ZoomBottom)
{
	unsigned int ZoomHeight = 0, DestnationHeight = 0, YMargin = 0;

	YMargin = sirius_SvpuDestnationYMargin(YCorrect);

	DestnationHeight = sirius_SvpuDestnationHeight(ZoomTop,ZoomBottom);

	ZoomHeight = DestnationHeight - YMargin*2;

	return ZoomHeight;
}

static unsigned int sirius_SvpuDisplayXzoom(unsigned int XCorrect,
                        unsigned int XZoomCorrect,
					    unsigned int SourceLeft,
					    unsigned int SourceRight,
					    unsigned int DestnationLeft,
					    unsigned int DestnationRight)
{
	unsigned int DisplayXzoom = 4096, SourceWidth = 0, DestnationWidth = 0, RealDisplayWidth = 0;
	unsigned int XMargin = 0;

	XMargin = sirius_SvpuDestnationXMargin(XCorrect);

	SourceWidth = sirius_SvpuSourceWidth(SourceLeft,SourceRight);

	DestnationWidth = sirius_SvpuDestnationWidth(DestnationLeft,DestnationRight);

	RealDisplayWidth = DestnationWidth - XMargin*2;

	DisplayXzoom = 4096 * (SourceWidth + XZoomCorrect) / (RealDisplayWidth - 0);

	return DisplayXzoom;
}

static unsigned int sirius_SvpuDisplayYzoom(unsigned int YCorrect,
                        unsigned int YZoomCorrect,
					    unsigned int SourceTop,
					    unsigned int SourceBottom,
					    unsigned int DestnationTop,
					    unsigned int DestnationBottom,
					    unsigned int VpuIsInterlace)
{
	//YCorrect value is the dispaly margin of vertical , value is change from 0 to 30
	unsigned int DisplayYzoom = 4096, SourceHeight = 0, DestnationHeight = 0, RealDisplayHeight = 0;
	unsigned int YMargin = 0;

	YMargin = sirius_SvpuDestnationYMargin(YCorrect);

	SourceHeight = sirius_SvpuSourceHeight(SourceTop,SourceBottom);

	DestnationHeight = sirius_SvpuDestnationHeight(DestnationTop,DestnationBottom);

	RealDisplayHeight = DestnationHeight - YMargin*2;

	if(VpuIsInterlace) {
		    DisplayYzoom = 4096 * (SourceHeight + YZoomCorrect) / (RealDisplayHeight);
	} else {
		    DisplayYzoom = 8192 * (SourceHeight + YZoomCorrect) / (RealDisplayHeight);
	}

	return DisplayYzoom;
}

static unsigned int sirius_SvpuDisplayBottomBias(unsigned int YCorrect,
						 unsigned int SourceTop,
						 unsigned int SourceBottom,
						 unsigned int DestnationTop,
						 unsigned int DestnationBottom,
						 unsigned int VpuIsInterlace)
{
	unsigned int DisplayYzoom = 4096, DisplayBottomBias = 0, SourceHeight = 0, DestnationHeight = 0;
	unsigned int RealDisplayHeight = 0, YMargin = 0;

	YMargin = sirius_SvpuDestnationYMargin(YCorrect);

	SourceHeight = sirius_SvpuSourceHeight(SourceTop,SourceBottom);

	DestnationHeight = sirius_SvpuDestnationHeight(DestnationTop,DestnationBottom);

	RealDisplayHeight = DestnationHeight - YMargin*2;

	DisplayYzoom = 4096 * (SourceHeight - 1) / (DestnationHeight - 1);

	if(VpuIsInterlace) {
		DisplayBottomBias = (DisplayYzoom - 4096) >> 1;
	} else {
		DisplayBottomBias = DisplayYzoom  >> 1;
	}
	return DisplayBottomBias;
}

static unsigned int sirius_SvpuDisplayLeftPos(unsigned int XCorrect, unsigned int DestnationLeft)
{
	unsigned int DisplayLeft = 0, XMargin = 0;

	XMargin = sirius_SvpuDestnationXMargin(XCorrect);

	DisplayLeft = DestnationLeft + XMargin;

	return DisplayLeft;
}

static unsigned int sirius_SvpuDisplayRightPos(unsigned int XCorrect, unsigned int DestnationRight)
{
	unsigned int DisplayRight = 0, XMargin = 0;

	XMargin = sirius_SvpuDestnationXMargin(XCorrect);

	DisplayRight = DestnationRight - XMargin;

	return DisplayRight;
}

static unsigned int sirius_SvpuDisplayTopPos(unsigned int YCorrect, unsigned int DestnationTop)
{
	unsigned int DisplayTop = 0, YMargin = 0;

	YMargin = sirius_SvpuDestnationYMargin(YCorrect);

	DisplayTop = DestnationTop + YMargin;

	return DisplayTop;
}

static unsigned int sirius_SvpuDisplayBottomPos(unsigned int YCorrect, unsigned int DestnationBottom)
{
	unsigned int DisplayBottom = 0, YMargin = 0;

	YMargin = sirius_SvpuDestnationYMargin(YCorrect);

	DisplayBottom = DestnationBottom - YMargin;

	return DisplayBottom;
}

int sirius_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info)
{
	int i, capindex, reqblock;
	unsigned int xMargin = 0, yMargin = 0, xShift = 0, yShift = 0, SoftwareSetSvpu = 0;//to be add to remote control key
	unsigned int result_value = 0;

	if(!siriussvpu_reg) {
		return -1;
	}

	SoftwareSetSvpu = svpuConfigPara.mode;
	xMargin += svpuConfigPara.xMargin;
	yMargin += svpuConfigPara.yMargin;
	xShift  += svpuConfigPara.xShift;
	yShift  += svpuConfigPara.yShift;

	SoftwareSetSvpu = 1;
	//set buffers
	sirius_svpu_config_buf(buf_info);
	capindex = sirius_svpu_get_capindex(mode_vout0, mode_vout1);
	sirius_set_filter(mode_vout0);
	if (capindex < 0) {
		gx_printf("\n%s:%d, config svpu error!", __func__, __LINE__);
		return -1;
	} else {
		SVPU_SET_CAP_YUVMODE(siriussvpu_reg->rCAP_CTRL, SVPU_YUV_422);
		SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

		if(SoftwareSetSvpu) {
			SVPU_SET_AUTO_CAPMODE_DISABLE(siriussvpu_reg->rCAP_CTRL);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_CAPMODE_DISABLE(siriussvpu_reg->rCAP_CTRL);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_CAP_LEFT(siriussvpu_reg->rCAP_H, svpuPhase[capindex].cap_left);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_CAP_RIGHT(siriussvpu_reg->rCAP_H, svpuPhase[capindex].cap_right);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_CAP_TOP(siriussvpu_reg->rCAP_V, svpuPhase[capindex].cap_top);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_CAP_BOTTOM(siriussvpu_reg->rCAP_V, svpuPhase[capindex].cap_bottom);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayLeftPos(xMargin, svpuPhase[capindex].spp_left) + xShift;
			SVPU_SET_SPP_LEFT(siriussvpu_reg->rVIEW_H, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayRightPos(xMargin, svpuPhase[capindex].spp_right + xShift);
			SVPU_SET_SPP_RIGHT(siriussvpu_reg->rVIEW_H, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayTopPos(yMargin, svpuPhase[capindex].spp_top + yShift);
			SVPU_SET_SPP_TOP(siriussvpu_reg->rVIEW_V, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayBottomPos(yMargin, svpuPhase[capindex].spp_bottom + yShift);
			SVPU_SET_SPP_BOTTOM(siriussvpu_reg->rVIEW_V, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayYzoom(yMargin,
							       svpuPhase[capindex].cap_v_zoom_correct,
							       svpuPhase[capindex].cap_top,
							       svpuPhase[capindex].cap_bottom,
							       svpuPhase[capindex].spp_top,svpuPhase[capindex].spp_bottom,
							       svpuPhase[capindex].cap_interlace);
			//Maxium threshhold, whose value once more than triple zoom , then open downscale
			if(result_value > 12288) {
				result_value = result_value >> 1;
				SVPU_VDOWNSCALE_ENABLE(siriussvpu_reg->rCAP_CTRL);
			} else {
				SVPU_VDOWNSCALE_DISABLE(siriussvpu_reg->rCAP_CTRL);
			}
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_SCE_VZOOM(siriussvpu_reg->rSCE_ZOOM, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayXzoom(xMargin,
							       svpuPhase[capindex].cap_h_zoom_correct,
							       svpuPhase[capindex].cap_left,
							       svpuPhase[capindex].cap_right,
							       svpuPhase[capindex].spp_left,
							       svpuPhase[capindex].spp_right);
			SVPU_SET_SCE_HZOOM(siriussvpu_reg->rSCE_ZOOM, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuZoomWidth(xMargin, svpuPhase[capindex].spp_left, svpuPhase[capindex].spp_right);
			SVPU_SET_SCE_ZOOM_LENGTH(siriussvpu_reg->rSCE_ZOOM1, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuZoomHeight(yMargin, svpuPhase[capindex].spp_top, svpuPhase[capindex].spp_bottom);
			SVPU_SET_SCE_ZOOM_HEIGHT(siriussvpu_reg->rSCE_ZOOM1, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_SCE_VT_PHASE(siriussvpu_reg->rZOOM_PHASE, 0);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			result_value = sirius_SvpuDisplayBottomBias(yMargin,
								    svpuPhase[capindex].cap_top,
								    svpuPhase[capindex].cap_bottom,
								    svpuPhase[capindex].spp_top,
								    svpuPhase[capindex].spp_bottom,
								    svpuPhase[capindex].cap_interlace);
			SVPU_SET_SCE_VB_PHASE(siriussvpu_reg->rZOOM_PHASE, result_value);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

		} else {
			SVPU_SET_AUTO_CAPMODE_ENABLE(siriussvpu_reg->rCAP_CTRL);

			SVPU_SET_SPP_LEFT(siriussvpu_reg->rVIEW_H, svpuPhase[capindex].spp_left);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_SPP_RIGHT(siriussvpu_reg->rVIEW_H, svpuPhase[capindex].spp_right);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

			SVPU_SET_SPP_TOP(siriussvpu_reg->rVIEW_V, svpuPhase[capindex].spp_top);
			SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
		}
	}

	//set caplevel
	SVPU_SET_CAPLEVEL(siriussvpu_reg->rCAP_CTRL, SVPU_LAYER_MIX);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	//set buf R/W reqblock
	reqblock = sirius_svpu_get_reqblock(SVPU_SURFACE_WIDTH, SVPU_SURFACE_BPP);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	SVPU_SET_BUFS_READ_REQBLOCK (siriussvpu_reg->rDISP_REQ_BLOCK, reqblock);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	SVPU_SET_BUFS_WRITE_REQBLOCK(siriussvpu_reg->rREQ_LENGTH,     reqblock);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	SVPU_SET_FRAME_MODE(siriussvpu_reg->rVPU_FRAME_MODE, FRAME_MODE_3);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);

	REG_SET_FIELD(&(siriussvpu_reg->rSYS_PARA), 0xff, 0x00, 0);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	REG_SET_FIELD(&(siriussvpu_reg->rSYS_PARA), 0xff, svpuPhase[capindex].cap_delay, 0);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	//VPU hot-reset
	if(SoftwareSetSvpu) {
		sirius_vpu_reset();
		SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	}

	return 0;
}

int sirius_svpu_set_para(GxVpuProperty_SVPUConfig *param)
{
	if(NULL == param) {
		return 1;
	}

	svpuConfigPara.mode = param->svpu_mode;
	svpuConfigPara.xMargin = param->x_margin;
	svpuConfigPara.yMargin = param->y_margin;
	svpuConfigPara.xShift  = param->x_shift;
	svpuConfigPara.yShift  = param->y_shift;

	return 0;
}

int sirius_svpu_run(void)
{
	if(!siriussvpu_reg)
		return -1;
	SVPU_DISP_ENABLE(siriussvpu_reg->rDISP_CTRL);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	SVPU_CAP_ENABLE(siriussvpu_reg->rCAP_CTRL);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	return 0;
}

int sirius_svpu_stop(void)
{
	if(!siriussvpu_reg)
		return -1;
	SVPU_CAP_DISABLE(siriussvpu_reg->rCAP_CTRL);
	SVPU_DISP_DISABLE(siriussvpu_reg->rDISP_CTRL);
	SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	gx_mdelay(30);
	return 0;
}

int sirius_svpu_set_video_dac(int enable)
{
	if(siriussvpu_reg){
		if(enable){
			SVPU_CAP_ENABLE(siriussvpu_reg->rCAP_CTRL);
			SVPU_DISP_ENABLE(siriussvpu_reg->rDISP_CTRL);
		}else{
			SVPU_CAP_DISABLE(siriussvpu_reg->rCAP_CTRL);
			SVPU_DISP_DISABLE(siriussvpu_reg->rDISP_CTRL);
		}
		SVPU_PARA_UPDATE(siriussvpu_reg->rPARA_UPDATE);
	}
	return 0;
}

