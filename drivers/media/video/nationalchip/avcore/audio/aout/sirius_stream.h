#ifndef __SIRIUS_STREAM_H__
#define __SIRIUS_STREAM_H__

#define MAX_STREAM_BODY (4)

typedef enum {
	SRC_R0 = (1<<0),
	SRC_R1 = (1<<1),
	SRC_R2 = (1<<2),
	SRC_R3 = (1<<3)
} AoutStreamSrc;

typedef enum {
	MIX_0_I2S = (1<<0),
	MIX_1_I2S = (1<<1),
	MIX_2_SPD = (1<<2),
	MIX_3_SPD = (1<<3)
} AoutStreamMix;

typedef enum {
	VIRT_MUTE_0 = (1 << 0),
	VIRT_MUTE_1 = (1 << 1)
} AoutStreamMuteMap;

typedef enum {
	HDMI_PORT  = 0x1,
	SPDIF_PORT = 0x2,
	DAC_PORT  = 0x3
} AoutStreamPort;

typedef enum {
	MIX_0_1_I2S_SEL_SRC_R0    = 0x0,
	MIX_0_1_I2S_SEL_SRC_R1    = 0x1,
	MIX_0_1_I2S_SEL_SRC_R0_R1 = 0x2,
	MIX_0_1_I2S_SEL_OFF,
} AoutStreamI2SSource;

typedef enum {
	MIX_2_SPD_SEL_OFF    = 0x0,
	MIX_2_SPD_SEL_IDLE   = 0x1,
	MIX_2_SPD_SEL_I2S    = 0x2,
	MIX_2_SPD_SEL_SDRAM  = 0x3,
	MIX_2_SPD_SEL_SRC_R2 = 0x4,
} AoutStreamSpd2Source;

typedef enum {
	MIX_3_SPD_SEL_OFF    = 0x0,
	MIX_3_SPD_SEL_IDLE   = 0x1,
	MIX_3_SPD_SEL_SRC_R3 = 0x4,
	MIX_3_SPD_SEL_HBR    = 0x5,
} AoutStreamSpd3Source;

typedef enum {
	SRC_CHANNEL_0 = 0x0,
	SRC_CHANNEL_1 = 0x1,
	SRC_CHANNEL_2 = 0x2,
	SRC_CHANNEL_3 = 0x3,
	SRC_CHANNEL_4 = 0x4,
	SRC_CHANNEL_5 = 0x5,
	SRC_CHANNEL_6 = 0x6,
	SRC_CHANNEL_7 = 0x7,
	SRC_CHANNEL_M = 0xf,
} AoutStreamChannel;

typedef enum {
	SLOW_VOL_1 = 0x0,
	SLOW_VOL_2 = 0x1,
	SLOW_VOL_3 = 0x2,
	SLOW_VOL_4 = 0x3,
	SLOW_VOL_5 = 0x4,
	SLOW_VOL_6 = 0x5,
	SLOW_VOL_7 = 0x6,
	SLOW_VOL_8 = 0x7,
} AoutStreamSlowVol;

typedef enum {
	SLOW_POINTS_1   = 0x0,
	SLOW_POINTS_2   = 0x1,
	SLOW_POINTS_4   = 0x2,
	SLOW_POINTS_8   = 0x3,
	SLOW_POINTS_16  = 0x4,
	SLOW_POINTS_32  = 0x5,
	SLOW_POINTS_64  = 0x6,
	SLOW_POINTS_128 = 0x7,
} AoutStreamSlowPoints;

typedef enum {
	PCM_BITS_32     = 0x0,
	PCM_BITS_28_L   = 0x1,
	PCM_BITS_24_L   = 0x2,
	PCM_BITS_16_L   = 0x3,
	PCM_BITS_28_R   = 0x4,
	PCM_BITS_24_M   = 0x5,
	PCM_BITS_20_ML  = 0x6,
	PCM_BITS_12_ML  = 0x7,
	PCM_BITS_24_R   = 0x8,
	PCM_BITS_20_MR  = 0x9,
	PCM_BITS_16_M   = 0xa,
	PCM_BITS_8_ML   = 0xb,
	PCM_BITS_16_R   = 0xc,
	PCM_BITS_12_MR  = 0xd,
	PCM_BITS_8_MR   = 0xe,
	PCM_BITS_0      = 0xf,
	PCM_BITS_16     = 0x10,
	PCM_BITS_8      = 0x11,
	PCM_BITS_8_WAVE = 0x12
} AoutStreamPcmBits;

typedef enum {
	PCM_ENDIAN_0 = 0x0,
	PCM_ENDIAN_1 = 0x1,
	PCM_ENDIAN_2 = 0x2,
	PCM_ENDIAN_3 = 0x3
} AoutStreamPcmEndian;

