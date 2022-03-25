#include "kernelcalls.h"
#include "gxav_common.h"
#include "include/audio_common.h"
#include "sirius_stream.h"
#include "sirius_debug.h"
#include "sirius_reg.h"
#include "aout/aout.h"
#include "hdmi_hal.h"
#include "clock_hal.h"
#include "sdc_hal.h"
#include "log_printf.h"

static unsigned int i2s_mclk[8] = {256, 384, 512,  768, 1024, 1536, 128, 192};
static unsigned int spd_mclk[5] = {256, 384, 512, 1024, 2048};
static unsigned int volume_table_df[101] = {
	 0,    1,   3,   5,   7,   9,  11,  13,  15,  17,
	 18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
	 28,  29,  30,  31,  32,  33,  34,  35,  36,  37,
	 38,  39,  40,  42,  44,  46,  48,  50,  52,  54,
	 56,  58,  60,  62,  64,  66,  68,  70,  72,  74,
	 76,  78,  80,  82,  84,  88,  90,  92,  94,  96,
	 97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
	107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
	133, 135, 137, 139, 141, 143, 145, 147, 149, 151,
	153, 155, 158, 161, 164, 167, 170, 173, 177, 181,
	181
};

static unsigned int volume_table_db[101] = {
	   0,   1,   1,   1,   1,   2,   2,   2,   2,   3,
	   3,   3,   4,   4,   5,   5,   6,   6,   7,   8,
	   9,  10,  11,  13,  14,  16,  18,  20,  23,  26,
	  29,  32,  36,  40,  45,  51,  57,  64,  72,  81,
	  91, 102, 114, 128, 144, 161, 181, 203, 228, 255,
	 287, 322, 361, 405, 454, 510, 572, 642, 720, 808,
	 906,1017,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024
};

static AoutRegs optReg = {
	.i2sAddr_0 = 0x8AD00000, //sirius
	.i2sAddr_1 = 0x8AD00040,
	.sdcAddr_0 = 0x8AC00000,
	.sdcAddr_1 = 0x8AC80000,
	.spdAddr_0 = 0x8AB00000,
	.spdAddr_1 = 0x8AB00070,
	.runClock  = 1188000000,
	.i2sRegs_0 = NULL,
	.i2sRegs_1 = NULL,
	.sdcRegs_0 = NULL,
	.sdcRegs_1 = NULL,
	.spdRegs_2 = NULL,
	.spdRegs_3 = NULL,
};

static unsigned int debug_aout_sync = DEBUG_AOUT_PTS_SYNC;
static char *sync_string[4] = {"CM", "SK", "RE", "FR"};
static char *body_string[4] = {"R0", "R1", "R2", "R3"};

static char *_body_string(AoutStreamSrc src)
{
	unsigned int i = 0;

	while ((src >> i)) i++;
	i--;

	return body_string[i];
}

static char *_sync_string(AoutStreamSyncState state)
{
	return sync_string[state];
}

