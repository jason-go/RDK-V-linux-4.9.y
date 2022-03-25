#ifndef _GXAV_COMMON_H_
#define _GXAV_COMMON_H_

#ifndef NULL
#define NULL 0
#endif

#define CRYSTAL_FREQ 27000000  /* HZ */

/*
 * GX_MIN()/GX_MAX() macros that also do strict
 * type-checking. See the pointer comparison
 * [(void) (&_min1 == &_min2);]that do this.
 */
#define GX_MIN(x, y) ({                   \
		typeof(x) _min1 = (x);            \
		typeof(y) _min2 = (y);            \
		(void) (&_min1 == &_min2);        \
		_min1 < _min2 ? _min1 : _min2; })

#define GX_MAX(x, y) ({                   \
		typeof(x) _max1 = (x);            \
		typeof(y) _max2 = (y);            \
		(void) (&_max1 == &_max2);        \
		_max1 > _max2 ? _max1 : _max2; })

#define GX_ABS(x) ({                      \
		int __x = (x);                    \
		(__x < 0) ? -__x : __x;           \
		})

#define GX_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))

typedef enum  gxav_chip_id {
	GXAV_ID_GX3211  = 0x3211,
	GXAV_ID_GX3201  = 0x3201,
	GXAV_ID_GX3113C = 0x6131,
	GXAV_ID_GX6605S = 0x6605,
	GXAV_ID_SIRIUS  = 0x6612,
	GXAV_ID_TAURUS  = 0x6616,
	GXAV_ID_GEMINI  = 0x6701,
	GXAV_ID_CYGNUS  = 0x6705,
}GxAvChipId;

typedef enum {
	GXAV_SDC_READ,
	GXAV_SDC_WRITE
}GxAvSdcOPMode;

typedef enum gxav_module_type {
	GXAV_MOD_SDC           = 0x00,
	GXAV_MOD_HDMI          = 0x01,
	GXAV_MOD_DEMUX         = 0x02,
	GXAV_MOD_VIDEO_DECODE  = 0x03,
	GXAV_MOD_AUDIO_DECODE  = 0x04,
	GXAV_MOD_VPU           = 0x05,
	GXAV_MOD_AUDIO_OUT     = 0x06,
	GXAV_MOD_VIDEO_OUT     = 0x07,
	GXAV_MOD_JPEG_DECODER  = 0x08,
	GXAV_MOD_STC           = 0x09,
	GXAV_MOD_MTC           = 0x0a,
	GXAV_MOD_ICAM          = 0x0b,
	GXAV_MOD_IEMM          = 0x0c,
	GXAV_MOD_DESCRAMBLER   = 0x0d,
	GXAV_MOD_GP            = 0x0e,
	GXAV_MOD_DVR           = 0x0f,
	GXAV_MOD_CLOCK         = 0x10,
	GXAV_MOD_SECURE        = 0x11,
	GXAV_MOD_MAX           = 0x12,
} GxAvModuleType;

typedef enum  gxav_module_state {
	GXAV_MOD_RUNNING,
	GXAV_MOD_STOP,
	GXAV_MOD_PAUSE,
} GxAvModuleState;

typedef struct gxav_point {
	unsigned int x;
	unsigned int y;
} GxAvPoint;

typedef struct gxav_rect {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
} GxAvRect;

typedef enum  gxav_direction {
	GXAV_PIN_INPUT  = 0x1,
	GXAV_PIN_OUTPUT = 0x2
} GxAvDirection;

typedef enum  gxav_pin_id {
	GXAV_PIN_ESA   = 0x1,
	GXAV_PIN_PCM   = 0x2,
	GXAV_PIN_AC3   = 0x3, //直接过滤的AC3数据
	GXAV_PIN_EAC3  = 0x4, //降级处理的AC3数据
	GXAV_PIN_ADESA = 0x5,
	GXAV_PIN_ADPCM = 0x6,
	GXAV_PIN_DTS   = 0x7,
	GXAV_PIN_AAC   = 0x8,
	GXAV_PIN_MAX   = 0xff
} GxAvPinId;

typedef struct gxav_channel_info {
	unsigned int pin_id;
	void *channel;
	GxAvDirection dir;
} GxAvChanInfo;

typedef enum  gxav_channel_flag {
	N  = 0x0,
	W  = 0x1,
	R  = 0x2,
	RW = 0x3
} GxAvChannelFlag;

typedef enum gxav_channel_type {
	GXAV_NO_PTS_FIFO   = (1 << 0),
	GXAV_PTS_FIFO      = (1 << 1),
	GXAV_WPROTECT_FIFO = (1 << 2),
	GXAV_RPROTECT_FIFO = (1 << 3),
} GxAvChannelType;

