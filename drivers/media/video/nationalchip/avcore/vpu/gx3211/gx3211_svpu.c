#include "gx3211_svpu.h"
#include "clock_hal.h"
#include "kernelcalls.h"
#include "gxav_common.h"

extern int gx3211_vpu_reset(void);

static volatile Gx3211SvpuReg *gx3211svpu_reg = NULL;

static struct{
#define MODE_480P_TO_PAL   (0)
#define MODE_576I_TO_PAL   (1)
#define MODE_576P_TO_PAL   (2)
#define MODE_720P_TO_PAL   (3)
#define MODE_1080I_TO_PAL  (4)
#define MODE_1080P_TO_PAL  (5)

#define MODE_480I_TO_NTSC   (6)
#define MODE_480P_TO_NTSC   (7)
#define MODE_576I_TO_NTSC   (8)
#define MODE_576P_TO_NTSC   (9)
#define MODE_720P_TO_NTSC   (10)
#define MODE_1080I_TO_NTSC  (11)
#define MODE_1080P_TO_NTSC  (12)
	int cap_v_downscale;
	int cap_left;
	int cap_right;
	int cap_top;
	int cap_bottom;
	int sce_v_zoom;
	int sce_h_zoom;
	int sce_zoom_length;
	int sce_zoom_height;
	int sce_vt_phase_bias;
	int sce_vb_phase_bias;
	int spp_left;
	int spp_right;
	int spp_top;
	int spp_bottom;
	int cap_mode;
} svpuPhase[13] = {
	/* 480p->pal */
    { 0, 0,  719, 0,  479,  6826,  4096, 720, 576, 0,   3413, 0, 719, 0, 573, 17},
	/* 576i->pal */
    { 0, 0,  719, 0,  575,  4096,  4096, 720, 576, 0,      0, 0, 719, 0, 573, 8 },
	/* 576p->pal */
    { 0, 0,  719, 0,  575,  8192,  4096, 720, 576, 0,   4096, 0, 719, 0, 573, 6 },
	/* 720p->pal */
    { 0, 0, 1279, 0,  719, 10261,  7286, 720, 576, 0,   5130, 0, 719, 0, 573, 5 },
	/* 1080i->pal */
    { 0, 0, 1919, 0, 1079,  7692, 10932, 720, 576, 0,   1798, 0, 719, 0, 573, 3 },
	/* 1080p->pal */
    { 1, 0, 1919, 0, 1079,  7692, 10932, 720, 576, 0,   3846, 0, 719, 0, 573, 1 },

	/* 480i->ntsc */
    { 0, 0,  719, 0,  479,  4096,  4096, 720, 480, 0,      0, 0, 711, 0, 477, 9 },
	/* 480p->ntsc */
    { 0, 0,  719, 0,  479,  8192,  4096, 720, 480, 0,   4096, 0, 711, 0, 477, 7 },
	/* 576i->ntsc */
    { 0, 0,  719, 0,  575,  4915,  4096, 720, 480, 0,    409, 0, 711, 0, 477, 18},
	/* 576p->ntsc */
    { 0, 0,  719, 0,  575,  9830,  4096, 720, 480, 0,   5915, 0, 711, 0, 477, 16},
	/* 720p->ntsc */
    { 0, 0, 1279, 0,  719, 12288,  7286, 720, 480, 0,   6144, 0, 711, 0, 477, 4 },
	/* 1080i->ntsc */
    { 0, 0, 1919, 0, 1079,  9237, 10932, 720, 480, 0,   2570, 0, 711, 0, 477, 2 },
	/* 1080p->ntsc */
    { 1, 0, 1919, 0, 1079,  9237, 10932, 720, 480, 0,   4618, 0, 711, 0, 477, 0 },

};

