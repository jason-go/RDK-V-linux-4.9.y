#ifndef __GXAV_AUDIOOUT_PROPERTYTYPES_H__
#define __GXAV_AUDIOOUT_PROPERTYTYPES_H__

#include "gxav_common.h"

typedef enum {
	PCM_OUTPUT = 0,
	AC3_OUTPUT,
	EAC3_OUTPUT,
	SMART_OUTPUT,
	SAFE_OUTPUT,
	RAW_OUTPUT,
	DTS_OUTPUT,
	AAC_OUTPUT,
	NOTANY_OUTPUT,
} GxAudioOutProperty_OutPutType;

typedef enum {
	AUDIO_OUT_I2S_OUT_DAC   = 0x1,
	AUDIO_OUT_I2S_IN_DAC    = 0x4,  // 输出到内置扬声器
	AUDIO_OUT_SPDIF         = 0x8,  // S/PDIF输出
	AUDIO_OUT_HDMI          = 0x10, // HDMI输出
} GxAudioOutProperty_OutPortType;

typedef enum {
	AUDIO_OUT_I2S0_PTS_SYNC = (0x1 << 0),
	AUDIO_OUT_I2S1_PTS_SYNC = (0x1 << 1),
	AUDIO_OUT_SPD0_PTS_SYNC = (0x1 << 2),
	AUDIO_OUT_SPD1_PTS_SYNC = (0x1 << 3),
} GxAudioOutDebug;

typedef struct {
	GxAvAudioDataType             indata_type;
	unsigned int                  audiodec_id;
} GxAudioOutProperty_ConfigSource;

typedef struct{
	GxAudioOutProperty_OutPortType   port_type;
	GxAudioOutProperty_OutPutType    data_type;
} GxAudioOutProperty_ConfigPort;

typedef struct {
	GxAudioOutProperty_OutPortType   port_type;
	union {
		struct {
			unsigned int samplefre;
			unsigned int channel;
		} hdmi;
	};
} GxAudioOutProperty_SetPort;

typedef struct {
	GxAudioOutProperty_OutPortType port_type;
	unsigned int                   onoff;
} GxAudioOutProperty_TurnPort;

typedef struct{
	int sync;
	unsigned int low_tolerance;
	unsigned int high_tolerance;
} GxAudioOutProperty_ConfigSync;

typedef struct{
	int mute;
} GxAudioOutProperty_Mute;

typedef enum{
	AUDIO_CHANNEL_0 = (1 << 0),
	AUDIO_CHANNEL_1 = (1 << 1),
	AUDIO_CHANNEL_2 = (1 << 2),
	AUDIO_CHANNEL_3 = (1 << 3),
	AUDIO_CHANNEL_4 = (1 << 4),
	AUDIO_CHANNEL_5 = (1 << 5),
	AUDIO_CHANNEL_6 = (1 << 6),
	AUDIO_CHANNEL_7 = (1 << 7),
	AUDIO_CHANNEL_M = (1 << 8)
} GxAudioOutProperty_OutData;

typedef struct {
	GxAudioOutProperty_OutData  left;
	GxAudioOutProperty_OutData  right;
	GxAudioOutProperty_OutData  lfe;
	GxAudioOutProperty_OutData  center;
	GxAudioOutProperty_OutData  left_surround;
	GxAudioOutProperty_OutData  right_surround;
	GxAudioOutProperty_OutData  left_backsurround;
	GxAudioOutProperty_OutData  right_backsurround;
	unsigned int                mono;
} GxAudioOutProperty_Channel;

typedef struct{
	unsigned int volume;
	unsigned int right_now;
	unsigned int step;
} GxAudioOutProperty_Volume;

typedef struct {
	unsigned int table_idx; //0 :def mode, 1: db mode
} GxAudioOutProperty_VolumeTable;

typedef struct{
	unsigned int pts;
} GxAudioOutProperty_Pts;

typedef struct{
	int offset_ms;
} GxAudioOutProperty_PtsOffset;

typedef struct{
	unsigned int enable;
} GxAudioOutProperty_PowerMute;

typedef struct{
	GxAvFrameStat frame_stat;
	unsigned int  state;

#define AUDIOOUT_INITED   1
#define AUDIOOUT_READY    2
#define AUDIOOUT_RUNNING  3
#define AUDIOOUT_PAUSED   4
#define AUDIOOUT_STOPED   5
#define AUDIOOUT_OVER     6

}GxAudioOutProperty_State;

typedef struct{
#define MIX_PCM       (1<<0)
#define MIX_ADPCM     (1<<1)
	int pcm_mix;
}GxAudioOutProperty_PcmMix;

typedef struct{
#define AUDIO_SLOW_SPEED     80
#define AUDIO_FAST_SPEED     120
	int speed;
	int speed_mute;
}GxAudioOutProperty_Speed;

typedef struct {
	GxAudioOutDebug  value;
} GxAudioOutProperty_Debug;

#endif /* __GXAV_AUDIOOUT_PROPERTYTYPES_H__ */

