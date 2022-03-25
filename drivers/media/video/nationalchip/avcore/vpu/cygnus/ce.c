#include "cygnus_vpu_reg.h"
#include "cygnus_vpu_internel.h"
#include "vout_hal.h"

static unsigned ce_zoom_param_h[] = {
	/*[0x07100]*/ 0x750afb01,
	/*[0x07104]*/ 0x0000fb0a,
	/*[0x07108]*/ 0x7611f801,
	/*[0x0710c]*/ 0x0000fc04,
	/*[0x07110]*/ 0x7119f703,
	/*[0x07114]*/ 0x0000fffd,
	/*[0x07118]*/ 0x6f21f403,
	/*[0x0711c]*/ 0x000000f9,
	/*[0x07120]*/ 0x692af303,
	/*[0x07124]*/ 0x000001f6,
	/*[0x07128]*/ 0x6233f203,
	/*[0x0712c]*/ 0x000003f3,
	/*[0x07130]*/ 0x5e3bf004,
	/*[0x07134]*/ 0x000003f0,
	/*[0x07138]*/ 0x5645ef04,
	/*[0x0713c]*/ 0x000003ef,
	/*[0x07140]*/ 0x4d4def04,
	/*[0x07144]*/ 0x000004ef,
};

static unsigned ce_zoom_param_v[] = {
	/*[0x07200]*/ 0x6813f802,
	/*[0x07204]*/ 0x0000f813,
	/*[0x07208]*/ 0x671af702,
	/*[0x0720c]*/ 0x0000f90d,
	/*[0x07210]*/ 0x6620f602,
	/*[0x07214]*/ 0x0000fa08,
	/*[0x07218]*/ 0x6227f503,
	/*[0x0721c]*/ 0x0000fc03,
	/*[0x07220]*/ 0x5f2ef403,
	/*[0x07224]*/ 0x0000fdff,
	/*[0x07228]*/ 0x5b35f402,
	/*[0x0722c]*/ 0x0000fefc,
	/*[0x07230]*/ 0x553df402,
	/*[0x07234]*/ 0x0000fff9,
	/*[0x07238]*/ 0x5044f401,
	/*[0x0723c]*/ 0x000000f7,
	/*[0x07240]*/ 0x4a4af501,
	/*[0x07244]*/ 0x000001f5,
};

/* ce */ 
int ce_init(void)
{
	int i;
	int val = 0;

	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);
	/*### 确认如何配置*/
	REG_SET_FIELD(&(reg->rCE_REQ_LENGTH), m_CE_ENDIAN_MODE, val, b_CE_ENDIAN_MODE);
	REG_SET_FIELD(&(reg->rCE_CAP_MODE), m_CAP_FORMAT_AUTO, 0x1, b_CAP_FORMAT_AUTO);
	for (i = 0; i < sizeof(ce_zoom_param_h)/sizeof(ce_zoom_param_h[0]); i++)
		REG_SET_FIELD(&(reg->rCE_HZOOM_PARA[i]), m_CE_HZOOM_PARA_L, ce_zoom_param_h[i], b_CE_HZOOM_PARA_L);
	for (i = 0; i < sizeof(ce_zoom_param_v)/sizeof(ce_zoom_param_v[0]); i++)
		REG_SET_FIELD(&(reg->rCE_VZOOM_PARA[i]), m_CE_VZOOM_PARA_L, ce_zoom_param_v[i], b_CE_VZOOM_PARA_L);
	/*### 确认如何配置*/
	//REG_SET_FIELD(&(reg->rCE_ZOOM_PARA_SIGN), m_CE_PHASE_0_ZMODE, val, b_CE_PHASE_0_ZMODE);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

