#ifndef __AUDIO_COMMON_H__
#define __AUDIO_COMMON_H__

#include "gxav_audioout_propertytypes.h"

typedef enum {
	AUDIO_SAMPLE_FRE_44KDOT1HZ = 0,
	AUDIO_SAMPLE_FRE_48KHZ,
	AUDIO_SAMPLE_FRE_32KHZ,
	AUDIO_SAMPLE_FRE_22KDOT05HZ,
	AUDIO_SAMPLE_FRE_24KHZ,
	AUDIO_SAMPLE_FRE_16KHZ,
	AUDIO_SAMPLE_FRE_96KHZ,
	AUDIO_SAMPLE_FRE_88KDOT2HZ,
	AUDIO_SAMPLE_FRE_128KHZ,
	AUDIO_SAMPLE_FRE_176KDOT4HZ,
	AUDIO_SAMPLE_FRE_192KHZ,
	AUDIO_SAMPLE_FRE_64KHZ,
	AUDIO_SAMPLE_FRE_12KHZ,
	AUDIO_SAMPLE_FRE_11KDOT025HZ,
	AUDIO_SAMPLE_FRE_9KDOT6HZ,
	AUDIO_SAMPLE_FRE_8KHZ
} GxAudioSampleFre;

typedef enum {
	AUDIO_SIGNAL_CHANNEL_NUM = 1,
	AUDIO_TWO_CHANNEL_NUM    = 2,
	AUDIO_THREE_CHANNEL_NUM  = 3,
	AUDIO_FOUR_CHANNEL_NUM   = 4,
	AUDIO_FIVE_CHANNEL_NUM   = 5,
	AUDIO_SIX_CHANNEL_NUM    = 6,
	AUDIO_SEVEN_CHANNEL_NUM  = 7,
	AUDIO_EIGHT_CHANNEL_NUM  = 8,
	AUDIO_OTHERS_CHANNEL_NUM = 0xff
} GxAudioChannels;

typedef enum {
	AUDIO_TYPE_AC3 = 0,
	AUDIO_TYPE_EAC3,
	AUDIO_TYPE_DTS,
	AUDIO_TYPE_ADTS_AAC_LC,
	AUDIO_TYPE_ADTS_HE_AAC,
	AUDIO_TYPE_ADTS_HE_AAC_V2,
	AUDIO_TYPE_LATM_AAC_LC,
	AUDIO_TYPE_LATM_HE_AAC,
	AUDIO_TYPE_LATM_HE_AAC_V2
} GxAudioFrameType;

typedef enum {
	AC3_FRAME_LENGTH = 1536,
	E_AC3_FRAME_LENGTH = 6144,
	AAC_LC_FRAME_LENGTH = 1024,
	HE_AAC_FRAME_LENGTH = 2048
} GxAudioFrameLength;

typedef struct {
	unsigned int frame_s_addr; // 相对地址
	unsigned int frame_e_addr;
	unsigned int frame_size;
	unsigned int sample_fre;  // 最高位表示采样率是否改变，1有效
	unsigned int sample_fre_change;
	unsigned int endian;
	unsigned int interlace;
	int pts;
	int vaild;

	/* for pcm node */
	int channel_num;
	int sample_data_len;
	unsigned int audio_standard;
	unsigned int dolby_type;
	unsigned int dolby_version;

	/* for enc node */
	unsigned int frame_bsmod;
	unsigned int frame_add_num;
	unsigned int frame_type;
	unsigned int frame_bytes;
} GxAudioFrameNode;

extern char gxav_audiodec_get_dolby_type (void);
extern int  gxav_audiodec_get_fifo       (int id, GxAvPinId pin_id, void **fifo);
extern int  gxav_audiodec_get_max_channel(int id, unsigned int *max_channel);
extern int  gxav_audiodec_set_volume     (int id, int volumedb);
extern int  gxav_audiodec_set_ad_ctrlinfo(int id, int enable);
extern int  gxav_audiodec_set_mono       (int id, int mono_en);

extern int  gxav_audioout_samplefre_index(unsigned int samplefre);
extern int  gxav_audioout_set_event(int id, GxAvAudioDataEvent data_event);

#endif