static int _ioremap(void)
{
	if (NULL == gx_request_mem_region(optReg.i2sAddr_0, sizeof(AoutI2SRegs))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (NULL == gx_request_mem_region(optReg.i2sAddr_1, sizeof(AoutI2SRegs))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (NULL == gx_request_mem_region(optReg.sdcAddr_0, sizeof(AoutSdcRegs))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (NULL == gx_request_mem_region(optReg.sdcAddr_1, sizeof(AoutSdcRegs))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (NULL == gx_request_mem_region(optReg.spdAddr_0, 0x70)) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (NULL == gx_request_mem_region(optReg.spdAddr_1, sizeof(AoutSpdRegs_1))) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	optReg.i2sRegs_0 = (AoutI2SRegs *)gx_ioremap(optReg.i2sAddr_0, sizeof(AoutI2SRegs));
	if (NULL == optReg.i2sRegs_0) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	optReg.i2sRegs_1 = (AoutI2SRegs *)gx_ioremap(optReg.i2sAddr_1, sizeof(AoutI2SRegs));
	if (NULL == optReg.i2sRegs_1) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	optReg.sdcRegs_0 = (AoutSdcRegs *)gx_ioremap(optReg.sdcAddr_0, sizeof(AoutSdcRegs));
	if (NULL == optReg.sdcRegs_0) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	optReg.sdcRegs_1 = (AoutSdcRegs *)gx_ioremap(optReg.sdcAddr_1, sizeof(AoutSdcRegs));
	if (NULL == optReg.sdcRegs_1) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	optReg.spdRegs_2 = (AoutSpdRegs_0 *)gx_ioremap(optReg.spdAddr_0, sizeof(AoutSpdRegs_0));
	if (NULL == optReg.spdRegs_2) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	optReg.spdRegs_3 = (AoutSpdRegs_1 *)gx_ioremap(optReg.spdAddr_1, sizeof(AoutSpdRegs_1));
	if (NULL == optReg.spdRegs_3) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}

static int _set_cvbs_mute(AoutStreamHeader *header, AoutStreamMuteMap muteMap, int mute)
{
	int cvbsMuteMap = header->cvbsMuteMap;

	if (mute)
		header->cvbsMuteMap |=  (muteMap);
	else
		header->cvbsMuteMap &= ~(muteMap);

	if (cvbsMuteMap && !header->cvbsMuteMap) {
		gxav_clock_audioout_dacinside_mute(0);
	} else if (!cvbsMuteMap && header->cvbsMuteMap) {
		gxav_clock_audioout_dacinside_mute(1);
	}

	return 0;
}

static int _set_hdmi_mute(AoutStreamHeader *header, AoutStreamMuteMap muteMap, int mute)
{
	int hdmiMuteMap = header->hdmiMuteMap;

	if (mute)
		header->hdmiMuteMap |=  (muteMap);
	else
		header->hdmiMuteMap &= ~(muteMap);

	if (hdmiMuteMap && !header->hdmiMuteMap) {
		gxav_hdmi_audioout_mute(0);
	} else if (!hdmiMuteMap && header->hdmiMuteMap) {
		gxav_hdmi_audioout_mute(1);
	}

	return 0;
}

static void _iounmap(void)
{
	gx_iounmap(optReg.i2sRegs_0);
	gx_iounmap(optReg.i2sRegs_1);
	gx_iounmap(optReg.sdcRegs_0);
	gx_iounmap(optReg.sdcRegs_1);
	gx_iounmap(optReg.spdRegs_2);
	gx_iounmap(optReg.spdRegs_3);
	gx_release_mem_region(optReg.i2sAddr_0, sizeof(AoutI2SRegs));
	gx_release_mem_region(optReg.i2sAddr_1, sizeof(AoutI2SRegs));
	gx_release_mem_region(optReg.sdcAddr_0, sizeof(AoutSdcRegs));
	gx_release_mem_region(optReg.sdcAddr_1, sizeof(AoutSdcRegs));
	gx_release_mem_region(optReg.spdAddr_0, sizeof(AoutSpdRegs_0));
	gx_release_mem_region(optReg.spdAddr_1, sizeof(AoutSpdRegs_1));

	optReg.i2sRegs_0 = NULL;
	optReg.i2sRegs_1 = NULL;
	optReg.sdcRegs_0 = NULL;
	optReg.sdcRegs_1 = NULL;
	optReg.spdRegs_2 = NULL;
	optReg.spdRegs_3 = NULL;
}

static AoutStreamSyncState _sync_frame_by_pts(AoutStreamBody *body, int pts)
{
	int stc = 0, offset = 0;
	GxSTCProperty_TimeResolution resolution;
	unsigned int low_tolerance = 0, high_tolerance = 0;
	AoutStreamHeader *header = body->header;

	gxav_stc_get_base_resolution(0, &resolution);

	if (pts == -1) {
		if (body->sync.syncStc == 0)
			body->sync.state = STREAM_PTS_NORMAL;
		else
			body->sync.state = STREAM_PTS_FREERUN;
		goto sync_result;
	}

	if (body->sync.recoveryStc)
		gxav_stc_write_apts(0, pts, 0);

	gxav_stc_read_stc(0, (unsigned int*)&stc);
	if (body->sync.syncStc == 0) {
		body->sync.state = STREAM_PTS_NORMAL;
		goto sync_result;
	}

	if ((stc & 0x1) == 0) {
		body->sync.state = STREAM_PTS_FREERUN;
		goto sync_result;
	}

	pts   += body->sync.delay_ms * (resolution.freq_HZ / 1000);
	offset = (pts - stc) / (resolution.freq_HZ / 1000);
	if (body->src == SRC_R1) {
		low_tolerance  = 30;
		high_tolerance = body->sync.highToleranceMs;
	} else {
		low_tolerance  = body->sync.lowToleranceMs;
		high_tolerance = body->sync.highToleranceMs;
	}

	if (abs(offset) <= low_tolerance)
		body->sync.state = STREAM_PTS_NORMAL;
	else if ((offset >= (0 - high_tolerance)) && (offset < (0 - low_tolerance)))
		body->sync.state = STREAM_PTS_SKIP;
	else if ((offset > low_tolerance) && (offset <= high_tolerance))
		body->sync.state = STREAM_PTS_REPEAT;
	else
		body->sync.state = STREAM_PTS_FREERUN;

sync_result:
	if (body->src == SRC_R1) {
		//SRC_R1数据不够，SRC_R0需要repeat，SRC_R1需要skip,直至SRC_R1不发生skip
		//避免SRC_R1经常处于skip状态
		if (body->sync.state == STREAM_PTS_SKIP)
			header->src1Skip = 1;
		else
			header->src1Skip = 0;
	} else {
		if (header->src1Skip)
			body->sync.state = STREAM_PTS_REPEAT;
	}

	{
		PTS_SYNC_LOG("%s - [%s]: pts.%u stc.%u offset.%3d low.%d, high.%d (sk: %d)\n",
				_body_string(body->src), _sync_string(body->sync.state),
				(unsigned int)(pts / (resolution.freq_HZ / 1000)),
				(unsigned int)(stc / (resolution.freq_HZ / 1000)),
				offset, low_tolerance, high_tolerance, header->src1Skip);
	}

	if (body->sync.state != STREAM_PTS_SKIP)
		body->frame.playFrameCnt++;
	else
		body->frame.loseFrameCnt++;
	body->sync.pts = pts;

	return body->sync.state;
}

static unsigned int _check_sample_freq(unsigned int samplefre)
{
	switch (samplefre) {
	case 16000:
	case 22050:
	case 24000:
	case 32000:
	case 44100:
	case 48000:
	case 64000:
	case 88200:
	case 96000:
	case 128000:
	case 176400:
	case 192000:
		break;
	default:
		gxlog_e(LOG_AOUT, "samplefre (%d)!!!\n",samplefre);
		samplefre = 48000;
		break;
	}

	return samplefre;
}

static int _check_frame_over(AoutStreamBody *body, unsigned int *last_r_addr)
{
	unsigned int frame_s_addr = aout_src_get_frame_start_addr       (&optReg, body->src);
	unsigned int frame_e_addr = aout_src_get_frame_end_addr         (&optReg, body->src);
	unsigned int frame_r_addr = aout_src_get_playing_frame_read_addr(&optReg, body->src);
	unsigned int res = ((frame_e_addr + 1) & 0x3) ? (4 - ((frame_e_addr + 1) & 0x3)) : 0;

	//硬件bug: buf回头时,读指针跑到下个buf位置
	//例如: (s: 0xfc00 e: 0x6ff, r: 0x10020, t: 0x10000)
	if ((*last_r_addr == frame_r_addr) && (frame_r_addr > body->buf.bufferSize))
		return 1;

	*last_r_addr = frame_r_addr;
	//res: 硬件读指针以4bytes对齐,因此R2/R3时,读指针超过frame_e_addr
	frame_r_addr = (body->buf.bufferSize + frame_r_addr - res - 1) % body->buf.bufferSize;

	return (((frame_s_addr == frame_e_addr) || (frame_r_addr == frame_e_addr)) ? 1 : 0);
}

static int _wait_frame_over(AoutStreamBody *body)
{
	unsigned int last_r_addr = (unsigned int)(-1);
	int retry = 10;
	int play_frame_over = _check_frame_over(body, &last_r_addr);

	while (!play_frame_over && retry) {
		play_frame_over = _check_frame_over(body, &last_r_addr);
		gx_mdelay(10);
		retry--;
	}

	if (retry == 0) {
		unsigned int frame_s_addr = 0;
		unsigned int frame_e_addr = 0;
		unsigned int frame_r_addr = 0;

		frame_s_addr = aout_src_get_frame_start_addr(&optReg, body->src);
		frame_e_addr = aout_src_get_frame_end_addr(&optReg, body->src);
		frame_r_addr = aout_src_get_playing_frame_read_addr(&optReg, body->src);
		gxlog_w(LOG_AOUT, "[%s] (s: 0x%x e: 0x%x, r: 0x%x, t: 0x%x)\n",
				_body_string(body->src), frame_s_addr, frame_e_addr, frame_r_addr, body->buf.bufferSize);
	}

	return 0;
}

static int _wait_all_frame_over(AoutStreamHeader *header)
{
	int ret = 0;

	aout_clr_request_reset_irq_state(&optReg);
	aout_enable_request_reset_irq   (&optReg, 0);
	aout_enable_request_reset       (&optReg, 1);
	while (!aout_get_complete_reset_irq_state(&optReg)) {
		gx_mdelay(10);
	}
	aout_enable_request_reset       (&optReg, 0);
	aout_clr_request_reset_irq_state(&optReg);

	return ret;
}

static int _post_all_frame_over(AoutStreamHeader *header)
{
	aout_enable_request_reset    (&optReg, 0);
	aout_enable_request_reset_irq(&optReg, 0);
	gx_sem_post(&header->semId);

	return 0;
}

static void _set_spd_clock_fre(AoutStreamSrc src, unsigned int samplefre)
{
	unsigned long long audio_clock = 0;
	unsigned int mclk_value;
	clock_params params;
	AoutStreamMix mix = ((src == SRC_R3) ? MIX_3_SPD : MIX_2_SPD);

	mclk_value  = spd_mclk[aout_spd_get_mclk(&optReg, mix)];
	audio_clock = samplefre * mclk_value;
	audio_clock = audio_clock * (1<<30);
	do_div(audio_clock, optReg.runClock);

	params.audio_spdif.clock_source = SPDIF_DTO;
	params.audio_spdif.value        = audio_clock;
	gxav_clock_setclock     (MODULE_TYPE_AUDIO_SPDIF, &params);
	gxav_clock_source_enable(MODULE_TYPE_AUDIO_SPDIF);

	gxlog_d(LOG_AOUT, "[%s] spd dto: %d*%d clk %lld\n",
			_body_string(src), mclk_value, samplefre, audio_clock);
	return;
}

static void _set_spd_samplefre(AoutStreamSrc src, unsigned int samplefre)
{
	GxAudioSampleFre samplefreIndex = gxav_audioout_samplefre_index(samplefre);
	unsigned int i2s_spd_cs_byte0 = 0x0004;
	unsigned int i2s_spd_cs1_byte1[12] ={
		0x0010/*44.1 khz*/, 0x0210/*48   khz*/, 0x0310/*32  khz*/, 0x0410/*22.05 khz*/, 0x0610/*24  khz*/, 0x0110/*16 khz*/,
		0x0a10/*96   khz*/, 0x0810/*88.2 khz*/, 0x8b10/*128 khz*/, 0x0c10/*176.4 khz*/, 0x0e10/*192 khz*/, 0x0b10/*64 khz*/
	};
	unsigned int i2s_spd_cs2_byte1[12] = {
		0x0020/*44.1 khz*/, 0x0220/*48   khz*/, 0x0320/*32  khz*/, 0x0420/*22.05 khz*/, 0x0620/*24  khz*/, 0x0120/*16 khz*/,
		0x0a20/*96   khz*/, 0x0820/*88.2 khz*/, 0x8b20/*128 khz*/, 0x0c20/*176.4 khz*/, 0x0e20/*192 khz*/, 0x0110/*64 khz*/
	};
	unsigned int i2s_spd_cs_byte2[12][2] = {
		{0x00bb, 0x00fb} /*44.1  khz*/,
		{0x009b, 0x00db} /*48    khz*/,
		{0x004b, 0x00cb} /*32    khz*/,
		{0x00ab, 0x00bb} /*22.05 khz*/,
		{0x002b, 0x009b} /*24    khz*/,
		{0x006b, 0x008b} /*16    khz*/,
		{0x00db, 0x005b} /*96    khz*/,
		{0x00fb, 0x007b} /*88.2  khz*/,
		{0x004b, 0x00eb} /*128   khz*/,
		{0x007b, 0x003b} /*176.4 khz*/,
		{0x005b, 0x001b} /*192   khz*/,
		{0x00cb, 0x004b} /*64    khz*/
	};

	unsigned int src_r2_spd_cs_byte0 = 0x0006;
	unsigned int src_r2_spd_cs_byte1[12] = {
		0x0000, 0x0200, 0x0300, 0x0400, 0x0600, 0x0100, 0x0a00, 0x0800, 0x8b00, 0x0c00, 0x0e00, 0x0b00
	};

	unsigned int src_r3_spd_cs_byte0 = 0x0006;
	unsigned int src_r3_spd_cs_byte1[12] = {
		0x0000, 0x0200, 0x0300, 0x0400, 0x0600, 0x0100, 0x0a00, 0x0800, 0x8b00, 0x0c00, 0x0e00, 0x0100
	};

	switch (src) {
	case SRC_R0:
	case SRC_R1:
		aout_spd_set_cl1(&optReg, MIX_2_SPD, ((i2s_spd_cs1_byte1[samplefreIndex] << 16) | i2s_spd_cs_byte0));
		aout_spd_set_cr1(&optReg, MIX_2_SPD, ((i2s_spd_cs2_byte1[samplefreIndex] << 16) | i2s_spd_cs_byte0));
		aout_spd_set_cl2(&optReg, MIX_2_SPD, (i2s_spd_cs_byte2[samplefreIndex][1]));
		aout_spd_set_cr2(&optReg, MIX_2_SPD, (i2s_spd_cs_byte2[samplefreIndex][1]));
		break;
	case SRC_R2:
		aout_spd_set_cl1(&optReg, MIX_2_SPD, ((src_r2_spd_cs_byte1[samplefreIndex] << 16) | src_r2_spd_cs_byte0));
		aout_spd_set_cr1(&optReg, MIX_2_SPD, ((src_r2_spd_cs_byte1[samplefreIndex] << 16) | src_r2_spd_cs_byte0));
		aout_spd_set_cl2(&optReg, MIX_2_SPD, 0);
		aout_spd_set_cr2(&optReg, MIX_2_SPD, 0);
		break;
	case SRC_R3:
		aout_spd_set_cl1(&optReg, MIX_3_SPD, ((src_r3_spd_cs_byte1[samplefreIndex] << 16) | src_r3_spd_cs_byte0));
		aout_spd_set_cr1(&optReg, MIX_3_SPD, ((src_r3_spd_cs_byte1[samplefreIndex] << 16) | src_r3_spd_cs_byte0));
		aout_spd_set_cl2(&optReg, MIX_3_SPD, 0);
		aout_spd_set_cr2(&optReg, MIX_3_SPD, 0);
		break;
	default:
		break;
	}
}

static int _set_spd_config(AoutStreamHeader *header, AoutStreamBody *body, AoutStreamSample *sample)
{
	if ((body->src == SRC_R0) || (body->src == SRC_R1)) {
		if (header->spd2Source == MIX_2_SPD_SEL_I2S) {
			header->spd2SampleFre =  sample->freq;
			_set_spd_clock_fre(body->src, sample->freq);
			_set_spd_samplefre(body->src, sample->freq);
		}
	} else if (body->src == SRC_R2) {
		if (header->spd2Source == MIX_2_SPD_SEL_SRC_R2) {
			//SRC_R2: normal freq 32kHz ~ 48KHz
			AoutStreamCddEndian endian = sample->endian ? CDD_ENDIAN_0 : CDD_ENDIAN_1;

			header->spd2SampleFre =  sample->freq;
			aout_src_set_cdd_endian(&optReg, body->src, endian);

			_set_spd_clock_fre(body->src, sample->freq);
			_set_spd_samplefre(body->src, sample->freq);
		}
	} else if (body->src == SRC_R3) {
		//SRC_R2: normal freq 32kHz ~ 192KHz, 128KHz ~ 192KHz, FS must be 4 * SRC_R2
		AoutStreamCddEndian endian = sample->endian ? CDD_ENDIAN_0 : CDD_ENDIAN_1;

		if ((sample->freq >= 128000) && (header->hdmiSelSrc != SRC_R2)){
			aout_spd_set_mclk(&optReg, MIX_2_SPD, SPD_MCLK_2048FS);
			_set_spd_clock_fre(SRC_R0, sample->freq / 4);
			_set_spd_samplefre(SRC_R0, sample->freq / 4);
			_set_spd_clock_fre(SRC_R1, sample->freq / 4);
			_set_spd_samplefre(SRC_R1, sample->freq / 4);
		}

		header->spd3SampleFre =  sample->freq;
		aout_src_set_cdd_endian(&optReg, body->src, endian);

		_set_spd_clock_fre(body->src, sample->freq);
		_set_spd_samplefre(body->src, sample->freq);
	}

	return 0;
}

static void _set_i2s_clock_fre(AoutStreamSrc src, unsigned int samplefre)
{
	unsigned long long audio_clock = 0;
	unsigned int mclk_value;
	clock_params params;

	mclk_value  = i2s_mclk[aout_dac_get_mclk(&optReg)];
	audio_clock = samplefre * mclk_value;
	audio_clock = audio_clock * (1<<30);
	do_div(audio_clock, optReg.runClock);


	params.audio_i2s.clock_source = I2S_DTO;
	params.audio_i2s.value        = audio_clock;
	gxav_clock_setclock     (MODULE_TYPE_AUDIO_I2S, &params);
	gxav_clock_source_enable(MODULE_TYPE_AUDIO_I2S);

	gxlog_d(LOG_AOUT, "[%s] i2s dto: %d*%d clk %lld\n",
			_body_string(src), mclk_value, samplefre, audio_clock);
	return;
}

static int _set_i2s_config(AoutStreamHeader *header, AoutStreamBody *body, AoutStreamSample *sample)
{
	if ((body->src == SRC_R0) || (body->src == SRC_R1)) {
		AoutStreamPcmEndian    endian = sample->endian ? PCM_ENDIAN_0 : PCM_ENDIAN_3;
		AoutStreamPcmChNum     chnum  = sample->channels - 1;
		AoutStreamPcmInterlace interlace = sample->interlace ? PCM_INTERLACE : PCM_NO_INTERLACE;

		header->i2sSampleFre = sample->freq;
		aout_src_set_pcm_channelnum(&optReg, body->src, chnum);
		aout_src_set_pcm_endian    (&optReg, body->src, endian);
		aout_src_set_pcm_interlace (&optReg, body->src, interlace);
		_set_i2s_clock_fre(body->src, sample->freq);
	}

	return 0;
}

static int _set_dac_config(AoutStreamHeader *header)
{
	aout_dac_set_mclk  (&optReg, DAC_MCLK_1024FS);
	aout_dac_set_bclk  (&optReg, DAC_BCLK_64FS);
	aout_dac_set_format(&optReg, DAC_FORMAT_I2S);
	aout_src_set_pcm_output_bits (&optReg, (SRC_R0|SRC_R1), PCM_OUTPUT_24_BITS);
	gxav_clock_audioout_dacinside(i2s_mclk[DAC_MCLK_1024FS]);
	if (header->cvbsMuteMap)
		gxav_clock_audioout_dacinside_mute(1);
	else
		gxav_clock_audioout_dacinside_mute(0);

	return 0;
}

static int _set_src_volume(AoutStreamHeader *header, AoutStreamBody *body)
{
	if (header->targetVol.rightNow) {
		if (body->processVol != header->targetVol.vol) {
			body->processVol = header->targetVol.vol;
			aout_src_set_volume(&optReg, body->src, header->vol_table[body->processVol]);
		}
	} else {
		if (body->processVol < header->targetVol.vol) {
			if ((body->processVol + header->targetVol.step) < header->targetVol.vol)
				body->processVol += header->targetVol.step;
			else
				body->processVol = header->targetVol.vol;
			aout_src_set_volume(&optReg, body->src, header->vol_table[body->processVol]);
		} else if (body->processVol > header->targetVol.vol) {
			if (body->processVol > (header->targetVol.vol + header->targetVol.step))
				body->processVol-= header->targetVol.step;
			else
				body->processVol = header->targetVol.vol;
			aout_src_set_volume(&optReg, body->src, header->vol_table[body->processVol]);
		}
	}

	return 0;
}

static int _set_port_config(AoutStreamHeader *header,
		AoutStreamBody *body,
		AoutStreamSample *sample,
		unsigned int change_flag)
{
	if ((body->src == SRC_R0) || (body->src == SRC_R1)) {
		if (header->hdmiSelChange || change_flag) {
			if (header->hdmiSelSrc & (SRC_R0 | SRC_R1)) {
				aout_port_select_mix  (&optReg, HDMI_PORT, MIX_0_I2S);
				_set_hdmi_mute(header, VIRT_MUTE_0, 1);
				gxav_hdmi_audioout_set (0);
				gxav_hdmi_audiosample_change(gxav_audioout_samplefre_index(sample->freq), sample->channels);
				header->hdmiSelChange  = 0;
				header->hdmiMuteFrames = 0;
				header->hdmiMuteDo     = 1;
			}
		}
	} else if ((body->src == SRC_R2) || (body->src == SRC_R3)) {
		if (header->spdifSelChange || change_flag) {
			if ((body->src == SRC_R2) && (header->spdifSelSrc == SRC_R2)) {
				aout_port_select_mix(&optReg, SPDIF_PORT, MIX_2_SPD);
				header->spdifSelChange = 0;
			} else if ((body->src == SRC_R3) && (header->spdifSelSrc == SRC_R3)) {
				aout_port_select_mix(&optReg, SPDIF_PORT, MIX_3_SPD);
				header->spdifSelChange = 0;
			}
		}

		if (header->hdmiSelChange || change_flag) {
			if ((body->src == SRC_R2) && (header->hdmiSelSrc == SRC_R2)) {
				aout_port_select_mix  (&optReg, HDMI_PORT, MIX_2_SPD);
				_set_hdmi_mute(header, VIRT_MUTE_0, 1);
				gxav_hdmi_audioout_set (1);
				gxav_hdmi_audiosample_change(gxav_audioout_samplefre_index(sample->freq), 2);
				header->hdmiSelChange  = 0;
				header->hdmiMuteFrames = 0;
				header->hdmiMuteDo     = 1;
			} else if ((body->src == SRC_R3) && (header->hdmiSelSrc == SRC_R3)) {
				aout_port_select_mix  (&optReg, HDMI_PORT, MIX_3_SPD);
				_set_hdmi_mute(header, VIRT_MUTE_0, 1);
				gxav_hdmi_audioout_set (1);
				gxav_hdmi_audiosample_change(gxav_audioout_samplefre_index(sample->freq), 2);
				header->hdmiSelChange  = 0;
				header->hdmiMuteFrames = 0;
				header->hdmiMuteDo     = 1;
			}
		}
	}

	return 0;
}

static int _play_empty_frame(AoutStreamHeader *header)
{
	aout_src_set_pcm_source         (&optReg, SRC_R0|SRC_R1, PCM_SOURCE_SDRAM);
	aout_src_set_playback_mode      (&optReg, SRC_R0|SRC_R1, 0x1);
	aout_src_enable_playback        (&optReg, SRC_R0|SRC_R1, 0x1);
	aout_mix_enable_stop_mute       (&optReg, MIX_0_I2S|MIX_1_I2S, 0);
	aout_src_start_config           (&optReg, SRC_R0|SRC_R1);
	aout_src_set_pcm_bits           (&optReg, SRC_R0|SRC_R1, 0xF);
	aout_src_set_frame_sample_points(&optReg, SRC_R0|SRC_R1, 0);
	aout_src_set_frame_start_addr   (&optReg, SRC_R0|SRC_R1, 0);
	aout_src_set_frame_end_addr     (&optReg, SRC_R0|SRC_R1, 0);
	aout_src_over_config            (&optReg, SRC_R0|SRC_R1);

	return 0;
}


static int _check_frame_node(AoutStreamBody *body, GxAudioFrameNode* node)
{
	unsigned int change_flag = 0;
	unsigned int upsample_enable = 0;
	AoutStreamSample sample;

	node->sample_fre = _check_sample_freq(node->sample_fre);
	sample.freq      = node->sample_fre;
	sample.channels  = node->channel_num;
	sample.interlace = node->interlace;
	sample.endian    = node->endian;
	if ((body->src == SRC_R0) || (body->src == SRC_R1)) {
		if (node->sample_fre < 32000) {
			sample.freq *= 2;
			upsample_enable = 1;
		} else
			upsample_enable = 0;
	}

	change_flag |= ((body->sample.freq     != node->sample_fre ) ? 1 : 0);
	change_flag |= ((body->sample.channels != node->channel_num) ? 1 : 0);
	if (change_flag) {
		if ((body->src == SRC_R0) || (body->src == SRC_R1)) {
			if (upsample_enable)
				aout_src_enable_pcm_upsample(&optReg, body->src, 1);
			else
				aout_src_enable_pcm_upsample(&optReg, body->src, 0);
		}

		_set_i2s_config((AoutStreamHeader *)body->header, body, &sample);
		_set_spd_config((AoutStreamHeader *)body->header, body, &sample);

		body->sample.freq      = node->sample_fre;
		body->sample.channels  = node->channel_num;
		body->sample.bits      = 32;
		body->sample.interlace = node->interlace;
		body->sample.endian    = node->endian;
	}

	_set_port_config((AoutStreamHeader *)body->header, body, &sample, change_flag);
	_set_src_volume ((AoutStreamHeader *)body->header, body);

	return 0;
}

static int _get_frame_node(AoutStreamBody *body, GxAudioFrameNode *node)
{
	if (node->vaild == 0) {
		gxfifo_get(body->buf.nodeFifo, node, sizeof(GxAudioFrameNode));
		gxlog_d(LOG_AOUT, "get : (%s) %d, %x %x\n",
				_body_string(body->src),
				gxfifo_len((body->buf.nodeFifo)) / sizeof(GxAudioFrameNode),
				node->frame_s_addr, node->frame_e_addr);
		node->vaild = 1;
	}

	return 0;
}

static int _free_frame_node(AoutStreamBody *body, GxAudioFrameNode *node)
{
	if (node->vaild == 1) {
		int len = (node->frame_e_addr + body->buf.bufferSize - node->frame_s_addr + 1) % body->buf.bufferSize;
		gxlog_d(LOG_AOUT, "free: (%s) %d %x %x\n",
				_body_string(body->src),
				gxfifo_len((body->buf.nodeFifo)) / sizeof(GxAudioFrameNode),
				node->frame_s_addr, node->frame_e_addr);
		gxav_sdc_length_set(body->buf.bufferId, len, GXAV_SDC_READ);
		node->vaild = 0;
	}

	return 0;
}

static int _set_play_frame_size(AoutStreamBody *body, GxAudioFrameNode *node, unsigned int playZero)
{
	if ((body->src == SRC_R2) || (body->src == SRC_R3)) {
		unsigned int codeLength = playZero ? 0x0 : node->frame_size;

		if (node->frame_type == AUDIO_TYPE_EAC3) {
			unsigned int pc_pd = (((((node->frame_bsmod & 0x7) << 8) | 0x15) << 16) | codeLength);

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 6144);
		} else if (node->frame_type == AUDIO_TYPE_DTS) {
			unsigned int pc_pd = (codeLength << 3);

			if (node->frame_bytes < 1024)
				pc_pd |= (0x0b << 16);
			else if (node->frame_bytes < 2048)
				pc_pd |= (0x0c << 16);
			else if (node->frame_bytes < 4096)
				pc_pd |= (0x0d << 16);
			else
				pc_pd |= (0x11 << 16);

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, node->frame_bytes);//fix hardware bug
			aout_spd_set_pause_pc_pd(&optReg, body->src, ((0x3 << 16) | (codeLength << 3)));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, node->frame_bytes);
		} else if (node->frame_type == AUDIO_TYPE_ADTS_AAC_LC) {
			unsigned int pc_pd = ((((1 << 8) | (0 << 5) | 0x14) << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 1024);
		} else if (node->frame_type == AUDIO_TYPE_ADTS_HE_AAC) {
			unsigned int pc_pd = ((((4 << 8) | (1 << 5) | 0x14) << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 2048);
		} else if (node->frame_type == AUDIO_TYPE_ADTS_HE_AAC_V2) {
			unsigned int pc_pd = ((((4 << 8) | (1 << 5) | 0x14) << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 2048);
		} else if (node->frame_type == AUDIO_TYPE_LATM_AAC_LC) {
			unsigned int pc_pd = ((((1 << 5) | 0x17) << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 1024);
		} else if (node->frame_type == AUDIO_TYPE_LATM_HE_AAC) {
			unsigned int pc_pd = ((((2 << 5) | 0x17) << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 2048);
		} else if (node->frame_type == AUDIO_TYPE_LATM_HE_AAC_V2) {
			unsigned int pc_pd = ((((1 << 9) | (2 << 5) | 0x17) << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 2048);
		} else {
			unsigned int pc_pd = ((0x1 << 16) | (codeLength << 3));

			aout_spd_set_pause_num  (&optReg, body->src, 0x0);
			aout_spd_set_pause_len  (&optReg, body->src, 0x60);
			aout_spd_set_pause_pc_pd(&optReg, body->src, (0x3 << 16));
			aout_spd_set_pa_pb      (&optReg, body->src, 0xf8724e1f);
			aout_spd_set_pc_pd      (&optReg, body->src, pc_pd);
			aout_src_set_frame_sample_points(&optReg, body->src, 1536);
		}
		aout_spd_set_data_len(&optReg, body->src, node->frame_size);
	} else if ((body->src == SRC_R0) || (body->src == SRC_R1)) {
		aout_src_set_pcm_bits           (&optReg, body->src, (playZero ? 0xF : node->sample_data_len));
		aout_src_set_frame_sample_points(&optReg, body->src, node->frame_size);
	}

	return 0;
}

static int _play_frame_node(AoutStreamBody *body, GxAudioFrameNode *node, unsigned int playZero)
{
	AoutStreamHeader *header = body->header;

	if (header->hdmiMuteDo) {
		if (header->hdmiMuteFrames > 5) {
			_set_hdmi_mute(header, VIRT_MUTE_0, 0);
			header->hdmiMuteDo = 0;
		}
		header->hdmiMuteFrames++;
	}
	gxlog_d(LOG_AOUT, "play: (%s) %x %x\n",
			_body_string(body->src), node->frame_s_addr, node->frame_e_addr);
	aout_src_start_config        (&optReg, body->src);
	_set_play_frame_size         (body, node, playZero);
	aout_src_set_frame_start_addr(&optReg, body->src, node->frame_s_addr);
	aout_src_set_frame_end_addr  (&optReg, body->src, node->frame_e_addr);
	aout_src_over_config         (&optReg, body->src);

	return 0;
}

static int _start_frame_node(AoutStreamBody *body)
{
#define FIRST_FREE_FRAME_COUNT (10)
	unsigned int frame_node_count = 0;

	if (body->overFrame)
		return 0;

get_node:
	if (body->buf.nodeFifo) {
		frame_node_count = gxfifo_len((body->buf.nodeFifo)) / sizeof(GxAudioFrameNode);
		if (frame_node_count > 0) {
			GxAudioFrameNode playNode;
			unsigned int sdcDataLen = 0, nodeDataLen = 0;

			gxfifo_peek(body->buf.nodeFifo, &playNode, sizeof(GxAudioFrameNode), 0);
			if (playNode.frame_e_addr > playNode.frame_s_addr)
				nodeDataLen = (playNode.frame_e_addr + 1 - playNode.frame_s_addr);
			else
				nodeDataLen = (body->buf.bufferSize + playNode.frame_e_addr + 1 - playNode.frame_s_addr);
			gxav_sdc_length_get(body->buf.bufferId, &sdcDataLen);
			if (nodeDataLen > sdcDataLen) {
				gxlog_w(LOG_AOUT, "[%s] (node DataLen: 0x%x [%d %x %x], sdc DataLen: 0x%x)\n",
						_body_string(body->src), nodeDataLen, frame_node_count,
						playNode.frame_s_addr, playNode.frame_e_addr, sdcDataLen);
				frame_node_count = 0;
			}
		}
	}

	if ((frame_node_count > 0) || body->playRepeat) {
		AoutStreamSyncState sync_state;

		body->hungry = 0;
		if (!body->playRepeat) {
			_get_frame_node  (body, &body->playNode);
			_check_frame_node(body, &body->playNode);
		}
		sync_state = _sync_frame_by_pts(body, body->playNode.pts);

		switch(sync_state) {
		case STREAM_PTS_FREERUN:
			{
				unsigned int playZero = 0;

				if (body->firstFreeFrames < FIRST_FREE_FRAME_COUNT) {
					body->firstFreeFrames += 1;
					playZero               = 1;
				}
				body->playRepeat = 0;
				_play_frame_node(body, &body->playNode, playZero);
			}
			break;
		case STREAM_PTS_NORMAL:
			body->playRepeat      = 0;
			body->firstFreeFrames = FIRST_FREE_FRAME_COUNT;
			_play_frame_node(body, &body->playNode, 0);
			break;
		case STREAM_PTS_REPEAT:
			body->playRepeat      = 1;
			body->firstFreeFrames = FIRST_FREE_FRAME_COUNT;
			_play_frame_node(body, &body->playNode, 1);
			break;
		case STREAM_PTS_SKIP:
			body->playRepeat      = 0;
			body->firstFreeFrames = FIRST_FREE_FRAME_COUNT;
			_free_frame_node(body, &body->playNode);
			goto get_node;
		}
	} else{
		body->hungry = 1;
	}

	return 0;
}

static int _play_frame_isr(AoutStreamBody *body)
{
	if (!body->playRepeat)
		_free_frame_node(body, &body->playNode);

	_start_frame_node(body);

	return 0;
}

AoutStreamHeader* stream_init(void)
{
	AoutStreamHeader *header = gx_mallocz(sizeof(AoutStreamHeader));

	if (_ioremap() < 0)
		return NULL;

	aout_config_pcm_slow_vol (&optReg, SLOW_VOL_4, SLOW_POINTS_16);
	aout_enable_pcm_slow_vol (&optReg, 1);
	aout_config_cddate_maxnum(&optReg, 1);
	aout_enable_cddata_cache (&optReg, 1);

	aout_mix_set_l_channel  (&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_0);
	aout_mix_set_r_channel  (&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_1);
	aout_mix_set_c_channel  (&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_2);
	aout_mix_set_ls1_channel(&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_3);
	aout_mix_set_rs1_channel(&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_4);
	aout_mix_set_ls2_channel(&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_5);
	aout_mix_set_rs2_channel(&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_6);
	aout_mix_set_cs_channel (&optReg, MIX_0_I2S|MIX_1_I2S, SRC_CHANNEL_7);
	aout_mix_set_l_channel  (&optReg, MIX_2_SPD,           SRC_CHANNEL_0);
	aout_mix_set_r_channel  (&optReg, MIX_2_SPD,           SRC_CHANNEL_1);

	aout_mix_enable_mute      (&optReg, MIX_0_I2S|MIX_1_I2S|MIX_2_SPD|MIX_3_SPD, 0);
	aout_mix_enable_stop_mute (&optReg, MIX_0_I2S|MIX_1_I2S|MIX_2_SPD|MIX_3_SPD, 0);
	aout_src_enable_reset     (&optReg, SRC_R0|SRC_R1|SRC_R2|SRC_R3, 0);
	aout_src_enable_silent_end(&optReg, SRC_R0|SRC_R1|SRC_R2|SRC_R3, 0);

	gxav_clock_audioout_dacinside_slow_enable(1, 0);
	gxav_clock_audioout_dacinside_slow_config(0x1, 0x40);
	_set_dac_config(header);
	gxav_clock_audioout_dacinside_slow_enable(1, 1);
	gx_mdelay(50);

	gxav_clock_audioout_dacinside_slow_enable(0, 0);
	gxav_clock_audioout_dacinside_slow_config(1, 0xf);
	gxav_clock_audioout_dacinside_slow_enable(0, 0);

	header->targetVol.vol      = 50;
	header->targetVol.rightNow = 0;
	header->targetVol.step     = 1;
	header->mute_status        = 0;
	header->i2sSource          = MIX_0_1_I2S_SEL_SRC_R0_R1;
	header->hdmiSelChange      = 0;
	header->src1Skip           = 0;
	header->hdmiSampleFre      = 48000;
	header->hdmiChannels       = 2;
	header->vol_table          = volume_table_df;

	return header;
}

void stream_uninit(AoutStreamHeader* header)
{
	_iounmap();
	gx_free(header);

	return;
}

AoutStreamBody *stream_open(AoutStreamSrc src)
{
	AoutStreamBody *body = gx_mallocz(sizeof(AoutStreamBody));

	if (NULL == body) {
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return NULL;
	}

	body->src     = src;
	body->sync.syncStc         = 1;
	body->sync.lowToleranceMs  = 100;
	body->sync.highToleranceMs = 4000;
	body->sync.state           = STREAM_PTS_NORMAL;
	body->sync.delay_ms        = 0;
	body->sync.pts             = 0;
	body->sync.recoveryStc     = 0;

	aout_src_enable_volume    (&optReg, body->src, 1);
	aout_src_set_pcm_mage     (&optReg, body->src, PCM_MAGE_NORMAL);

	return body;
}

void stream_close(AoutStreamBody *body)
{
	aout_src_enable_volume    (&optReg, body->src, 0);
	gx_free(body);

	return;
}

int stream_config_buffer(AoutStreamBody *body, AoutStreamBuffer *buf)
{
	aout_src_set_buffer_start_addr(&optReg, body->src, buf->bufferAddr);
	aout_src_set_buffer_size      (&optReg, body->src, buf->bufferSize);
	gx_memcpy(&body->buf, buf, sizeof(AoutStreamBuffer));

	return 0;
}

int stream_set_sync(AoutStreamBody *body, AoutStreamSync *sync)
{
	body->sync.syncStc = sync->syncStc;
	body->sync.lowToleranceMs  = sync->lowToleranceMs;
	body->sync.highToleranceMs = sync->highToleranceMs;

	return 0 ;
}

int stream_set_delay(AoutStreamBody *body, AoutStreamSync *sync)
{
	body->sync.delay_ms = sync->delay_ms;

	return 0;
}

int stream_set_recovery(AoutStreamBody *body, AoutStreamSync *sync)
{
	body->sync.recoveryStc = sync->recoveryStc;

	return 0;
}

int stream_get_recovery(AoutStreamBody *body, AoutStreamSync *sync)
{
	sync->recoveryStc = body->sync.recoveryStc;

	return 0;
}

int stream_get_pts(AoutStreamBody *body, AoutStreamSync *sync)
{
	sync->pts = body->sync.pts;

	return 0;
}

int stream_get_frame(AoutStreamBody *body, AoutStreamFrame *frame)
{
	frame->playFrameCnt = body->frame.playFrameCnt;
	frame->loseFrameCnt = body->frame.loseFrameCnt;

	return 0;
}

int stream_start(AoutStreamBody *body)
{
	AoutStreamHeader *header = body->header;

	body->hungry             = 0;
	body->overFrame          = 0;
	body->pauseFrame         = 0;
	body->playRepeat         = 0;
	body->firstFreeFrames    = 0;
	body->processVol         = 0;
	body->frame.playFrameCnt = 0;
	body->frame.loseFrameCnt = 0;

	gx_memset(&body->sample,    0, sizeof(AoutStreamSample));
	gx_memset(&body->playNode,  0, sizeof(GxAudioFrameNode));
	gx_sem_create(&body->semId, 0);

	aout_src_set_volume             (&optReg, body->src, header->vol_table[0]);
	aout_src_set_pcm_source         (&optReg, body->src, PCM_SOURCE_SDRAM);
	aout_src_set_playback_mode      (&optReg, body->src, 0x1);
	aout_src_enable_playback        (&optReg, body->src, 0x1);
	aout_src_clr_new_frame_irq_state(&optReg, body->src);
	aout_src_forbiden_request_sdc   (&optReg, body->src, 0x0);
	aout_src_enable_new_frame_irq   (&optReg, body->src, 0x1);

	_start_frame_node(body);

	return 0;
}

int stream_stop(AoutStreamBody *body)
{
	AoutStreamHeader *header = body->header;

	body->overFrame  = 1;
	body->pauseFrame = 1;
	_wait_frame_over(body);

	if (header->hdmiMuteDo) {
		_set_hdmi_mute(header, VIRT_MUTE_0, 0);
		header->hdmiMuteDo     = 0;
		header->hdmiMuteFrames = 0;
	}
	aout_src_enable_new_frame_irq   (&optReg, body->src, 0x0);
	aout_src_clr_new_frame_irq_state(&optReg, body->src);
	gx_sem_delete(&body->semId);

	return 0;
}

int stream_pause(AoutStreamBody *body)
{
	body->overFrame  = 1;
	body->pauseFrame = 1;
	_wait_frame_over(body);
	aout_src_enable_new_frame_irq   (&optReg, body->src, 0x0);
	aout_src_clr_new_frame_irq_state(&optReg, body->src);

	return 0;
}

int stream_resume(AoutStreamBody *body)
{
	body->overFrame = 0;
	_play_frame_isr(body);
	aout_src_enable_new_frame_irq(&optReg, body->src, 0x1);
	body->pauseFrame = 0;

	return 0;
}

int stream_write_callback(AoutStreamBody *body, int overflow)
{
	if (body->pauseFrame)
		return 0;

	if (body->hungry) {
		_start_frame_node(body);
	}

	return 0;
}

int stream_reset(AoutStreamHeader *header)
{
	unsigned int i = 0;

	_wait_all_frame_over(header);
	//hot不能复位spdif数据,因此采用冷复位
	//ac3通路停止->播放有残留帧声音,pcm通路停止->播放正常.
	gxav_clock_cold_rst(MODULE_TYPE_AUDIO_SPDIF);

	aout_src_enable_reset     (&optReg, SRC_R0|SRC_R1|SRC_R2|SRC_R3, 0);
	aout_config_pcm_slow_vol  (&optReg, SLOW_VOL_4, SLOW_POINTS_16);
	aout_enable_pcm_slow_vol  (&optReg, 1);
	aout_config_cddate_maxnum (&optReg, 1);
	aout_enable_cddata_cache  (&optReg, 1);
	aout_src_enable_volume    (&optReg, SRC_R0|SRC_R1, 1);
	aout_src_set_pcm_mage     (&optReg, SRC_R0|SRC_R1, PCM_MAGE_NORMAL);

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamBody *body = header->body[i];
		if (body) {
			aout_src_set_frame_start_addr   (&optReg, body->src, 0x0);
			aout_src_set_frame_end_addr     (&optReg, body->src, 0x0);
			aout_src_set_frame_sample_points(&optReg, body->src, 0x0);
		}
	}
	stream_set_mute  (header, header->mute_status);
	stream_set_track (header, &header->track_status);
	header->src1Skip = 0;

	return 0;
}


int stream_interrupt(AoutStreamHeader *header)
{
	unsigned int i = 0;
	unsigned char request_reset_state = aout_get_request_reset_irq_state(&optReg);
	unsigned char request_reset_inten = aout_get_request_reset_irq_inten(&optReg);

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamBody *body = header->body[i];
		if (body) {
			unsigned char new_frame_state   = aout_src_get_new_frame_irq_state (&optReg, body->src);
			unsigned char new_frame_inten   = aout_src_get_new_frame_irq_inten (&optReg, body->src);

			if (new_frame_state && new_frame_inten) {
				aout_src_clr_new_frame_irq_state(&optReg, body->src);
				_play_frame_isr(header->body[i]);
			}
		}
	}

	if (request_reset_state && request_reset_inten) {
		aout_clr_request_reset_irq_state(&optReg);
		_post_all_frame_over(header);
	}

	return 0;
}

int stream_set_mute(AoutStreamHeader *header, int mute)
{
	//this mode will avoid noise, don't use aout_mix_enable_mute
	mute = mute ? 1 : 0;
	aout_mix_enable_stop_mute(&optReg, MIX_0_I2S|MIX_1_I2S|MIX_2_SPD|MIX_3_SPD, mute);
	header->mute_status = mute;

	return 0;
}

int stream_get_mute(AoutStreamHeader *header, int *mute)
{
	*mute = header->mute_status;

	return 0;
}

int stream_set_mute_now(AoutStreamHeader *header, int mute)
{
	//aout_mix_enable_mute(&optReg, MIX_0_I2S|MIX_1_I2S|MIX_2_SPD|MIX_3_SPD, mute);

	_set_cvbs_mute(header, VIRT_MUTE_0, mute);

	return 0;
}

int stream_set_voltable(AoutStreamHeader *header, unsigned int idx)
{
	switch (idx) {
	case 0:
		header->vol_table = volume_table_df;
		break;
	case 1:
		header->vol_table = volume_table_db;
		break;
	default:
		return -1;
	}

	return 0;
}

int stream_set_volume(AoutStreamHeader *header, AoutStreamVol *vol)
{
	if (vol->vol > 100)
		vol->vol = 100;

	header->targetVol = *vol;

	return 0;
}

int stream_get_volume(AoutStreamHeader *header, AoutStreamVol *vol)
{
	*vol = header->targetVol;

	return 0;
}

int stream_drop_volume(AoutStreamHeader *header)
{
	int i = header->targetVol.vol;

	for (; i >= 0; i-=5) {
		aout_src_set_volume(&optReg, SRC_R0|SRC_R1, header->vol_table[i]);
		gx_mdelay(1);
	}

	aout_src_set_volume(&optReg, SRC_R0|SRC_R1, header->vol_table[0]);
	return 0;
}

int stream_rise_volume(AoutStreamHeader *header)
{
	int i = 0;

	for (; i <= header->targetVol.vol; i+=5) {
		aout_src_set_volume(&optReg, SRC_R0|SRC_R1, header->vol_table[i]);
		gx_mdelay(1);
	}

	aout_src_set_volume(&optReg, SRC_R0|SRC_R1, header->vol_table[header->targetVol.vol]);
	return 0;
}

int stream_set_track(AoutStreamHeader *header, AoutStreamTrack *track)
{
	aout_mix_set_l_channel  (&optReg, MIX_0_I2S|MIX_1_I2S, track->lChannel);
	aout_mix_set_r_channel  (&optReg, MIX_0_I2S|MIX_1_I2S, track->rChannel);
	aout_mix_set_c_channel  (&optReg, MIX_0_I2S|MIX_1_I2S, track->cChannel);
	aout_mix_set_ls1_channel(&optReg, MIX_0_I2S|MIX_1_I2S, track->ls1Channel);
	aout_mix_set_rs1_channel(&optReg, MIX_0_I2S|MIX_1_I2S, track->rs1Channel);
	aout_mix_set_ls2_channel(&optReg, MIX_0_I2S|MIX_1_I2S, track->ls2Channel);
	aout_mix_set_rs2_channel(&optReg, MIX_0_I2S|MIX_1_I2S, track->rs2Channel);
	aout_mix_set_cs_channel (&optReg, MIX_0_I2S|MIX_1_I2S, track->csChannel);

	if (header->spd2Source == MIX_2_SPD_SEL_I2S) {
		aout_mix_set_l_channel(&optReg, MIX_2_SPD, track->lChannel);
		aout_mix_set_r_channel(&optReg, MIX_2_SPD, track->rChannel);
	}

	header->track_status = *track;

	return 0;
}

int stream_set_source(AoutStreamHeader *header, AoutStreamI2SSource i2s_source)
{
	AoutStreamMix       mixer;
	AoutStreamSelSource source;

	mixer = MIX_0_I2S | MIX_1_I2S;
	source.i2s_0_1 = i2s_source;
	aout_mix_select_source(&optReg, mixer, source);

	header->i2sSource = i2s_source;

	return 0;
}

int stream_config_port(AoutStreamHeader *header, AoutStreamPort port)
{
	if (port == DAC_PORT)
		_set_dac_config(header);

	return 0;
}

int stream_config_src(AoutStreamHeader *header, AoutStreamSrc src)
{
	AoutStreamMix       mixer;
	AoutStreamSelSource source;

	if ((src & (SRC_R0 | SRC_R1)) == (SRC_R0 | SRC_R1)) {
		mixer = MIX_0_I2S | MIX_1_I2S;
		source.i2s_0_1 = header->i2sSource;
		aout_mix_select_source(&optReg, mixer, source);
	} else if ((src & SRC_R0) == SRC_R0) {
		mixer = MIX_0_I2S | MIX_1_I2S;
		source.i2s_0_1 = MIX_0_1_I2S_SEL_SRC_R0;
		aout_mix_select_source(&optReg, mixer, source);
	} else if ((src & SRC_R1) == SRC_R1) {
		mixer = MIX_0_I2S | MIX_1_I2S;
		source.i2s_0_1 = MIX_0_1_I2S_SEL_SRC_R1;
		aout_mix_select_source(&optReg, mixer, source);
	} else {
		mixer = MIX_0_I2S | MIX_1_I2S;
		source.i2s_0_1 = MIX_0_1_I2S_SEL_OFF;
		aout_mix_select_source(&optReg, mixer, source);
	}

	if ((src & SRC_R2) == SRC_R2) {
		mixer = MIX_2_SPD;
		header->spd2Source = source.spd_2 = MIX_2_SPD_SEL_SRC_R2;
		aout_mix_select_source(&optReg, mixer, source);
		aout_spd_set_mclk     (&optReg, mixer, SPD_MCLK_512FS);
	} else {
		mixer = MIX_2_SPD;
		header->spd2Source = source.spd_2 = MIX_2_SPD_SEL_I2S;
		aout_mix_select_source(&optReg, mixer, source);
		aout_spd_set_mclk     (&optReg, mixer, SPD_MCLK_512FS);
	}

	if ((src & SRC_R3) == SRC_R3) {
		mixer = MIX_3_SPD;
		source.spd_3 = MIX_3_SPD_SEL_SRC_R3;
		aout_mix_select_source(&optReg, mixer, source);
		aout_spd_set_mclk     (&optReg, mixer, SPD_MCLK_512FS);
	} else {
		mixer = MIX_3_SPD;
		source.spd_3 = MIX_3_SPD_SEL_OFF;
		aout_mix_select_source(&optReg, mixer, source);
	}

	return 0;
}

int stream_port_select_src(AoutStreamHeader *header, AoutStreamPort port, AoutStreamSrc src)
{
	if (HDMI_PORT == port) {
		if (src & (SRC_R0 | SRC_R1))
			header->hdmiSelSrc = (SRC_R0 | SRC_R1);
		else if (src & SRC_R2)
			header->hdmiSelSrc = SRC_R2;
		else if (src & SRC_R3)
			header->hdmiSelSrc = SRC_R3;
		header->hdmiSelChange = 1;
	}

	if (SPDIF_PORT == port) {
		if (src & SRC_R2)
			header->spdifSelSrc = SRC_R2;
		else if (src & SRC_R3)
			header->spdifSelSrc = SRC_R3;
		header->spdifSelChange = 1;
	}

	return 0;
}

int stream_set_resource(AoutStreamResource *res)
{
	optReg.i2sAddr_0 = res->i2sAddr_0;
	optReg.i2sAddr_1 = res->i2sAddr_1;
	optReg.sdcAddr_0 = res->sdcAddr_0;
	optReg.sdcAddr_1 = res->sdcAddr_1;
	optReg.spdAddr_0 = res->spdAddr_0;
	optReg.spdAddr_1 = res->spdAddr_1;
	optReg.runClock  = res->runClock;

	return 0;
}

int stream_power_mute(AoutStreamHeader *header, int mute)
{
	int src_pcm_temp = mute ? 0 : 0xff800000;
	int dst_pcm_temp = mute ? 0xff800000 : 0;

	header->cvbsMuteMap = 0;
	_set_dac_config  (header);
	_play_empty_frame(header);
	if (mute) {
		gxav_hdmi_audioout_mute(1);
		aout_src_set_pcm_value  (&optReg, SRC_R0|SRC_R1, src_pcm_temp);
		aout_src_set_pcm_times  (&optReg, SRC_R0|SRC_R1, 1);
		aout_src_set_pcm_source (&optReg, SRC_R0|SRC_R1, PCM_SOURCE_CPU);
		aout_src_enable_playback(&optReg, SRC_R0|SRC_R1, 0x1);
		aout_src_enable_work    (&optReg, SRC_R0|SRC_R1, 0x1);
		//pcm value from 0x8000 to 0,than pwm from 50% to 0%
		while (src_pcm_temp > dst_pcm_temp) {
			src_pcm_temp -= 0x100;
			aout_src_set_pcm_value (&optReg, SRC_R0|SRC_R1, src_pcm_temp);
			gx_udelay(10);
		}
	} else {
		aout_src_set_pcm_value  (&optReg, SRC_R0|SRC_R1, src_pcm_temp);
		aout_src_set_pcm_times  (&optReg, SRC_R0|SRC_R1, 1);
		aout_src_set_pcm_source (&optReg, SRC_R0|SRC_R1, PCM_SOURCE_CPU);
		aout_src_enable_playback(&optReg, SRC_R0|SRC_R1, 0x1);
		aout_src_enable_work    (&optReg, SRC_R0|SRC_R1, 0x1);
		//pcm value from 0x8000 to 0,than pwm from 0% to 50%
		while (src_pcm_temp < dst_pcm_temp) {
			src_pcm_temp += 0x100;
			aout_src_set_pcm_value (&optReg, SRC_R0|SRC_R1, src_pcm_temp);
			gx_udelay(10);
		}
		gxav_hdmi_audioout_mute(0);
		aout_src_set_pcm_source(&optReg, SRC_R0|SRC_R1, PCM_SOURCE_SDRAM);
	}

	return 0;
}

int stream_set_i2s_work(AoutStreamHeader *header)
{
	unsigned int samplefre = header->hdmiSampleFre;
	unsigned int channels  = header->hdmiChannels;

	//解决某些电视机hdmi杂音问题，需要I2S一直有输出状态
	aout_src_set_pcm_channelnum(&optReg, SRC_R0|SRC_R1, channels);
	aout_src_set_pcm_source    (&optReg, SRC_R0|SRC_R1, PCM_SOURCE_CPU);
	aout_src_set_pcm_value     (&optReg, SRC_R0|SRC_R1, 0x0);
	aout_src_enable_playback   (&optReg, SRC_R0|SRC_R1, 0x1);
	aout_src_enable_work       (&optReg, SRC_R0|SRC_R1, 0x1);
	gxav_hdmi_audioout_set (0);
	gxav_hdmi_audiosample_change(gxav_audioout_samplefre_index(samplefre), channels);

	return 0;
}

int stream_set_hdmi_port(AoutStreamHeader *header, unsigned int samplefre, unsigned int channels)
{
	gxav_hdmi_audioout_set (0);
	gxav_hdmi_audioout_mute(1);
	gxav_hdmi_audiosample_change(gxav_audioout_samplefre_index(samplefre), channels);
	gxav_hdmi_audioout_mute(0);
	header->hdmiSampleFre = samplefre;
	header->hdmiChannels  = channels;

	return 0;
}

int stream_turn_port(AoutStreamHeader *header, AoutStreamPort port, int onoff)
{
	if (port == HDMI_PORT) {
		if (onoff) {
			_set_hdmi_mute(header, VIRT_MUTE_1, onoff);
			gxav_hdmi_audioout_set(0);//fix: hdmi spdif can`t mute
		} else {
			header->hdmiSelChange = 1;//fix: hdmi spdif can`t mute
			_set_hdmi_mute(header, VIRT_MUTE_1, onoff);
		}
	} else if(port == DAC_PORT) {
		_set_cvbs_mute(header, VIRT_MUTE_1, onoff);
	} else if(port == SPDIF_PORT) {
		if (CHIP_IS_CYGNUS)
			aout_spdif_port_mute(&optReg, SPDIF_PORT, onoff);
		else
			gxlog_e(LOG_AOUT, "Unsupport SPDIF to disable\n");
	}

	return 0;
}
