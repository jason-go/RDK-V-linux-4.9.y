#include "mfixer.h"
#include "pcrfixer.h"
#include "stc_hal.h"
#include "stc_module.h"
#include "gx3211_stc_reg.h"

static struct gxav_stc gx3211_stc[STC_UNIT];
struct reg_stc *gx3211_stc_reg[STC_UNIT];

static struct multifix gx3211_mfixer;
static struct pcr_fixer gx3211_pcrfixer;

static unsigned int GX3211_STC0_BASE_ADDR = 0x4003560;
static unsigned int GX3211_STC1_BASE_ADDR = 0x4013560;
static int gx3211_stc_init(void)
{
	int i, addr[STC_UNIT];

	addr[0] = GX3211_STC0_BASE_ADDR;
	addr[1] = GX3211_STC1_BASE_ADDR;

	memset(&gx3211_stc, 0, sizeof(gx3211_stc));
	memset(&gx3211_stc_reg, 0, sizeof(gx3211_stc_reg));

	for (i=0; i<STC_UNIT; i++){
		//if (!gx_request_mem_region(addr[i], sizeof(struct reg_stc))) {
		//	gx_printf("request STC mem region failed!\n");
		//	return -1;
		//}

		gx3211_stc_reg[i] = gx_ioremap(addr[i], sizeof(struct reg_stc));
		if (!gx3211_stc_reg[i]) {
			gx_printf("ioremap STC space failed!\n");
			return -1;
		}
	}

	return 0;
}

static int gx3211_stc_cleanup(void)
{
	int i;

	for (i=0; i<STC_UNIT; i++){
		gx_iounmap(gx3211_stc_reg[i]);
	}

	return 0;
}

static int gx3211_stc_open(int id)
{
	return 0;
}

static int gx3211_stc_close(int id)
{
	return 0;
}

static void _set_sync_mode(struct reg_stc *reg, int sub, int mode)
{
	unsigned long temp;

	switch (mode) {
	case PCR_RECOVER:
	case AVPTS_RECOVER:
		temp = (STC_RECOV_MODE_PCR | STC_RECOV_GAIN_1 | STC_RECOV_OVER_TIME_SWITCH
				| STC_RECOV_TIMING_ERR_SWITCH | STC_RECOV_STC_LOAD_SWITCH
				| STC_RECOV_BASE_ERR_SWITCH | (0x03 << BIT_STC_RECOV_PCR_FOLLOW_CNT)
				| (0x03 << BIT_STC_RECOV_ERR_LOST_CNT) | (0x3f << BIT_STC_RECOV_BASE_ERR_GATE)
				| (0x02 << BIT_STC_RECOV_BASE_GATE));
		REG_SET_VAL(&(reg->stc_offset), 0x0);
		break;

	case APTS_RECOVER:
		temp = (STC_RECOV_MODE_PLAYBACK | STC_RECOV_GAIN_0 | STC_RECOV_CLR_OVER_TIME_SWITCH
				| STC_RECOV_CLR_TIMING_ERR_SWITCH | STC_RECOV_STC_LOAD_SWITCH | STC_RECOV_BASE_ERR_SWITCH
				| (0x03 << BIT_STC_RECOV_PCR_FOLLOW_CNT) | (0x03 << BIT_STC_RECOV_ERR_LOST_CNT)
				| (0xff << BIT_STC_RECOV_BASE_ERR_GATE) | (0x08 << BIT_STC_RECOV_BASE_GATE));
		REG_SET_VAL(&(reg->stc_offset), 0x0);
		REG_SET_VAL(&(reg->stc_offset_1), 0x0);
		break;

	case NO_RECOVER:
		temp = (STC_RECOV_MODE_NOSYNC | STC_RECOV_GAIN_1 | STC_RECOV_OVER_TIME_SWITCH
				| STC_RECOV_TIMING_ERR_SWITCH | STC_RECOV_STC_LOAD_SWITCH
				| STC_RECOV_BASE_ERR_SWITCH | (0x03 << BIT_STC_RECOV_PCR_FOLLOW_CNT)
				| (0x00 << BIT_STC_RECOV_ERR_LOST_CNT) | (0x3f << BIT_STC_RECOV_BASE_ERR_GATE)
				| (0x02 << BIT_STC_RECOV_BASE_GATE));
		REG_SET_VAL(&(reg->stc_offset), 0x0);
		REG_SET_VAL(&(reg->stc_offset_1), 0x0);
		break;

	default:
		temp = (STC_RECOV_MODE_PCR | STC_RECOV_GAIN_1 | STC_RECOV_OVER_TIME_SWITCH
				| STC_RECOV_TIMING_ERR_SWITCH | STC_RECOV_STC_LOAD_SWITCH | STC_RECOV_BASE_ERR_SWITCH
				| (0x03 << BIT_STC_RECOV_PCR_FOLLOW_CNT) | (0x03 << BIT_STC_RECOV_ERR_LOST_CNT)
				| (0x3f << BIT_STC_RECOV_BASE_ERR_GATE) | (0x02 << BIT_STC_RECOV_BASE_GATE));
		break;
	}

	if(sub == 0){
		REG_SET_BIT(&(reg->stc_cnt),20);
		REG_CLR_BIT(&(reg->stc_cnt),20);
		REG_SET_VAL(&(reg->recover), temp);
	}else if (sub ==1){
		REG_SET_BIT(&(reg->stc_ctrl_1),20);
		REG_CLR_BIT(&(reg->stc_ctrl_1),20);
		REG_SET_VAL(&(reg->recovery_1), temp);
	}

}


