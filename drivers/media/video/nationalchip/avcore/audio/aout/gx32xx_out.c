#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_event_type.h"
#include "gxav_common.h"
#include "clock_hal.h"
#include "sdc_hal.h"
#include "include/audio_common.h"
#include "include/audio_module.h"
#include "aout/aout.h"
#include "log_printf.h"

#include "hdmi_hal.h"
#include "gx32xx_out.h"
#include "gx32xx_reg.h"
#include "gx32xx_i2s.h"
#include "porting.h"

volatile struct audiospdif_regs     *gx3201_audiospdif_reg  = NULL;
volatile struct audiospdif_raw_regs *gx3201_audiospdif_raw_reg = NULL;

#define PCM_AOUT_DEBUG  (1<<0)
#define AC3_AOUT_DEBUG  (1<<1)
#define EAC3_AOUT_DEBUG (1<<2)
#define AD_AOUT_DEBUG   (1<<3)
#define HUNGRY_FREEZE_COUNT (5)
char audioout_debug = 0;
#define AUDIOOUT_DEBUG_SYNC(s, pts, stc, offset, low, high, debug_type) do{ \
	if (audioout_debug&debug_type || audioout->debug.value) \
	gxlog_i(LOG_AOUT, "%x - %s: pts %10u, stc %10u, offset %10d, low %d, high %d\n", \
			debug_type, s, (pts==-1)?0:pts, (stc==-1)?0:stc, offset, low, high); \
}while(0)

static unsigned int gx3201_repeat_unit_len[] = {
	98, 90, 135, 196, 180, 270, 45, 49,
	24, 22,  68, 360, 392, 450, 540,
};

static unsigned int gx3201_audio_freq[] = {
	44100, 48000, 32000, 22050 , 24000 ,
	16000, 96000, 88200, 128000,176400, 192000,
	64000, 12000, 11025, 9600  , 8000  ,
};

static unsigned int gx3201_audio_spd_mclk[] = {
	256 , 384 , 512, 1024,
	2048, 1536, 128, 192,
};

static unsigned int gx3201_audio_i2s_mclk[] = {
	256 , 384 , 512, 768,
	1024, 1536, 128, 192,
};

static unsigned int gx3201_dac_mix_table[] = {
	3, 0, 1, 2
};

typedef struct {
	unsigned int db;
	unsigned int mag;
} audioi2s_volume;

static audioi2s_volume volume_table[101] = {
	{32, 0}, {31, 0}, {30, 0}, {30, 0}, {29, 0}, {29, 0}, {28, 0}, {28, 0},
	{27 ,0}, {27, 0}, {26, 0}, {26, 0}, {25, 0}, {25, 0}, {24, 0}, {24, 0},
	{23, 0}, {23, 0}, {22, 0}, {22, 0}, {21, 0}, {21, 0}, {20, 0}, {20, 0},
	{19, 0}, {19, 0}, {19, 0}, {18, 0}, {18, 0}, {18, 0}, {17, 0}, {17, 0},
	{17, 0}, {16, 0}, {16, 0}, {16, 0}, {15, 0}, {15, 0}, {15, 0}, {14, 0},
	{14, 0}, {14, 0}, {13, 0}, {13, 0}, {13, 0}, {12, 0}, {12, 0}, {12, 0},
	{11, 0}, {11, 0}, {11, 0}, {10, 0}, {10, 0}, {10, 0}, { 9, 0}, { 9, 0},
	{ 9, 0}, { 8, 0}, { 8, 0}, { 8, 0}, { 7, 0}, { 7, 0}, { 7, 0}, { 6, 0},
	{ 6, 0}, { 6, 0}, { 5, 0}, { 5, 0}, { 5, 0}, { 4, 0}, { 4, 0}, { 4, 0},
	{ 4, 0}, { 3, 0}, { 3, 0}, { 3, 0}, { 3, 0}, { 3, 0}, { 3, 0}, { 3, 0},
	{ 8, 1}, { 8, 1}, { 8, 1}, { 8, 1}, { 8, 1}, { 8, 1}, { 8, 1}, { 7, 1},
	{ 7, 1}, { 7, 1}, { 7, 1}, { 7, 1}, { 7, 1}, { 7, 1}, { 6, 1}, { 6, 1},
	{ 6, 1}, { 6, 1}, { 6, 1}, { 6, 1}, { 6, 1}
};

static void _set_audioout_spd_module_clk(volatile struct audiospdif_regs *rp, int clock)
{
	unsigned int mAUDIO_SPD_MCLK, bAUDIO_SPD_MCLK;

	if (CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		bAUDIO_SPD_MCLK = (16);
		mAUDIO_SPD_MCLK = (0x07 <<  bAUDIO_SPD_MCLK);
		clock &= 0x07;
	}
	else {
		bAUDIO_SPD_MCLK = (4);
		mAUDIO_SPD_MCLK = (0x03 <<  bAUDIO_SPD_MCLK);
		clock &= 0x03;
	}

	SET_FEILD(rp->audio_play_spdif_ctrl, mAUDIO_SPD_MCLK, (clock << bAUDIO_SPD_MCLK));
}

static int _get_audioout_spd_module_clk(volatile struct audiospdif_regs *rp)
{
	unsigned int mAUDIO_SPD_MCLK, bAUDIO_SPD_MCLK;

	if (CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		bAUDIO_SPD_MCLK = (16);
		mAUDIO_SPD_MCLK = (0x07 <<  bAUDIO_SPD_MCLK);
	}
	else {
		bAUDIO_SPD_MCLK = (4);
		mAUDIO_SPD_MCLK = (0x03 <<  bAUDIO_SPD_MCLK);
	}

	return GET_FEILD(rp->audio_play_spdif_ctrl, mAUDIO_SPD_MCLK, bAUDIO_SPD_MCLK);
}

static int _get_audioout_repeat_len(int time_offset, unsigned int samplefre)
{
	int zero_sample_len;
	unsigned int samplefre_index;

	samplefre_index = gxav_audioout_samplefre_index(samplefre);
	zero_sample_len = time_offset/gx3201_repeat_unit_len[samplefre_index];
	if (zero_sample_len > 300)
		zero_sample_len = 300;

	return zero_sample_len;
}

static void _set_audioout_spd_raw_module_clk(volatile struct audiospdif_regs *rp, int clock)
{
	unsigned int mAUDIO_SPD_RAW_MCLK, bAUDIO_SPD_RAW_MCLK;

	if (CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		bAUDIO_SPD_RAW_MCLK = (19);
		mAUDIO_SPD_RAW_MCLK = (0x07 <<  bAUDIO_SPD_RAW_MCLK);
		clock &= 0x07;
	}
	else {
		bAUDIO_SPD_RAW_MCLK = (12);
		mAUDIO_SPD_RAW_MCLK = (0x03 <<  bAUDIO_SPD_RAW_MCLK);
		clock &= 0x03;
	}

	SET_FEILD(rp->audio_play_spdif_ctrl, mAUDIO_SPD_RAW_MCLK, (clock << bAUDIO_SPD_RAW_MCLK));
}

static int _get_audioout_spd_raw_module_clk(volatile struct audiospdif_regs *rp)
{
	unsigned int mAUDIO_SPD_RAW_MCLK, bAUDIO_SPD_RAW_MCLK;

	if (CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
		bAUDIO_SPD_RAW_MCLK = (19);
		mAUDIO_SPD_RAW_MCLK = (0x07 <<  bAUDIO_SPD_RAW_MCLK);
	}
	else {
		bAUDIO_SPD_RAW_MCLK = (12);
		mAUDIO_SPD_RAW_MCLK = (0x03 <<  bAUDIO_SPD_RAW_MCLK);
	}

	return GET_FEILD(rp->audio_play_spdif_ctrl, mAUDIO_SPD_RAW_MCLK, bAUDIO_SPD_RAW_MCLK);
}

int _get_audioout_spd_res_len(void)
{
	unsigned int end_addr, play_addr, buf_size;

	buf_size  = AUDIO_GET_SPD_BUFFER_SIZE(gx3201_audiospdif_reg);
	end_addr  = AUDIO_GET_SPD_NEWFRAME_END_ADDR(gx3201_audiospdif_reg)+1;
	play_addr = AUDIO_GET_SPD_BUFFER_READ_ADDR(gx3201_audiospdif_reg);

	if (buf_size == 0)
		return 0;

	return (buf_size + end_addr - play_addr)%buf_size;
}

static int _get_audioout_clock_base(void)
{
	switch(gxcore_chip_probe())
	{
	case GXAV_ID_GX3211:
	case GXAV_ID_GX3201:
	case GXAV_ID_GX6605S:
		return 1188000000;
	case GXAV_ID_GX3113C:
		return 864000000;
	default:
		return 0;
	}
}

static void _set_audioout_clock_freq(struct gxav_audioout *audioout,
		int mclk_index, int samplefre_index, enum audio_object object, int clock_speed)
{
	unsigned long long audio_clock = 0;
	unsigned int mclk_value;
	clock_params params;

	if (CHIP_IS_GX3201) {
		if (object == AUDIO_OBJECT_I2S)
			mclk_value = gx3201_audio_i2s_mclk[mclk_index];
		else
			mclk_value = gx3201_audio_spd_mclk[mclk_index];

		params.audio_i2s.clock_source = I2S_PLL;
		params.audio_i2s.sample_index = samplefre_index;
		params.audio_i2s.clock_speed  = clock_speed;
		if (audioout->clock.pll.i2s_clock_index != samplefre_index ||
				audioout->clock.pll.i2s_clock_speed != clock_speed) {
			audioout->clock.pll.i2s_clock_index = samplefre_index;
			audioout->clock.pll.i2s_clock_speed = clock_speed;
			gxav_clock_setclock(MODULE_TYPE_AUDIO_I2S, &params);
		}

		gxlog_d(LOG_AOUT, "i2s pll: %d*%d=%d\n", \
				mclk_value, gx3201_audio_freq[samplefre_index], mclk_value*gx3201_audio_freq[samplefre_index]);
	} else {
		if (object == AUDIO_OBJECT_I2S) {
			mclk_value = gx3201_audio_i2s_mclk[mclk_index];
			audio_clock = gx3201_audio_freq[samplefre_index]*mclk_value;
			audio_clock = audio_clock*clock_speed;
			do_div(audio_clock, 100);
			audio_clock = audio_clock * (1<<30);
			do_div(audio_clock, _get_audioout_clock_base());

			params.audio_i2s.clock_source = I2S_DTO;
			params.audio_i2s.value = audio_clock;
			if (audioout->clock.dto.i2s_clock != audio_clock) {
				audioout->clock.dto.i2s_clock = audio_clock;
				gxav_clock_setclock(MODULE_TYPE_AUDIO_I2S, &params);
				gxav_clock_source_enable(MODULE_TYPE_AUDIO_I2S);
			}

			gxlog_d(LOG_AOUT, "i2s dto: %d*%d=%d\n", \
					mclk_value, gx3201_audio_freq[samplefre_index], mclk_value*gx3201_audio_freq[samplefre_index]);
		} else if (object == AUDIO_OBJECT_SPDIF) {
			mclk_value = gx3201_audio_spd_mclk[mclk_index];
			audio_clock = gx3201_audio_freq[samplefre_index]*mclk_value;
			audio_clock = audio_clock*clock_speed;
			do_div(audio_clock, 100);
			audio_clock = audio_clock * (1<<30);
			do_div(audio_clock, _get_audioout_clock_base());

			params.audio_spdif.clock_source = SPDIF_DTO;
			params.audio_spdif.value = audio_clock;
			if (audioout->clock.dto.spd_clock != audio_clock) {
				audioout->clock.dto.spd_clock = audio_clock;
				gxav_clock_setclock(MODULE_TYPE_AUDIO_SPDIF, &params);
			}

			gxlog_d(LOG_AOUT, "spd dto: %d*%d=%d\n", \
					mclk_value, gx3201_audio_freq[samplefre_index], mclk_value*gx3201_audio_freq[samplefre_index]);
		}
	}
}

void _set_audioout_spdif_samplefre(GxAudioSampleFre samplefre, enum stream_type object)
{
	//音频的channel status表
	//44.1 / 48 / 32 / 22.05 / 24 / 16（无意义）/ 96 /  88.2 /  176.4 / 192 / 64(无意义)khz
	//lsf/no lsf
	//CL1=(pcm_cs1_byte1<<16)|pcm_cs_byte0  //left ch
	//CR1=(pcm_cs2_byte1<<16)|pcm_cs_byte0  //right ch
	//CL2=CR2=pcm_cs_byte2
	unsigned int pcm_cs_byte0 = 0x0004;
	unsigned int pcm_cs1_byte1[12] = { 0x0010/*44.1*/, 0x0210/*48*/, 0x0310/*32*/, 0x0410/*22.05*/, 0x0610/*24*/,
		0x0110/*16*/, 0x0a10/*96*/, 0x0810/*88.2*/,0x8b10/*128*/,0x0c10/*176.4*/,0x0e10/*192*/,0x0b10/*64*/ };
	unsigned int pcm_cs2_byte1[12] = { 0x0020, 0x0220, 0x0320, 0x0420, 0x0620, 0x0120, 0x0a20, 0x0820, 0x8b20, 0x0c20, 0x0e20, 0x0110 };

	unsigned int pcm_cs_byte2[12][2] = { {0x00bb, 0x00fb}/*44.1*/, {0x009b, 0x00db}/*48*/, {0x004b, 0x00cb}/*32*/,{0x00ab, 0x00bb}/*22.05*/,
		{0x002b, 0x009b}/*24*/,{0x006b, 0x008b}/*16*/,{0x00db, 0x005b}/*96*/,{0x00fb, 0x007b}/*88.2*/,{0x004b,0x00eb}/*128*/,{0x007b, 0x003b}/*176.4*/, {0x005b, 0x001b}/*192*/,{0x00cb, 0x004b}/*64*/ };
	//CL1=CR1=(spd_cs_byte1<<16)|spd_cs_byte0
	//CL2=CR2=0
	unsigned int spd_cs_byte0 = 0x0006;
	unsigned int spd_cs_byte1[12] = { 0x0000, 0x0200, 0x0300, 0x0400, 0x0600, 0x0100, 0x0a00, 0x0800,0x8b00,0x0c00, 0x0e00, 0x0b00 };

	switch (object) {
	case SPDIF_STREAM_TYPE_PCM:	//pcm
		//配置线形pcm channel status
		AUDIO_SET_CHANNEL_STATUS_CL1(gx3201_audiospdif_reg, ((pcm_cs1_byte1[samplefre]<<16)|pcm_cs_byte0));
		AUDIO_SET_CHANNEL_STATUS_CR1(gx3201_audiospdif_reg, ((pcm_cs2_byte1[samplefre]<<16)|pcm_cs_byte0));

		AUDIO_SET_CHANNEL_STATUS_CL2(gx3201_audiospdif_reg, pcm_cs_byte2[samplefre][1]);
		AUDIO_SET_CHANNEL_STATUS_CR2(gx3201_audiospdif_reg, pcm_cs_byte2[samplefre][1]);
		break;
	case SPDIF_STREAM_TYPE_ENC:	//NONPCM
		//配置Spdif Channel Status
		AUDIO_SET_CHANNEL_STATUS_CL1(gx3201_audiospdif_reg, ((spd_cs_byte1[samplefre]<<16)|spd_cs_byte0));
		AUDIO_SET_CHANNEL_STATUS_CR1(gx3201_audiospdif_reg, ((spd_cs_byte1[samplefre]<<16)|spd_cs_byte0));

		AUDIO_SET_CHANNEL_STATUS_CL2(gx3201_audiospdif_reg, 0);
		AUDIO_SET_CHANNEL_STATUS_CR2(gx3201_audiospdif_reg, 0);
		break;
	default:
		break;
	}
}