typedef enum {
	PCM_INTERLACE    = 0x0,
	PCM_NO_INTERLACE = 0x1
} AoutStreamPcmInterlace;

typedef enum {
	PCM_CHANNEL_1 = 0x0,
	PCM_CHANNEL_2 = 0x1,
	PCM_CHANNEL_3 = 0x2,
	PCM_CHANNEL_4 = 0x3,
	PCM_CHANNEL_5 = 0x4,
	PCM_CHANNEL_6 = 0x5,
	PCM_CHANNEL_7 = 0x6,
	PCM_CHANNEL_8 = 0x7
} AoutStreamPcmChNum;

typedef enum {
	PCM_SOURCE_SDRAM   = 0x0,
	PCM_SOURCE_CPU     = 0x1,
	PCM_SOURCE_AUTOGEN = 0x2,
	PCM_SOURCE_M_SEQ   = 0x3
} AoutStreamPcmSource;

typedef enum {
	PCM_MAGE_NORMAL  = 0x0,
	PCM_MAGE_TWICE   = 0x1,
	PCM_MAGE_4_TIMES = 0x2,
	PCM_MAGE_8_TIMES = 0x3
} AoutStreamPcmMage;

typedef enum {
	PCM_OUTPUT_16_BITS = 0x0,
	PCM_OUTPUT_18_BITS = 0x1,
	PCM_OUTPUT_20_BITS = 0x2,
	PCM_OUTPUT_24_BITS = 0x3
} AoutStreamPcmOutputBits;

typedef enum {
	CDD_ENDIAN_0 = 0x0,
	CDD_ENDIAN_1 = 0x1,
	CDD_ENDIAN_2 = 0x2,
	CDD_ENDIAN_3 = 0x3
} AoutStreamCddEndian;

typedef enum {
	SPD_MCLK_256FS   = 0x0,
	SPD_MCLK_384FS   = 0x1,
	SPD_MCLK_512FS   = 0x2,
	SPD_MCLK_1024FS  = 0x3,
	SPD_MCLK_2048FS  = 0x4,
} AoutStreamSpdMclk;

typedef enum {
	DAC_MCLK_256FS   = 0x0,
	DAC_MCLK_384FS   = 0x1,
	DAC_MCLK_512FS   = 0x2,
	DAC_MCLK_768FS   = 0x3,
	DAC_MCLK_1024FS  = 0x4,
	DAC_MCLK_1536FS  = 0x5,
	DAC_MCLK_128FS   = 0x6,
	DAC_MCLK_192FS   = 0x7
} AoutStreamDacMclk;

typedef enum {
	DAC_BCLK_32FS = 0x0,
	DAC_BCLK_48FS = 0x1,
	DAC_BCLK_64FS = 0x2
} AoutStreamDacBclk;

typedef enum {
	DAC_FORMAT_I2S   = 0x0,
	DAC_FORMAT_LEFT  = 0x1,
	DAC_FORMAT_RIGHT = 0x2
} AoutStreamDacFormat;

typedef enum {
	STREAM_PTS_NORMAL = 0,
	STREAM_PTS_SKIP,
	STREAM_PTS_REPEAT,
	STREAM_PTS_FREERUN
} AoutStreamSyncState;

typedef struct {
	union {
		AoutStreamI2SSource  i2s_0_1;
		AoutStreamSpd2Source spd_2;
		AoutStreamSpd3Source spd_3;
	};
} AoutStreamSelSource;

typedef struct {
	struct gxfifo *nodeFifo;
	int           bufferId;
	unsigned int  bufferAddr;
	unsigned int  bufferSize;//每个通道的大小
} AoutStreamBuffer;

typedef struct {
	unsigned int             recoveryStc;
	unsigned int             syncStc;
	unsigned int             lowToleranceMs;
	unsigned int             highToleranceMs;
	AoutStreamSyncState      state;
	int                      delay_ms;
	int                      pts;
} AoutStreamSync;

typedef struct {
	unsigned int             playFrameCnt;
	unsigned int             loseFrameCnt;
} AoutStreamFrame;

typedef struct {
	unsigned int             vol;
	unsigned int             rightNow;
	unsigned int             step;
} AoutStreamVol;

typedef struct {
	AoutStreamChannel lChannel;
	AoutStreamChannel rChannel;
	AoutStreamChannel cChannel;
	AoutStreamChannel ls1Channel;
	AoutStreamChannel rs1Channel;
	AoutStreamChannel ls2Channel;
	AoutStreamChannel rs2Channel;
	AoutStreamChannel csChannel;
} AoutStreamTrack;

typedef struct {
	unsigned int             freq;
	unsigned int             channels;
	unsigned int             bits;
	unsigned int             interlace;
	unsigned int             endian;
} AoutStreamSample;