static int gx3211_stc_set_source(int id, GxSTCProperty_Config* source)
{
	switch(source->mode) {
	case PCR_RECOVER:
	case AVPTS_RECOVER:
		_set_sync_mode(gx3211_stc_reg[id], 0, NO_RECOVER);
		_set_sync_mode(gx3211_stc_reg[id], 1, NO_RECOVER);
		break;
	case APTS_RECOVER:
	case VPTS_RECOVER:
	case FIXD_APTS_RECOVER:
	case FIXD_VPTS_RECOVER:
		_set_sync_mode(gx3211_stc_reg[id], 0, APTS_RECOVER);
		_set_sync_mode(gx3211_stc_reg[id], 1, NO_RECOVER);
		break;
	case NO_RECOVER:
	case PURE_APTS_RECOVER:
		_set_sync_mode(gx3211_stc_reg[id], 0, NO_RECOVER);
		_set_sync_mode(gx3211_stc_reg[id], 1, NO_RECOVER);
		break;
	default:
		return -1;
	}

	if (source->mode != PCR_RECOVER && source->mode != AVPTS_RECOVER) {
		REG_SET_FIELD(&(gx3211_stc_reg[id]->pcr_cfg), (0xFFFF << 0), 0x1FFF, 0);
	}

	gx3211_stc[id].source = source->mode;
	return 0;
}

static int gx3211_stc_get_source(int id, GxSTCProperty_Config* source)
{
	source->mode = gx3211_stc[id].source;

	return 0;
}

static void gx3211_stc_reverse(struct reg_stc *reg, int enable)
{
	if(enable){
		REG_SET_BIT(&(reg->stc_cnt), BIT_STC_REVERSE);
		REG_SET_BIT(&(reg->stc_ctrl_1), BIT_STC_REVERSE);
	}else{
		REG_CLR_BIT(&(reg->stc_cnt), BIT_STC_REVERSE);
		REG_CLR_BIT(&(reg->stc_ctrl_1), BIT_STC_REVERSE);
	}
	return;
}

static int gx3211_stc_set_resolution(int id, GxSTCProperty_TimeResolution* resolution)
{
	int freq_HZ = resolution->freq_HZ;

	if (freq_HZ < 0) {
		freq_HZ = 0-freq_HZ;
		gx3211_stc_reverse(gx3211_stc_reg[id], 1);
	}
	else {
		gx3211_stc_reverse(gx3211_stc_reg[id], 0);
	}

	REG_SET_FIELD(&(gx3211_stc_reg[id]->stc_cnt), (0xFFFF << 0), (STC_DTO/(freq_HZ * 2* 2)), 0);
	REG_SET_FIELD(&(gx3211_stc_reg[id]->stc_ctrl_1), (0xFFFF << 0), (STC_DTO/(freq_HZ * 2* 2)), 0);

	gx3211_stc[id].resolution = resolution->freq_HZ;

	return 0;
}

static int gx3211_stc_get_resolution(int id, GxSTCProperty_TimeResolution* resolution)
{
	resolution->freq_HZ = gx3211_stc[id].resolution;

	return 0;
}