void _set_audioout_spdif_raw_samplefre(GxAudioSampleFre samplefre, enum stream_type object)
{
	//音频的channel status表
	//44.1 / 48 / 32 / 22.05 / 24 / 16（无意义）/ 96 /  88.2 /  176.4 / 192 / 64(无意义)khz
	//lsf/no lsf
	//CL1=(pcm_cs1_byte1<<16)|pcm_cs_byte0  //left ch
	//CR1=(pcm_cs2_byte1<<16)|pcm_cs_byte0  //right ch
	//CL2=CR2=pcm_cs_byte2
	unsigned int pcm_cs_byte0 = 0x0004;
	unsigned int pcm_cs1_byte1[12] = { 0x0010/*44.1*/, 0x0210/*48*/, 0x0310/*32*/, 0x0410/*22.05*/, 0x0610/*24*/,
		0x0110/*16*/, 0x0a10/*96*/, 0x0810/*88.2*/,0x8b10/*128*/,0x0c10/*176.4*/,0x0e10/*192*/,0x0110/*64*/ };
	unsigned int pcm_cs2_byte1[12] = { 0x0020, 0x0220, 0x0320, 0x0420, 0x0620, 0x0120, 0x0a20, 0x0820, 0x8b20, 0x0c20, 0x0e20, 0x0110 };

	unsigned int pcm_cs_byte2[12][2] = { {0x00bb, 0x00fb}/*44.1*/, {0x009b, 0x00db}/*48*/, {0x004b, 0x00cb}/*32*/,{0x00ab, 0x00bb}/*22.05*/,
		{0x002b, 0x009b}/*24*/,{0x006b, 0x004b}/*16*/,{0x00db, 0x005b}/*96*/,{0x00fb, 0x007b}/*88.2*/,{0x004b,0x00eb}/*128*/, {0x007b, 0x003b}/*176.4*/, {0x005b, 0x001b}/*192*/,{0x00cb, 0x004b}/*64*/ };
	//CL1=CR1=(spd_cs_byte1<<16)|spd_cs_byte0
	//CL2=CR2=0
	unsigned int spd_cs_byte0 = 0x0006;
	unsigned int spd_cs_byte1[12] = { 0x0000, 0x0200, 0x0300, 0x0400, 0x0600, 0x0100, 0x0a00, 0x0800, 0x8b00, 0x0c00, 0x0e00, 0x0100 };

	switch (object) {
	case SPDIF_STREAM_TYPE_PCM:	//pcm
		//配置线形pcm channel status
		AUDIO_SET_CHANNEL_STATUS_CL1(gx3201_audiospdif_raw_reg, ((pcm_cs1_byte1[samplefre]<<16)|pcm_cs_byte0));
		AUDIO_SET_CHANNEL_STATUS_CR1(gx3201_audiospdif_raw_reg, ((pcm_cs2_byte1[samplefre]<<16)|pcm_cs_byte0));

		AUDIO_SET_CHANNEL_STATUS_CL2(gx3201_audiospdif_raw_reg, pcm_cs_byte2[samplefre][1]);
		AUDIO_SET_CHANNEL_STATUS_CR2(gx3201_audiospdif_raw_reg, pcm_cs_byte2[samplefre][1]);
		break;
	case SPDIF_STREAM_TYPE_ENC:	//NONPCM
		//配置Spdif Channel Status
		AUDIO_SET_CHANNEL_STATUS_CL1(gx3201_audiospdif_raw_reg, ((spd_cs_byte1[samplefre]<<16)|spd_cs_byte0));
		AUDIO_SET_CHANNEL_STATUS_CR1(gx3201_audiospdif_raw_reg, ((spd_cs_byte1[samplefre]<<16)|spd_cs_byte0));

		AUDIO_SET_CHANNEL_STATUS_CL2(gx3201_audiospdif_raw_reg, 0);
		AUDIO_SET_CHANNEL_STATUS_CR2(gx3201_audiospdif_raw_reg, 0);
		break;
	default:
		break;
	}
}

static void _set_audioout_mute(struct gxav_audioout *audioout, unsigned int object)
{
	switch (object) {
	case SPDIF_STREAM_TYPE_PCM:
		{
			if (audioout->pcm_mute_status == AUDIO_MUTE)
				return;
			gxlog_d(LOG_AOUT, "%s %d [mute success]\n", __func__, __LINE__);

			audioout->pcm_mute_status = AUDIO_MUTE;
			audioout->pcm_ch_value = gxav_get_i2s_channel_output(0);
			gxav_set_i2s_channel_mute(0);
			gxav_set_i2s_channel_mute(1);
		}
		break;
	case SPDIF_STREAM_TYPE_ENC:
		{
			if (audioout->spd_mute_status == AUDIO_MUTE)
				return;
			gxlog_d(LOG_AOUT, "%s %d [mute success]\n", __func__, __LINE__);

			audioout->spd_mute_status = AUDIO_MUTE;
			audioout->spd_ch_active = 1;
		}
		break;
	default:
		break;
	}
}

static void _set_audioout_unmute(struct gxav_audioout *audioout, unsigned int object)
{
	switch (object) {
	case SPDIF_STREAM_TYPE_PCM:		//pcm
		if (audioout->pcm_mute_status == AUDIO_UNMUTE) {
			return;
		}

		gxlog_d(LOG_AOUT, "%s %d [unmute success]\n", __func__, __LINE__);
		gxav_set_i2s_channel_output(0, audioout->pcm_ch_value);
		gxav_set_i2s_channel_output(1, audioout->pcm_ch_value);
		audioout->pcm_mute_status = AUDIO_UNMUTE;
		break;

	case SPDIF_STREAM_TYPE_ENC:		//non-pcm
		if (audioout->spd_mute_status == AUDIO_UNMUTE) {
			return;
		}

		gxlog_d(LOG_AOUT, "%s %d [unmute success]\n", __func__, __LINE__);
		audioout->spd_mute_status = AUDIO_UNMUTE;
		audioout->spd_ch_active = 1;
		break;

	default:
		break;
	}
}

static int _set_audioout_magnification(struct gxav_audioout* audioout, unsigned int audio_level)
{
	//音量调节同时控制I2S输出和SPDIF线性输出(pcm模式1和pcm模式2)
	switch (audio_level) {
	case AMPLIFY_LEVEL_LOW:
		gxav_set_i2s_magnification(0, AMPLIFY_LEVEL_LOW);
		gxav_set_i2s_magnification(1, AMPLIFY_LEVEL_LOW);
		AUDIO_SET_SPD_PCM_MAGNIFICATION(gx3201_audiospdif_reg, AMPLIFY_LEVEL_LOW);
		break;
	case AMPLIFY_LEVEL_MID:
		gxav_set_i2s_magnification(0, AMPLIFY_LEVEL_MID);
		gxav_set_i2s_magnification(1, AMPLIFY_LEVEL_MID);
		AUDIO_SET_SPD_PCM_MAGNIFICATION(gx3201_audiospdif_reg, AMPLIFY_LEVEL_MID);
		break;
	case AMPLIFY_LEVEL_HIGH:
		gxav_set_i2s_magnification(0, AMPLIFY_LEVEL_HIGH);
		gxav_set_i2s_magnification(1, AMPLIFY_LEVEL_HIGH);
		AUDIO_SET_SPD_PCM_MAGNIFICATION(gx3201_audiospdif_reg, AMPLIFY_LEVEL_HIGH);
		break;
	case AMPLIFY_LEVEL_EX_HIGH:
		gxav_set_i2s_magnification(0, AMPLIFY_LEVEL_EX_HIGH);
		gxav_set_i2s_magnification(1, AMPLIFY_LEVEL_EX_HIGH);
		AUDIO_SET_SPD_PCM_MAGNIFICATION(gx3201_audiospdif_reg, AMPLIFY_LEVEL_EX_HIGH);
		break;
	default:
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static void _recovery_audioout_status(struct gxav_audioout* audioout)
{
	_set_audioout_magnification(audioout, audioout->level);

	if (audioout->pcm_mute_status == AUDIO_MUTE) {
		gxav_set_i2s_channel_mute(0);
		gxav_set_i2s_channel_mute(1);
	} else {
		gxav_set_i2s_channel_output(0, audioout->pcm_ch_value);
		gxav_set_i2s_channel_output(1, audioout->pcm_ch_value);
	}

	if (audioout->spd_mute_status == AUDIO_MUTE) {
		AUDIO_SPD_CHANNEL_L_SEL(gx3201_audiospdif_reg, 0x08);
		AUDIO_SPD_CHANNEL_R_SEL(gx3201_audiospdif_reg, 0x08);
	} else
		AUDIO_SET_SPD_CHANNEL_OUTPUT(gx3201_audiospdif_reg, audioout->spd_ch_value);

	return;
}

static void _set_audiout_channel(struct gxav_audiochannel *ch, GxAudioOutProperty_OutData channel)
{
	unsigned int i = 0,j = 0;

	if ((channel < AUDIO_CHANNEL_MIN_VALUE) || (channel > AUDIO_CHANNEL_MAX_VALUE)) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return;
	}

	gx_memset(ch, 0, sizeof(struct gxav_audiochannel));
	for(i = 0,j = 0; i < MAX_CHANNEL; i++) {
		if (channel & (1 << i)) {
			ch->c_num++;
			ch->channel_data[j++] = i;
		}
	}
}

static void _inquire_audioout_unmute(struct gxav_audioout *audioout)
{
	if (audioout->unmute_active) {
		_set_audioout_unmute(audioout, SPDIF_STREAM_TYPE_PCM);
		_set_audioout_unmute(audioout, SPDIF_STREAM_TYPE_ENC);
	} else {
		audioout->unmute_inquire = 1;
	}
}

static void _active_audioout_unmute(struct gxav_audioout *audioout)
{
	if (audioout->unmute_inquire) {
		_set_audioout_unmute(audioout, SPDIF_STREAM_TYPE_PCM);
		_set_audioout_unmute(audioout, SPDIF_STREAM_TYPE_ENC);
		audioout->unmute_inquire = 0;
	}

	audioout->unmute_active = 1;
}

static void _active_audioout_channel(struct gxav_audioout *audioout)
{
	if (audioout->spd_ch_active) {
		if (audioout->spd_mute_status == AUDIO_MUTE) {
			unsigned int spd_channel_value = AUDIO_GET_SPD_CHANNEL_OUTPUT(gx3201_audiospdif_reg);
			AUDIO_SET_SPD_CHANNEL_OUTPUT(gx3201_audiospdif_reg, (spd_channel_value|0x88));
			AUDIO_SET_SPD_RAW_MUTE(gx3201_audiospdif_raw_reg, 1);
		} else {
			if (audioout->spdif_outdata_type == SPDIF_SRC_PCM)
				AUDIO_SET_SPD_CHANNEL_OUTPUT(gx3201_audiospdif_reg, audioout->spd_ch_value);
			else
				AUDIO_SET_SPD_CHANNEL_OUTPUT(gx3201_audiospdif_reg, 0x0);
			AUDIO_SET_SPD_RAW_MUTE(gx3201_audiospdif_raw_reg, 0);
		}
		audioout->spd_ch_active = 0;
	}
}

static int _set_audioout_volume(struct gxav_audioout* audioout, unsigned int volume)
{
	volume = (volume > 100) ? 100 : volume;

	audioout->level = volume_table[volume].mag;
	_set_audioout_magnification(audioout, volume_table[volume].mag);
	gxav_audiodec_set_volume(audioout->i2s_pcm[0].dec_id, volume_table[volume].db);//down value db
	audioout->volume = volume;
	if (audioout->volume == 0) {
		_set_audioout_mute(audioout, SPDIF_STREAM_TYPE_PCM);	//pcm
		_set_audioout_mute(audioout, SPDIF_STREAM_TYPE_ENC);	//spdif
	} else {
		if (audioout->mute_inquire == 0) {
			_inquire_audioout_unmute(audioout);
		}
	}

	return 0;
}


static int _audioout_iounmap(void)
{
	gxav_uinit_i2s_module(0);
	gxav_uinit_i2s_module(1);

	gx_iounmap(gx3201_audiospdif_reg);
	gx_iounmap(gx3201_audiospdif_raw_reg);
	gx3201_audiospdif_reg = NULL;
	gx3201_audiospdif_raw_reg = NULL;
	gx_release_mem_region(AUDIO_SPDIF_BASE_ADDR, sizeof(struct audiospdif_regs));
	gx_release_mem_region(AUDIO_SPDIF_RAW_BASE_ADDR, sizeof(struct audiospdif_raw_regs));

	return 0;
}

static int _audioout_ioremap(void)
{
	unsigned char *audiospdif_mapped_addr = NULL;
	unsigned char *audiospdif_raw_mapped_addr = NULL;

	//I2S
	gxav_init_i2s_module(0);
	gxav_init_i2s_module(1);

	//SPDIF
	if (NULL == gx_request_mem_region(AUDIO_SPDIF_BASE_ADDR, sizeof(struct audiospdif_regs))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		goto error;
	}
	audiospdif_mapped_addr = gx_ioremap(AUDIO_SPDIF_BASE_ADDR, sizeof(struct audiospdif_regs));
	if (!audiospdif_mapped_addr) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		goto error;
	}
	gx3201_audiospdif_reg = (struct audiospdif_regs *)audiospdif_mapped_addr;
	if (NULL == gx_request_mem_region(AUDIO_SPDIF_RAW_BASE_ADDR, sizeof(struct audiospdif_raw_regs))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		goto error;
	}
	audiospdif_raw_mapped_addr = gx_ioremap(AUDIO_SPDIF_RAW_BASE_ADDR, sizeof(struct audiospdif_raw_regs));
	if (!audiospdif_raw_mapped_addr) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		goto error;
	}
	gx3201_audiospdif_raw_reg = (struct audiospdif_raw_regs *)audiospdif_raw_mapped_addr;

	return 0;

error:
	_audioout_iounmap();
	return -1;
}

static void _printf_audioout_hdmi_data(unsigned int outdata_type,
		unsigned indata_type, unsigned int src_type, unsigned int edid_type)
{
	char hdmi_string[128] = {0};

	strcat(hdmi_string, "[HDMI : ");
	switch (outdata_type) {
	case HDMI_SRC_PCM:
		strcat(hdmi_string, "pcm");
		break;
	case HDMI_SRC_AC3:
		strcat(hdmi_string, "dly");
		break;
	case HDMI_SRC_EAC3:
		strcat(hdmi_string, "dly+");
		break;
	case HDMI_SRC_DTS:
		strcat(hdmi_string, "dts");
		break;
	case HDMI_SRC_AAC:
		strcat(hdmi_string, "aac");
		break;
	default:
		strcat(hdmi_string, "not");
		break;
	}
	strcat(hdmi_string, "] (INPUT: ");
	if (indata_type & PCM_TYPE)
		strcat(hdmi_string, "|pcm|");
	if (indata_type & AC3_TYPE)
		strcat(hdmi_string, "|dly|");
	if (indata_type & EAC3_TYPE)
		strcat(hdmi_string, "|dly+|");
	if (indata_type & ADPCM_TYPE)
		strcat(hdmi_string, "|adpcm|");
	if (indata_type & DTS_TYPE)
		strcat(hdmi_string, "|dts|");
	if (indata_type & AAC_TYPE)
		strcat(hdmi_string, "|aac|");
	strcat(hdmi_string, ") (MODE: ");
	switch (src_type) {
		case PCM_OUTPUT:
			strcat(hdmi_string, "pcm");
			break;
		case AC3_OUTPUT:
			strcat(hdmi_string, "dly");
			break;
		case EAC3_OUTPUT:
			strcat(hdmi_string, "dly+");
			break;
		case SMART_OUTPUT:
			strcat(hdmi_string, "samrt");
			break;
		case SAFE_OUTPUT:
			strcat(hdmi_string, "safe");
			break;
		case RAW_OUTPUT:
			strcat(hdmi_string, "raw");
			break;
		case DTS_OUTPUT:
			strcat(hdmi_string, "dts");
			break;
		case AAC_OUTPUT:
			strcat(hdmi_string, "aac");
			break;
		default:
			strcat(hdmi_string, "not");
			break;
	}
	strcat(hdmi_string, ") (EDID: ");
	if (edid_type & EDID_AUDIO_LINE_PCM)
		strcat(hdmi_string, "|pcm|");
	if (edid_type & EDID_AUDIO_AC3)
		strcat(hdmi_string, "|dly|");
	if (edid_type & EDID_AUDIO_EAC3)
		strcat(hdmi_string, "|dly+|");
	if (edid_type & EDID_AUDIO_DTS)
		strcat(hdmi_string, "|dts|");
	if (edid_type & EDID_AUDIO_AAC)
		strcat(hdmi_string, "|aac|");
	if (edid_type == 0)
		strcat(hdmi_string, "|not|");
	strcat(hdmi_string, ")");

	gxlog_i(LOG_AOUT, "%s\n", hdmi_string);
}

