#include "kernelcalls.h"
#include "gxav_common.h"
#include "include/audio_common.h"
#include "gxav_audioout_propertytypes.h"
#include "gx32xx_out.h"
#include "gx32xx_reg.h"
#include "gx32xx_i2s.h"

#define CHECK_I2S_RETURN(reg) if (reg == NULL) return -1;

struct i2s_modules {
	unsigned int        i2s_addr;
	unsigned int        sdc_addr;
	struct i2s_regs     *i2s_reg;
	struct i2s_sdc_regs *sdc_reg;
};

struct i2s_modules i2s_module[2] = {{0, 0, NULL, NULL}, {0, 0, NULL, NULL}};

int gxav_set_i2s_dac_params(int id, struct audio_dac* dac)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_I2S_SET_CLK_SEL(i2s_module[id].i2s_reg,      dac->mclock);
	AUDIO_I2S_SET_BCK_SEL(i2s_module[id].i2s_reg,      dac->bclock);
	AUDIO_I2S_SET_PCM_FORMAT(i2s_module[id].i2s_reg,   dac->format);
	AUDIO_I2S_SET_PCM_WORD_LEN(i2s_module[id].i2s_reg, dac->wordlen);
	AUDIO_I2S_SET_SILENT(i2s_module[id].i2s_reg);

	return 0;
}

int gxav_set_i2s_dac_mix(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	if (gxav_get_i2s_multi()) {
		AUDIO_MIX_PCM_TO_I2S(i2s_module[id].i2s_reg, value);
		AUDIO_MIX_PCM_TO_SPD(i2s_module[id].i2s_reg, value);
	}

	return 0;
}

int gxav_set_i2s_channel_init(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_CHANNLE_L_SEL(i2s_module[id].i2s_reg,   AUDIO_SELECT_0_CHANNEL);
	AUDIO_CHANNLE_R_SEL(i2s_module[id].i2s_reg,   AUDIO_SELECT_1_CHANNEL);
	AUDIO_CHANNLE_LS_SEL(i2s_module[id].i2s_reg,  AUDIO_SELECT_2_CHANNEL);
	AUDIO_CHANNLE_RS_SEL(i2s_module[id].i2s_reg,  AUDIO_SELECT_3_CHANNEL);
	AUDIO_CHANNLE_C_SEL(i2s_module[id].i2s_reg,   AUDIO_SELECT_4_CHANNEL);
	AUDIO_CHANNLE_LFE_SEL(i2s_module[id].i2s_reg, AUDIO_SELECT_5_CHANNEL);
	AUDIO_CHANNEL_LBS_SEL(i2s_module[id].i2s_reg, AUDIO_SELECT_6_CHANNEL);
	AUDIO_CHANNEL_RBS_SEL(i2s_module[id].i2s_reg, AUDIO_SELECT_7_CHANNEL);

	return 0;
}

int gxav_set_i2s_channel_mute(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_CHANNLE_L_SEL(i2s_module[id].i2s_reg,   0x08);
	AUDIO_CHANNLE_R_SEL(i2s_module[id].i2s_reg,   0x08);
	AUDIO_CHANNLE_LS_SEL(i2s_module[id].i2s_reg,  0x08);
	AUDIO_CHANNLE_RS_SEL(i2s_module[id].i2s_reg,  0x08);
	AUDIO_CHANNLE_C_SEL(i2s_module[id].i2s_reg,   0x08);
	AUDIO_CHANNLE_LFE_SEL(i2s_module[id].i2s_reg, 0x08);
	AUDIO_CHANNEL_LBS_SEL(i2s_module[id].i2s_reg, 0x08);
	AUDIO_CHANNEL_RBS_SEL(i2s_module[id].i2s_reg, 0x08);

	return 0 ;
}

int gxav_get_i2s_clock(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	return AUDIO_GET_I2S_MODULE_CLK(i2s_module[id].i2s_reg);
}

int gxav_set_i2s_channel_output(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_SET_SRL_CHANNLE_OUTPUT(i2s_module[id].i2s_reg, value);
	return 0;
}

int gxav_get_i2s_channel_output(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	return AUDIO_GET_SRL_CHANNLE_OUTPUT(i2s_module[id].i2s_reg);
}

int gxav_set_i2s_channel_nums(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_CONFIG_PCM_CHANNEL_NUMS(i2s_module[id].i2s_reg, value);
	return 0;
}

int gxav_set_i2s_magnification(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_SET_PCM_MAGNIFICATION(i2s_module[id].i2s_reg, value);
	return 0;
}

