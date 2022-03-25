#include "cygnus_vpu_reg.h"
#include "cygnus_vpu_internel.h"

int vpu_get_version()
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	return REG_GET_FIELD(&(reg->rVERSION), m_VERSION, b_VERSION);
}

int vpu_all_layers_en(int en)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	REG_SET_FIELD(&(reg->rSYS_CTROL), m_DISPLAY_OFF, ((!en)&0x1), b_DISPLAY_OFF);
	return 0;
}

int vpu_int_en(int bit)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	switch (bit) {
	case VPU_INT_ALL_LAYER_IDLE :
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_IDLE);
		break;
	case VPU_INT_LINE:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_LINE);
		break;
	case VPU_INT_FIELD_START:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FIELDSTART);
		break;
	case VPU_INT_FRAME_START:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FRAMESTART);
		break;
	case VPU_INT_FIELD_END:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FIELDEND);
		break;
	case VPU_INT_FRAME_END:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FRAMEEND);
		break;
	case VPU_INT_PP0:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP0);
		break;
	case VPU_INT_PP1:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP1);
		break;
	case VPU_INT_PIC:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PIC);
		break;
	case VPU_INT_OSD0:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_OSD0);
		break;
	case VPU_INT_OSD1:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_OSD1);
		break;
	case VPU_INT_PP2:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP2);
		break;
	case VPU_INT_CE:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_CE);
		break;
	default:
		ret = -1;
	}

	return ret;
}

int vpu_int_dis(int bit)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	switch (bit) {
	case VPU_INT_ALL_LAYER_IDLE :
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_IDLE);
		break;
	case VPU_INT_LINE:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_LINE);
		break;
	case VPU_INT_FIELD_START:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FIELDSTART);
		break;
	case VPU_INT_FRAME_START:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FRAMESTART);
		break;
	case VPU_INT_FIELD_END:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FIELDEND);
		break;
	case VPU_INT_FRAME_END:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FRAMEEND);
		break;
	case VPU_INT_PP0:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP0);
		break;
	case VPU_INT_PP1:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP1);
		break;
	case VPU_INT_PIC:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PIC);
		break;
	case VPU_INT_OSD0:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_OSD0);
		break;
	case VPU_INT_OSD1:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_OSD1);
		break;
	case VPU_INT_PP2:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP2);
		break;
	case VPU_INT_CE:
		REG_CLR_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_CE);
		break;
	default:
		ret = -1;
	}

	return ret;
}

int vpu_int_en_status(int bit)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	switch (bit) {
	case VPU_INT_ALL_LAYER_IDLE :
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_IDLE);
		break;
	case VPU_INT_LINE:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_LINE);
		break;
	case VPU_INT_FIELD_START:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FIELDSTART);
		break;
	case VPU_INT_FRAME_START:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FRAMESTART);
		break;
	case VPU_INT_FIELD_END:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FIELDEND);
		break;
	case VPU_INT_FRAME_END:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_FRAMEEND);
		break;
	case VPU_INT_PP0:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP0);
		break;
	case VPU_INT_PP1:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP1);
		break;
	case VPU_INT_PIC:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PIC);
		break;
	case VPU_INT_OSD0:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_OSD0);
		break;
	case VPU_INT_OSD1:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_OSD1);
		break;
	case VPU_INT_PP2:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_PP2);
		break;
	case VPU_INT_CE:
		REG_GET_BIT(&(reg->rINT_MASK), b_INTERRPUTMASK_CE);
		break;
	default:
		ret = -1;
	}

	return ret;
}