static void _printf_audioout_spdif_data(unsigned int outdata_type, unsigned indata_type, unsigned int src_type)
{
	char spdif_string[128] = {0};

	strcat(spdif_string, "[SPDIF: ");
	switch (outdata_type) {
	case SPDIF_SRC_PCM:
		strcat(spdif_string, "pcm");
		break;
	case SPDIF_SRC_AC3:
		strcat(spdif_string, "dly");
		break;
	case SPDIF_SRC_EAC3:
		strcat(spdif_string, "dly+");
		break;
	case SPDIF_SRC_DTS:
		strcat(spdif_string, "dts");
		break;
	case SPDIF_SRC_AAC:
		strcat(spdif_string, "aac");
		break;
	default:
		strcat(spdif_string, "not");
		break;
	}
	strcat(spdif_string, "] (INPUT: ");
	if (indata_type & PCM_TYPE)
		strcat(spdif_string, "|pcm|");
	if (indata_type & AC3_TYPE)
		strcat(spdif_string, "|dly|");
	if (indata_type & EAC3_TYPE)
		strcat(spdif_string, "|dly+|");
	if (indata_type & ADPCM_TYPE)
		strcat(spdif_string, "|adpcm|");
	if (indata_type & DTS_TYPE)
		strcat(spdif_string, "|dts|");
	if (indata_type & AAC_TYPE)
		strcat(spdif_string, "|aac|");
	strcat(spdif_string, ") (MODE: ");
	switch (src_type) {
		case PCM_OUTPUT:
			strcat(spdif_string, "pcm");
			break;
		case AC3_OUTPUT:
			strcat(spdif_string, "dly");
			break;
		case EAC3_OUTPUT:
			strcat(spdif_string, "dly+");
			break;
		case SMART_OUTPUT:
			strcat(spdif_string, "samrt");
			break;
		case SAFE_OUTPUT:
			strcat(spdif_string, "safe");
			break;
		case RAW_OUTPUT:
			strcat(spdif_string, "raw");
			break;
		case DTS_OUTPUT:
			strcat(spdif_string, "dts");
			break;
		case AAC_OUTPUT:
			strcat(spdif_string, "aac");
			break;
		default:
			strcat(spdif_string, "not");
			break;
	}
	strcat(spdif_string, ")");

	gxlog_i(LOG_AOUT, "%s\n", spdif_string);
}

static int _check_audioout_hdmi_data(struct gxav_audioout *audioout)
{
	int outdata_type = HDMI_SRC_NOTANY;
	int edid_audio_codes = gxav_hdmi_audio_codes_get();

	if (audioout->hdmisrc == RAW_OUTPUT) {
		if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
			outdata_type = HDMI_SRC_EAC3;
		else if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
			outdata_type = HDMI_SRC_AC3;
		else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
			outdata_type = HDMI_SRC_DTS;
		else if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
			outdata_type = HDMI_SRC_AAC;
		else if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
			outdata_type = HDMI_SRC_PCM;
	} else if (audioout->hdmisrc == SMART_OUTPUT || audioout->hdmisrc == SAFE_OUTPUT) {
		if ((edid_audio_codes & EDID_AUDIO_EAC3) &&
				((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)) {
			outdata_type = HDMI_SRC_EAC3;
		} else if ((edid_audio_codes & EDID_AUDIO_AC3) &&
				((audioout->indata_type & AC3_TYPE) == AC3_TYPE)) {
			outdata_type = HDMI_SRC_AC3;
		} else if ((edid_audio_codes & EDID_AUDIO_LINE_PCM) &&
				(audioout->indata_type & PCM_TYPE) == PCM_TYPE) {
			outdata_type = HDMI_SRC_PCM;
		} else if ((edid_audio_codes & EDID_AUDIO_DTS) &&
				(audioout->indata_type & DTS_TYPE) == DTS_TYPE) {
			outdata_type = HDMI_SRC_DTS;
		} else if ((edid_audio_codes & EDID_AUDIO_AAC) &&
				((audioout->indata_type & AAC_TYPE) == AAC_TYPE)) {
			outdata_type = HDMI_SRC_AAC;
		} else if (edid_audio_codes == 0) {
			if (audioout->hdmisrc == SMART_OUTPUT) {
				if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
					outdata_type = HDMI_SRC_EAC3;
				else if ((audioout->indata_type & AC3_TYPE ) == AC3_TYPE)
					outdata_type = HDMI_SRC_AC3;
				else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
					outdata_type = HDMI_SRC_DTS;
				else if ((audioout->indata_type & AAC_TYPE ) == AAC_TYPE)
					outdata_type = HDMI_SRC_AAC;
				else if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
					outdata_type = HDMI_SRC_PCM;
			} else if (audioout->hdmisrc == SAFE_OUTPUT) {
				if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
					outdata_type = HDMI_SRC_PCM;
				else if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
					outdata_type = HDMI_SRC_EAC3;
				else if ((audioout->indata_type & AC3_TYPE ) == AC3_TYPE)
					outdata_type = HDMI_SRC_AC3;
				else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
					outdata_type = HDMI_SRC_DTS;
				else if ((audioout->indata_type & AAC_TYPE ) == AAC_TYPE)
					outdata_type = HDMI_SRC_AAC;
			}
		}
	} else if (audioout->hdmisrc == PCM_OUTPUT) {
		if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
			outdata_type = HDMI_SRC_PCM;
	} else if (audioout->hdmisrc == AC3_OUTPUT) {
		if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
			outdata_type = HDMI_SRC_AC3;
	} else if (audioout->hdmisrc == EAC3_OUTPUT) {
		if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
			outdata_type = HDMI_SRC_EAC3;
	} else if (audioout->hdmisrc == DTS_OUTPUT) {
		if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
			outdata_type = HDMI_SRC_DTS;
	} else if (audioout->hdmisrc == AAC_OUTPUT) {
		if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
			outdata_type = HDMI_SRC_AAC;
	}

	if (audioout->hdmi_outdata_type != outdata_type) {
		audioout->hdmi_outdata_type = outdata_type;
		audioout->hdmisrc_change = 1;
		_printf_audioout_hdmi_data(audioout->hdmi_outdata_type,
				audioout->indata_type, audioout->hdmisrc, edid_audio_codes);
	}

	return 0;
}

static int _check_audioout_spdif_data(struct gxav_audioout *audioout)
{
	int outdata_type = 0;

	if (audioout->spdsrc == SMART_OUTPUT || audioout->spdsrc == SAFE_OUTPUT) {
		if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
			outdata_type = SPDIF_SRC_AC3;
		else if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
			outdata_type = SPDIF_SRC_PCM;
		else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
			outdata_type = SPDIF_SRC_DTS;
		else if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
			outdata_type = SPDIF_SRC_AAC;
		else
			outdata_type = SPDIF_SRC_NOTANY;
	} else if (audioout->spdsrc == RAW_OUTPUT) {
		if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
			outdata_type = SPDIF_SRC_AC3;
		else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
			outdata_type = SPDIF_SRC_DTS;
		else if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
			outdata_type = SPDIF_SRC_AAC;
		else if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
			outdata_type = SPDIF_SRC_PCM;
		else
			outdata_type = SPDIF_SRC_NOTANY;
	} else if (audioout->spdsrc == PCM_OUTPUT) {
		if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
			outdata_type = SPDIF_SRC_PCM;
		else
			outdata_type = SPDIF_SRC_NOTANY;
	} else if (audioout->spdsrc == AC3_OUTPUT) {
		if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
			outdata_type = SPDIF_SRC_AC3;
		else
			outdata_type = SPDIF_SRC_NOTANY;
	} else if (audioout->spdsrc == DTS_OUTPUT) {
		if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
			outdata_type = SPDIF_SRC_DTS;
		else
			outdata_type = SPDIF_SRC_NOTANY;
	} else if (audioout->spdsrc == AAC_OUTPUT) {
		if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
			outdata_type = SPDIF_SRC_AAC;
		else
			outdata_type = SPDIF_SRC_NOTANY;
	}

	if (audioout->spdif_outdata_type != outdata_type) {
		audioout->spdif_outdata_type = outdata_type;
		audioout->spdsrc_change = 1;
		_printf_audioout_spdif_data(audioout->spdif_outdata_type, audioout->indata_type, audioout->spdsrc);
	}

	return 0;
}

static int _check_audioout_recovery_src(struct gxav_audioout *audioout)
{
	if (audioout->data_event & PCM_ERROR) {
		audioout->indata_type &= ~(PCM_TYPE);
		audioout->data_event  &= ~(PCM_ERROR);
	}

	if (audioout->data_event & AC3_ERROR) {
		audioout->indata_type &= ~(AC3_TYPE);
		audioout->data_event  &= ~(AC3_ERROR);
	}

	if (audioout->data_event & EAC3_ERROR) {
		audioout->indata_type &= ~(EAC3_TYPE);
		audioout->data_event  &= ~(EAC3_ERROR);
	}

	if (audioout->data_event & AAC_ERROR) {
		audioout->indata_type &= ~(AAC_TYPE);
		audioout->data_event  &= ~(AAC_ERROR);
	}

	if (audioout->data_event & DTS_ERROR) {
		audioout->indata_type &= ~(DTS_TYPE);
		audioout->data_event  &= ~(DTS_ERROR);
	}

	if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
		audioout->recovery_src = PCM_RECOVERY;
	else if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
		audioout->recovery_src = EAC3_RECOVERY;
	else if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
		audioout->recovery_src = AAC_RECOVERY;
	else if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
		audioout->recovery_src = AC3_RECOVERY;
	else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
		audioout->recovery_src = DTS_RECOVERY;

	return 0;
}

#define SPD_PLAY_CDDATA_LOAD_DATA(regs, node, pause_len)                              \
	do {                                                                              \
		/* 配置pause帧的长度 */                                                       \
		AUDIO_SPD_SET_PAUSE_NUM(regs, pause_len);                                     \
		if (node->frame_type == AUDIO_TYPE_EAC3) {                                    \
			AUDIO_SPD_SET_PC(regs, (((node->frame_bsmod&0x7)<<8)|E_AC3_STREAM_FLAG)); \
			AUDIO_SPD_SET_PD(regs, node->frame_size);                                 \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs, E_AC3_FRAME_LENGTH);                   \
		} else if (node->frame_type == AUDIO_TYPE_DTS) {                              \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs, (node->frame_bytes));                  \
			if (node->frame_bytes < 1024)                                             \
			AUDIO_SPD_SET_PC(regs, 11/*DTS_STREAM_FLAG*/);                            \
			else if (node->frame_bytes < 2048)                                        \
			AUDIO_SPD_SET_PC(regs, 12/*DTS_STREAM_FLAG*/);                            \
			else if (node->frame_bytes < 4096)                                        \
			AUDIO_SPD_SET_PC(regs, 13/*DTS_STREAM_FLAG*/);                            \
			else                                                                      \
			AUDIO_SPD_SET_PC(regs, 17/*DTS_STREAM_FLAG*/);                            \
		} else if(node->frame_type == AUDIO_TYPE_ADTS_AAC_LC){                        \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_PC(regs,((1<<8)|(0<<5)|ADTS_STREAM_FLAG));                  \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs,AAC_LC_FRAME_LENGTH);                   \
		} else if(node->frame_type == AUDIO_TYPE_ADTS_HE_AAC){                        \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_PC(regs,((4<<8)|(1<<5)|ADTS_STREAM_FLAG));                  \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs,HE_AAC_FRAME_LENGTH);                   \
		} else if(node->frame_type == AUDIO_TYPE_ADTS_HE_AAC_V2){                     \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_PC(regs,((4<<8)|(1<<5)|ADTS_STREAM_FLAG));                  \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs,HE_AAC_FRAME_LENGTH);                   \
		} else if(node->frame_type == AUDIO_TYPE_LATM_AAC_LC){                        \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_PC(regs,((1<<5)|LATM_STREAM_FLAG));                         \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs,AAC_LC_FRAME_LENGTH);                   \
		} else if(node->frame_type == AUDIO_TYPE_LATM_HE_AAC){                        \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_PC(regs,((2<<5)|LATM_STREAM_FLAG));                         \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs,HE_AAC_FRAME_LENGTH);                   \
		} else if(node->frame_type == AUDIO_TYPE_LATM_HE_AAC_V2){                     \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_PC(regs,((1<<9)|(2<<5)|LATM_STREAM_FLAG));                  \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs,HE_AAC_FRAME_LENGTH);                   \
		} else {                                                                      \
			AUDIO_SPD_SET_PC(regs, AC3_STREAM_FLAG);                                  \
			AUDIO_SPD_SET_PD(regs, ((node->frame_size)<<3));                          \
			AUDIO_SPD_SET_NEWFRAME_LENTH(regs, AC3_FRAME_LENGTH);                     \
		}                                                                             \
		/* 配置非线性数据一帧的帧长 */                                                \
		AUDIO_SPD_NEWFRAME_START_ADDR(regs, node->frame_s_addr);                      \
		AUDIO_SPD_NEWFRAME_END_ADDR(regs, node->frame_e_addr);                        \
		AUDIO_SPD_SET_DATA_LENTH(regs, node->frame_size);                             \
		AUDIO_SPD_SET_NEW_FRAME(regs);                                                \
	}while(0)

