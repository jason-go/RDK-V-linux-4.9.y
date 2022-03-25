#ifndef __SIRIUS_REG_H__
#define __SIRIUS_REG_H__

typedef struct {
	unsigned int AUDIO_PLAY_I2S_DACINFO;       //0x00
	unsigned int AUDIO_PLAY_I2S_INDATA;
	unsigned int AUDIO_PLAY_I2S_OUTDATA;
	unsigned int AUDIO_PLAY_I2S_CHANNELsel;
	unsigned int AUDIO_PLAY_I2S_CPUSetPCM;     //0x10
	unsigned int AUDIO_PLAY_I2S_FIFOCTRL;
	unsigned int AUDIO_PLAY_I2S_DATAINV;
	unsigned int AUDIO_PLAY_I2S_INTEN;
	unsigned int AUDIO_PLAY_I2S_INT;           //0x20
	unsigned int AUDIO_PLAY_I2S_TIME_GATE;
	unsigned int AUDIO_PLAY_I2S_playingINDATA;
} AoutI2SRegs;

typedef struct {
	unsigned int AUDIO_PLAY_I2S_BUFFER_START_ADDR;        //0x00
	unsigned int AUDIO_PLAY_I2S_BUFFER_SIZE;
	unsigned int AUDIO_PLAY_I2S_BUFFER_SDC_ADDR;
	unsigned int AUDIO_PLAY_I2S_newFRAME_START_ADDR;
	unsigned int AUDIO_PLAY_I2S_newFRAME_END_ADDR;        //0x10
	unsigned int AUDIO_PLAY_I2S_playingFRAME_START_ADDR;
	unsigned int AUDIO_PLAY_I2S_playingFRAME_END_ADDR;
	unsigned int AUDIO_PLAY_I2S_newFrame_pcmLEN;
	unsigned int AUDIO_PLAY_I2S_playingFrame_pcmLEN;      //0x20
	unsigned int AUDIO_PLAY_I2S_RES0[3];
	unsigned int AUDIO_PLAY_I2S_SET_NEWFRAME_CONTROL;     //0x30
} AoutSdcRegs;

typedef struct {
	unsigned int AUDIO_SPDIF_CTRL;                     //0x00
	unsigned int AUDIO_SPDIF_INPUTDATA_INFO;
	unsigned int AUDIO_SPDIF_INT_EN;
	unsigned int AUDIO_SPDIF_INT;
	unsigned int AUDIO_SPDIF_PA_PB;                    //0x10
	unsigned int AUDIO_SPDIF_PC_PD;
	unsigned int AUDIO_SPDIF_CL1;
	unsigned int AUDIO_SPDIF_CL2;
	unsigned int AUDIO_SPDIF_CR1;                      //0x20
	unsigned int AUDIO_SPDIF_CR2;
	unsigned int AUDIO_SPDIF_U;
	unsigned int AUDIO_SPDIF_LAT_PAUSE;
	unsigned int AUDIO_SPDIF_DAT_PAU_LEN;              //0x30
	unsigned int AUDIO_SPDIF_newFRAME_LEN;
	unsigned int AUDIO_SPDIF_newFRAME_START_ADDR;
	unsigned int AUDIO_SPDIF_newFRAME_END_ADDR;
	unsigned int AUDIO_SPDIF_BUFFER_START_ADDR;        //0x40
	unsigned int AUDIO_SPDIF_BUFFER_SIZE;
	unsigned int AUDIO_SPDIF_PAUSE_PC_PD;
	unsigned int AUDIO_SPDIF_playingFRAME_LEN;
	unsigned int AUDIO_SPDIF_playingFRAME_START_ADDR;  //0x50
	unsigned int AUDIO_SPDIF_playingFRAME_END_ADDR;
	unsigned int AUDIO_SPDIF_BUFFER_READ_ADDR;
	unsigned int AUDIO_SPDIF_playingPCMFRAME_INFO;
	unsigned int AUDIO_SPDIF_PCM_TIME_GATE;           //0x60
	unsigned int AUDIO_SPDIF_RUBISH_DATA_SADDR;
	unsigned int AUDIO_SPDIF_RUBISH_DATA_LENGTH;
	unsigned int AUDIO_SPDIF_newFrame_pcmLEN;
	unsigned int AUDIO_SPDIF_playingFrame_pcmLEN;     //0x70
} AoutSpdRegs_0;