int ce_set_format(GxColorFormat format)
{
	int val = 0;
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	/* 0：存储格式为YUV422，全交织；1：存储格式为YUV422，半交织；2：存储格式为YUV420，半交织;3:存储格式为YUV444，非交织 */
	if (format == GX_COLOR_FMT_YCBCR422)
		val = 0;
	if (format == GX_COLOR_FMT_YCBCR422_Y_UV)
		val = 1;
	if (format == GX_COLOR_FMT_YCBCR420_Y_UV)
		val = 2;
	if (format == GX_COLOR_FMT_YCBCR444_Y_U_V)
		val = 3;
	REG_SET_FIELD(&(reg->rCE_CAP_MODE), m_CAP_DATA_TYPE, val, b_CAP_DATA_TYPE);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}


int ce_set_mode(int mode)
{
	int val = 0;
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	if (mode == CE_MODE_MANUAL) {
		REG_SET_FIELD(&(reg->rCE_CAP_MODE), m_SINGLE_FRAME_CAP_ENABLE, 1, b_SINGLE_FRAME_CAP_ENABLE);
		REG_CLR_BIT(&(reg->rCE_CAP_MODE), b_CAP_FORMAT_AUTO);
		REG_SET_FIELD(&(reg->rCE_DEST_SIZE), m_SCE_ZOOM_PROGRESIVE, 0, b_SCE_ZOOM_PROGRESIVE);
		/*### 依赖增加当前输出制式寄存器*/
		REG_SET_FIELD(&(reg->rCE_ZOOM_PHASE), m_ZOOM_MODE, val, b_ZOOM_MODE);
	} else {
		REG_SET_FIELD(&(reg->rCE_CAP_MODE), m_SINGLE_FRAME_CAP_ENABLE, 0, b_SINGLE_FRAME_CAP_ENABLE);
		REG_SET_FIELD(&(reg->rCE_DEST_SIZE), m_SCE_ZOOM_PROGRESIVE, 0, b_SCE_ZOOM_PROGRESIVE);
		REG_SET_BIT(&(reg->rCE_CAP_MODE), b_CAP_FORMAT_AUTO);
		/*### 依赖增加当前输出制式寄存器*/
		REG_SET_FIELD(&(reg->rCE_ZOOM_PHASE), m_ZOOM_MODE, val, b_ZOOM_MODE);
	}
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

int ce_set_layer(int layer_sel)
{
	int val = 0;
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	if ((layer_sel >> GX_LAYER_VPP) && 0x1)
		val |= (1<<1);
	if ((layer_sel >> GX_LAYER_VPP1) && 0x1)
		val |= (1<<1);
	if ((layer_sel >> GX_LAYER_SPP) && 0x1)
		val |= (1<<0);
	if ((layer_sel >> GX_LAYER_OSD) && 0x1)
		val |= (1<<3);
	if ((layer_sel >> GX_LAYER_SOSD) && 0x1)
		val |= (1<<4);
	if (layer_sel == GX_LAYER_MIX_ALL)
		val = (1 << 1) | (1 << 0) | (1 << 3);

	REG_SET_FIELD(&(reg->rCE_CAP_CTROL), m_CAP_SURFACE_EN, val, b_CAP_SURFACE_EN);
	REG_SET_FIELD(&(reg->rCE_CAP_CTROL), m_CE_FRAME_MODE, 0x1, b_CE_FRAME_MODE);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

int ce_set_buf(CygnusVpuSurface *surface)
{
	unsigned w, h, i, bpp;
	unsigned addr_y = 0, addr_u = 0, addr_v = 0;
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	w = surface->width;
	h = surface->height;
	addr_y = gx_virt_to_phys((unsigned)surface->buffer);
	bpp = 16;
	if (surface->color_format == GX_COLOR_FMT_YCBCR444_Y_U_V) {
		addr_u = addr_y + w*h;
		addr_v = addr_u + w*h;
		bpp = 8;
	}
	if (surface->color_format == GX_COLOR_FMT_YCBCR422_Y_UV) {
		addr_u = addr_y + w*h;
		addr_v = addr_u;
		bpp = 8;
	}

	for(i = 0; i < 4; i++) {
		REG_SET_FIELD(&(reg->ce_addr[i].rCE_Y_TOP_START_ADDR), m_CE_Y_TOP_FIELD_START_ADDR, addr_y, b_CE_Y_TOP_FIELD_START_ADDR);
		REG_SET_FIELD(&(reg->ce_addr[i].rCE_Y_BOTTOM_START_ADDR), m_CE_Y_BOTTOM_FIELD_START_ADDR, addr_y+w * bpp / 8, b_CE_Y_BOTTOM_FIELD_START_ADDR);

		REG_SET_FIELD(&(reg->ce_addr[i].rCE_U_TOP_START_ADDR), m_CE_U_TOP_FIELD_START_ADDR, addr_u, b_CE_U_TOP_FIELD_START_ADDR);
		REG_SET_FIELD(&(reg->ce_addr[i].rCE_U_BOTTOM_START_ADDR), m_CE_U_BOTTOM_FIELD_START_ADDR, addr_u+w * bpp / 8, b_CE_U_BOTTOM_FIELD_START_ADDR);

		REG_SET_FIELD(&(reg->ce_addr[i].rCE_V_TOP_START_ADDR), m_CE_V_TOP_FIELD_START_ADDR, addr_v, b_CE_V_TOP_FIELD_START_ADDR);
		REG_SET_FIELD(&(reg->ce_addr[i].rCE_V_BOTTOM_START_ADDR), m_CE_V_BOTTOM_FIELD_START_ADDR, addr_v+w * bpp / 8, b_CE_V_BOTTOM_FIELD_START_ADDR);
	}

	REG_SET_FIELD(&(reg->rCE_FRAME_WIDTH), m_CE_FRAME_STRIDE, w, b_CE_FRAME_STRIDE);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

enum {
	VOUT_I_MODE,
	VOUT_P_MODE
};

static int get_mode_type(GxVideoOutProperty_Mode mode)
{
	int ret = VOUT_I_MODE;
	switch(mode) {
		case GXAV_VOUT_480I:
		case GXAV_VOUT_576I:
		case GXAV_VOUT_1080I_50HZ:
		case GXAV_VOUT_1080I_60HZ:
			ret = VOUT_I_MODE;
		case GXAV_VOUT_480P:
		case GXAV_VOUT_576P:
		case GXAV_VOUT_720P_50HZ:
		case GXAV_VOUT_720P_60HZ:
		case GXAV_VOUT_1080P_50HZ:
		case GXAV_VOUT_1080P_60HZ:
			ret = VOUT_P_MODE;
		default:
			break;
	}

	return (ret);
}

int ce_zoom(GxAvRect *clip, GxAvRect *view)
{
	int h_zoom, v_zoom, height;
	int val = (clip->height == view->height);
	struct vout_dvemode dvemode;
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	dvemode.id = ID_VPU;
	gxav_videoout_get_dvemode(0, &dvemode);
	if(VOUT_I_MODE == get_mode_type(dvemode.mode)) {
		height = clip->height;
	} else {
		height = clip->height * 2;
	}

	/*src*/
	REG_SET_FIELD(&(reg->rCE_CAP_H), m_CAP_LEFT, clip->x, b_CAP_LEFT);
	REG_SET_FIELD(&(reg->rCE_CAP_H), m_CAP_RIGHT, clip->x+clip->width, b_CAP_RIGHT);
	REG_SET_FIELD(&(reg->rCE_CAP_V), m_CAP_TOP, clip->y, b_CAP_TOP);
	REG_SET_FIELD(&(reg->rCE_CAP_V), m_CAP_BOTTOM, clip->y+height, b_CAP_BOTTOM);
	/*dst*/
	REG_SET_FIELD(&(reg->rCE_DEST_SIZE), m_SCE_ZOOM_HEIGHT, view->height, b_SCE_ZOOM_HEIGHT);
	REG_SET_FIELD(&(reg->rCE_DEST_SIZE), m_SCE_ZOOM_LENGTH, view->width, b_SCE_ZOOM_LENGTH);
	REG_SET_FIELD(&(reg->rCE_FRAME_WIDTH), m_CE_FRAME_STRIDE, view->width, b_CE_FRAME_STRIDE);
	/*zoom*/
	h_zoom = clip->width*4096/view->width;
	REG_SET_FIELD(&(reg->rCE_ZOOM), m_CAP_H_ZOOM, h_zoom, b_CAP_H_ZOOM);

	v_zoom = height*4096/view->height;
	if (v_zoom >= 4096*4) {
		v_zoom = clip->height/2*4096/view->height;
		REG_SET_FIELD(&(reg->rCE_CAP_CTROL), m_CAP_V_DOWNSCALE, 1, b_CAP_V_DOWNSCALE);
	}
	REG_SET_FIELD(&(reg->rCE_ZOOM), m_CAP_V_ZOOM, v_zoom, b_CAP_V_ZOOM);

	/*### 依赖硬件提供参数*/
	REG_SET_FIELD(&(reg->rCE_ZOOM_PHASE), m_SCE_VT_PHASE_BIAS, val, b_SCE_VT_PHASE_BIAS);
	REG_SET_FIELD(&(reg->rCE_ZOOM_PHASE), m_SCE_VB_PHASE_BIAS, val, b_SCE_VB_PHASE_BIAS);

	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

int ce_enable(int en)
{
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	en = en&0x1;
	REG_SET_FIELD(&(reg->rCE_CAP_ENABLE), m_CAP_ENABLE, en, b_CAP_ENABLE);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

int cec_int_en(int bit)
{
/*
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FRAME_FULL_EN, val, b_INT_CE_FRAME_FULL_EN);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_IDLE_EN, val, b_INT_CE_IDLE_EN);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FIFO_FULL_EN, val, b_INT_CE_FIFO_FULL_EN);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FIFO_ALMOST_FULL_EN, val, b_INT_CE_FIFO_ALMOST_FULL_EN);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
*/
	return 0;
}

int cec_int_statue()
{
/*
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FRAME_FULL, val, b_INT_CE_FRAME_FULL);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_IDLE, val, b_INT_CE_IDLE);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FIFO_FULL, val, b_INT_CE_FIFO_FULL);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FIFO_ALMOST_FULL, val, b_INT_CE_FIFO_ALMOST_FULL);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
*/
	return 0;
}

int cec_int_clr(int bit)
{
/*
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FRAME_FULL, val, b_INT_CE_FRAME_FULL);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_IDLE, val, b_INT_CE_IDLE);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FIFO_FULL, val, b_INT_CE_FIFO_FULL);
	REG_SET_FIELD(&(reg->rCE_BUFFER_INT), m_INT_CE_FIFO_ALMOST_FULL, val, b_INT_CE_FIFO_ALMOST_FULL);
	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
*/
	return 0;
}

int ce_bind_to_vpp2(int en)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	REG_SET_FIELD(&(reg->rSYS_CTROL), m_CE_BOUND_TO_PP2_EN, en, b_CE_BOUND_TO_PP2_EN);
	return 0;
}

int ce_isr()/*是否完成，是否出错*/
{
	return 0;
}

int ce_set_access_buf(unsigned addr, unsigned len) 
{
	unsigned start_addr = gx_virt_to_phys(addr);
	struct cygnus_vpu_ce_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_CE);

	REG_SET_FIELD(&(reg->rCE_BUFF_START_ADDR), m_CE_BUFFER_ADDR, start_addr, b_CE_BUFFER_ADDR);
	REG_SET_FIELD(&(reg->rCE_BUFF_SIZE), m_CE_BUFFER_SIZE, len, b_CE_BUFFER_SIZE);

	REG_SET_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	REG_CLR_BIT(&(reg->rCE_PARA_UPDATE), b_CE_PARA_UPDATE);
	return 0;
}