static int _sync_adpcm_by_pts(struct gxav_audioout *audioout,
		int frame_count, int pts, int stc, int low_tolerance, int high_tolerance, int debug_type)
{
	int offset;
	GxSTCProperty_TimeResolution resolution;

	if (audioout->sync == 0) {
		audioout->sync_state = PTS_NORMAL;
		goto sync_result;
	}

	if ((pts == -1) || (stc == -1)) {
		audioout->sync_state = PTS_NORMAL;
		goto sync_result;
	}

	gxav_stc_get_base_resolution(0, &resolution);
	pts += audioout->offset_ms*(resolution.freq_HZ/1000);
	offset = (pts - stc)/(resolution.freq_HZ/1000);

	if (abs(offset) <= low_tolerance) {
		audioout->sync_state = PTS_NORMAL;
		AUDIOOUT_DEBUG_SYNC("cm sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
	} else if ((offset >= (0 - high_tolerance)) && (offset < (0 - low_tolerance))) {
		audioout->sync_state = PTS_SKIP;
		AUDIOOUT_DEBUG_SYNC("sk sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
	} else if ((offset > low_tolerance) && (offset <= high_tolerance)) {
		audioout->sync_state = PTS_REPEAT;
		audioout->spd_src2.enc_offset_time = offset;
		audioout->spd_src3.enc_offset_time = offset;
		AUDIOOUT_DEBUG_SYNC("rp sync", pts, stc, offset, audioout->low_tolerance, audioout->high_tolerance, debug_type);
	} else {
		audioout->sync_state = PTS_NORMAL;
		AUDIOOUT_DEBUG_SYNC("fr sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
	}

sync_result:
	return audioout->sync_state;
}

static int _play_hdmi_mute(struct gxav_audioout* audioout)
{
	if (audioout->hdmiMuteDo) {
		if (audioout->hdmiMuteFrames > 5) {
			gxav_hdmi_audioout_mute(0);
			audioout->hdmiMuteDo = 0;
		}
		audioout->hdmiMuteFrames++;
	}
	return 0;
}

static int _sync_audioout_by_pts(struct gxav_audioout *audioout,
		int frame_count, int pts, int add_low_sync, int recovery_stc, int debug_type)
{
	int stc, offset;
	GxSTCProperty_TimeResolution resolution;
	unsigned int low_tolerance, high_tolerance;

	if (pts == -1) {
		audioout->sync_state = PTS_NORMAL;
		goto sync_result;
	}

	gxav_stc_get_base_resolution(0, &resolution);
	pts += audioout->offset_ms*(resolution.freq_HZ/1000);

	if (recovery_stc) {
		gxav_stc_write_apts(0, pts, 0);
		gxav_stc_read_stc(0, (unsigned int*)&stc);
		audioout->sync_stc = stc;
	} else
		stc = audioout->sync_stc;

	if ((audioout->sync == 0) || ((stc & 0x1) == 0)) {
		audioout->sync_state = PTS_NORMAL;
		goto sync_result;
	}

	offset = (pts - stc)/(resolution.freq_HZ/1000);
	low_tolerance  = audioout->low_tolerance + add_low_sync;
	high_tolerance = audioout->high_tolerance;

	if (abs(offset) <= low_tolerance) {
		audioout->sync_state = PTS_NORMAL;
		AUDIOOUT_DEBUG_SYNC("cm sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
	} else if ((offset >= (0 - high_tolerance)) && (offset < (0 - low_tolerance))) {
		if (frame_count < 3) {
			audioout->sync_state = PTS_NORMAL;
			AUDIOOUT_DEBUG_SYNC("cm sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
		} else {
			audioout->sync_state = PTS_SKIP;
			AUDIOOUT_DEBUG_SYNC("sk sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
		}
	} else if ((offset > low_tolerance) && (offset <= high_tolerance)) {
		audioout->sync_state = PTS_REPEAT;
		audioout->spd_src2.enc_offset_time = offset;
		audioout->spd_src3.enc_offset_time = offset;
		AUDIOOUT_DEBUG_SYNC("rp sync", pts, stc, offset, audioout->low_tolerance, audioout->high_tolerance, debug_type);
	} else {
		audioout->sync_state = PTS_NORMAL;
		AUDIOOUT_DEBUG_SYNC("fr sync", pts, stc, offset, low_tolerance, high_tolerance, debug_type);
	}

sync_result:
	if(recovery_stc) {
		if (audioout->sync_state == PTS_NORMAL) {
			audioout->play_pcm_pts  = pts;
			audioout->stat.play_frame_cnt++;
		} else
			audioout->stat.lose_sync_cnt++;
	}
	return audioout->sync_state;
}

static void _set_pcm_data_to_spdif (struct gxav_audioout* audioout)
{
	AUDIO_SPD_SAMPLE_DATA_STORE_NON_INTERLACE(gx3201_audiospdif_reg);
	AUDIO_SET_SPD_SAMPLE_DATA_ENDIAN(gx3201_audiospdif_reg, 0x0);
	if (CHIP_IS_GX6605S && NO_MULTI_I2S && audioout->spdif_outdata_type == SPDIF_SRC_PCM) {
		AUDIO_SET_SPD_BUFFER_START_ADDR(gx3201_audiospdif_reg, audioout->i2s_pcm[0].pcm_in_start_addr);
		AUDIO_SET_SPD_BUFFER_SIZE      (gx3201_audiospdif_reg, audioout->i2s_pcm[0].pcm_in_size);
		AUDIO_SPDINT_SET_NEW_PCM_CLR   (gx3201_audiospdif_reg);
		AUDIO_SET_SPD_PLAY_MODE        (gx3201_audiospdif_reg, SPD_PLAY_PCM2);
	} else
		AUDIO_SET_SPD_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_PCM1);
}

static void _set_pcm_data_to_i2s(struct gxav_audioout* audioout, int id)
{
	gxav_set_i2s_dac_mix(id, gx3201_dac_mix_table[audioout->pcm_mix]);
	gxav_set_i2s_interlace(id, 0);
	gxav_set_i2s_endian(id, 1);
	gxav_set_i2s_pcm_source_sel(id, PCM_SEL_SDRAM_MODE);
	gxav_init_i2s_isr(id);
}

static int _get_adpcm_data_node(struct gxav_audioout* audioout, GxAudioFrameNode* node)
{
	if (node->vaild == 0) {
		gxfifo_get(audioout->i2s_adpcm.pcm_fifo, node, sizeof(GxAudioFrameNode));
		node->vaild = 1;
	}
	return 0;
}

static int _add_adpcm_data_to_pcm(struct gxav_audioout *audioout, GxAudioFrameNode* pcm, GxAudioFrameNode* adpcm)
{
	int i = 0, j = 0;

	if ((pcm->frame_size != adpcm->frame_size) ||
			(pcm->vaild == 0) || (adpcm->vaild == 0)) {
		gxlog_i(LOG_AOUT, "%d %d %d %d\n", pcm->frame_size, adpcm->frame_size, pcm->vaild, adpcm->vaild);
		return 0;
	}

	for(j = 0; j < audioout->max_channel_num; j++)
	{
		unsigned int virt_pcm_start_addr = audioout->i2s_pcm[0].pcm_in_start_addr;
		unsigned int virt_adpcm_start_addr = audioout->i2s_adpcm.pcm_in_start_addr;
		unsigned int pcm_offset = pcm->frame_s_addr;
		unsigned int adpcm_offset = adpcm->frame_s_addr;

		virt_pcm_start_addr += j*audioout->i2s_pcm[0].pcm_in_size;
		virt_adpcm_start_addr += j*audioout->i2s_adpcm.pcm_in_size;
		for(i = 0; i < pcm->frame_size; i++) {
			int tmp_val0 = 0, tmp_val1 = 0;
			int *virt_pcm_addr, *virt_adpcm_addr;

			if ((pcm_offset+3) >= audioout->i2s_pcm[0].pcm_in_size) {
				pcm_offset = 0;
			}

			if ((adpcm_offset+3) >= audioout->i2s_adpcm.pcm_in_size) {
				adpcm_offset = 0;
			}

			virt_pcm_addr = (int*)gx_phys_to_virt(virt_pcm_start_addr + pcm_offset);
			virt_adpcm_addr = (int*)gx_phys_to_virt(virt_adpcm_start_addr + adpcm_offset);

			if ((audioout->pcm_mix & MIX_PCM) == MIX_PCM) {
				tmp_val0 = *virt_pcm_addr;
				tmp_val0 = GET_ENDIAN_32(tmp_val0);
			}

			if ((audioout->pcm_mix & MIX_ADPCM) == MIX_ADPCM) {
				tmp_val1 = *virt_adpcm_addr;
				tmp_val1 = GET_ENDIAN_32(tmp_val1);
			}

			tmp_val0 += tmp_val1;

			*virt_pcm_addr = GET_ENDIAN_32(tmp_val0);
			pcm_offset += 4;
			adpcm_offset += 4;
		}
	}

	return 0;
}

static int _free_adpcm_data_node(struct gxav_audioout* audioout, GxAudioFrameNode* node)
{
	if (node->vaild == 1) {
		unsigned int adpcm_buf_size = audioout->i2s_adpcm.pcm_in_size;
		int len = (node->frame_e_addr+adpcm_buf_size-node->frame_s_addr+1)%adpcm_buf_size;

		gxav_sdc_length_set(audioout->i2s_adpcm.pcm_in_id, len, GXAV_SDC_READ);
		node->vaild = 0;
	}
	return 0;
}

#define AOUT_PLAY_ADPCM (0)
#define AOUT_WAIT_ADPCM (1)
#define AOUT_LACK_ADPCM (2)
static int _start_play_adpcm_data(struct gxav_audioout *audioout, int pts)
{
	int retVal = AOUT_LACK_ADPCM;
#define ADPCM_LOW_TOLERANCE 30 //30ms between pcm and adpcm

get_node1:
	if ((audioout->i2s_adpcm.pcm_repeating == 1) ||
			(audioout->i2s_adpcm.pcm_fifo &&
			 gxfifo_len(audioout->i2s_adpcm.pcm_fifo) >= sizeof(GxAudioFrameNode)))
	{
		int frame_count = 0;
		GxAudioFrameNode *node= &audioout->i2s_adpcm.pcm_play_node;
		enum pts_status sync_state;

		if (audioout->i2s_adpcm.pcm_repeating == 0)
			_get_adpcm_data_node(audioout, node);

		frame_count = gxfifo_len(audioout->i2s_adpcm.pcm_fifo)/sizeof(GxAudioFrameNode);
		sync_state = _sync_adpcm_by_pts(audioout,
				frame_count, node->pts, pts, ADPCM_LOW_TOLERANCE, audioout->high_tolerance, AD_AOUT_DEBUG);
		switch(sync_state)
		{
		case PTS_NORMAL:
			_add_adpcm_data_to_pcm(audioout, &audioout->i2s_pcm[0].pcm_play_node, node);
			_free_adpcm_data_node(audioout, node);
			audioout->i2s_adpcm.pcm_repeating = 0;
			return AOUT_PLAY_ADPCM;
		case PTS_SKIP:
			_free_adpcm_data_node(audioout, node);
			audioout->i2s_adpcm.pcm_repeating = 0;
			retVal = AOUT_WAIT_ADPCM;
			goto get_node1;
		case PTS_REPEAT:
			_add_adpcm_data_to_pcm(audioout, &audioout->i2s_pcm[0].pcm_play_node, node);
			audioout->i2s_adpcm.pcm_repeating = 1;
			return AOUT_PLAY_ADPCM;
		}
	}

	return retVal;
}

static int _get_pcm_data_node(struct gxav_audioout* audioout, int id, GxAudioFrameNode* node)
{
	if (node->vaild == 0) {
		gxfifo_get(audioout->i2s_pcm[id].pcm_fifo, node, sizeof(GxAudioFrameNode));
		node->vaild = 1;
	}
	return 0;
}

static int _free_pcm_data_node(struct gxav_audioout* audioout, int id, GxAudioFrameNode* node)
{
	if (node->vaild == 1) {
		unsigned int pcm_buf_size = audioout->i2s_pcm[id].pcm_in_size;
		int len = (node->frame_e_addr+pcm_buf_size-node->frame_s_addr+1)%pcm_buf_size;
		gxav_sdc_length_set(audioout->i2s_pcm[id].pcm_in_id, len, GXAV_SDC_READ);
		node->vaild = 0;
	}
	return 0;
}

static void _play_pcm_data_node(struct gxav_audioout *audioout, int id, GxAudioFrameNode* node)
{
	_play_hdmi_mute(audioout);

	if (CHIP_IS_GX6605S && NO_MULTI_I2S && audioout->spdif_outdata_type == SPDIF_SRC_PCM) {//6605s only play pcm2 mode
		AUDIO_SPD_SAMPLE_DATA_LEN_SEL(gx3201_audiospdif_reg, node->sample_data_len);
		AUDIO_SPD_NEWFRAME_START_ADDR(gx3201_audiospdif_reg, node->frame_s_addr);
		AUDIO_SPD_NEWFRAME_END_ADDR  (gx3201_audiospdif_reg, node->frame_e_addr);
		AUDIO_SPD_SET_NEWFRAME_PCMLEN(gx3201_audiospdif_reg, node->frame_size);
		AUDIO_SPD_SET_NEW_FRAME      (gx3201_audiospdif_reg);
	}

	gxav_set_i2s_new_frame(id, 0);
	gxav_set_i2s_sample_data_len(id, node->sample_data_len);
	gxav_set_i2s_load_data(id, node->frame_s_addr, node->frame_e_addr, node->frame_size);
	gxav_set_i2s_isr_enable(id, 1);
	gxav_set_i2s_new_frame(id, 1);

	gxav_hdmi_acr_enable(1);
}

static int _check_pcm_sample_len(struct gxav_audioout *audioout, int id, GxAudioFrameNode* node)
{
	audioout->i2s_pcm[id].pcm_sample_data_len = node->sample_data_len;
	return 0;
}

static int _check_pcm_data_node(struct gxav_audioout *audioout, int id, GxAudioFrameNode* node)
{
	unsigned int samplefre  = node->sample_fre;
	unsigned int channelnum = node->channel_num;

	if (node->sample_fre_change || audioout->pcm_speed_change ||
			(audioout->hdmisrc_change && (audioout->hdmi_outdata_type == HDMI_SRC_PCM)) ||
			(audioout->spdsrc_change && (audioout->spdif_outdata_type == SPDIF_SRC_PCM))) {
		unsigned int pcm_channel_value, i2s_mclk_index;
		unsigned int i2s_samplefre_index = 0, spd_samplefre_index = 0;

		if (samplefre < 32000) {
			gxav_set_i2s_src_enable(id, 1);
			if (CHIP_IS_GX6605S && NO_MULTI_I2S && audioout->spdif_outdata_type == SPDIF_SRC_PCM)
				spd_samplefre_index = gxav_audioout_samplefre_index(samplefre);
			else
				spd_samplefre_index = gxav_audioout_samplefre_index(samplefre*2);
			i2s_samplefre_index = gxav_audioout_samplefre_index(samplefre*2);
		} else {
			gxav_set_i2s_src_enable(id, 0);
			spd_samplefre_index = gxav_audioout_samplefre_index(samplefre);
			i2s_samplefre_index = gxav_audioout_samplefre_index(samplefre);
		}

		i2s_mclk_index  = gxav_get_i2s_clock(id);
		_set_audioout_clock_freq(audioout, i2s_mclk_index, i2s_samplefre_index, AUDIO_OBJECT_I2S, audioout->speed);
		gxav_set_i2s_channel_nums(id, (channelnum-1));

		if (audioout->spdif_outdata_type == SPDIF_SRC_PCM) {
			if (CHIP_IS_GX3201 == 0) {
				unsigned int spd_mclk_index = _get_audioout_spd_module_clk(gx3201_audiospdif_reg);
				_set_audioout_clock_freq(audioout, spd_mclk_index, spd_samplefre_index, AUDIO_OBJECT_SPDIF, audioout->speed);
			}
			// 播放模式为输出pcm_mode1数据
			AUDIO_SPD_SET_SPDIF_SPD_SEL(gx3201_audiospdif_reg);
			_set_audioout_spdif_samplefre(spd_samplefre_index, SPDIF_STREAM_TYPE_PCM);
			AUDIO_SPD_CONFIG_PCM_CHANNEL_NUMS(gx3201_audiospdif_reg, (channelnum-1));
			audioout->spdsrc_change = 0;
		}

		if (audioout->hdmi_outdata_type == HDMI_SRC_PCM) {
			AUDIO_SPD_SET_HDMI_SOURCE_I2S(gx3201_audiospdif_reg);
			gxav_hdmi_audioout_mute(1);
			gxav_hdmi_audioout_set(0);
			gxav_hdmi_audiosample_change(i2s_samplefre_index, channelnum);
			audioout->hdmiMuteDo     = 1;
			audioout->hdmiMuteFrames = 0;
			audioout->hdmisrc_change = 0;
		}

		if (audioout->stereo) {
			if (channelnum > 2) {
				if (node->dolby_version == 1)
					pcm_channel_value = 0x76542310;
				else
					pcm_channel_value = 0x76431520;
			} else
				pcm_channel_value = 0x76543210;
		} else
			pcm_channel_value = audioout->pcm_ch_value;

		if (audioout->pcm_mute_status == AUDIO_UNMUTE) {
			gxav_set_i2s_channel_output(id, pcm_channel_value);
		} else {
			audioout->pcm_ch_value = pcm_channel_value;
			audioout->pcm_speed_change = 0;
		}
	}
	return 0;
}

static int _start_play_pcm_data(struct gxav_audioout *audioout, int id)
{
	int retVal;

get_node:
	if ((audioout->i2s_pcm[id].pcm_repeating == 1) ||
			(audioout->i2s_pcm[id].pcm_fifo &&
			 gxfifo_len(audioout->i2s_pcm[id].pcm_fifo) >= sizeof(GxAudioFrameNode)))
	{
		int frame_count  = 0;
		int recovery_src = 0, debug_type = 0, add_low_sync = 0;
		GxAudioFrameNode *node= &audioout->i2s_pcm[id].pcm_play_node;

		audioout->i2s_pcm[id].pcm_hungry = 0;
		if (audioout->i2s_pcm[id].pcm_repeating == 0) {
			_get_pcm_data_node(audioout, id, node);
			_check_pcm_sample_len(audioout, id, node);
			if (id == 0)
				_check_pcm_data_node(audioout, id, node);
			audioout->i2s_pcm[id].pcm_add_adpcm = 0;
		} else {
			node->sample_data_len = audioout->i2s_pcm[id].pcm_sample_data_len;
		}

		if (id == 0) {
			recovery_src = (audioout->recovery_src==PCM_RECOVERY)?1:0;
			debug_type   = PCM_AOUT_DEBUG;
			add_low_sync = 0;
		} else if (id == 1) {
			recovery_src = 0;
			debug_type   = AD_AOUT_DEBUG;
			add_low_sync = 300;
		}

		frame_count = gxfifo_len(audioout->i2s_pcm[id].pcm_fifo)/sizeof(GxAudioFrameNode);
		retVal = _sync_audioout_by_pts(audioout, frame_count, node->pts, add_low_sync, recovery_src, debug_type);
		if ((audioout->multi_i2s == 0) &&
				(audioout->i2s_adpcm.pcm_fifo != NULL) &&
				(audioout->i2s_pcm[id].pcm_add_adpcm == 0)) {
			int ret = _start_play_adpcm_data(audioout, node->pts);
			if (ret != AOUT_PLAY_ADPCM) {
				if (audioout->pcm_mix == MIX_ADPCM) {
					_free_pcm_data_node(audioout, id, node);
					goto get_node;
				} else if (audioout->pcm_mix == (MIX_PCM|MIX_ADPCM)) {
					if (ret == AOUT_WAIT_ADPCM) {
						retVal = PTS_REPEAT;
						audioout->lack_adpcm_count = 0;
					} else if (ret == AOUT_LACK_ADPCM) {
						if (audioout->lack_adpcm_count < 100) {
							retVal = PTS_REPEAT;
							audioout->lack_adpcm_count++;
						}
					}
				}
			} else
				audioout->i2s_pcm[id].pcm_add_adpcm = 1;
		}

		switch(retVal)
		{
		case PTS_NORMAL:
			{
				if (audioout->i2s_pcm[id].pcm_play_count < 2) {
					audioout->i2s_pcm[id].pcm_play_count++;
					node->sample_data_len = (node->sample_data_len&0x10)?node->sample_data_len:0x0F;
				}
				if (audioout->pcm_speed_mute)
					node->sample_data_len = (node->sample_data_len&0x10)?node->sample_data_len:0x0F;

				audioout->i2s_pcm[id].pcm_repeating = 0;
				_play_pcm_data_node(audioout, id,  node);
				break;
			}
		case PTS_SKIP:
			{
				audioout->i2s_pcm[id].pcm_repeating = 0;
				_free_pcm_data_node(audioout, id, node);
				goto get_node;
			}
		case PTS_REPEAT:
			{
				audioout->i2s_pcm[id].pcm_repeating = 1;
				node->sample_data_len = (node->sample_data_len&0x10)?node->sample_data_len:0x0F;
				_play_pcm_data_node(audioout, id, node);
				break;
			}
		default:
			break;
		}
	}
	else {
		audioout->i2s_pcm[id].pcm_hungry = 1;
		audioout->i2s_pcm[id].pcm_data_operate = I2S_PLAY_PCM;
		return -1;
	}

	return 0;
}

static int _resume_play_pcm_data(struct gxav_audioout* audioout, int id)
{
	int ret = -1;

	if (audioout->i2s_pcm[id].pcm_hungry) {
		switch(audioout->i2s_pcm[id].pcm_data_operate)
		{
		case I2S_PLAY_PCM:
			{
				unsigned int frame_count = 0;

				if (audioout->i2s_pcm[id].pcm_fifo)
					frame_count = gxfifo_len(audioout->i2s_pcm[id].pcm_fifo) / sizeof(GxAudioFrameNode);

				if (frame_count >= HUNGRY_FREEZE_COUNT)
					ret = _start_play_pcm_data(audioout, id);
			}
			break;
		default:
			break;
		}
	}
	return ret;
}

int _play_zero_data(void)
{
	unsigned int samplefre = 48000;
	unsigned int channels  = 2;

	//解决某些电视机hdmi杂音问题，需要I2S一直有输出状态
	gxav_set_i2s_channel_nums  (0, (channels - 1));
	gxav_set_i2s_pcm_source_sel(0, PCM_SEL_CPU_MODE);
	gxav_set_i2s_cpu_set_pcm   (0, 0x0);
	gxav_set_i2s_work_enable   (0);
	gxav_hdmi_audioout_set (0);
	gxav_hdmi_audiosample_change(gxav_audioout_samplefre_index(samplefre), channels);

	return 0;
}

static int pcm_writed_callback(unsigned int id, unsigned int len, unsigned int overflow, void *arg)
{
	int idx = 0;
	unsigned int adframe_len = 0, frame_len = 0;
	struct gxav_audioout *audioout = (struct gxav_audioout*)arg;

	if ((audioout->out_state != AUDIOOUT_READY) &&
			(audioout->out_state != AUDIOOUT_RUNNING)) {
		return 0;
	}

	if (audioout->i2s_adpcm.pcm_fifo) {
		gxav_sdc_length_get(audioout->i2s_adpcm.pcm_in_id, &adframe_len);
	}

	for (idx = 0; idx < 2; idx++) {
		if (audioout->i2s_pcm[idx].pcm_in_id == id)
			break;
	}

	if (idx >= 2) return -1;

	if (audioout->i2s_pcm[idx].pcm_fifo) {
		gxav_sdc_length_get(audioout->i2s_pcm[idx].pcm_in_id, &frame_len);
	}

	_active_audioout_unmute(audioout);
	_active_audioout_channel(audioout);
	_resume_play_pcm_data(audioout, idx);

	return 0;
}

static int pcm_complete_out_isr(struct gxav_audioout *audioout, int id)
{
	gxav_set_i2s_isr_enable(id, 0);
	gxav_clr_i2s_isr_status(id);
	if (CHIP_IS_GX6605S && NO_MULTI_I2S && audioout->spdif_outdata_type == SPDIF_SRC_PCM) {
		if (id == 0) {
			while(!AUDIO_SPDINT_GET_NEW_PCM_STATUS(gx3201_audiospdif_reg));
			AUDIO_SPDINT_SET_NEW_PCM_CLR(gx3201_audiospdif_reg);
			while(AUDIO_SPDINT_GET_NEW_PCM_STATUS(gx3201_audiospdif_reg));
		}
	}

	if (audioout->i2s_pcm[id].pcm_repeating == 0)
		_free_pcm_data_node(audioout, id, &audioout->i2s_pcm[id].pcm_play_node);

	_start_play_pcm_data(audioout, id);
	return 0;
}

static void _set_src2_data_to_spdif (struct gxav_audioout *audioout)
{
	AUDIO_SPD_SAMPLE_DATA_STORE_INTERLACE(gx3201_audiospdif_reg);
	AUDIO_SET_SPD_SAMPLE_DATA_ENDIAN(gx3201_audiospdif_reg, 0x0);
	//配置压缩数据大小端
	AUDIO_SPD_SET_CDDATA_FORMAT(gx3201_audiospdif_reg, GET_ENDIAN_32(0));
	// 配置pause帧的长度
	AUDIO_SPD_SET_PAUSE_LENTH(gx3201_audiospdif_reg, 96);
	// 配置非线性数据一帧的样点数
	AUDIO_SPD_SET_NEWFRAME_LENTH(gx3201_audiospdif_reg, 1536);
	//配置Pa、Pb
	AUDIO_SPD_SET_PA_PB(gx3201_audiospdif_reg, 0xf8724e1f);
	// 配置Pc
	AUDIO_SPD_SET_PC(gx3201_audiospdif_reg, PAUSE_STREAM_FLAG);
	AUDIO_SPD_PAUSE_SET_PC(gx3201_audiospdif_reg, PAUSE_STREAM_FLAG);
	// 配置PD
	AUDIO_SPD_PAUSE_SET_PD(gx3201_audiospdif_reg, 96);
	if (SPD_FIX_CHIP) {
		AUDIO_SET_SPD_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_NONPCM);

		AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_reg);
		AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_reg, 1);
	} else {
		AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_reg);
		// Cd_play_done中断使能
		AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_reg);

		AUDIO_SET_SPD_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_NONPCM);
	}
}