typedef struct {
	unsigned int AUDIO_SPDIF_CTRL;                     //0x00
	unsigned int AUDIO_SPDIF_INPUTDATA_INFO;
	unsigned int AUDIO_SPDIF_INT_EN;
	unsigned int AUDIO_SPDIF_INT;
	unsigned int AUDIO_SPDIF_PA_PB;                    //0x10
	unsigned int AUDIO_SPDIF_PC_PD;
	unsigned int AUDIO_SPDIF_CL1;
	unsigned int AUDIO_SPDIF_CL2;
	unsigned int AUDIO_SPDIF_CR1;                      //0x20
	unsigned int AUDIO_SPDIF_CR2;
	unsigned int AUDIO_SPDIF_U;
	unsigned int AUDIO_SPDIF_LAT_PAUSE;
	unsigned int AUDIO_SPDIF_DAT_PAU_LEN;              //0x30
	unsigned int AUDIO_SPDIF_newFRAME_LEN;
	unsigned int AUDIO_SPDIF_newFRAME_START_ADDR;
	unsigned int AUDIO_SPDIF_newFRAME_END_ADDR;
	unsigned int AUDIO_SPDIF_BUFFER_START_ADDR;        //0x40
	unsigned int AUDIO_SPDIF_BUFFER_SIZE;
	unsigned int AUDIO_SPDIF_PAUSE_PC_PD;
	unsigned int AUDIO_SPDIF_playingFRAME_LEN;
	unsigned int AUDIO_SPDIF_playingFRAME_START_ADDR;  //0x50
	unsigned int AUDIO_SPDIF_playingFRAME_END_ADDR;
	unsigned int AUDIO_SPDIF_BUFFER_READ_ADDR;
	unsigned int AUDIO_SPDIF_RUBISH_DATA_SADDR;
	unsigned int AUDIO_SPDIF_RUBISH_DATA_LENGTH;      //0x60
	unsigned int AUDIO_SPDIF_CL3;
	unsigned int AUDIO_SPDIF_CL4;
	unsigned int AUDIO_SPDIF_CL5;
	unsigned int AUDIO_SPDIF_CL6;                     //0x70
	unsigned int AUDIO_SPDIF_CR3;
	unsigned int AUDIO_SPDIF_CR4;
	unsigned int AUDIO_SPDIF_CR5;
	unsigned int AUDIO_SPDIF_CR6;                     //0x80
	unsigned int AUDIO_SPDIF_COUNT_STUFF;
	unsigned int AUDIO_SPDIF_NDS_CONFIG;
	unsigned int AUDIO_SPDIF_PCM_DMA_VOL_CTRL;
	unsigned int AUDIO_SPDIF_FRAME_MUTE_CTRL;
} AoutSpdRegs_1;

static unsigned int mask[33] = {
	0x00000000,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
	0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

#define AOUT_REG_SET_VAL(reg, value) do {                                \
	(*(volatile unsigned int *)reg) = value;                             \
} while(0)

#define AOUT_REG_GET_VAL(reg)                                            \
	(*(volatile unsigned int *)reg)

#define AOUT_REG_SET_FIELD(reg, val, c, offset) do {                     \
	unsigned int Reg = *(volatile unsigned int *)reg;                    \
	Reg &= ~((mask[c])<<(offset));                                       \
	Reg |= ((val)&(mask[c]))<<(offset);                                  \
	(*(volatile unsigned int *)reg) = Reg;                               \
} while(0)

#define AOUT_REG_GET_FIELD(reg, c, offset)                               \
	(((*(volatile unsigned int *)reg)>>(offset))&(mask[c]))