static void _stc_set_time(struct reg_stc *reg, int sub, unsigned int value, int immd)
{
	unsigned int cur_recov_mode = 0;

	if(sub == 0)
	{
		cur_recov_mode = REG_GET_FIELD(&(reg->recover), (0x3<<30), 30);

		REG_SET_BIT(&(reg->stc_cnt), BIT_STC_PAUSE);

		REG_CLR_BITS(&(reg->recover), (0x3<<30));
		REG_SET_BITS(&(reg->recover), (0x2<<30));

		if(immd){
			REG_SET_BIT(&(reg->stc_cnt), BIT_STC_UPDATE);
			REG_SET_VAL(&(reg->audio_dec_apts), value);
			REG_CLR_BIT(&(reg->stc_cnt), BIT_STC_UPDATE);
		}
		else
			REG_SET_VAL(&(reg->audio_dec_apts), value);

		REG_SET_FIELD(&(reg->recover), (0x3<<30),\
				cur_recov_mode, 30);

		REG_CLR_BIT(&(reg->stc_cnt), BIT_STC_PAUSE);
	}
	else
	{
		cur_recov_mode = REG_GET_FIELD(&(reg->recovery_1), (0x3<<30), 30);

		REG_SET_BIT(&(reg->stc_ctrl_1), BIT_STC_PAUSE);

		REG_CLR_BITS(&(reg->recovery_1), (0x3<<30));
		REG_SET_BITS(&(reg->recovery_1), (0x2<<30));

		if(immd){
			REG_SET_BIT(&(reg->stc_ctrl_1), BIT_STC_UPDATE);
			REG_SET_VAL(&(reg->audio_dec_apts), value);
			REG_CLR_BIT(&(reg->stc_ctrl_1), BIT_STC_UPDATE);
		}
		else
			REG_SET_VAL(&(reg->audio_dec_apts), value);

		REG_SET_FIELD(&(reg->recovery_1), (0x3<<30),\
				cur_recov_mode, 30);

		REG_CLR_BIT(&(reg->stc_ctrl_1), BIT_STC_PAUSE);
	}
}

static int gx3211_stc_set_time(int id, GxSTCProperty_Time* time)
{
	_stc_set_time(gx3211_stc_reg[id], 0, time->time, 1);

	return 0;
}

static int gx3211_stc_get_time(int id, GxSTCProperty_Time* time)
{
	int stc0, stc1, pcr;

	pcr  = REG_GET_VAL(&(gx3211_stc_reg[id]->pcr));
	stc0 = REG_GET_VAL(&(gx3211_stc_reg[id]->stc));
	stc1 = REG_GET_VAL(&(gx3211_stc_reg[id]->stc_1));

	if (IS_AVPTS_RECOVERY(gx3211_stc[id].source)) {
		pcr = mfixer_get(&gx3211_mfixer, stc1, pcr, gx3211_pcrfixer.apts);
		time->time = pcrfixer_get(&gx3211_pcrfixer, stc1, pcr);
	} else if (IS_APTS_RECOVERY(gx3211_stc[id].source) || IS_VPTS_RECOVERY(gx3211_stc[id].source)) {
		pcr = stc0;
		time->time = pcrfixer_get(&gx3211_pcrfixer, stc1, pcr);
	} else if (IS_FIXD_APTS_RECOVERY(gx3211_stc[id].source) || IS_FIXD_VPTS_RECOVERY(gx3211_stc[id].source) ) {
		time->time = stc0;
	} else if (IS_NO_RECOVERY(gx3211_stc[id].source)) {
		time->time = stc0;
	} else if (IS_PURE_APTS_RECOVERY(gx3211_stc[id].source)) {
		time->time = (gx3211_pcrfixer.apts == 0) ? 0 : (gx3211_pcrfixer.apts|0x1);
	} else if (IS_PCR_RECOVERY(gx3211_stc[id].source)) {
		time->time = mfixer_get(&gx3211_mfixer, stc1, pcr, gx3211_pcrfixer.apts);
	}

	return 0;
}

static void _3211_stc_enable(int id, int enable)
{
	struct reg_stc *reg = gx3211_stc_reg[id];

	if (enable) {
		pcrfixer_init(&gx3211_pcrfixer, gx3211_stc[id].resolution);
		mfixer_init(&gx3211_mfixer, gx3211_stc[id].resolution);
		REG_SET_BIT(&(reg->stc_cnt), BIT_STC_ENABLE);
		REG_SET_BIT(&(reg->stc_ctrl_1), BIT_STC_ENABLE);
	}else {
		REG_CLR_BIT(&(reg->stc_cnt), BIT_STC_ENABLE);
		REG_CLR_BIT(&(reg->stc_ctrl_1), BIT_STC_ENABLE);
	}
	return;
}

static void _3211_stc_pause(int id, int enable)
{
	struct reg_stc *reg = gx3211_stc_reg[id];

	if (enable) {
		REG_SET_BIT(&(reg->stc_cnt), BIT_STC_PAUSE);
		REG_SET_BIT(&(reg->stc_ctrl_1), BIT_STC_PAUSE);
	}else {
		REG_CLR_BIT(&(reg->stc_cnt), BIT_STC_PAUSE);
		REG_CLR_BIT(&(reg->stc_ctrl_1), BIT_STC_PAUSE);
	}
	return;
}