static unsigned int gx3211_svpu_filter_table[] = {
	0x808000,0x7e8200,0x7c8400,0x7a8600,
	0x788800,0x768a00,0x748c00,0x728e00,
	0x709000,0x6e9200,0x6c9400,0x6a9600,
	0x689800,0x669a00,0x649c00,0x629e00,
	0x60a000,0x5ea200,0x5ca400,0x5aa600,
	0x58a800,0x56aa00,0x54ac00,0x52ae00,
	0x50b000,0x4eb200,0x4cb400,0x4ab600,
	0x48b800,0x46ba00,0x44bc00,0x42be00,
	0x40c000,0x3ec200,0x3cc400,0x3ac600,
	0x38c800,0x36ca00,0x34cc00,0x32ce00,
	0x30d000,0x2ed200,0x2cd400,0x2ad600,
	0x28d800,0x26da00,0x24dc00,0x22de00,
	0x20e000,0x1ee200,0x1ce400,0x1ae600,
	0x18e800,0x16ea00,0x14ec00,0x12ee00,
	0x10f000,0x0ef200,0x0cf400,0x0af600,
	0x08f800,0x06fa00,0x04fc00,0x02fe00,
};

static int gx3211_svpu_iounmap(void)
{
	if(gx3211svpu_reg) {
		gx_iounmap(gx3211svpu_reg);
		gx_release_mem_region(SVPU_REG_BASE_ADDR, sizeof(Gx3211SvpuReg));
		gx3211svpu_reg = NULL;
	}
	return 0;
}

static int gx3211_svpu_ioremap(void)
{
	if(!gx_request_mem_region(SVPU_REG_BASE_ADDR, sizeof(Gx3211SvpuReg))) {
		gx_printf("request_mem_region failed");
		goto SVPU_IOMAP_ERROR;
	}

	gx3211svpu_reg = gx_ioremap(SVPU_REG_BASE_ADDR, sizeof(Gx3211SvpuReg));
	if(!gx3211svpu_reg) {
		goto SVPU_IOMAP_ERROR;
	}
	return 0;

SVPU_IOMAP_ERROR:
	gx3211_svpu_iounmap();
	return -1;
}

#define IS_PAL_MODE(mode)\
	((mode)==GXAV_VOUT_PAL || (mode)==GXAV_VOUT_PAL_N || (mode)==GXAV_VOUT_PAL_NC)
#define IS_NTSC_MODE(mode)\
	((mode)==GXAV_VOUT_PAL_M || (mode)==GXAV_VOUT_NTSC_M || (mode)==GXAV_VOUT_NTSC_443)
static int gx3211_svpu_get_capindex(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1)
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
	case GXAV_VOUT_720P_60HZ:
		capindex = IS_PAL_MODE(mode_vout1) ? MODE_720P_TO_PAL : MODE_720P_TO_NTSC;
		break;
	case GXAV_VOUT_1080P_50HZ:
	case GXAV_VOUT_1080P_60HZ:
		capindex = IS_PAL_MODE(mode_vout1) ? MODE_1080P_TO_PAL : MODE_1080P_TO_NTSC;
		break;
	case GXAV_VOUT_1080I_50HZ:
	case GXAV_VOUT_1080I_60HZ:
		capindex = IS_PAL_MODE(mode_vout1) ? MODE_1080I_TO_PAL : MODE_1080I_TO_NTSC;
		break;
	default:
		capindex = -1;
		break;
	}

	if (capindex < 0)
		gx_printf("\nvpu_mode = %3d:svpu_mode = %3d, is not support!", mode_vout0, mode_vout1);

	return capindex;
}