static int _get_src2_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node)
{
	if (node->vaild == 0) {
		gxfifo_get(audioout->spd_src2.enc_fifo, node, sizeof(GxAudioFrameNode));
		node->vaild = 1;
	}

	return 0;
}

static int _free_src2_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node)
{
	if (node->vaild == 1)
	{
		int len = node->frame_size + node->frame_add_num;
		gxav_sdc_length_set(audioout->spd_src2.enc_in_id, len, GXAV_SDC_READ);
		node->vaild = 0;
	}

	return 0;
}

static void _play_src2_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node, int pause_len)
{
	_play_hdmi_mute(audioout);

	SPD_PLAY_CDDATA_LOAD_DATA(gx3201_audiospdif_reg, node, pause_len);
	if (SPD_FIX_CHIP) {
		//:...
	} else {
		AUDIO_SPDINT_CD_PLAY_DONE_ENABLE(gx3201_audiospdif_reg);
	}
	gxav_hdmi_acr_enable(1);
}

static void _check_src2_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node)
{
	if (node->sample_fre_change || audioout->spd_speed_change ||
			(audioout->hdmisrc_change && (audioout->hdmi_outdata_type == HDMI_SRC_AC3)) ||
			(audioout->hdmisrc_change && (audioout->hdmi_outdata_type == HDMI_SRC_DTS)) ||
			(audioout->spdsrc_change && (audioout->spdif_outdata_type == SPDIF_SRC_AC3)) ||
			(audioout->spdsrc_change && (audioout->spdif_outdata_type == SPDIF_SRC_DTS))) {
		unsigned int samplefre_index = gxav_audioout_samplefre_index(node->sample_fre);
		unsigned int spd_mclk_index;

		if ((audioout->spdif_outdata_type == SPDIF_SRC_AC3) ||
				(audioout->hdmi_outdata_type == HDMI_SRC_AC3) ||
				(audioout->spdif_outdata_type == SPDIF_SRC_DTS) ||
				(audioout->hdmi_outdata_type == HDMI_SRC_DTS))
			_set_audioout_spdif_samplefre(samplefre_index, SPDIF_STREAM_TYPE_ENC);

		//set spd module clock
		{
			if (CHIP_IS_GX3211 || CHIP_IS_GX6605S) {
				if ((node->sample_fre == 192000) ||
						((audioout->spdif_outdata_type == SPDIF_SRC_AC3 ||
						  audioout->spdif_outdata_type == SPDIF_SRC_DTS) &&
						 (audioout->hdmi_outdata_type == HDMI_SRC_AC3 ||
						  audioout->hdmi_outdata_type == HDMI_SRC_DTS ||
						  audioout->hdmi_outdata_type == HDMI_SRC_NOTANY))) {
					_set_audioout_spdif_raw_samplefre(samplefre_index, SPDIF_STREAM_TYPE_ENC);
					_set_audioout_spd_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_512FS);
				}
			}
			spd_mclk_index    = _get_audioout_spd_module_clk(gx3201_audiospdif_reg);
			_set_audioout_clock_freq(audioout, spd_mclk_index, samplefre_index, AUDIO_OBJECT_SPDIF, audioout->speed);
		}

		if ((audioout->spdif_outdata_type == SPDIF_SRC_AC3) ||
				(audioout->spdif_outdata_type == SPDIF_SRC_DTS)) {
			AUDIO_SPD_SET_SPDIF_SPD_SEL(gx3201_audiospdif_reg);
			audioout->spdsrc_change = 0;
		}

		if ((audioout->hdmi_outdata_type == HDMI_SRC_AC3) ||
				(audioout->hdmi_outdata_type == HDMI_SRC_DTS)) {
			AUDIO_SPD_SET_HDMI_SPD_SEL(gx3201_audiospdif_reg);
			gxav_hdmi_audioout_mute(1);
			gxav_hdmi_audioout_set(1);
			gxav_hdmi_audiosample_change(samplefre_index, 2);
			audioout->hdmiMuteDo     = 1;
			audioout->hdmiMuteFrames = 0;
			audioout->hdmisrc_change = 0;
		}
		audioout->spd_speed_change = 0;
	}
}

static int _start_play_src2_data(struct gxav_audioout *audioout)
{
	int retVal;

get_node:
	if ((audioout->spd_src2.enc_repeating == 1 ) ||
			(audioout->spd_src2.enc_fifo &&
			 gxfifo_len(audioout->spd_src2.enc_fifo) >= sizeof(GxAudioFrameNode)))
	{
		int frame_count  = 0;
		unsigned int recovery_src = 0;
		GxAudioFrameNode *node= &audioout->spd_src2.enc_play_node;

		audioout->spd_src2.enc_hungry = 0;
		if (audioout->spd_src2.enc_repeating == 0) {
			_get_src2_data_node(audioout, node);
			_check_src2_data_node(audioout, node);
		}

		recovery_src = (audioout->recovery_src==AC3_RECOVERY || audioout->recovery_src == DTS_RECOVERY)?1:0;
		frame_count = gxfifo_len(audioout->spd_src2.enc_fifo)/sizeof(GxAudioFrameNode);
		retVal = _sync_audioout_by_pts(audioout, frame_count, node->pts, 0, recovery_src, AC3_AOUT_DEBUG);
		switch(retVal)
		{
		case PTS_NORMAL:
			{
				audioout->spd_src2.enc_repeating = 0;
				_play_src2_data_node(audioout, node, 0);
				break;
			}
		case PTS_SKIP:
			{
				audioout->spd_src2.enc_repeating = 0;
				_free_src2_data_node(audioout, node);
				goto get_node;
			}
		case PTS_REPEAT:
			{
				int pause_len = _get_audioout_repeat_len(audioout->spd_src2.enc_offset_time, node->sample_fre);
				audioout->spd_src2.enc_repeating = 1;
				_play_src2_data_node(audioout, &audioout->spd_src2.enc_play_node, pause_len);
				break;
			}
		default:
			break;
		}
	}
	else {
		audioout->spd_src2.enc_hungry = 1;
		audioout->spd_src2.enc_data_operate = SPD_PLAY_CDDATA;
	}
	return 0;
}

static int _resume_play_src2_data(struct gxav_audioout* audioout)
{
	int ret = -1;

	if (audioout->spd_src2.enc_hungry)
	{
		switch(audioout->spd_src2.enc_data_operate)
		{
		case SPD_PLAY_CDDATA:
			{
				unsigned int frame_count = 0;

				if (audioout->spd_src2.enc_fifo)
					frame_count = gxfifo_len(audioout->spd_src2.enc_fifo) / sizeof(GxAudioFrameNode);

				if (frame_count >= HUNGRY_FREEZE_COUNT)
					ret = _start_play_src2_data(audioout);
			}
		default:
			break;
		}
	}
	return ret;
}

static int src2_writed_callback(unsigned int id, unsigned int len, unsigned int overflow, void *arg)
{
	unsigned int ac3_len = 0;
	struct gxav_audioout *audioout = (struct gxav_audioout*)arg;

	if ((audioout->out_state != AUDIOOUT_READY) &&
			(audioout->out_state != AUDIOOUT_RUNNING)) {
		return 0;
	}

	if (audioout->spd_src2.enc_fifo) {
		gxav_sdc_length_get(audioout->spd_src2.enc_in_id, &ac3_len);
	}

	_active_audioout_unmute(audioout);
	_active_audioout_channel(audioout);
	_resume_play_src2_data(audioout);

	return 0;
}

static int src2_complete_done_isr(struct gxav_audioout *audioout)
{
	if (SPD_FIX_CHIP) {
		AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_reg);
	} else {
		AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_reg);
		AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_reg);
		while(AUDIO_SPDINT_GET_CD_PLAY_DONE_STATUS(gx3201_audiospdif_reg));

		AUDIO_SPD_CLEAR_FIFO_STATUS(gx3201_audiospdif_reg);
		AUDIO_SPD_CLEAR_FIFO_STATUS_CLR(gx3201_audiospdif_reg);
	}

	if (audioout->spd_src2.enc_repeating == 0)
		_free_src2_data_node(audioout, &audioout->spd_src2.enc_play_node);

	_start_play_src2_data(audioout);

	return 0;
}