typedef struct gxav_gate_info {
	unsigned int almost_empty;
	unsigned int almost_full;
	unsigned int cache_max;
} GxAvGateInfo;

typedef struct gxav_sync_params {
	unsigned int pcr_err_gate;
	unsigned int pcr_err_time;
	unsigned int apts_err_gate;
	unsigned int apts_err_time;
	unsigned int audio_low_tolerance;
	unsigned int audio_high_tolerance;
	int stc_offset;
} GxAvSyncParams;

typedef struct gxav_device_capability {
	unsigned int module_cnt[GXAV_MOD_MAX];
} GxAVDeviceCapability;

typedef struct gxav_frame_stat {
	unsigned long long play_frame_cnt;
	unsigned long long error_frame_cnt;
	unsigned long long filter_frame_cnt;
	unsigned long long lose_sync_cnt;
	unsigned long long decode_frame_cnt;
	unsigned int       synced_flag;
	struct {
		unsigned int enable;
		unsigned int code;
	} AFD;
} GxAvFrameStat;

typedef enum {
	PCM_TYPE     = (1<<0),
	AC3_TYPE     = (1<<1),
	EAC3_TYPE    = (1<<2),
	ADPCM_TYPE   = (1<<3),
	DTS_TYPE     = (1<<4),
	AAC_TYPE     = (1<<5),
} GxAvAudioDataType;

typedef enum {
	PCM_ERROR     = (1<<0),
	AC3_ERROR     = (1<<1),
	EAC3_ERROR    = (1<<2),
	ADPCM_ERROR   = (1<<3),
	DTS_ERROR     = (1<<4),
	AAC_ERROR     = (1<<5),
} GxAvAudioDataEvent;

typedef enum {
	AS_NONE = 0,
	AS_ACCESS_RESTRICT = (1<<0),
	AS_ACCESS_ALIGN    = (1<<1),
} GxAvAdvancedSecurity;

typedef struct {
	unsigned int addr;
	unsigned int size;
} GxAvASDataBlock;

typedef enum {
	EDID_AUDIO_LINE_PCM = (1<<0),
	EDID_AUDIO_AC3      = (1<<1),
	EDID_AUDIO_EAC3     = (1<<2),
	EDID_AUDIO_DTS      = (1<<3),
	EDID_AUDIO_AAC      = (1<<4),
} GxAvHdmiEdid;

typedef enum videoout_hdmi_type {
    HDMI_RGB_OUT = 1,
    HDMI_YUV_422 = 2,
    HDMI_YUV_444 = 4,
} GxAvHdmiEncodingOut;

typedef enum videoout_hdmi_color_depth {
	HDMI_COLOR_DEPTH_8BIT  = 1,
	HDMI_COLOR_DEPTH_10BIT = 2,
	HDMI_COLOR_DEPTH_12BIT = 3,
} GxAvHdmiColorDepth;

typedef struct gxav_rational {
    int num; ///< numerator
    int den; ///< denominator
} GxAvRational;

typedef struct {
	unsigned int   status; //1: dump finish
	unsigned char *buffer;
	unsigned int   size;
} GxAvDebugDump;

enum av_hdcp_state {
	AV_HDCP_IDLE = -1,
	AV_HDCP_SUCCESS = 0,
	AV_HDCP_FAILED = 1,
};

typedef struct gxav_hdcp_state {
	enum av_hdcp_state state;
} GxAvHDCPState;

#define CHIP_IS_GX3201  (gxcore_chip_probe() == GXAV_ID_GX3201)
#define CHIP_IS_GX3211  (gxcore_chip_probe() == GXAV_ID_GX3211)
#define CHIP_IS_GX3113C (gxcore_chip_probe() == GXAV_ID_GX3113C)
#define CHIP_IS_GX6605S (gxcore_chip_probe() == GXAV_ID_GX6605S)
#define CHIP_IS_SIRIUS  (gxcore_chip_probe() == GXAV_ID_SIRIUS)
#define CHIP_IS_TAURUS  (gxcore_chip_probe() == GXAV_ID_TAURUS)
#define CHIP_IS_GEMINI  (gxcore_chip_probe() == GXAV_ID_GEMINI)
#define CHIP_IS_CYGNUS  (gxcore_chip_probe() == GXAV_ID_CYGNUS)

#define CHIP_SUB_ID     (gxcore_chip_sub_probe())

#define __maybe_unused  __attribute__((unused))

#endif