int gxav_set_i2s_src_enable(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	if (value == 1)
		AUDIO_I2S_SET_DO_SRC_EN(i2s_module[id].i2s_reg);
	else
		AUDIO_I2S_CLR_DO_SRC_EN(i2s_module[id].i2s_reg);

	return 0;
}

int gxav_set_i2s_interlace(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	if (value == 1)
		AUDIO_SAMPLE_DATA_STORE_INTERLACE(i2s_module[id].i2s_reg);
	else
		AUDIO_SAMPLE_DATA_STORE_NON_INTERLACE(i2s_module[id].i2s_reg);

	return 0;
}

int gxav_set_i2s_endian(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_SET_SAMPLE_DATA_ENDIAN(i2s_module[id].i2s_reg, 0x0);

	return 0;
}

int gxav_init_i2s_isr(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_SET_SRL_EXPORT_DATA_TIMES(i2s_module[id].i2s_reg, 0x01);
	AUDIO_PLAY_BACK_ENABLE(i2s_module[id].i2s_reg);
	AUDIO_I2S_MODULE_ENABLE(i2s_module[id].i2s_reg);
	AUDIO_I2S_SET_NEWPCM_INT_CLR(i2s_module[id].i2s_reg);
	AUDIO_I2S_SET_NEWPCM_INT_ENABLE(i2s_module[id].i2s_reg);
	AUDIO_REQUEST_TO_READ(i2s_module[id].i2s_reg);

	return 0;
}

int gxav_set_i2s_isr_enable(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	if (value == 1)
		AUDIO_I2S_SET_NEWPCM_INT_ENABLE(i2s_module[id].i2s_reg);
	else
		AUDIO_I2S_SET_NEWPCM_INT_DISABLE(i2s_module[id].i2s_reg);

	return 0;
}

int gxav_clr_i2s_isr_status(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_I2S_SET_NEWPCM_INT_CLR(i2s_module[id].i2s_reg);

	return 0;
}

int gxav_set_i2s_sample_data_len(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_SAMPLE_DATA_LEN_SEL(i2s_module[id].i2s_reg, value);
	return 0;
}

int gxav_set_i2s_buffer(int id, unsigned int s_addr, int size)
{
	CHECK_I2S_RETURN(i2s_module[id].sdc_reg);

	AUDIO_SET_I2S_BUFFER_START_ADDR(i2s_module[id].sdc_reg, s_addr);
	AUDIO_SET_I2S_BUFFER_SIZE(i2s_module[id].sdc_reg, size);

	return 0;
}

int gxav_set_i2s_load_data(int id, unsigned int s_addr, unsigned int e_addr, int len)
{
	CHECK_I2S_RETURN(i2s_module[id].sdc_reg);

	AUDIO_SET_I2S_NEWFRAME_START_ADDR(i2s_module[id].sdc_reg, s_addr);
	AUDIO_SET_I2S_NEWFRAME_END_ADDR  (i2s_module[id].sdc_reg, e_addr);
	AUDIO_SET_I2S_NEWFRAME_PCMLEN    (i2s_module[id].sdc_reg, len);

	return 0;
}

int gxav_set_i2s_new_frame(int id, int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);
	CHECK_I2S_RETURN(i2s_module[id].sdc_reg);

	if (value == 0) {
		if (CHIP_IS_GX3201 == 0)
			SET_BIT(i2s_module[id].sdc_reg->audio_play_i2s_new_frame_ctrl, 1);
	} else {
		if (CHIP_IS_GX3201 == 0)
			SET_BIT(i2s_module[id].sdc_reg->audio_play_i2s_new_frame_ctrl, 2);
		else
			SET_BIT(i2s_module[id].i2s_reg->audio_play_i2s_indata, 13);
	}
	return 0;
}

int gxav_get_i2s_isr_status(int id)
{
	unsigned int I2S_int, I2S_int_en;

	if (i2s_module[id].i2s_reg == NULL) return 0;

	I2S_int    = AUDIO_I2S_GET_INT_STATUS(i2s_module[id].i2s_reg);
	I2S_int_en = AUDIO_I2S_GET_INT_EN_VAL(i2s_module[id].i2s_reg);

	return (I2S_int & I2S_int_en & 0x002);
}

int gxav_get_i2s_data_len(int id)
{
	unsigned int end_addr, play_addr, buf_size;

	CHECK_I2S_RETURN(i2s_module[id].sdc_reg);

	buf_size  = AUDIO_GET_I2S_BUFFER_SIZE(i2s_module[id].sdc_reg);
	end_addr  = AUDIO_GET_I2S_NEWFRAME_END_ADDR(i2s_module[id].sdc_reg)+1;
	play_addr = AUDIO_GET_I2S_BUFFER_SDC_ADDR(i2s_module[id].sdc_reg);

	if (buf_size == 0)
		return 0;

	return (buf_size + end_addr - play_addr)%buf_size;
}