static void _set_src3_data_to_spdif (struct gxav_audioout *audioout)
{
	AUDIO_SPD_SAMPLE_DATA_STORE_INTERLACE(gx3201_audiospdif_raw_reg);
	AUDIO_SET_SPD_SAMPLE_DATA_ENDIAN(gx3201_audiospdif_raw_reg, 0x0);
	AUDIO_SPD_SET_CDDATA_FORMAT(gx3201_audiospdif_raw_reg, GET_ENDIAN_32(0));
	// 配置pause帧的长度
	AUDIO_SPD_SET_PAUSE_LENTH(gx3201_audiospdif_raw_reg, 96);
	// 配置非线性数据一帧的样点数
	AUDIO_SPD_SET_NEWFRAME_LENTH(gx3201_audiospdif_raw_reg, 1536);
	//配置Pa、Pb
	AUDIO_SPD_SET_PA_PB(gx3201_audiospdif_raw_reg, 0xf8724e1f);
	// 配置Pc
	AUDIO_SPD_SET_PC(gx3201_audiospdif_raw_reg, PAUSE_STREAM_FLAG);
	AUDIO_SPD_PAUSE_SET_PC(gx3201_audiospdif_raw_reg, PAUSE_STREAM_FLAG);
	// 配置PD
	AUDIO_SPD_PAUSE_SET_PD(gx3201_audiospdif_raw_reg, 96);
	if (SPD_FIX_CHIP) {
		AUDIO_SET_SPD_RAW_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_NONPCM);
		AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_raw_reg);
		AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_raw_reg, 1);
	} else {
		// 清中断
		AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_raw_reg);
		// Cd_play_done中断使能
		AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_raw_reg);

		AUDIO_SET_SPD_RAW_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_NONPCM);
	}
}

static int _get_src3_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node)
{
	if (node->vaild == 0) {
		gxfifo_get(audioout->spd_src3.enc_fifo, node, sizeof(GxAudioFrameNode));
		node->vaild = 1;
	}

	return 0;
}

static int _free_src3_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node)
{
	if (node->vaild == 1)
	{
		int len = node->frame_size;
		gxav_sdc_length_set(audioout->spd_src3.enc_in_id, len, GXAV_SDC_READ);
		node->vaild = 0;
	}

	return 0;
}

static void _play_src3_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node, int pause_len)
{
	_play_hdmi_mute(audioout);

	SPD_PLAY_CDDATA_LOAD_DATA(gx3201_audiospdif_raw_reg, node, pause_len);
	if (SPD_FIX_CHIP) {
		//:..
	} else {
		AUDIO_SPDINT_CD_PLAY_DONE_ENABLE(gx3201_audiospdif_raw_reg);
	}
	gxav_hdmi_acr_enable(1);
}

static void _check_src3_data_node(struct gxav_audioout *audioout, GxAudioFrameNode* node)
{
	if (node->sample_fre_change || audioout->spd_speed_change ||
			(audioout->hdmisrc_change && (audioout->hdmi_outdata_type == HDMI_SRC_EAC3)) ||
			(audioout->hdmisrc_change && (audioout->hdmi_outdata_type == HDMI_SRC_AAC)) ||
			(audioout->spdsrc_change && (audioout->spdif_outdata_type == SPDIF_SRC_EAC3)) ||
			(audioout->spdsrc_change && (audioout->spdif_outdata_type == SPDIF_SRC_AAC))) {
		unsigned int samplefre_index = gxav_audioout_samplefre_index(node->sample_fre);
		unsigned int d_samplefre_index, spd_raw_mclk_index;

		if ((audioout->spdif_outdata_type == SPDIF_SRC_EAC3) ||
				(audioout->spdif_outdata_type == SPDIF_SRC_AAC) ||
				(audioout->hdmi_outdata_type == SPDIF_SRC_EAC3) ||
				(audioout->hdmi_outdata_type == HDMI_SRC_AAC))
			_set_audioout_spdif_raw_samplefre(samplefre_index, SPDIF_STREAM_TYPE_ENC);

		//set spd raw module clock
		{
			if (node->frame_type == AUDIO_TYPE_EAC3) {
				if (CHIP_IS_GX3201) {
					_set_audioout_spd_raw_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_256FS);
					d_samplefre_index = gxav_audioout_samplefre_index((node->sample_fre>>2));
				} else {
					if ((audioout->spdif_outdata_type == SPDIF_SRC_AC3) &&
							(audioout->hdmi_outdata_type == HDMI_SRC_EAC3 ||
							 audioout->hdmi_outdata_type == HDMI_SRC_NOTANY))
						_set_audioout_spd_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_2048FS);
					d_samplefre_index = samplefre_index;
				}
			}else if ((node->frame_type >= AUDIO_TYPE_ADTS_AAC_LC)&&(node->frame_type<=AUDIO_TYPE_LATM_HE_AAC_V2)) {
					_set_audioout_spd_raw_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_512FS);
					_set_audioout_spd_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_512FS);
					d_samplefre_index = samplefre_index;
			}else
				d_samplefre_index = samplefre_index;

			spd_raw_mclk_index = _get_audioout_spd_raw_module_clk(gx3201_audiospdif_reg);
			_set_audioout_clock_freq(audioout, spd_raw_mclk_index, d_samplefre_index, AUDIO_OBJECT_SPDIF, audioout->speed);
		}

		if ((audioout->spdif_outdata_type == SPDIF_SRC_EAC3) ||
				(audioout->spdif_outdata_type == SPDIF_SRC_AAC)) {
			// 播放模式为输出non_pcm数据
			AUDIO_SPD_SET_SPDIF_SPD_RAW_SEL(gx3201_audiospdif_reg);
			audioout->spdsrc_change = 0;
		}

		if ((audioout->hdmi_outdata_type == HDMI_SRC_EAC3) ||
				(audioout->hdmi_outdata_type == HDMI_SRC_AAC)) {
			AUDIO_SPD_SET_HDMI_SPD_RAW_SEL(gx3201_audiospdif_reg);
			gxav_hdmi_audioout_mute(1);
			gxav_hdmi_audioout_set(1);
			gxav_hdmi_audiosample_change(samplefre_index, 2);
			audioout->hdmiMuteDo     = 1;
			audioout->hdmiMuteFrames = 0;
			audioout->hdmisrc_change = 0;
		}
		audioout->spd_speed_change = 0;
	}
}

static int _start_play_src3_data(struct gxav_audioout *audioout)
{
	int retVal;

get_node:
	if ((audioout->spd_src3.enc_repeating == 1 ) ||
			(audioout->spd_src3.enc_fifo &&
			 gxfifo_len(audioout->spd_src3.enc_fifo) >= sizeof(GxAudioFrameNode)))
	{
		int frame_count  = 0;
		unsigned int recovery_src = 0;
		GxAudioFrameNode *node= &audioout->spd_src3.enc_play_node;

		audioout->spd_src3.enc_hungry = 0;
		if (audioout->spd_src3.enc_repeating == 0) {
			_get_src3_data_node(audioout, node);
			_check_src3_data_node(audioout, node);
		}

		recovery_src = ((audioout->recovery_src == EAC3_RECOVERY) || (audioout->recovery_src == AAC_RECOVERY))?1:0;
		frame_count  = gxfifo_len(audioout->spd_src3.enc_fifo)/sizeof(GxAudioFrameNode);
		retVal = _sync_audioout_by_pts(audioout, frame_count, node->pts, 0, recovery_src, EAC3_AOUT_DEBUG);

		switch(retVal)
		{
		case PTS_NORMAL:
			{
				audioout->spd_src3.enc_repeating = 0;
				_play_src3_data_node(audioout, node, 0);
				break;
			}
		case PTS_SKIP:
			{
				audioout->spd_src3.enc_repeating = 0;
				_free_src3_data_node(audioout, node);
				goto get_node;
			}
		case PTS_REPEAT:
			{
				int pause_len = _get_audioout_repeat_len(audioout->spd_src3.enc_offset_time, node->sample_fre);
				audioout->spd_src3.enc_repeating = 1;
				_play_src3_data_node(audioout, &audioout->spd_src3.enc_play_node, pause_len);
				break;
			}
		default:
			break;
		}
	}
	else {
		audioout->spd_src3.enc_hungry = 1;
		audioout->spd_src3.enc_data_operate = SPD_PLAY_CDDATA;
	}
	return 0;
}

static int _resume_play_src3_data(struct gxav_audioout* audioout)
{
	int ret = -1;

	if (audioout->spd_src3.enc_hungry)
	{
		switch(audioout->spd_src3.enc_data_operate)
		{
		case SPD_PLAY_CDDATA:
			{
				unsigned int frame_count = 0;

				if (audioout->spd_src3.enc_fifo)
					frame_count = gxfifo_len(audioout->spd_src3.enc_fifo) / sizeof(GxAudioFrameNode);

				if (frame_count >= HUNGRY_FREEZE_COUNT)
					ret = _start_play_src3_data(audioout);
			}
		default:
			break;
		}
	}

	return ret;
}

static int src3_writed_callback(unsigned int id, unsigned int len, unsigned int overflow, void *arg)
{
	unsigned int eac3_len = 0;
	struct gxav_audioout *audioout = (struct gxav_audioout*)arg;

	if ((audioout->out_state != AUDIOOUT_READY) &&
			(audioout->out_state != AUDIOOUT_RUNNING)) {
		return 0;
	}

	if (audioout->spd_src3.enc_fifo) {
		gxav_sdc_length_get(audioout->spd_src3.enc_in_id, &eac3_len);
	}

	_active_audioout_unmute(audioout);
	_active_audioout_channel(audioout);
	_resume_play_src3_data(audioout);

	return 0;
}

static int src3_complete_done_isr(struct gxav_audioout *audioout)
{
	if (SPD_FIX_CHIP) {
		AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_raw_reg);
	} else {
		AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_raw_reg);
		AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_raw_reg);
		while(AUDIO_SPDINT_GET_CD_PLAY_DONE_STATUS(gx3201_audiospdif_raw_reg));

		AUDIO_SPD_RAW_CLEAR_FIFO_STATUS(gx3201_audiospdif_reg);
		AUDIO_SPD_RAW_CLEAR_FIFO_STATUS_CLR(gx3201_audiospdif_reg);
	}

	if (audioout->spd_src3.enc_repeating == 0)
		_free_src3_data_node(audioout, &audioout->spd_src3.enc_play_node);

	_start_play_src3_data(audioout);

	return 0;
}

static int gx3201_audioout_init(void)
{
	if (_audioout_ioremap() < 0)
		return -1;

	gxav_set_i2s_channel_init(0);
	gxav_set_i2s_channel_init(1);
	AUDIO_SET_SPD_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_OFF);
	AUDIO_SET_SPD_RAW_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_OFF);
	AUDIO_SPD_CHANNEL_L_SEL(gx3201_audiospdif_reg, AUDIO_SELECT_0_CHANNEL);
	AUDIO_SPD_CHANNEL_R_SEL(gx3201_audiospdif_reg, AUDIO_SELECT_1_CHANNEL);

	_play_zero_data();

	return 0;
}

static int gx3201_audioout_open(struct gxav_audioout *audioout)
{
	audioout->pcm_mute_status = AUDIO_UNMUTE;
	audioout->spd_mute_status = AUDIO_UNMUTE;
	audioout->compresstopcm  = 0;
	audioout->sync           = 1;
	audioout->low_tolerance  = 100;
	audioout->high_tolerance = 4000;
	audioout->offset_ms      = 0;
	audioout->pcm_mix        = MIX_PCM|MIX_ADPCM;
	audioout->multi_i2s      = gxav_get_i2s_multi();

	audioout->out_state = AUDIOOUT_STOPED;

	//output: enable clock must set clock freq, or will be sha sha for special power amplifier
	_set_audioout_clock_freq(audioout, 0, 1, AUDIO_OBJECT_SPDIF, 100);

	return 0;
}

static int gx3201_audioout_config_source(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigSource *config)
{
	if (config->audiodec_id == 1) {
		if (audioout->multi_i2s) {
			gx_memset(&audioout->i2s_pcm[1].pcm_play_node,   0,  sizeof(GxAudioFrameNode));
			audioout->i2s_pcm[1].pcm_hungry    = 0;
			audioout->i2s_pcm[1].pcm_play_count= 0;
			audioout->i2s_pcm[1].pcm_repeating = 0;
			audioout->i2s_pcm[1].dec_id        = 1;
		} else {
			gx_memset(&audioout->i2s_adpcm.pcm_play_node,  0,  sizeof(GxAudioFrameNode));
			audioout->i2s_adpcm.pcm_repeating  = 0;
			audioout->i2s_adpcm.dec_id         = 1;
		}

		return 0;
	}

	audioout->indata_type = config->indata_type;

	gx_memset(&audioout->i2s_pcm[0].pcm_play_node,   0,  sizeof(GxAudioFrameNode));
	gx_memset(&audioout->spd_src2.enc_play_node,      0,  sizeof(GxAudioFrameNode));
	gx_memset(&audioout->spd_src3.enc_play_node,     0,  sizeof(GxAudioFrameNode));
	audioout->i2s_pcm[0].pcm_hungry    = 0;
	audioout->i2s_pcm[0].pcm_play_count= 0;
	audioout->i2s_pcm[0].pcm_repeating = 0;
	audioout->i2s_pcm[0].dec_id        = 0;
	audioout->spd_src2.enc_hungry       = 0;
	audioout->spd_src2.enc_repeating    = 0;
	audioout->spd_src2.dec_id           = 0;
	audioout->spd_src3.enc_hungry      = 0;
	audioout->spd_src3.enc_repeating   = 0;
	audioout->spd_src3.dec_id          = 0;
	audioout->speed         = 100;
	audioout->play_pcm_pts  = -1;
	audioout->sync_stc      = 0;
	audioout->pcm_speed_change  = 0;
	audioout->pcm_speed_mute    = 0;
	audioout->spd_speed_change  = 0;
	audioout->hdmisrc_change    = 0;
	audioout->hdmi_outdata_type = 0;
	audioout->spdsrc_change     = 0;
	audioout->spdif_outdata_type= 0;
	audioout->out_state     = AUDIOOUT_INITED;
	audioout->data_event    = 0;

	return 0;
}

static int lodac_config_flag = 1;
static int gx3201_audioout_config_port(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigPort *config)
{
	if ((config->port_type & AUDIO_OUT_I2S_OUT_DAC) == AUDIO_OUT_I2S_OUT_DAC) {
		struct audio_dac dac;

		gxav_clock_multiplex_pinsel(PIN_TYPE_AUDIOOUT_I2S);	//i2s audio pin[27:26]=0
		dac.mclock = AUDIO_DACLOCK_256FS;
		dac.bclock = AUDIO_BCK_SEL_64FS;
		dac.format = AUDIO_DATA_FORMAT_I2S;
		dac.wordlen = AUDIO_PCM_WORD_LEN_24BIT;
		gxav_set_i2s_dac_params(0, &dac);
		audioout->i2ssrc = config->data_type;
	}

	if ((config->port_type & AUDIO_OUT_I2S_IN_DAC) == AUDIO_OUT_I2S_IN_DAC) {
		struct audio_dac dac;

		dac.mclock = AUDIO_DACLOCK_1024FS;
		dac.bclock = AUDIO_BCK_SEL_64FS;
		dac.format = AUDIO_DATA_FORMAT_I2S;
		dac.wordlen = AUDIO_PCM_WORD_LEN_24BIT;
		gxav_set_i2s_dac_params(0, &dac);

		if (lodac_config_flag) {
			gxav_clock_audioout_dacinside_clock_enable(0);
		}
		gxav_clock_audioout_dacinside(gx3201_audio_i2s_mclk[dac.mclock]);

		if(lodac_config_flag) {
			int pcm_temp = 0xff800000;
			gxav_hdmi_audioout_mute(1);
			gxav_set_i2s_pcm_source_sel(0, PCM_SEL_CPU_MODE);//cpu set pcm mode
			gxav_set_i2s_cpu_set_pcm(0, pcm_temp);
			gxav_set_i2s_work_enable(0);

			gx_mdelay(20);

			gxav_clock_audioout_dacinside_clock_enable(1);
			while (pcm_temp < 0) {
				pcm_temp  += 0x100;
				gxav_set_i2s_cpu_set_pcm(0, pcm_temp);//pcm value from 0x8000 to 0,than pwm from 0% to 50%
				gx_udelay(1);
			}
			gxav_set_i2s_pcm_source_sel(0, PCM_SEL_SDRAM_MODE);//reading from sdram mode
			gxav_hdmi_audioout_mute(0);
			lodac_config_flag = 0;
		}
		audioout->i2ssrc = config->data_type;
	}

	if ((config->port_type & AUDIO_OUT_SPDIF) == AUDIO_OUT_SPDIF) {
		//gx3201 spd共用i2s时钟，因此设置spd的倍频与i2s一致，gx3211 spd独立使用时钟
		AUDIO_SPD_SET_SLIENT(gx3201_audiospdif_reg);

		if (CHIP_IS_GX3201 == 0) {
			_set_audioout_spd_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_2048FS);
			_set_audioout_spd_raw_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_512FS);
		} else {
			_set_audioout_spd_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_1024FS);
			_set_audioout_spd_raw_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_1024FS);
		}

		if (CHIP_IS_GX3201 == 0)
			gxav_clock_source_enable(MODULE_TYPE_AUDIO_SPDIF);
		else
			gxav_clock_source_disable(MODULE_TYPE_AUDIO_SPDIF);

		audioout->spdsrc = config->data_type;
	}

	if ((config->port_type & AUDIO_OUT_HDMI) == AUDIO_OUT_HDMI) {
		audioout->hdmisrc = config->data_type;
	}

	return 0;
}