static void _3211_stc_clear(int id)
{
	struct reg_stc *reg = gx3211_stc_reg[id];

	REG_SET_BIT(&(reg->stc_cnt), BIT_STC_UPDATE);
	REG_SET_BIT(&(reg->stc_ctrl_1), BIT_STC_UPDATE);

	REG_SET_VAL(&(reg->audio_dec_apts), 0);

	REG_CLR_BIT(&(reg->stc_ctrl_1), BIT_STC_UPDATE);
	REG_CLR_BIT(&(reg->stc_cnt), BIT_STC_UPDATE);
	return;
}

static int gx3211_stc_play(int id)
{
	_3211_stc_pause(id, 0);
	_3211_stc_enable(id, 1);
	_3211_stc_clear(id);

	return 0;
}

static int gx3211_stc_stop(int id)
{
	_3211_stc_enable(id, 0);

	return 0;
}

static int gx3211_stc_pause(int id)
{
	_3211_stc_pause(id, 1);

	return 0;
}

static int gx3211_stc_resume(int id)
{
	_3211_stc_pause(id, 0);

	return 0;
}

static int gx3211_stc_write_apts(int id, unsigned int value, int immd)
{
	int stc1, pcr;

	gx3211_pcrfixer.apts = value;

	if (IS_APTS_RECOVERY(gx3211_stc[id].source) || IS_FIXD_APTS_RECOVERY(gx3211_stc[id].source)) {
		gx3211_pcrfixer.apts = value;
		_stc_set_time(gx3211_stc_reg[id], 0, value, immd);
	}

	pcr  = REG_GET_VAL(&(gx3211_stc_reg[id]->pcr));
	stc1 = REG_GET_VAL(&(gx3211_stc_reg[id]->stc_1));
	mfixer_fix(&gx3211_mfixer, stc1, pcr, gx3211_pcrfixer.apts);

	return 0;
}

int gx3211_stc_write_vpts(int id, unsigned int value, int immd)
{
	gx3211_pcrfixer.vpts = value;

	if (value != 0) {
		if (IS_VPTS_RECOVERY(gx3211_stc[id].source) || IS_FIXD_VPTS_RECOVERY(gx3211_stc[id].source)) {
			_stc_set_time(gx3211_stc_reg[id], 0, value, immd);
		}
	}

	return 0;
}

static int gx3211_stc_invaild_apts(unsigned int value)
{
	gx3211_pcrfixer.apts = value;

	return 0;
}

static int gx3211_stc_read_apts(int id, unsigned int *value)
{
	*value = gx3211_pcrfixer.apts;

	return 0;
}

static int gx3211_stc_read_pcr(int id, unsigned int *value)
{
	*value = REG_GET_VAL(&(gx3211_stc_reg[id]->pcr));

	return 0;
}

static int gx3211_stc_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	GX3211_STC0_BASE_ADDR = res->regs[0];
	GX3211_STC1_BASE_ADDR = res->regs[1];

	return 0;
}

static struct stc_ops gx3211_stc_ops = {
	.init = gx3211_stc_init,
	.cleanup = gx3211_stc_cleanup,
	.open = gx3211_stc_open,
	.close = gx3211_stc_close,
	.set_source = gx3211_stc_set_source,
	.get_source = gx3211_stc_get_source,
	.set_resolution = gx3211_stc_set_resolution,
	.get_resolution = gx3211_stc_get_resolution,
	.set_time = gx3211_stc_set_time,
	.get_time = gx3211_stc_get_time,
	.play = gx3211_stc_play,
	.stop = gx3211_stc_stop,
	.pause = gx3211_stc_pause,
	.resume = gx3211_stc_resume,
	.read_apts = gx3211_stc_read_apts,
	.read_pcr  = gx3211_stc_read_pcr,
	.write_apts = gx3211_stc_write_apts,
	.write_vpts = gx3211_stc_write_vpts,
	.invaild_apts = gx3211_stc_invaild_apts,
};

struct gxav_module_ops gx3211_stc_module = {
	.module_type = GXAV_MOD_STC,
	.count = 1,
	.irqs = {-1},
	.irq_flags = {-1},
	.init = gx_stc_init,
	.cleanup = gx_stc_cleanup,
	.open = gx_stc_open,
	.close = gx_stc_close,
	.setup = gx3211_stc_setup,
	.set_property = gx_stc_set_property,
	.get_property = gx_stc_get_property,
	.priv = &gx3211_stc_ops
};