typedef struct {
	AoutStreamSrc            src;
	AoutStreamBuffer         buf;
	AoutStreamSync           sync;
	AoutStreamSample         sample;
	AoutStreamFrame          frame;
	GxAudioFrameNode         playNode;
	unsigned int             playRepeat;
	unsigned int             firstFreeFrames;
	unsigned int             hungry;
	gx_sem_id                semId;
	unsigned int             overFrame;
	unsigned int             processVol;
	void                     *header;
	unsigned int             pauseFrame;
	unsigned int             debug;
} AoutStreamBody;

typedef struct {
	gx_sem_id                semId;
	AoutStreamVol            targetVol; //0 ~ 100 volume
	unsigned int             mute_status;
	AoutStreamTrack          track_status;
	AoutStreamBody           *body[MAX_STREAM_BODY];
	AoutStreamSpd2Source     spd2Source;
	AoutStreamI2SSource      i2sSource;
	unsigned int             i2sSampleFre;
	unsigned int             spd2SampleFre;
	unsigned int             spd3SampleFre;
	unsigned int             hdmiSelChange;
	AoutStreamSrc            hdmiSelSrc;
	unsigned int             hdmiMuteDo;
	unsigned int             hdmiMuteFrames;
	unsigned int             spdifSelChange;
	AoutStreamSrc            spdifSelSrc;
	unsigned int             src1Skip;
	unsigned int             hdmiSampleFre;
	unsigned int             hdmiChannels;
	unsigned int            *vol_table;
	AoutStreamMuteMap        cvbsMuteMap;
	AoutStreamMuteMap        hdmiMuteMap;
} AoutStreamHeader;

typedef struct {
	unsigned int i2sAddr_0;
	unsigned int i2sAddr_1;
	unsigned int sdcAddr_0;
	unsigned int sdcAddr_1;
	unsigned int spdAddr_0;
	unsigned int spdAddr_1;
	unsigned int runClock;
} AoutStreamResource;

extern int stream_set_resource(AoutStreamResource *res);

extern AoutStreamHeader* stream_init     (void);
extern void              stream_uninit   (AoutStreamHeader *header);

extern int stream_reset          (AoutStreamHeader *header);
extern int stream_interrupt      (AoutStreamHeader *header);
extern int stream_set_mute       (AoutStreamHeader *header, int  mute);
extern int stream_get_mute       (AoutStreamHeader *header, int *mute);
extern int stream_set_mute_now   (AoutStreamHeader *header, int  mute);
extern int stream_set_voltable   (AoutStreamHeader *header, unsigned int idx);
extern int stream_set_volume     (AoutStreamHeader *header, AoutStreamVol *vol);
extern int stream_get_volume     (AoutStreamHeader *header, AoutStreamVol *vol);
extern int stream_drop_volume    (AoutStreamHeader *header);
extern int stream_rise_volume    (AoutStreamHeader *header);
extern int stream_set_track      (AoutStreamHeader *header, AoutStreamTrack *track);
extern int stream_set_hdmi_port  (AoutStreamHeader *header, unsigned int samplefre, unsigned int channels);
extern int stream_set_source     (AoutStreamHeader *header, AoutStreamI2SSource i2s_source);
extern int stream_config_src     (AoutStreamHeader *header, AoutStreamSrc src);
extern int stream_config_port    (AoutStreamHeader *header, AoutStreamPort port);
extern int stream_port_select_src(AoutStreamHeader *header, AoutStreamPort port, AoutStreamSrc src);
extern int stream_power_mute     (AoutStreamHeader *header, int mute);
extern int stream_set_i2s_work   (AoutStreamHeader *header);
extern int stream_turn_port      (AoutStreamHeader *header, AoutStreamPort port, int onoff);

extern AoutStreamBody* stream_open (AoutStreamSrc src);
extern void            stream_close(AoutStreamBody *body);

extern int stream_config_buffer (AoutStreamBody *body, AoutStreamBuffer *buf);
extern int stream_write_callback(AoutStreamBody *body, int overflow);
extern int stream_start         (AoutStreamBody *body);
extern int stream_stop          (AoutStreamBody *body);
extern int stream_pause         (AoutStreamBody *body);
extern int stream_resume        (AoutStreamBody *body);
extern int stream_set_sync      (AoutStreamBody *body, AoutStreamSync *sync);
extern int stream_set_delay     (AoutStreamBody *body, AoutStreamSync *sync);
extern int stream_set_recovery  (AoutStreamBody *body, AoutStreamSync *sync);
extern int stream_get_recovery  (AoutStreamBody *body, AoutStreamSync *sync);
extern int stream_get_pts       (AoutStreamBody *body, AoutStreamSync *sync);
extern int stream_get_frame     (AoutStreamBody *body, AoutStreamFrame *frame);

#endif