int gx3211_svpu_config_buf(SvpuSurfaceInfo *buf_info)
{
	int i;
	unsigned buf_block_size = SVPU_SURFACE_WIDTH*SVPU_SURFACE_HEIGHT;

	if(gx3211svpu_reg && buf_info) {
		for(i = 0; i < SVPU_SURFACE_NUM; i ++) {
			unsigned buf_base_addr = gx_virt_to_phys(buf_info->buf_addrs[i]);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].y_topfield_addr,    buf_base_addr+buf_block_size*0);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].y_bottomfield_addr, buf_base_addr+buf_block_size*0+SVPU_SURFACE_WIDTH);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].u_topfield_addr,    buf_base_addr+buf_block_size*1);
			REG_SET_VAL(&gx3211svpu_reg->rBUF[i].u_bottomfield_addr, buf_base_addr+buf_block_size*1+SVPU_SURFACE_WIDTH);
			if (CHIP_IS_GX6605S) {
				// 4:2:2 模式 v_addr 不用配置
			}
			else {
				REG_SET_VAL(&gx3211svpu_reg->rBUF[i].v_topfield_addr,    buf_base_addr+buf_block_size*2);
				REG_SET_VAL(&gx3211svpu_reg->rBUF[i].v_bottomfield_addr, buf_base_addr+buf_block_size*2+SVPU_SURFACE_WIDTH);
			}
		}
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	}

	return 0;
}

int gx3211_svpu_get_buf(SvpuSurfaceInfo *buf_info)
{
	int i;

	if (buf_info) {
		for(i = 0; i < SVPU_SURFACE_NUM; i ++) {
			buf_info->buf_addrs[i] = gx_phys_to_virt(gx3211svpu_reg->rBUF[i].y_topfield_addr);
		}
	}

	return 0;
}

static int gx3211_svpu_get_reqblock(int width, int bpp)
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

int gx3211_svpu_init(void)
{
	int i, table_len;

	gx3211_svpu_ioremap();

	REG_SET_VAL(&(gx3211svpu_reg->rSYS_PARA), 0x23); //svpu k2t not pass
	SVPU_CAP_DISABLE(gx3211svpu_reg->rCAP_CTRL);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	SVPU_SET_CAP_YUVMODE(gx3211svpu_reg->rCAP_CTRL, SVPU_YUV_422);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);

	table_len = sizeof(gx3211_svpu_filter_table)/sizeof(gx3211_svpu_filter_table[0]);
	for (i = 0; i < table_len; i++) {
		SVPU_SET_FLIKER_FLITER(gx3211svpu_reg->rSCE_PHASE_PARA[i], gx3211_svpu_filter_table[i]);
	}

	return 0;
}

int gx3211_svpu_cleanup(void)
{
	if(!gx3211svpu_reg)
		return -1;

	gx3211_svpu_stop();
	gx3211_svpu_iounmap();
	return 0;
}

int gx3211_svpu_get_sce_height(void)
{
	int height = 0;

	if(!gx3211svpu_reg)
		return -1;

	height = SVPU_GET_SCE_ZOOM_HEIGHT(gx3211svpu_reg->rSCE_ZOOM1);

	return (height);
}

int gx3211_svpu_get_sce_length(void)
{
	int width = 0;

	if(!gx3211svpu_reg)
		return -1;

	width = SVPU_GET_SCE_ZOOM_LENGTH(gx3211svpu_reg->rSCE_ZOOM1);

	return (width);
}

