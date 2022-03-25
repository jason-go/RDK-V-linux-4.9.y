#ifndef __GXAV_AUDIODEC_PROPERTYTYPES_H__
#define __GXAV_AUDIODEC_PROPERTYTYPES_H__

#include "gxav_common.h"

typedef  enum {
	CODEC_MPEG12A = 1,
	CODEC_AVSA,
	CODEC_AC3,
	CODEC_EAC3,
	CODEC_RA_AAC,
	CODEC_RA_RA8LBR,
	CODEC_DRA,
	CODEC_MPEG4_AAC,
	CODEC_WMA,
	CODEC_VORBIS,
	CODEC_OPUS,
	CODEC_PCM,
	CODEC_DTS,
	CODEC_FLAC,
	CODEC_SBC,
} GxAudioDecProperty_Type;

typedef enum {
	STREAM_ES = 0,
	STREAM_TS
} GxAudioDecProperty_StreamType;

typedef enum{
	DECODE_MODE  = (1<<0),
	BYPASS_MODE  = (1<<1),
	CONVERT_MODE = (1<<2)
} GxAudioDecProperty_Mode;

typedef enum {
	AUDIO_TO_AVOUT  = 0,
	AUDIO_TO_MEMORY,
}GxAudioDecProperty_Sink;

struct real_property {
	unsigned char header[2048];
	unsigned int  header_size;
};

struct mpeg4_aac_property {
	unsigned int  format;
#define LATM_FORMAT 0
#define ADTS_FORMAT 1
	unsigned char header[2048];
	unsigned int  header_size;
};

struct dolby_property {
	int header[100];
	unsigned int header_size;
};

typedef struct {
	GxAudioDecProperty_Type type;
	GxAudioDecProperty_Mode mode;
	GxAudioDecProperty_Sink sink;
	union {
		struct real_property      real;
		struct mpeg4_aac_property mpeg4_aac;
		struct dolby_property     dolby;
	} u;
	unsigned int audioout_id;
    unsigned int down_mix;
	unsigned int anti_error_code;
	void* ormem_start;
	unsigned int ormem_size;
	unsigned int enough_data;
	GxAudioDecProperty_StreamType stream_type;
	int                           stream_pid;
} GxAudioDecProperty_Config;

typedef struct {
#define PCM_BIG_ENDIAN 1
#define PCM_LITTLE_ENDIAN 0
	unsigned int samplefre;
	unsigned int bitwidth;
	unsigned int channelnum;
	unsigned int endian;
	unsigned int intelace;
	unsigned int float_en;
} GxAudioDecProperty_PcmInfo;

typedef struct {
	GxAvAudioDataType outdata_type;
} GxAudioDecProperty_OutInfo;

typedef enum {
	SAMPLEFRE_44KDOT1HZ   = (1 << 0),
	SAMPLEFRE_48KHZ       = (1 << 1),
	SAMPLEFRE_32KHZ       = (1 << 2),
	SAMPLEFRE_22KDOT05HZ  = (1 << 3),
	SAMPLEFRE_24KHZ       = (1 << 4),
	SAMPLEFRE_96KHZ       = (1 << 5),
	SAMPLEFRE_16KHZ       = (1 << 6),
	SAMPLEFRE_12KHZ       = (1 << 7),
	SAMPLEFRE_11KDOT025HZ = (1 << 8),
	SAMPLEFRE_9KDOT6HZ    = (1 << 9),
	SAMPLEFRE_8KHZ        = (1 << 10)
}GxAudioDecProperty_SampleFre;

typedef struct {
	GxAvAudioDataType data_type;
	unsigned int      bitrate;
	unsigned int      samplefre;
	unsigned int      channelnum;
	unsigned int      decode_frame_cnt;
} GxAudioDecProperty_FrameInfo;

typedef struct{
	unsigned int pts;
} GxAudioDecProperty_Pts;