typedef struct {
	unsigned int   i2sAddr_0;
	unsigned int   i2sAddr_1;
	unsigned int   sdcAddr_0;
	unsigned int   sdcAddr_1;
	unsigned int   spdAddr_0;
	unsigned int   spdAddr_1;
	unsigned int   runClock;
	AoutI2SRegs   *i2sRegs_0;  //baseAddr: Audio_play_I2S Reg
	AoutI2SRegs   *i2sRegs_1;  //baseAddr: Audio_play_I2S Reg + 0x40
	AoutSdcRegs   *sdcRegs_0;  //baseAddr: Audio_play_sdc Reg
	AoutSdcRegs   *sdcRegs_1;  //baseAddr: Audio_play_Ad_sdc Reg
	AoutSpdRegs_0 *spdRegs_2;  //baseAddr: Audio_play_SPDIF_ADDR
	AoutSpdRegs_1 *spdRegs_3;  //baseAddr: Audio_play_SPDIF_ADDR + 0x70
} AoutRegs;

static void aout_enable_pcm_slow_vol(AoutRegs *optReg, int enable)
{
	enable = (enable == 0) ? 0 : 1;
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO),     enable, 1, 14);
	AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), enable, 1, 28);
}

static void aout_config_pcm_slow_vol(AoutRegs *optReg, AoutStreamSlowVol stepNum, AoutStreamSlowPoints stepLen)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), stepNum, 3, 27);
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), stepLen, 3, 24);
}

static void aout_enable_cddata_cache(AoutRegs *optReg, int enable)
{
	enable = (enable == 0) ? 0 : 1;
	AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL),  enable, 1, 4);
	AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL),  enable, 1, 12);
}

static void aout_config_cddate_maxnum(AoutRegs *optReg, unsigned int value)
{
	AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), value, 4, 23);
}

static void aout_enable_request_reset(AoutRegs *optReg, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), enable, 1, 31);
}

static void aout_enable_request_reset_irq(AoutRegs *optReg, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), enable, 1, 8);
}

static unsigned int aout_get_request_reset_irq_state(AoutRegs *optReg)
{
	return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), 1, 8);
}

static unsigned int aout_get_request_reset_irq_inten(AoutRegs *optReg)
{
	return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), 1, 8);
}

static void aout_clr_request_reset_irq_state(AoutRegs *optReg)
{
	AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), (0x1<<8));
}

static unsigned int aout_get_complete_reset_irq_state(AoutRegs *optReg)
{
	return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), 1, 8);
}

static void aout_mix_set_l_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 0);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 0);
	}

	if (MIX_1_I2S == (MIX_1_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO),     value, 3, 16);
	}

	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), value, 3, 16);
	}
}

static void aout_mix_set_r_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 4);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 4);
	}

	if (MIX_1_I2S == (MIX_1_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO),     value, 3, 20);
	}

	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), value, 3, 20);
	}
}

static void aout_mix_set_c_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 16);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 16);
	}
}

static void aout_mix_set_ls1_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 8);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 8);
	}
}

static void aout_mix_set_rs1_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 12);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 12);
	}
}

static void aout_mix_set_ls2_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 20);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 20);
	}
}

static void aout_mix_set_rs2_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 24);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 24);
	}
}

static void aout_mix_set_cs_channel(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamChannel value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 28);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), value, 3, 28);
	}
}

static void aout_mix_enable_mute(AoutRegs *optReg, AoutStreamMix mixer, int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1,  3);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1,  7);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 11);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 15);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 19);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 23);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 27);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 31);
	}

	if (MIX_1_I2S == (MIX_1_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1,  3);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1,  7);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 11);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 15);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 19);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 23);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 27);
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CHANNELsel), enable, 1, 31);
	}

	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), enable, 1, 19);
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), enable, 1, 23);
	}

	if (MIX_3_SPD == (MIX_3_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INPUTDATA_INFO), enable, 1, 13);
	}
}

static void aout_mix_enable_stop_mute(AoutRegs *optReg, AoutStreamMix mixer, int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if ((MIX_0_I2S == (MIX_0_I2S & mixer)) || (MIX_1_I2S == (MIX_1_I2S & mixer))) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), enable, 1, 13);
	}

	if ((MIX_2_SPD == (MIX_2_SPD & mixer)) || (MIX_3_SPD == (MIX_3_SPD & mixer))) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 22);
	}
}