static int gx3201_audioout_config_sync(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigSync *config)
{
	audioout->sync           = config->sync;
	audioout->low_tolerance  = config->low_tolerance;
	audioout->high_tolerance = config->high_tolerance;

	return 0;
}

static int gx3201_audioout_power_mute(struct gxav_audioout* audioout, GxAudioOutProperty_PowerMute *power_mute)
{
	int src_pcm_temp = power_mute->enable ? 0 : 0xff800000;
	int dst_pcm_temp = power_mute->enable ? 0xff800000 : 0;
	struct audio_dac dac;

	dac.mclock  = AUDIO_DACLOCK_1024FS;
	dac.bclock  = AUDIO_BCK_SEL_64FS;
	dac.format  = AUDIO_DATA_FORMAT_I2S;
	dac.wordlen = AUDIO_PCM_WORD_LEN_24BIT;

	gxav_set_i2s_dac_params(0, &dac);
	_inquire_audioout_unmute(audioout);//解决静音时，调用power mute导致爆音问题
	if (power_mute->enable) {
		gxav_hdmi_audioout_mute(1);
		gxav_set_i2s_pcm_source_sel(0, PCM_SEL_CPU_MODE);//cpu set pcm mode
		gxav_set_i2s_cpu_set_pcm(0, src_pcm_temp);
		gxav_set_i2s_work_enable(0);
		while (src_pcm_temp > dst_pcm_temp) {
			src_pcm_temp  -= 0x100;
			gxav_set_i2s_cpu_set_pcm(0, src_pcm_temp);//pcm value from 0x8000 to 0,than pwm from 0% to 50%
			gx_udelay(1);
		}
	} else {
		gxav_set_i2s_pcm_source_sel(0, PCM_SEL_CPU_MODE);//cpu set pcm mode
		gxav_set_i2s_cpu_set_pcm(0, src_pcm_temp);
		gxav_set_i2s_work_enable(0);
		while (src_pcm_temp < dst_pcm_temp) {
			src_pcm_temp += 0x100;
			gxav_set_i2s_cpu_set_pcm(0, src_pcm_temp);//pcm value from 0x8000 to 0,than pwm from 0% to 50%
			gx_udelay(1);
		}
		gxav_hdmi_audioout_mute(0);
		gxav_set_i2s_pcm_source_sel(0, PCM_SEL_SDRAM_MODE);//reading from sdram mode
	}

	return 0;
}

static int gx3201_audioout_link_fifo(struct gxav_audioout *audioout, struct audioout_fifo *fifo)
{
	if (fifo == NULL) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (GXAV_PIN_INPUT == fifo->direction)
	{
		unsigned int audiodec_id = 0;
		unsigned char buf_id    = fifo->channel_id;
		unsigned int start_addr = fifo->buffer_start_addr;
		unsigned int end_addr   = fifo->buffer_end_addr;
		unsigned int buf_size   = (end_addr - start_addr + 1);

		if (GXAV_PIN_PCM == fifo->pin_id) {
			audiodec_id = audioout->i2s_pcm[0].dec_id;
			gxav_audiodec_get_max_channel(audiodec_id, &audioout->max_channel_num);
			buf_size = buf_size / audioout->max_channel_num;

			audioout->i2s_pcm[0].pcm_in_id         = buf_id;
			audioout->i2s_pcm[0].pcm_in_start_addr = start_addr;
			audioout->i2s_pcm[0].pcm_in_size       = buf_size;

			gxav_set_i2s_buffer(0, start_addr, buf_size);

			fifo->channel->indata	  = audioout;
			fifo->channel->incallback = pcm_writed_callback;
			gxav_audiodec_get_fifo(audiodec_id, GXAV_PIN_PCM, (void **)&audioout->i2s_pcm[0].pcm_fifo);
		} else if (GXAV_PIN_ADPCM == fifo->pin_id) {

			if (audioout->multi_i2s) {
				audiodec_id = audioout->i2s_pcm[1].dec_id;
				gxav_audiodec_get_max_channel(audiodec_id, &audioout->max_channel_num);
				buf_size = buf_size / audioout->max_channel_num;

				audioout->i2s_pcm[1].pcm_in_id         = buf_id;
				audioout->i2s_pcm[1].pcm_in_start_addr = start_addr;
				audioout->i2s_pcm[1].pcm_in_size       = buf_size;

				gxav_set_i2s_buffer(1, start_addr, buf_size);

				fifo->channel->indata     = audioout;
				fifo->channel->incallback = pcm_writed_callback;
				gxav_audiodec_get_fifo(audiodec_id, GXAV_PIN_ADPCM, (void **)&audioout->i2s_pcm[1].pcm_fifo);
			} else {
				audiodec_id = audioout->i2s_adpcm.dec_id;
				gxav_audiodec_get_max_channel(audiodec_id, &audioout->max_channel_num);
				buf_size = buf_size / audioout->max_channel_num;

				audioout->i2s_adpcm.pcm_in_id         = buf_id;
				audioout->i2s_adpcm.pcm_in_start_addr = start_addr;
				audioout->i2s_adpcm.pcm_in_size       = buf_size;
				gxav_audiodec_get_fifo(audiodec_id, GXAV_PIN_ADPCM, (void **)&audioout->i2s_adpcm.pcm_fifo);
			}
		} else if (GXAV_PIN_AC3 == fifo->pin_id|| GXAV_PIN_DTS == fifo->pin_id) {
			audioout->spd_src2.enc_in_id         = buf_id;
			audioout->spd_src2.enc_in_start_addr = start_addr;
			audioout->spd_src2.enc_in_size       = buf_size;

			AUDIO_SET_SPD_BUFFER_START_ADDR(gx3201_audiospdif_reg, start_addr);
			AUDIO_SET_SPD_BUFFER_SIZE(gx3201_audiospdif_reg, buf_size);

			fifo->channel->indata     = audioout;
			fifo->channel->incallback = src2_writed_callback;
			audiodec_id = audioout->spd_src2.dec_id;
			gxav_audiodec_get_fifo(audiodec_id, fifo->pin_id, (void **)&audioout->spd_src2.enc_fifo);
		} else if (GXAV_PIN_EAC3 == fifo->pin_id || GXAV_PIN_AAC == fifo->pin_id) {
			audioout->spd_src3.enc_in_id         = buf_id;
			audioout->spd_src3.enc_in_start_addr = start_addr;
			audioout->spd_src3.enc_in_size       = buf_size;

			AUDIO_SET_SPD_BUFFER_START_ADDR(gx3201_audiospdif_raw_reg, start_addr);
			AUDIO_SET_SPD_BUFFER_SIZE(gx3201_audiospdif_raw_reg, buf_size);

			fifo->channel->indata     = audioout;
			fifo->channel->incallback = src3_writed_callback;
			audiodec_id = audioout->spd_src3.dec_id;
			gxav_audiodec_get_fifo(audiodec_id, GXAV_PIN_EAC3, (void **)&audioout->spd_src3.enc_fifo);
		} else {
			gxlog_e(LOG_AOUT, "%s %d [pin_id: %d]\n", __func__, __LINE__, fifo->pin_id);
			return -1;
		}
	}
	else {
		gxlog_e(LOG_AOUT, "%s %d [direction: %d]\n", __func__, __LINE__, fifo->direction);
		return -1;
	}

	return 0;
}

static int gx3201_audioout_start(struct gxav_audioout *audioout)
{
	if (audioout->out_state != AUDIOOUT_INITED) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	memset(&audioout->clock, 0, sizeof(audio_clock_params));
	memset(&audioout->stat,  0, sizeof(audioout->stat));
	audioout->lack_adpcm_count = 0;
	audioout->out_state = AUDIOOUT_READY;

	_check_audioout_hdmi_data(audioout);
	_check_audioout_spdif_data(audioout);
	_check_audioout_recovery_src(audioout);

	if (CHIP_IS_GX6605S) {
		//patch: 6605s hdmi output by spdif is only by 512FS, but pcm output only by 256FS/384FS,
		//so we select 256FS in pcm data, can not output both ac3 for hdmi and pcm for spdif
		if (audioout->indata_type == PCM_TYPE) {
			_set_audioout_spd_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_256FS);
			_set_audioout_spd_raw_module_clk(gx3201_audiospdif_reg, SPD_CLOCK_256FS);
		} else if (audioout->spdsrc == PCM_OUTPUT) {
			audioout->spdsrc = SMART_OUTPUT;
		}
	}

	if (SPD_FIX_CHIP) {
		AUDIO_SPD_ENABLE_CACHE_FIFO   (gx3201_audiospdif_reg,     1);
		AUDIO_SPD_SET_CACHE_FIFO_COUNT(gx3201_audiospdif_raw_reg, 1);
	}

	if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE) {
		_set_pcm_data_to_i2s(audioout, 0);
		_set_pcm_data_to_i2s(audioout, 1);
		if (audioout->indata_type == PCM_TYPE ||
				((audioout->indata_type & AAC_TYPE) == AAC_TYPE &&
				 (audioout->spdif_outdata_type == SPDIF_SRC_PCM))) {
			AUDIO_SPD_CLEAR_RESET_STATUS(gx3201_audiospdif_reg);
			_set_pcm_data_to_spdif (audioout);
		}
		_start_play_pcm_data(audioout, 0);
		_start_play_pcm_data(audioout, 1);
	}

	if (((audioout->indata_type & AC3_TYPE) == AC3_TYPE) ||
			((audioout->indata_type & DTS_TYPE) == DTS_TYPE)) {
		AUDIO_SPD_CLEAR_RESET_STATUS(gx3201_audiospdif_reg);
		_set_src2_data_to_spdif (audioout);
		_start_play_src2_data(audioout);
	}

	if (((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE) ||
			((audioout->indata_type & AAC_TYPE) == AAC_TYPE)) {
		AUDIO_SPD_RAW_CLEAR_RESET_STATUS(gx3201_audiospdif_reg);
		_set_src3_data_to_spdif (audioout);
		_start_play_src3_data(audioout);
	}

	audioout->out_state = AUDIOOUT_RUNNING;

	return 0;
}

static int gx3201_audioout_set_channel(struct gxav_audioout *audioout, GxAudioOutProperty_Channel *channel)
{
	struct gxav_audiochannel audio_ch;
	unsigned int mono_en = 0;
	unsigned int pcm_channel_value = 0;
	unsigned int spd_channel_value = 0;

	if (channel->left != channel->right)
		audioout->stereo = 1;
	else
		audioout->stereo = 0;

	if (channel->mono) {
		mono_en = 1;
		channel->right = channel->left = AUDIO_CHANNEL_0|AUDIO_CHANNEL_1;
	}

	// 选择通道给DAC的左声道输出.
	// 选择通道给SPDIF的左声道输出.跟I2S的通道一一对应输出.
	_set_audiout_channel(&audio_ch, channel->left);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1) {
		pcm_channel_value |= (audio_ch.channel_data[0] << 0);
		spd_channel_value |= (audio_ch.channel_data[0] << 0);
	}
	else {
		pcm_channel_value |= (0 << 0);
		spd_channel_value |= (0 << 0);
	}

	// 选择通道给DAC的右声道输出.
	// 选择通道给SPDIF的右声道输出.跟I2S的通道一一对应输出.
	_set_audiout_channel(&audio_ch, channel->right);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1) {
		pcm_channel_value |= (audio_ch.channel_data[0] << 4);
		spd_channel_value |= (audio_ch.channel_data[0] << 4);
	}
	else {
		pcm_channel_value |= (1 << 4);
		spd_channel_value |= (1 << 4);
	}

	// 选择通道给DAC的左环绕输出.
	_set_audiout_channel(&audio_ch, channel->left_surround);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1)
		pcm_channel_value |= (audio_ch.channel_data[0] << 8);
	else {
		pcm_channel_value |= (2 << 8);
	}

	// 选择通道给DAC的右环绕输出.
	_set_audiout_channel(&audio_ch, channel->right_surround);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1)
		pcm_channel_value |= (audio_ch.channel_data[0] << 12);
	else {
		pcm_channel_value |= (3 << 12);
	}

	// 选择通道给DAC的中心声道输出.
	_set_audiout_channel(&audio_ch, channel->center);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1)
		pcm_channel_value |= (audio_ch.channel_data[0] << 16);
	else {
		pcm_channel_value |= (4 << 16);
	};

	// 选择通道给DAC的低频声道输出.
	_set_audiout_channel(&audio_ch, channel->lfe);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1)
		pcm_channel_value |= (audio_ch.channel_data[0] << 20);
	else {
		pcm_channel_value |= (5 << 20);
	};

	// 选择通道给DAC的左后环绕输出.
	_set_audiout_channel(&audio_ch, channel->left_backsurround);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1)
		pcm_channel_value |= (audio_ch.channel_data[0] << 24);
	else {
		pcm_channel_value |= (6 << 24);
	};

	// 选择通道给DAC的右后环绕输出.
	_set_audiout_channel(&audio_ch, channel->right_backsurround);
	if (audio_ch.c_num <= 0) {
		goto err;
	}
	else if (audio_ch.c_num == 1)
		pcm_channel_value |= (audio_ch.channel_data[0] << 28);
	else {
		pcm_channel_value |= (7 << 28);
	};

	gxav_audiodec_set_mono(audioout->i2s_pcm[0].dec_id, mono_en);

	//判断是否已经是静音的，如果静音，那么不能配置音频寄存器，以免出现
	//配置声道从而出现声音，只能记录配置的声道值，再下次不静音操作的时候
	//把声道值配置到音频寄存器中.
	if (audioout->pcm_mute_status == AUDIO_MUTE && audioout->spd_mute_status == AUDIO_MUTE) {
		audioout->pcm_ch_value = pcm_channel_value;
		audioout->spd_ch_value = spd_channel_value;

		gxlog_d(LOG_AOUT, "%s %d [pcm:0x%x, spd:0x%x]\n",
				__func__, __LINE__, audioout->pcm_ch_value, audioout->spd_ch_value);
		return 0;
	}

	gxav_set_i2s_channel_output(0, pcm_channel_value);
	gxav_set_i2s_channel_output(1, pcm_channel_value);
	audioout->pcm_ch_value = pcm_channel_value;
	audioout->spd_ch_value = spd_channel_value;
	audioout->spd_ch_active = 1;

	return 0;

err:
	gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
	return -1;
}

static int gx3201_audioout_set_volume(struct gxav_audioout *audioout, GxAudioOutProperty_Volume *volume)
{
	_set_audioout_volume(audioout, volume->volume);
	return 0;
}

static int gx3201_audioout_get_volume(struct gxav_audioout *audioout, GxAudioOutProperty_Volume *volume)
{
	volume->volume = audioout->volume;

	return 0;
}

static int gx3201_audioout_set_mute(struct gxav_audioout *audioout, GxAudioOutProperty_Mute *mute)
{
	if ((mute->mute < 0) || (mute->mute > 1)) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	audioout->mute_status = mute->mute ? AUDIO_MUTE: AUDIO_UNMUTE;

	if (mute->mute) {
		gxlog_d(LOG_AOUT, "%s %d [mute start]\n", __func__, __LINE__);
		audioout->mute_inquire = 1;
		audioout->unmute_inquire = 0;
		_set_audioout_mute(audioout, SPDIF_STREAM_TYPE_PCM);	//pcm
		_set_audioout_mute(audioout, SPDIF_STREAM_TYPE_ENC);	//spdif
	}
	else {
		gxlog_d(LOG_AOUT, "%s %d [unmute start]\n", __func__, __LINE__);
		audioout->mute_inquire = 0;
		if (audioout->volume != 0) {
			_inquire_audioout_unmute(audioout);
		}
	}

	return 0;
}