int vpu_int_clr(int bit)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	switch (bit) {
	case VPU_INT_ALL_LAYER_IDLE :
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_IDLE);
		break;
	case VPU_INT_LINE:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_LINE);
		break;
	case VPU_INT_FIELD_START:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_FIELDSTART);
		break;
	case VPU_INT_FRAME_START:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_FRAMESTART);
		break;
	case VPU_INT_FIELD_END:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_FIELDEND);
		break;
	case VPU_INT_FRAME_END:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_FRAMEEND);
		break;
	case VPU_INT_PP0:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_PP0);
		break;
	case VPU_INT_PP1:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_PP1);
		break;
	case VPU_INT_PIC:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_PIC);
		break;
	case VPU_INT_OSD0:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_OSD0);
		break;
	case VPU_INT_OSD1:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_OSD1);
		break;
	case VPU_INT_PP2:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_PP2);
		break;
	case VPU_INT_CE:
		REG_SET_BIT(&(reg->rINT_CLEAR), b_INTERRPUTMASK_CE);
		break;
	default:
		ret = -1;
	}

	return ret;
}

int vpu_int_status(int bit)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	switch (bit) {
	case VPU_INT_ALL_LAYER_IDLE :
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_IDLE);
		break;
	case VPU_INT_LINE:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_LINE);
		break;
	case VPU_INT_FIELD_START:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_FIELDSTART);
		break;
	case VPU_INT_FRAME_START:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_FRAMESTART);
		break;
	case VPU_INT_FIELD_END:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_FIELDEND);
		break;
	case VPU_INT_FRAME_END:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_FRAMEEND);
		break;
	case VPU_INT_PP0:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_PP0);
		break;
	case VPU_INT_PP1:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_PP1);
		break;
	case VPU_INT_PIC:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_PIC);
		break;
	case VPU_INT_OSD0:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_OSD0);
		break;
	case VPU_INT_OSD1:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_OSD1);
		break;
	case VPU_INT_PP2:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_PP2);
		break;
	case VPU_INT_CE:
		REG_GET_BIT(&(reg->rINT_STATUS), b_INTERRPUTMASK_CE);
		break;
	default:
		ret = -1;
	}

	return ret;
}

int vpu_dac_en(GxVpuDacID dac_id, int en)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	if(en)
		REG_CLR_BIT(&(reg->rDAC_POWER), b_ALL_POWERDOWN);
	else
		REG_SET_BIT(&(reg->rDAC_POWER), b_ALL_POWERDOWN);

	if (dac_id & VPU_DAC_0) {
		if (en)
			REG_CLR_BIT(&(reg->rDAC_POWER), b_DAC0_POWERDOWN);
		else
			REG_SET_BIT(&(reg->rDAC_POWER), b_DAC0_POWERDOWN);
	}
	if (dac_id & VPU_DAC_1) {
		if (en)
			REG_CLR_BIT(&(reg->rDAC_POWER), b_DAC1_POWERDOWN);
		else
			REG_SET_BIT(&(reg->rDAC_POWER), b_DAC1_POWERDOWN);
	}
	if (dac_id & VPU_DAC_2) {
		if (en)
			REG_CLR_BIT(&(reg->rDAC_POWER), b_DAC2_POWERDOWN);
		else
			REG_SET_BIT(&(reg->rDAC_POWER), b_DAC2_POWERDOWN);
	}
	if (dac_id & VPU_DAC_3) {
		if (en)
			REG_CLR_BIT(&(reg->rDAC_POWER), b_DAC3_POWERDOWN);
		else
			REG_SET_BIT(&(reg->rDAC_POWER), b_DAC3_POWERDOWN);
	}

	return 0;
}

int vpu_dac_status(GxVpuDacID dac_id)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	if (reg && dac_id>=VPU_DAC_0 && dac_id<=VPU_DAC_ALL) {
		if (REG_GET_BIT(&reg->rDAC_POWER, b_ALL_POWERDOWN) == 1) {
			if((dac_id&VPU_DAC_0) == VPU_DAC_0)
				ret |= (REG_GET_BIT(&(reg->rDAC_POWER), b_DAC0_POWERDOWN) << VPU_DAC_0);
			if((dac_id&VPU_DAC_1) == VPU_DAC_1)
				ret |= (REG_GET_BIT(&(reg->rDAC_POWER), b_DAC1_POWERDOWN) << VPU_DAC_1);
			if((dac_id&VPU_DAC_2) == VPU_DAC_2)
				ret |= (REG_GET_BIT(&(reg->rDAC_POWER), b_DAC2_POWERDOWN) << VPU_DAC_2);
			if((dac_id&VPU_DAC_3) == VPU_DAC_3)
				ret |= (REG_GET_BIT(&(reg->rDAC_POWER), b_DAC3_POWERDOWN) << VPU_DAC_3);
		}
	}

	return ret;
}