static void aout_mix_select_source(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamSelSource value)
{
	if (MIX_0_I2S == (MIX_0_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value.i2s_0_1, 2, 9);
	}

	if (MIX_1_I2S == (MIX_1_I2S & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value.i2s_0_1, 2, 11);
	}

	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), value.spd_2, 3, 0);
	}

	if (MIX_3_SPD == (MIX_3_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), value.spd_3, 3, 8);
	}
}

static void aout_dac_set_abck(AoutRegs *optReg, unsigned int value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 2, 7);
}

static void aout_dac_set_mclk(AoutRegs *optReg, AoutStreamDacMclk value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 3, 4);
}

static AoutStreamDacMclk aout_dac_get_mclk(AoutRegs *optReg)
{
	return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), 3, 4);
}

static void aout_dac_set_bclk(AoutRegs *optReg, AoutStreamDacBclk value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 2, 2);
}

static void aout_dac_set_format(AoutRegs *optReg, AoutStreamDacFormat value)
{
	AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DACINFO), value, 2, 0);
}

static void aout_spd_set_mclk(AoutRegs *optReg, AoutStreamMix mixer, AoutStreamSpdMclk value)
{
	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), value, 3, 16);
	}

	if ((MIX_3_SPD & mixer) == MIX_3_SPD) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), value, 3, 19);
	}
}

static unsigned int aout_spd_get_mclk(AoutRegs *optReg, AoutStreamMix mixer)
{
	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 3, 16);
	}

	if ((MIX_3_SPD & mixer) == MIX_3_SPD) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 3, 19);
	}

	return 0;
}

static void aout_spd_set_cl1(AoutRegs *optReg, AoutStreamMix mixer, unsigned int value)
{
	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_CL1), value);
	}

	if ((MIX_3_SPD & mixer) == MIX_3_SPD) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_CL1), value);
	}
}

static void aout_spd_set_cr1(AoutRegs *optReg, AoutStreamMix mixer, unsigned int value)
{
	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_CR1), value);
	}

	if ((MIX_3_SPD & mixer) == MIX_3_SPD) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_CR1), value);
	}
}

static void aout_spd_set_cl2(AoutRegs *optReg, AoutStreamMix mixer, unsigned int value)
{
	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_CL2), value);
	}

	if ((MIX_3_SPD & mixer) == MIX_3_SPD) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_CL2), value);
	}
}

static void aout_spd_set_cr2(AoutRegs *optReg, AoutStreamMix mixer, unsigned int value)
{
	if (MIX_2_SPD == (MIX_2_SPD & mixer)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_CR2), value);
	}

	if ((MIX_3_SPD & mixer) == MIX_3_SPD) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_CR2), value);
	}
}

static void aout_spd_set_pause_num(AoutRegs *optReg, AoutStreamSrc src, int value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_LAT_PAUSE), value);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_LAT_PAUSE), value);
	}
}

static void aout_spd_set_pause_len(AoutRegs *optReg, AoutStreamSrc src, int value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_DAT_PAU_LEN), value, 16, 0);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_DAT_PAU_LEN), value, 16, 0);
	}
}

static void aout_spd_set_data_len(AoutRegs *optReg, AoutStreamSrc src, int value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_DAT_PAU_LEN), value, 16, 16);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_DAT_PAU_LEN), value, 16, 16);
	}
}

static void aout_spd_set_pause_pc_pd(AoutRegs *optReg, AoutStreamSrc src, int value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_PAUSE_PC_PD), value);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_PAUSE_PC_PD), value);
	}
}

static void aout_spd_set_pa_pb(AoutRegs *optReg, AoutStreamSrc src, int value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_PA_PB), value);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_PA_PB), value);
	}
}

static void aout_spd_set_pc_pd(AoutRegs *optReg, AoutStreamSrc src, int value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_PC_PD), value);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_PC_PD), value);
	}
}

static void aout_src_enable_volume(AoutRegs *optReg, AoutStreamSrc src, int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), enable, 1, 26);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), enable, 1, 26);
	}
}

static void aout_src_set_volume(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), value, 10, 16);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), value, 10, 16);
	}
}

static void aout_src_clr_frame(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), 1, 1, 14);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), 1, 1, 14);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), 1, 1, 31);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INPUTDATA_INFO), 1, 1, 31);
	}
}