static int gx3201_audioout_get_mute(struct gxav_audioout *audioout, GxAudioOutProperty_Mute *mute)
{
	if (audioout->mute_status == AUDIO_MUTE)
		mute->mute = 1;
	else
		mute->mute = 0;

	return 0;
}

static int gx3201_audioout_get_state(struct gxav_audioout *audioout, GxAudioOutProperty_State *state)
{
	state->frame_stat = audioout->stat;
	state->state = audioout->out_state;
	return 0;
}

static int gx3201_audioout_mix_pcm(struct gxav_audioout *audioout, GxAudioOutProperty_PcmMix *config)
{
	audioout->pcm_mix = config->pcm_mix;

	if (audioout->pcm_mix == (MIX_PCM|MIX_ADPCM))
		gxav_audiodec_set_ad_ctrlinfo(1, 1);
	else
		gxav_audiodec_set_ad_ctrlinfo(1, 0);

	gxav_set_i2s_dac_mix(0, gx3201_dac_mix_table[audioout->pcm_mix]);

	return 0;
}

static int gx3201_audioout_set_speed(struct gxav_audioout *audioout, GxAudioOutProperty_Speed *speed)
{
	if ((speed->speed > 120) || (speed->speed < 80))
		return -1;

	audioout->pcm_speed_mute = speed->speed_mute;
	if (audioout->speed != speed->speed) {
		audioout->speed    = speed->speed;
		audioout->pcm_speed_change = 1;
		audioout->spd_speed_change = 1;
	}

	return 0;
}

static int gx3201_audioout_get_pts(struct gxav_audioout* audioout, GxAudioOutProperty_Pts *pts)
{
	pts->pts = audioout->play_pcm_pts;

	return 0;
}

static int gx3201_audioout_set_ptsoffset(struct gxav_audioout* audioout, GxAudioOutProperty_PtsOffset *pts)
{
	audioout->offset_ms = pts->offset_ms;

	return 0;
}

static int gx3201_audioout_set_event(struct gxav_audioout* audioout, GxAvAudioDataEvent data_event)
{
	audioout->data_event |= data_event;
	return 0;
}


static int gx3201_audioout_pause(struct gxav_audioout *audioout)
{
	if (audioout->out_state == AUDIOOUT_RUNNING) {
		audioout->out_state = AUDIOOUT_PAUSED;

		gxav_set_i2s_isr_enable(0, 0);
		gxav_set_i2s_isr_enable(1, 0);
		if (SPD_FIX_CHIP) {
			AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_reg,     0);
			AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_raw_reg, 0);
		} else {
			AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_raw_reg);
			AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_reg);
		}
	}

	//apts is invaild for fix pcr
	gxav_stc_invaild_apts(0);

	return 0;
}

static int gx3201_audioout_resume(struct gxav_audioout *audioout)
{
	if (audioout->out_state == AUDIOOUT_PAUSED) {
		_resume_play_pcm_data(audioout, 0);
		_resume_play_pcm_data(audioout, 1);
		_resume_play_src2_data(audioout);
		_resume_play_src3_data(audioout);

		audioout->out_state = AUDIOOUT_RUNNING;
		gxav_set_i2s_isr_enable(0, 1);
		gxav_set_i2s_isr_enable(1, 1);
		if (SPD_FIX_CHIP) {
			AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_reg,     1);
			AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_raw_reg, 1);
		} else {
			AUDIO_SPDINT_CD_PLAY_DONE_ENABLE(gx3201_audiospdif_reg);
			AUDIO_SPDINT_CD_PLAY_DONE_ENABLE(gx3201_audiospdif_raw_reg);
		}
	}

	return 0;
}

static int gx3201_audioout_unlink_fifo(struct gxav_audioout *audioout, struct audioout_fifo *fifo)
{
	if (fifo == NULL) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (GXAV_PIN_INPUT == fifo->direction) {
		if (GXAV_PIN_PCM == fifo->pin_id) {
			gxav_set_i2s_buffer(0, 0, 0);

			fifo->channel->incallback = NULL;
			fifo->channel->indata = NULL;
		} else if (GXAV_PIN_AC3 == fifo->pin_id || GXAV_PIN_DTS == fifo->pin_id) {

			AUDIO_SET_SPD_BUFFER_START_ADDR(gx3201_audiospdif_reg, 0);
			AUDIO_SET_SPD_BUFFER_SIZE(gx3201_audiospdif_reg, 0);

			fifo->channel->incallback = NULL;
			fifo->channel->indata	  = NULL;
		} else if (GXAV_PIN_EAC3 == fifo->pin_id || GXAV_PIN_AAC == fifo->pin_id) {
			AUDIO_SET_SPD_BUFFER_START_ADDR(gx3201_audiospdif_raw_reg, 0);
			AUDIO_SET_SPD_BUFFER_SIZE(gx3201_audiospdif_raw_reg, 0);

			fifo->channel->incallback = NULL;
			fifo->channel->indata	  = NULL;
		} else if (GXAV_PIN_ADPCM == fifo->pin_id) {
			fifo->channel->incallback = NULL;
			fifo->channel->indata	  = NULL;
		} else {
			gxlog_e(LOG_AOUT, "%s %d [pin_id: %d]\n", __func__, __LINE__, fifo->pin_id);
			return -1;
		}
	} else {
		gxlog_e(LOG_AOUT, "%s %d [direction: %d]\n", __func__, __LINE__, fifo->direction);
		return -1;
	}

	return 0;
}

static int gx3201_audioout_stop(struct gxav_audioout *audioout)
{
	if ((audioout->out_state != AUDIOOUT_STOPED) &&
			(audioout->out_state != AUDIOOUT_INITED))
	{
		int length, times;
#define AUDIO_RETRY_COST_DATA_TIMES 10

		audioout->out_state = AUDIOOUT_STOPED;

		gxav_set_i2s_sample_data_len(0, 0xf);
		gxav_set_i2s_sample_data_len(1, 0xf);
		AUDIO_SPD_SAMPLE_DATA_LEN_SEL(gx3201_audiospdif_reg, 0xf);
		gxav_set_i2s_isr_enable(0, 0);
		gxav_set_i2s_isr_enable(1, 0);
		if (SPD_FIX_CHIP) {
			AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_reg,     0);
			AUDIO_SPD_SET_CACHE_FIFO_INT_EN(gx3201_audiospdif_raw_reg, 0);
		} else {
			AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_reg);
			AUDIO_SPDINT_CD_PLAY_DONE_DISABLE(gx3201_audiospdif_raw_reg);
		}

		times = 0;
		length = AUDIO_SPD_GET_DATA_LENTH(gx3201_audiospdif_raw_reg);
		while (length && (times++ < AUDIO_RETRY_COST_DATA_TIMES)) {
			gx_mdelay(10);
			length = AUDIO_SPD_GET_DATA_LENTH(gx3201_audiospdif_raw_reg);
		}

		if (length != 0) {
			gxlog_w(LOG_AOUT, "eac3 data %d not all be used, it will be big-pang\n", length);
		}

		times = 0;
		length = AUDIO_SPD_GET_DATA_LENTH(gx3201_audiospdif_reg);
		while (length && (times++ < AUDIO_RETRY_COST_DATA_TIMES)) {
			gx_mdelay(10);
			length = AUDIO_SPD_GET_DATA_LENTH(gx3201_audiospdif_reg);
		}
		if (length != 0) {
			gxlog_w(LOG_AOUT, "ac3 data %d not all be used, it will be big-pang\n", length);
		}

		times = 0;
		length = gxav_get_i2s_data_len(0);
		while ((length > 0) && (times++ < AUDIO_RETRY_COST_DATA_TIMES)) {
			gx_mdelay(10);
			length = gxav_get_i2s_data_len(0);
		}
		if (length > 0) {
			gxlog_w(LOG_AOUT, "i2s pcm data [%d]not all be used, it will be big-pang\n", length);
		}

		times = 0;
		length = _get_audioout_spd_res_len();
		while ((length > 0) && (times++ < AUDIO_RETRY_COST_DATA_TIMES)) {
			gx_mdelay(10);
			length = _get_audioout_spd_res_len();
		}
		if (length > 0) {
			gxlog_w(LOG_AOUT, "spd pcm data [%d]not all be used, it will be big-pang\n", length);
		}

		// 清相关中断状态
		gxav_clr_i2s_isr_status(0);
		gxav_clr_i2s_isr_status(1);
		if (SPD_FIX_CHIP) {
			AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_reg);
			AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_raw_reg);
		} else {
			AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_reg);
			AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_raw_reg);
		}

		AUDIO_SPD_CLEAR_FIFO_STATUS(gx3201_audiospdif_reg);
		while(AUDIO_SPD_GET_FIFO_STATUS(gx3201_audiospdif_reg) != 0);
		AUDIO_SPD_CLEAR_FIFO_STATUS_CLR(gx3201_audiospdif_reg);

		AUDIO_SPD_RAW_CLEAR_FIFO_STATUS(gx3201_audiospdif_reg);
		AUDIO_SPD_RAW_CLEAR_FIFO_STATUS_CLR(gx3201_audiospdif_reg);

		audioout->i2s_pcm[0].pcm_hungry = 0;
		audioout->i2s_pcm[1].pcm_hungry = 0;
		audioout->i2s_pcm[0].pcm_fifo   = NULL;
		audioout->i2s_pcm[1].pcm_fifo   = NULL;
		audioout->i2s_adpcm.pcm_fifo    = NULL;
		audioout->spd_src2.enc_hungry    = 0;
		audioout->spd_src2.enc_fifo      = NULL;
		audioout->spd_src3.enc_hungry   = 0;
		audioout->spd_src3.enc_fifo     = NULL;

		if (CHIP_IS_GX6605S && NO_MULTI_I2S) {
			AUDIO_SET_SPD_BUFFER_START_ADDR(gx3201_audiospdif_reg, 0);
			AUDIO_SET_SPD_BUFFER_SIZE(gx3201_audiospdif_reg, 0);
			gxav_clock_cold_rst(MODULE_TYPE_AUDIO_SPDIF);
			_recovery_audioout_status(audioout);
			gxav_set_i2s_work_enable(0);
		}
		AUDIO_SET_SPD_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_OFF);
		AUDIO_SET_SPD_RAW_PLAY_MODE(gx3201_audiospdif_reg, SPD_PLAY_OFF);
	}

	gxav_stc_invaild_apts(0);
	_play_zero_data();

	return 0;
}

static int gx3201_audioout_close(struct gxav_audioout *audioout)
{
	gx3201_audioout_stop(audioout);
	return 0;
}

static int gx3201_audioout_uninit(void)
{
	_audioout_iounmap();
	return 0;
}

static int gx3201_audioout_interrupt(struct gxav_audioout *audioout)
{
	unsigned int src3_en = 0;
	unsigned int src2_en = 0;

	if (SPD_FIX_CHIP) {
		unsigned int src3_int_st = AUDIO_SPD_GET_CACHE_FIFO_INT_ST(gx3201_audiospdif_raw_reg);
		unsigned int src3_int_en = AUDIO_SPD_GET_CACHE_FIFO_INT_EN(gx3201_audiospdif_raw_reg);
		unsigned int src2_int_st = AUDIO_SPD_GET_CACHE_FIFO_INT_ST(gx3201_audiospdif_reg);
		unsigned int src2_int_en = AUDIO_SPD_GET_CACHE_FIFO_INT_EN(gx3201_audiospdif_reg);

		src2_en = src2_int_st & src2_int_en;
		src3_en = src3_int_st & src3_int_en;
	} else {
		unsigned int src3_int_st = AUDIO_SPD_GET_INT_STATUS(gx3201_audiospdif_raw_reg);
		unsigned int src3_int_en = AUDIO_SPD_GET_INT_EN_VAL(gx3201_audiospdif_raw_reg);
		unsigned int src2_int_st    = AUDIO_SPD_GET_INT_STATUS(gx3201_audiospdif_reg);
		unsigned int src2_int_en = AUDIO_SPD_GET_INT_EN_VAL(gx3201_audiospdif_reg);

		src2_en = (src2_int_st & src2_int_en & 0x4);
		src3_en = (src3_int_st & src3_int_en & 0x4);
	}

	if ((audioout->out_state != AUDIOOUT_READY) &&
			(audioout->out_state != AUDIOOUT_RUNNING) &&
			(audioout->out_state != AUDIOOUT_PAUSED)) {
		gxav_clr_i2s_isr_status(0);
		gxav_clr_i2s_isr_status(1);
		if (SPD_FIX_CHIP) {
			AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_reg);
			AUDIO_SPD_CLR_CACHE_FIFO_INT_ST(gx3201_audiospdif_raw_reg);
		} else {
			AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_raw_reg);
			AUDIO_SPDINT_CD_PLAY_DONE_CLR(gx3201_audiospdif_reg);
		}
		return 0;
	}

	_check_audioout_hdmi_data(audioout);
	_check_audioout_recovery_src(audioout);

	if (gxav_get_i2s_isr_status(0)) {
		pcm_complete_out_isr(audioout, 0);
	}

	if (gxav_get_i2s_isr_status(1)) {
		pcm_complete_out_isr(audioout, 1);
	}

	if (src3_en) {
		src3_complete_done_isr(audioout);
	}

	if (src2_en) {
		src2_complete_done_isr(audioout);
	}

	return 0;
}

static struct audioout_hal_ops gx3201_audioout_ops = {
	.init          = gx3201_audioout_init,
	.uninit        = gx3201_audioout_uninit,
	.open          = gx3201_audioout_open,
	.close         = gx3201_audioout_close,
	.link          = gx3201_audioout_link_fifo,
	.unlink        = gx3201_audioout_unlink_fifo,
	.config_port   = gx3201_audioout_config_port,
	.config_source = gx3201_audioout_config_source,
	.config_sync   = gx3201_audioout_config_sync,
	.power_mute    = gx3201_audioout_power_mute,
	.run           = gx3201_audioout_start,
	.stop          = gx3201_audioout_stop,
	.pause         = gx3201_audioout_pause,
	.resume        = gx3201_audioout_resume,
	.set_mute      = gx3201_audioout_set_mute,
	.get_mute      = gx3201_audioout_get_mute,
	.set_volume    = gx3201_audioout_set_volume,
	.get_volume    = gx3201_audioout_get_volume,
	.set_channel   = gx3201_audioout_set_channel,
	.get_state     = gx3201_audioout_get_state,
	.mix_pcm       = gx3201_audioout_mix_pcm,
	.set_speed     = gx3201_audioout_set_speed,
	.get_pts       = gx3201_audioout_get_pts,
	.set_ptsoffset = gx3201_audioout_set_ptsoffset,
	.set_event     = gx3201_audioout_set_event,
	.irq           = gx3201_audioout_interrupt
};

struct gxav_module_ops gx3201_audioout_module = {
#define INTC_SOURCE_AUDIO_I2S             (38)
#define INTC_SOURCE_AUDIO_SPDIF           (39)
#define INTC_SOURCE_AUDIO_SPDIF_RAW       (40)
	.module_type = GXAV_MOD_AUDIO_OUT,
	.count = 1,
	.irqs = {INTC_SOURCE_AUDIO_I2S, INTC_SOURCE_AUDIO_SPDIF, INTC_SOURCE_AUDIO_SPDIF_RAW, -1},
	.irq_names = {"aout_srl", "aout_spd", "aout_ac3"},
	.event_mask = 0xffffffff,
	.init = gx_audioout_init,
	.cleanup = gx_audioout_uninit,
	.open = gx_audioout_open,
	.close = gx_audioout_close,
	.set_property = gx_audioout_set_property,
	.get_property = gx_audioout_get_property,
	.interrupts[INTC_SOURCE_AUDIO_I2S] = gx_audioout_i2s_interrupt,
	.interrupts[INTC_SOURCE_AUDIO_SPDIF] = gx_audioout_i2s_interrupt,
	.interrupts[INTC_SOURCE_AUDIO_SPDIF_RAW] = gx_audioout_spdif_interrupt,
	.priv = &gx3201_audioout_ops,
};