typedef enum {
	AUDIODEC_DEBUG_DECODE_ESA       = (0x1 << 0),
	AUDIODEC_DEBUG_DECODE_TASK      = (0x1 << 1),
	AUDIODEC_DEBUG_DECODE_FIND_PTS  = (0x1 << 2),
	AUDIODEC_DEBUG_DECODE_FIX_PTS   = (0x1 << 3),
	AUDIODEC_DEBUG_BYPASS_ESA       = (0x1 << 4),
	AUDIODEC_DEBUG_BYPASS_TASK      = (0x1 << 5),
	AUDIODEC_DEBUG_BYPASS_FIND_PTS  = (0x1 << 6),
	AUDIODEC_DEBUG_BYPASS_FIX_PTS   = (0x1 << 7),
	AUDIODEC_DEBUG_CONVERT_ESA      = (0x1 << 8),
	AUDIODEC_DEBUG_CONVERT_TASK     = (0x1 << 9),
	AUDIODEC_DEBUG_CONVERT_FIND_PTS = (0x1 << 10),
	AUDIODEC_DEBUG_CONVERT_FIX_PTS  = (0x1 << 11),
} GxAcodecDebug;

typedef enum{
	AUDIODEC_INITED,
	AUDIODEC_READY,
	AUDIODEC_RUNNING,
	AUDIODEC_PAUSED,
	AUDIODEC_STOPED,
	AUDIODEC_ERROR,
	AUDIODEC_OVER
}GxAcodecState;

typedef enum{
	AUDIODEC_ERR_NONE,
	AUDIODEC_ERR_ESA_OVERFLOW,
	AUDIODEC_ERR_UNSUPPORT_CODECTYPE
}GxAcodecErrCode;

typedef struct{
	GxAcodecState state;
	GxAcodecErrCode err_code;
}GxAudioDecProperty_State;

typedef struct {
	unsigned int err_no;
#define STREAM_UNSUPPORT    (1 << 0)
} GxAudioDecProperty_Errno;

typedef struct {
	GxAudioDecProperty_Type      type;
	GxAudioDecProperty_SampleFre samplefre;
	unsigned int                 channelnum;
	unsigned int                 workbuf_size;
} GxAudioDecProperty_Capability;

typedef struct {
	GxAudioDecProperty_Mode      mode;
	GxAudioDecProperty_Type      type;
	unsigned int                 support;
} GxAudioDecProperty_SupportAD;

typedef struct {
	int pcm_pts;
	unsigned int pcm_len;    // 样点为单位
	int pcm_samplefre;       // 最高位表示采样率是否改变，1有效
	int pcm_channel;
	int pcm_wordtype;
	int pcm_unit_time;

	unsigned int addr1;  //用户空间地址
	unsigned int size1;
	unsigned int addr2;  //用户空间地址
	unsigned int size2;
} GxAudioDecProperty_FrameData;

typedef struct {
	unsigned int ad_text_tag;
	unsigned long long esa_size;
	unsigned int ad_fade_byte;
	unsigned int ad_pan_byte;
} GxAudioDecProperty_ContrlInfo;

typedef struct {
#define SCR_MAP_LEN (64)
#define SCR_MAP_KEY_LEN   (16)
#define SCR_RAND_DATA_LEN (16)
#define SCR_CHIP_ID_LEN   (4)
#define SCR_SCR_KEY_LEN   (16)
	unsigned char map[SCR_MAP_LEN];
	unsigned char map_key[SCR_MAP_KEY_LEN];
	unsigned char rand_data[SCR_RAND_DATA_LEN];
	unsigned char scr_key[SCR_SCR_KEY_LEN];
	unsigned char chip_id[SCR_CHIP_ID_LEN];
} GxAudioDecProperty_DecodeKey;

typedef struct {
#define DEC_KEY_LEN       (16)
	unsigned char dec_key[DEC_KEY_LEN];
	unsigned int  dec_result;
} GxAudioDecProperty_GetKey;

typedef struct {
#define BOOST_VOLUME_MIN_INDEX (0)
#define BOOST_VOLUME_MAX_INDEX (32)
	unsigned int index;
} GxAudioDecProperty_BoostVolume;

typedef struct {
	GxAcodecDebug  value;
} GxAudioDecProperty_Debug;

#endif /* __GXAV_AUDIODEC_PROPERTYTYPES_H__ */