static void aout_src_start_config(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_SET_NEWFRAME_CONTROL), 1, 1, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_SET_NEWFRAME_CONTROL), 1, 1, 1);
	}
}

static void aout_src_over_config(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_SET_NEWFRAME_CONTROL), 1, 1, 2);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_SET_NEWFRAME_CONTROL), 1, 1, 2);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), 1, 1, 30);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INPUTDATA_INFO), 1, 1, 30);
	}
}

static void aout_src_set_pcm_bits(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmBits value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), value, 5, 8);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), value, 5, 8);
	}
}

static AoutStreamPcmBits aout_src_get_pcm_bits(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_playingINDATA), 5, 8);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_playingINDATA), 5, 8);
	}

	return PCM_BITS_16_R;
}

static void aout_src_set_pcm_endian(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmEndian value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), value, 2, 5);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), value, 2, 5);
	}
}

static AoutStreamPcmEndian aout_src_get_pcm_endian(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_playingINDATA), 1, 5);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_playingINDATA), 1, 5);
	}

	return PCM_ENDIAN_0;
}

static void aout_src_set_pcm_interlace(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmInterlace value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), value, 1, 4);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), value, 1, 4);
	}
}

static void aout_src_set_pcm_channelnum(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmChNum value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INDATA), value, 4, 0);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INDATA), value, 4, 0);
	}
}

static void aout_src_set_pcm_source(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmSource value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), value, 2, 6);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), value, 2, 6);
	}
}

static void aout_src_set_pcm_mage(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmMage value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), value, 2, 4);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), value, 2, 4);
	}
}

static void aout_src_set_pcm_output_bits(AoutRegs *optReg, AoutStreamSrc src, AoutStreamPcmOutputBits value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), value, 2, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), value, 2, 1);
	}
}

static void aout_src_enable_pcm_upsample(AoutRegs *optReg, AoutStreamSrc src, int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 3);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 3);
	}
}

static void aout_src_set_cdd_endian(AoutRegs *optReg, AoutStreamSrc src, AoutStreamCddEndian value)
{
	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), value, 2, 24);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INPUTDATA_INFO), value, 2, 24);
	}
}

static void aout_src_enable_silent_end(AoutRegs *optReg, AoutStreamSrc src, int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 0);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 0);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INPUTDATA_INFO), enable, 1, 13);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INPUTDATA_INFO), enable, 1, 13);
	}

}

static void aout_src_set_pcm_value(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_CPUSetPCM), value);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_CPUSetPCM), value);
	}
}

static void aout_src_set_pcm_times(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), value, 3, 5);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), value, 3, 5);
	}
}

static void aout_src_enable_work(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 4);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 4);
	}
}

static void aout_src_enable_playback(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 3);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 3);
	}
}

static void aout_src_set_playback_mode(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), value, 3, 5);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), value, 3, 5);
	}
}

static void aout_src_forbiden_request_sdc(AoutRegs *optReg, AoutStreamSrc src, unsigned int forbiden)
{
	forbiden = (forbiden == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), forbiden, 1, 2);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), forbiden, 1, 2);
	}
}

static void aout_src_enable_clear_point(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 1);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 4);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 12);
	}
}

static void aout_src_enable_clear_data(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 0);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_FIFOCTRL), enable, 1, 0);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 3);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 11);
	}
}

static void aout_src_set_inv_data(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_DATAINV), value);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_DATAINV), value);
	}
}

static void aout_src_enable_reset(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 8);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_OUTDATA), enable, 1, 8);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 31);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), enable, 1, 30);
	}
}

static void aout_src_enable_new_frame_irq(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), enable, 1, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INTEN), enable, 1, 1);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT_EN), enable, 1, 18);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT_EN), enable, 1, 18);
	}
}

static void aout_src_clr_new_frame_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), (0x1<<1));
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT), (0x1<<1));
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_INT),  (0x1<<18));
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_INT),  (0x1<<18));
	}
}

static unsigned int aout_src_get_new_frame_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), 1, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT), 1, 1);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT),  1, 18);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT),  1, 18);
	}

	return 0;
}

