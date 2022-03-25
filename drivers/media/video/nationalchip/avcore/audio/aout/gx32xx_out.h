#ifndef __GX3201_AUDIO_OUT_H__
#define __GX3201_AUDIO_OUT_H__

#define AUDIO_BUF_ADDR_MASK               (0xFFFFFFFF)

enum {
	SPD_CLOCK_256FS = 0,
	SPD_CLOCK_384FS,
	SPD_CLOCK_512FS,
	SPD_CLOCK_1024FS,
	SPD_CLOCK_2048FS,
};

enum {
	ADCCLOCK_128FS = 0,
	ADCCLOCK_192FS,
	ADCCLOCK_256FS,
	ADCCLOCK_384FS,
	ADCCLOCK_512FS,
	ADCCLOCK_768FS,
	ADCCLOCK_1024FS,
	ADCCLOCK_1536FS
};

enum {
	SPD_PLAY_OFF = 0,
	SPD_PLAY_IDLE,
	SPD_PLAY_PCM1,
	SPD_PLAY_PCM2,
	SPD_PLAY_NONPCM,
	SPD_PLAY_HBR
};

enum stream_type {
	SPDIF_STREAM_TYPE_PCM = 0,
	SPDIF_STREAM_TYPE_ENC
};

enum mute_status {
	AUDIO_UNMUTE = 0,
	AUDIO_MUTE
};

enum amplify_level {
	AMPLIFY_LEVEL_LOW = 0,
	AMPLIFY_LEVEL_MID,
	AMPLIFY_LEVEL_HIGH,
	AMPLIFY_LEVEL_EX_HIGH
};

enum audio_object {
	AUDIO_OBJECT_SPDIF = 0,
	AUDIO_OBJECT_I2S
};

enum adc_mclock {
	AUDIO_ADC_MCLOCK_128FS  = 128,
	AUDIO_ADC_MCLOCK_192FS  = 192,
	AUDIO_ADC_MCLOCK_256FS  = 256,
	AUDIO_ADC_MCLOCK_384FS  = 384,
	AUDIO_ADC_MCLOCK_512FS  = 512,
	AUDIO_ADC_MCLOCK_768FS  = 768,
	AUDIO_ADC_MCLOCK_1024FS = 1024,
	AUDIO_ADC_MCLOCK_1536FS = 1536
};

enum adc_word_len {
	AUDIO_ADC_WORD_LEN_08BIT = 0,
	AUDIO_ADC_WORD_LEN_16BIT,
	AUDIO_ADC_WORD_LEN_18BIT,
	AUDIO_ADC_WORD_LEN_20BIT,
	AUDIO_ADC_WORD_LEN_24BIT,
	AUDIO_ADC_WORD_LEN_32BIT
};

enum channel_select {
	AUDIO_SELECT_0_CHANNEL = 0,
	AUDIO_SELECT_1_CHANNEL,
	AUDIO_SELECT_2_CHANNEL,
	AUDIO_SELECT_3_CHANNEL,
	AUDIO_SELECT_4_CHANNEL,
	AUDIO_SELECT_5_CHANNEL,
	AUDIO_SELECT_6_CHANNEL,
	AUDIO_SELECT_7_CHANNEL,
	AUDIO_SPECIAL_TO_DAC_AC97,
	AUDIO_SPECIAL_TO_DAC_AC97_FROM_SPD
};

enum fs_play_back {
	AUDIO_PLAY_BACK_FS = 0,
	AUDIO_PLAY_BACK_FS_2
};

enum dac_type {
	DAC_CS4344 = 0,
	DAC_PT8211,
	DAC_PT8234,
	DAC_CS4334,
	DAC_CS4345,
	DAC_MAX
};

enum da_clock {
	AUDIO_DACLOCK_256FS   = 0,
	AUDIO_DACLOCK_384FS,
	AUDIO_DACLOCK_512FS,
	AUDIO_DACLOCK_768FS,
	AUDIO_DACLOCK_1024FS,
	AUDIO_DACLOCK_1536FS,
	AUDIO_DACLOCK_128FS,
	AUDIO_DACLOCK_192FS
};

enum bck_sel {
	AUDIO_BCK_SEL_32FS = 0,
	AUDIO_BCK_SEL_48FS,
	AUDIO_BCK_SEL_64FS
};

enum da_format {
	AUDIO_DATA_FORMAT_I2S = 0,
	AUDIO_DATA_FORMAT_LEFT_JUSTIFIED,
	AUDIO_DATA_FORMAT_RIGHT_JUSTIFIED,
	AUDIO_DATA_FORMAT_AC97
};

enum pcm_word_len {
	AUDIO_PCM_WORD_LEN_16BIT = 0,
	AUDIO_PCM_WORD_LEN_18BIT,
	AUDIO_PCM_WORD_LEN_20BIT,
	AUDIO_PCM_WORD_LEN_24BIT
};

enum i2s_pcm_mode {
	PCM_SEL_SDRAM_MODE = 0,
	PCM_SEL_CPU_MODE
};

enum pts_status {
	PTS_NORMAL = 0,
	PTS_SKIP      = -1,
	PTS_REPEAT    = -2
};

enum stream_data_type {
	AC3_STREAM_FLAG = 0x01,
	PAUSE_STREAM_FLAG = 0x03,
	E_AC3_STREAM_FLAG =0x15,
	ADTS_STREAM_FLAG = 0x14,// 20
	LATM_STREAM_FLAG = 0x17 // 23
};

struct audio_dac {
	enum dac_type       type;
	enum da_clock       mclock;
	enum bck_sel        bclock;
	enum da_format      format;
	enum pcm_word_len   wordlen;
};

struct gxav_audiochannel {
	int c_num;
#define MAX_CHANNEL    10
#define AUDIO_CHANNEL_MIN_VALUE  (AUDIO_CHANNEL_0)
#define AUDIO_CHANNEL_MAX_VALUE  \
	(AUDIO_CHANNEL_0|AUDIO_CHANNEL_1|AUDIO_CHANNEL_2|AUDIO_CHANNEL_3    \
	 |AUDIO_CHANNEL_4|AUDIO_CHANNEL_5|AUDIO_CHANNEL_6|AUDIO_CHANNEL_7|AUDIO_CHANNEL_M)

	int channel_data[MAX_CHANNEL];
};

#endif