int vpu_dac_auto_en(int en)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC_POWERDOWN_BYSELF, (en&0x1), b_DAC_POWERDOWN_BYSELF);
	return 0;
}

int vpu_dac_gain(int id, int val)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	if (id == VPU_DAC_0)
		REG_SET_FIELD(&(reg->rDAC_GAIN), m_GDAC0_IPS, val, b_GDAC0_IPS);
	if (id == VPU_DAC_1)
		REG_SET_FIELD(&(reg->rDAC_GAIN), m_GDAC1_IPS, val, b_GDAC1_IPS);
	if (id == VPU_DAC_2)
		REG_SET_FIELD(&(reg->rDAC_GAIN), m_GDAC2_IPS, val, b_GDAC2_IPS);
	if (id == VPU_DAC_3)
		REG_SET_FIELD(&(reg->rDAC_GAIN), m_GDAC3_IPS, val, b_GDAC3_IPS);
	return 0;
}

int vpu_dac_bind(int dac_id, int data_channel_id)
{
/*
	REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC_3_FLAG, val, b_DAC_3_FLAG);
	REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC_2_FLAG, val, b_DAC_2_FLAG);
	REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC_1_FLAG, val, b_DAC_1_FLAG);
	REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC_0_FLAG, val, b_DAC_0_FLAG);
*/
	return 0;
}

int vpu_dac_set_source(int dac_id, GxVpuMixerID src)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	if (dac_id == VPU_DAC_0)
		REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC0_SOURCE_SEL, (src&0x1), b_DAC0_SOURCE_SEL);
	if (dac_id == VPU_DAC_1)
		REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC1_SOURCE_SEL, (src&0x1), b_DAC1_SOURCE_SEL);
	if (dac_id == VPU_DAC_2)
		REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC2_SOURCE_SEL, (src&0x1), b_DAC2_SOURCE_SEL);
	if (dac_id == VPU_DAC_3)
		REG_SET_FIELD(&(reg->rDAC_SOURCE_SEL), m_DAC3_SOURCE_SEL, (src&0x1), b_DAC3_SOURCE_SEL);

	return 0;
}

/* vbi */
int vpu_vbi_en(int en)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	REG_SET_FIELD(&(reg->rVBI_CTRL), m_VBI_EN, (en&0x1), b_VBI_EN);
	return 0;
}

int vpu_vbi_set_data_length(int len)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	REG_SET_FIELD(&(reg->rVBI_CTRL), m_VBI_LEN, len, b_VBI_LEN);
	return 0;
}

int vpu_vbi_set_data_addr(int addr)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	REG_SET_FIELD(&(reg->rVBI_FIRST_ADDR), m_VBI_FIRST_ADDR, addr, b_VBI_FIRST_ADDR);
	return 0;
}

int vpu_vbi_set_endian(int endian)//参数取值如何确定？文档不详细
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	REG_SET_FIELD(&(reg->rVBI_CTRL), m_VBI_ENDPOINT, endian, b_VBI_ENDPOINT);
	return 0;
}

typedef enum {
	MIXER_LAYER_NONE,
	MIXER_LAYER_PIC,
	MIXER_LAYER_PP0,
	MIXER_LAYER_PP1,
	MIXER_LAYER_OSD,
	MIXER_LAYER_OSD1
} MixerInnerLayer;

static MixerInnerLayer mixer_get_layer(GxLayerID layer)
{
	switch(layer)
	{
		case GX_LAYER_OSD:
			return (MIXER_LAYER_OSD);
		case GX_LAYER_SPP:
			return (MIXER_LAYER_PIC);
		case GX_LAYER_VPP:
			return (MIXER_LAYER_PP0);
		default:
			return (MIXER_LAYER_NONE);
	}
}