static unsigned int aout_src_get_new_frame_irq_inten(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), 1, 1);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INTEN), 1, 1);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT_EN),  1, 18);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT_EN),  1, 18);
	}

	return 0;
}

static void aout_src_enable_over_frame_irq(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), enable, 1, 2);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INTEN), enable, 1, 2);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT_EN), enable, 1, 2);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT_EN), enable, 1, 2);
	}
}

static void aout_src_clr_over_frame_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), (0x1<<2));
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT), (0x1<<2));
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_INT),  (0x1<<2));
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_INT),  (0x1<<2));
	}
}

static unsigned int aout_src_get_over_frame_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), 1, 2);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT), 1, 2);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT),  1, 2);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT),  1, 2);
	}

	return 0;
}

static unsigned int aout_src_get_over_frame_irq_inten(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), 1, 2);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INTEN), 1, 2);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT_EN),  1, 2);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT_EN),  1, 2);
	}

	return 0;
}

static void aout_src_enable_empty_frame_irq(AoutRegs *optReg, AoutStreamSrc src, unsigned int enable)
{
	enable = (enable == 0) ? 0 : 1;

	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), enable, 1, 3);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INTEN), enable, 1, 3);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT_EN), enable, 1, 18);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT_EN), enable, 1, 18);
	}
}

static void aout_src_clr_empty_frame_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), (0x1<<3));
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		AOUT_REG_SET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT), (0x1<<3));
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_INT),  (0x1<<18));
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_INT),  (0x1<<18));
	}
}

static unsigned int aout_src_get_empty_frame_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT), 1, 3);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT), 1, 3);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT),  1, 18);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT),  1, 18);
	}

	return 0;
}

static unsigned int aout_src_get_empty_frame_irq_inten(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == (SRC_R0 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INTEN), 1, 3);
	}

	if (SRC_R1 == (SRC_R1 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INTEN), 1, 3);
	}

	if (SRC_R2 == (SRC_R2 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_INT_EN),  1, 18);
	}

	if (SRC_R3 == (SRC_R3 & src)) {
		return AOUT_REG_GET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_INT_EN),  1, 18);
	}

	return 0;
}
static unsigned int aout_src_get_irq_state(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->i2sRegs_0->AUDIO_PLAY_I2S_INT));
	}

	if (SRC_R1 == src) {
		return AOUT_REG_GET_VAL(&(optReg->i2sRegs_1->AUDIO_PLAY_I2S_INT));
	}

	if (SRC_R2 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_INT));
	}

	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_INT));
	}

	return 0;
}

static void aout_src_set_buffer_start_addr(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_BUFFER_START_ADDR), value);
	}

	if (SRC_R1 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_BUFFER_START_ADDR), value);
	}

	if (SRC_R2 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_BUFFER_START_ADDR), value);
	}

	if (SRC_R3 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_BUFFER_START_ADDR), value);
	}
}

static void aout_src_set_buffer_size(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_BUFFER_SIZE), value);
	}

	if (SRC_R1 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_BUFFER_SIZE), value);
	}

	if (SRC_R2 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_BUFFER_SIZE), value);
	}

	if (SRC_R3 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_BUFFER_SIZE), value);
	}
}

static void aout_src_set_frame_start_addr(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_newFRAME_START_ADDR), value);
	}

	if (SRC_R1 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_newFRAME_START_ADDR), value);
	}

	if (SRC_R2 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_newFRAME_START_ADDR), value);
	}

	if (SRC_R3 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_newFRAME_START_ADDR), value);
	}
}

static unsigned int aout_src_get_frame_start_addr(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_newFRAME_START_ADDR));
	}

	if (SRC_R1 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_newFRAME_START_ADDR));
	}

	if (SRC_R2 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_newFRAME_START_ADDR));
	}

	if (SRC_R3 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_newFRAME_START_ADDR));
	}

	return 0;
}

static void aout_src_set_frame_end_addr(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_newFRAME_END_ADDR), value);
	}

	if (SRC_R1 == src) {
		AOUT_REG_SET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_newFRAME_END_ADDR), value);
	}

	if (SRC_R2 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_newFRAME_END_ADDR), value);
	}

	if (SRC_R3 == src) {
		AOUT_REG_SET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_newFRAME_END_ADDR), value);
	}
}