int gx3211_svpu_config(GxVideoOutProperty_Mode mode_vout0, GxVideoOutProperty_Mode mode_vout1, SvpuSurfaceInfo *buf_info)
{
	int capindex, reqblock;

	if(!gx3211svpu_reg)
		return -1;

	//set buffers
	gx3211_svpu_config_buf(buf_info);
	capindex = gx3211_svpu_get_capindex(mode_vout0, mode_vout1);
	if (capindex < 0) {
		gx_printf("\n%s:%d, config svpu error!", __func__, __LINE__);
		return -1;
	} else {

		if (CHIP_IS_GX6605S) {
			SVPU_SET_CAP_YUVMODE(gx3211svpu_reg->rCAP_CTRL, SVPU_YUV_422);
			SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		}
		SVPU_SET_CAPMODE_DISABLE(gx3211svpu_reg->rCAP_CTRL);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);

		if (svpuPhase[capindex].cap_v_downscale != 0) {
			SVPU_VDOWNSCALE_ENABLE(gx3211svpu_reg->rCAP_CTRL);
			SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		}
		else {
			SVPU_VDOWNSCALE_DISABLE(gx3211svpu_reg->rCAP_CTRL);
			SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		}

		SVPU_SET_CAP_LEFT(gx3211svpu_reg->rCAP_H, svpuPhase[capindex].cap_left);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_CAP_RIGHT(gx3211svpu_reg->rCAP_H, svpuPhase[capindex].cap_right);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_CAP_TOP(gx3211svpu_reg->rCAP_V, svpuPhase[capindex].cap_top);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_CAP_BOTTOM(gx3211svpu_reg->rCAP_V, svpuPhase[capindex].cap_bottom);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SCE_VZOOM(gx3211svpu_reg->rSCE_ZOOM, svpuPhase[capindex].sce_v_zoom);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SCE_HZOOM(gx3211svpu_reg->rSCE_ZOOM, svpuPhase[capindex].sce_h_zoom);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SCE_ZOOM_LENGTH(gx3211svpu_reg->rSCE_ZOOM1, svpuPhase[capindex].sce_zoom_length);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SCE_ZOOM_HEIGHT(gx3211svpu_reg->rSCE_ZOOM1, svpuPhase[capindex].sce_zoom_height);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SCE_VT_PHASE(gx3211svpu_reg->rZOOM_PHASE, svpuPhase[capindex].sce_vt_phase_bias);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SCE_VB_PHASE(gx3211svpu_reg->rZOOM_PHASE, svpuPhase[capindex].sce_vb_phase_bias);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SPP_LEFT(gx3211svpu_reg->rVIEW_H, svpuPhase[capindex].spp_left);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SPP_RIGHT(gx3211svpu_reg->rVIEW_H, svpuPhase[capindex].spp_right);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SPP_TOP(gx3211svpu_reg->rVIEW_V, svpuPhase[capindex].spp_top);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
		SVPU_SET_SPP_BOTTOM(gx3211svpu_reg->rVIEW_V, svpuPhase[capindex].spp_bottom);
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	}

	//set caplevel
	SVPU_SET_CAPLEVEL(gx3211svpu_reg->rCAP_CTRL, SVPU_LAYER_MIX);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	//set buf R/W reqblock
	reqblock = gx3211_svpu_get_reqblock(SVPU_SURFACE_WIDTH, SVPU_SURFACE_BPP);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	SVPU_SET_BUFS_READ_REQBLOCK (gx3211svpu_reg->rDISP_REQ_BLOCK, reqblock);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	SVPU_SET_BUFS_WRITE_REQBLOCK(gx3211svpu_reg->rREQ_LENGTH,     reqblock);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	SVPU_SET_FRAME_MODE(gx3211svpu_reg->rVPU_FRAME_MODE, FRAME_MODE_3);
	//para update
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);

	//VPU hot-reset
	gx3211_vpu_reset();
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);

	return 0;
}

int gx3211_svpu_run(void)
{
	if(!gx3211svpu_reg)
		return -1;
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	SVPU_CAP_ENABLE(gx3211svpu_reg->rCAP_CTRL);
	SVPU_DISP_ENABLE(gx3211svpu_reg->rDISP_CTRL);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	return 0;
}

int gx3211_svpu_stop(void)
{
	if(!gx3211svpu_reg)
		return -1;
	SVPU_CAP_DISABLE(gx3211svpu_reg->rCAP_CTRL);
	SVPU_DISP_DISABLE(gx3211svpu_reg->rDISP_CTRL);
	SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	gx_mdelay(30);
	return 0;
}

int gx3211_svpu_set_video_dac(int enable)
{
	if(gx3211svpu_reg){
		if(enable){
			SVPU_CAP_ENABLE(gx3211svpu_reg->rCAP_CTRL);
			SVPU_DISP_ENABLE(gx3211svpu_reg->rDISP_CTRL);
		}else{
			SVPU_CAP_DISABLE(gx3211svpu_reg->rCAP_CTRL);
			SVPU_DISP_DISABLE(gx3211svpu_reg->rDISP_CTRL);
		}
		SVPU_PARA_UPDATE(gx3211svpu_reg->rPARA_UPDATE);
	}
	return 0;
}