int vpu_mixer_set_layers(GxVpuMixerLayers *cfg)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	int layer = MIXER_LAYER_NONE;

	if (cfg->id == HD_MIXER) {
		if (cfg->layers[0] != GX_LAYER_NONE) {
			layer = mixer_get_layer(cfg->layers[0]);
			REG_SET_FIELD(&(reg->rHD_MIX_LAYER_SEL), m_HD_LAYER0_SEL, layer, b_HD_LAYER0_SEL);
		}
		if (cfg->layers[1] != GX_LAYER_NONE) {
			layer = mixer_get_layer(cfg->layers[1]);
			REG_SET_FIELD(&(reg->rHD_MIX_LAYER_SEL), m_HD_LAYER1_SEL, layer, b_HD_LAYER1_SEL);
		}
		if (cfg->layers[2] != GX_LAYER_NONE) {
			layer = mixer_get_layer(cfg->layers[2]);
			REG_SET_FIELD(&(reg->rHD_MIX_LAYER_SEL), m_HD_LAYER2_SEL, layer, b_HD_LAYER2_SEL);
		}
	}

	if (cfg->id == SD_MIXER) {
		if (cfg->layers[0] != GX_LAYER_NONE) {
			layer = mixer_get_layer(cfg->layers[0]);
			REG_SET_FIELD(&(reg->rSD_MIX_LAYER_SEL), m_SD_LAYER0_SEL, layer, b_SD_LAYER0_SEL);
		}
		if (cfg->layers[1] != GX_LAYER_NONE) {
			layer = mixer_get_layer(cfg->layers[1]);
			REG_SET_FIELD(&(reg->rSD_MIX_LAYER_SEL), m_SD_LAYER1_SEL, layer, b_SD_LAYER1_SEL);
		}
		if (cfg->layers[2] != GX_LAYER_NONE) {
			layer = mixer_get_layer(cfg->layers[2]);
			REG_SET_FIELD(&(reg->rSD_MIX_LAYER_SEL), m_SD_LAYER2_SEL, layer, b_SD_LAYER2_SEL);
		}
	}
	return 0;
}

int vpu_mixer_set_backcolor(GxVpuMixerID id, GxColor color)
{
	unsigned val = color.y << 16 | color.cb << 8 | color.cr;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	if (id == HD_MIXER)
		REG_SET_FIELD(&(reg->rHD_MIX_BACK_COLOR), m_HDP_BACKCOLOR, val, b_HDP_BACKCOLOR);
	if (id == SD_MIXER)
		REG_SET_FIELD(&(reg->rSD_MIX_BACK_COLOR), m_SDP_BACKCOLOR, val, b_SDP_BACKCOLOR);
	return 0;
}

int vpu_bbt_crc_en(int en)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	REG_SET_FIELD(&(reg->rBBT_TEST), m_CRC_EN, 1, b_CRC_EN);
	return 0;
}

int vpu_bbt_crc_mode(int mode)
{
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);
	REG_SET_FIELD(&(reg->rBBT_TEST), m_CRC_MODE, (mode&0x1), b_CRC_MODE);
	return 0;
}

enum {
	CRC_VOUT_CHANNEL_0,
	CRC_VOUT_CHANNEL_1,
	CRC_VOUT_CHANNEL_2,
};
int vpu_bbt_read_crc(int channel_id)
{
	int ret = 0;
	struct cygnus_vpu_sys_reg *reg = get_reg_by_sub_id(CYGNUS_VPU_SUB_SYS);

	if (channel_id == CRC_VOUT_CHANNEL_0)
		ret = REG_GET_FIELD(&(reg->rBBT_CRC0), m_CRC_OUT0, b_CRC_OUT0);
	if (channel_id == CRC_VOUT_CHANNEL_1)
		ret = REG_GET_FIELD(&(reg->rBBT_CRC0), m_CRC_OUT1, b_CRC_OUT1);
	if (channel_id == CRC_VOUT_CHANNEL_2)
		ret = REG_GET_FIELD(&(reg->rBBT_CRC1), m_CRC_OUT2, b_CRC_OUT2);

	return ret;
}