int gxav_set_i2s_pcm_source_sel(int id, enum i2s_pcm_mode mode)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_I2S_SET_PCM_SOURCE_SEL(i2s_module[id].i2s_reg, mode);
	return 0;
}

int gxav_set_i2s_cpu_set_pcm(int id, unsigned int value)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_I2S_CPU_SET_PCM(i2s_module[id].i2s_reg, value);
	return 0;
}

int gxav_set_i2s_work_enable(int id)
{
	CHECK_I2S_RETURN(i2s_module[id].i2s_reg);

	AUDIO_PLAY_BACK_ENABLE(i2s_module[id].i2s_reg);
	AUDIO_I2S_MODULE_ENABLE(i2s_module[id].i2s_reg);
	AUDIO_SET_SRL_EXPORT_DATA_TIMES(i2s_module[id].i2s_reg, 1);
	return 0;
}

int gxav_init_i2s_module(int id)
{
	int gx6605s_chip = CHIP_IS_GX6605S;

	if ((!NO_MULTI_I2S && (((id == 1) && !gx6605s_chip) || (id >= 2))) || (NO_MULTI_I2S && (id >= 1)))
		return -1;

	if (i2s_module[id].i2s_reg && i2s_module[id].sdc_reg)
		return 0;

	if (id == 0) {
		i2s_module[id].i2s_addr = I2S_PLAY_BASE_ADDR;
		i2s_module[id].sdc_addr  = I2S_PLAY_SDC_BASE_ADDR;
	} else if (id == 1) {
		i2s_module[id].i2s_addr = I2S_PLAY_BASE_ADDR1;
		i2s_module[id].sdc_addr  = I2S_PLAY_SDC_BASE_ADDR1;
	}

	if (NULL == gx_request_mem_region(i2s_module[id].i2s_addr, sizeof(struct i2s_regs)))
		goto error_i2s_init;

	if (NULL == gx_request_mem_region(i2s_module[id].sdc_addr, sizeof(struct i2s_sdc_regs)))
		goto error_i2s_init;

	i2s_module[id].i2s_reg = (struct i2s_regs*)gx_ioremap(i2s_module[id].i2s_addr, sizeof(struct i2s_regs));
	if (i2s_module[id].i2s_reg == NULL)
		goto error_i2s_init;

	i2s_module[id].sdc_reg = (struct i2s_sdc_regs*)gx_ioremap(i2s_module[id].sdc_addr, sizeof(struct i2s_sdc_regs));
	if (i2s_module[id].sdc_reg == NULL)
		goto error_i2s_init;

	return 0;

error_i2s_init:
	if (i2s_module[id].i2s_reg) {
		gx_iounmap(i2s_module[id].i2s_reg);
		gx_release_mem_region(i2s_module[id].i2s_addr, sizeof(struct i2s_regs));
		i2s_module[id].i2s_reg  = NULL;
		i2s_module[id].i2s_addr = 0;
	}

	if (i2s_module[id].sdc_reg) {
		gx_iounmap(i2s_module[id].sdc_reg);
		gx_release_mem_region(i2s_module[id].sdc_addr, sizeof(struct i2s_sdc_regs));
		i2s_module[id].sdc_reg  = NULL;
		i2s_module[id].sdc_addr = 0;
	}
	return -1;
}

int gxav_uinit_i2s_module(int id)
{
	int gx6605s_chip = CHIP_IS_GX6605S;

	if ((!NO_MULTI_I2S && (((id == 1) && !gx6605s_chip) || (id >= 2))) || (NO_MULTI_I2S && (id >= 1)))
		return -1;

	if (i2s_module[id].i2s_reg) {
		gx_iounmap(i2s_module[id].i2s_reg);
		gx_release_mem_region(i2s_module[id].i2s_addr, sizeof(struct i2s_regs));
		i2s_module[id].i2s_reg  = NULL;
		i2s_module[id].i2s_addr = 0;
	}

	if (i2s_module[id].sdc_reg) {
		gx_iounmap(i2s_module[id].sdc_reg);
		gx_release_mem_region(i2s_module[id].sdc_addr, sizeof(struct i2s_sdc_regs));
		i2s_module[id].sdc_reg  = NULL;
		i2s_module[id].sdc_addr = 0;
	}

	return 0;
}

int gxav_get_i2s_multi(void)
{
	int i = 0, count = 0;

	for (i = 0; i < 2; i++) {
		if (i2s_module[i].i2s_reg && i2s_module[i].sdc_reg)
			count++;
	}

	return ((count > 1)?1:0);
}