static unsigned int aout_src_get_frame_end_addr(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_newFRAME_END_ADDR));
	}

	if (SRC_R1 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_newFRAME_END_ADDR));
	}

	if (SRC_R2 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_newFRAME_END_ADDR));
	}

	if (SRC_R3 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_newFRAME_END_ADDR));
	}

	return 0;
}

static unsigned int aout_src_get_playing_frame_start_addr(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_playingFRAME_START_ADDR));
	}

	if (SRC_R1 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_playingFRAME_START_ADDR));
	}

	if (SRC_R2 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_playingFRAME_START_ADDR));
	}

	if (SRC_R3 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_playingFRAME_START_ADDR));
	}

	return 0;
}

static unsigned int aout_src_get_playing_frame_end_addr(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_playingFRAME_END_ADDR));
	}

	if (SRC_R1 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_playingFRAME_END_ADDR));
	}

	if (SRC_R2 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_playingFRAME_END_ADDR));
	}

	if (SRC_R3 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_playingFRAME_END_ADDR));
	}

	return 0;
}

static unsigned int aout_src_get_playing_frame_read_addr(AoutRegs *optReg, AoutStreamSrc src)
{
	if (SRC_R0 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_BUFFER_SDC_ADDR));
	}

	if (SRC_R1 == src) {
		return AOUT_REG_GET_VAL(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_BUFFER_SDC_ADDR));
	}

	if (SRC_R2 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_2->AUDIO_SPDIF_BUFFER_READ_ADDR));
	}

	if (SRC_R3 == src) {
		return AOUT_REG_GET_VAL(&(optReg->spdRegs_3->AUDIO_SPDIF_BUFFER_READ_ADDR));
	}

	return 0;
}

static void aout_src_set_frame_sample_points(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R0 == src) {
		AOUT_REG_SET_FIELD(&(optReg->sdcRegs_0->AUDIO_PLAY_I2S_newFrame_pcmLEN), value, 16, 0);
	}

	if (SRC_R1 == src) {
		AOUT_REG_SET_FIELD(&(optReg->sdcRegs_1->AUDIO_PLAY_I2S_newFrame_pcmLEN), value, 16, 0);
	}

	if (SRC_R2 == src) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_newFRAME_LEN), value, 16, 0);
	}

	if (SRC_R3 == src) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_newFRAME_LEN), value, 16, 0);
	}
}

static void aout_port_select_mix(AoutRegs *optReg, AoutStreamPort port, AoutStreamMix mixer)
{
	if (HDMI_PORT == port) { //hdmi select 4-i2s/hbr and spd_2/spd_3
		if ((MIX_0_I2S == mixer) || (MIX_1_I2S == mixer)) {
			AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 0, 1, 7);
		}

		if (MIX_2_SPD == mixer) {
			AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 1, 1, 29);
		} else if (MIX_3_SPD == mixer) {
			AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 0, 1, 29);
		}
	}

	if (SPDIF_PORT == port) {
		if (MIX_2_SPD == mixer) {
			AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 0, 1, 28);
		} else if (MIX_3_SPD == mixer) {
			AOUT_REG_SET_FIELD(&(optReg->spdRegs_2->AUDIO_SPDIF_CTRL), 1, 1, 28);
		}
	}
}

static void aout_src_set_frame_mute(AoutRegs *optReg, AoutStreamSrc src, unsigned int value)
{
	if (SRC_R2 == src) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_FRAME_MUTE_CTRL), (value ? 1 : 0), 1, 0);
	}

	if (SRC_R3 == src) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_FRAME_MUTE_CTRL), (value ? 1 : 0), 1, 8);
	}
}

static void aout_spdif_port_mute(AoutRegs *optReg, AoutStreamPort port, unsigned int value)
{
	if (SPDIF_PORT == port) {
		AOUT_REG_SET_FIELD(&(optReg->spdRegs_3->AUDIO_SPDIF_FRAME_MUTE_CTRL), (value ? 1: 0), 1, 16);
	}
}

#endif
