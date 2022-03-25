#ifndef __AOUT_H__
#define __AOUT_H__

#include "avcore.h"
#include "gxav_audioout_propertytypes.h"
#include "kernelcalls.h"
#include "fifo.h"
#include "stc_hal.h"

#define AUDIOOUT_MAX (1)

typedef enum
{
	PCM_SKIP_FRAME,
	PCM_DEC_FRAME,
	PCM_FILL_DATA,
	PCM_REAPT_FRAME,

	I2S_PLAY_PCM,
	SPD_PLAY_PCM
}PCM_DataOperate;

typedef enum
{
	SPD_PARSE_FRAME,
	SPD_FILL_DATA,
	SPD_FIND_SYNC,

	SPD_PLAY_CDDATA
}SPD_DataOperate;

typedef enum
{
	CVT_ENC_FRAME,
	CVT_FILL_DATA,

} CVT_DataOperate;

typedef enum {
	HDMI_SRC_PCM = 1,
	HDMI_SRC_AC3,
	HDMI_SRC_EAC3,
	HDMI_SRC_DTS,
	HDMI_SRC_AAC,
	HDMI_SRC_NOTANY
} HDMI_OUTPUT_TYPE;

typedef enum {
	SPDIF_SRC_PCM = 1,
	SPDIF_SRC_AC3,
	SPDIF_SRC_EAC3,
	SPDIF_SRC_DTS,
	SPDIF_SRC_AAC,
	SPDIF_SRC_NOTANY
} SPDIF_OUTPUT_TYPE;

struct pcm_to_i2s {
	unsigned int dec_id;
	unsigned int pcm_in_id;
	unsigned int pcm_in_start_addr;
	unsigned int pcm_in_size;
	unsigned int pcm_hungry;
	unsigned int pcm_repeating;
	unsigned int pcm_sample_data_len;
	unsigned int pcm_play_count;
	unsigned int pcm_add_adpcm;
	PCM_DataOperate  pcm_data_operate;
	GxAudioFrameNode pcm_play_node;
	struct gxfifo*   pcm_fifo;
};

struct enc_to_spd {
	unsigned int dec_id;
	unsigned int enc_in_id;
	unsigned int enc_in_start_addr;
	unsigned int enc_in_size;
	unsigned int enc_repeating;
	unsigned int enc_offset_time;
	unsigned int enc_hungry;
	SPD_DataOperate  enc_data_operate;
	GxAudioFrameNode enc_play_node;
	struct gxfifo*   enc_fifo;
};

typedef union {
	struct {
		unsigned int i2s_clock_index;
		unsigned int i2s_clock_speed;
	}pll;
	struct {
		unsigned int i2s_clock;
		unsigned int spd_clock;
	}dto;
}audio_clock_params;

struct gxav_audioout {
#define UNUSED      0
#define USED        1
	int out_id;
	int out_used;
	unsigned int out_state;

	int multi_i2s;
	int lack_adpcm_count;
	struct pcm_to_i2s   i2s_pcm[2];
	struct enc_to_spd   spd_src2;
	struct enc_to_spd   spd_src3;
	struct pcm_to_i2s   i2s_adpcm;

	unsigned int pcm_ch_value;
	unsigned int spd_ch_value;
	unsigned int spd_ch_active;
	unsigned int pcm_mute_status;
	unsigned int spd_mute_status;
	unsigned int mute_status;
	unsigned int mute_inquire;
	unsigned int unmute_inquire;
	unsigned int unmute_active;
	unsigned int update;
	unsigned int compresstopcm;

	int sync;
	unsigned int low_tolerance;
	unsigned int high_tolerance;
	unsigned int sync_state;

	unsigned int source_type;
	unsigned int indata_type;
	unsigned int hdmisrc_change;
	unsigned int hdmi_outdata_type;
	unsigned int spdsrc_change;
	unsigned int spdif_outdata_type;
	int i2ssrc;
	int spdsrc;
	int hdmisrc;
#define PCM_RECOVERY  0
#define AC3_RECOVERY  1
#define EAC3_RECOVERY 2
#define DTS_RECOVERY  3
#define AAC_RECOVERY  3
	int recovery_src;
	int offset_ms;

	unsigned int volume;
	unsigned int level;
	unsigned int stereo;

	unsigned int pcm_mix;
	unsigned int speed;
	unsigned int pcm_speed_change;
	unsigned int pcm_speed_mute;
	unsigned int spd_speed_change;
	unsigned int max_channel_num;
	int play_pcm_pts;
	int sync_stc;
	audio_clock_params clock;

	GxAvFrameStat stat;

	struct audioout_hal_ops *ops;
	GxAvAudioDataEvent data_event;

	unsigned int             hdmiMuteDo;
	unsigned int             hdmiMuteFrames;
	GxAudioOutProperty_Debug  debug;
};

struct audioout_fifo {
	unsigned char   channel_id;
	unsigned char   buffer_id;
	unsigned int    buffer_start_addr;
	unsigned int    buffer_end_addr;

	unsigned char   channel_pts_id;
	unsigned char   pts_buffer_id;
	unsigned int    pts_start_addr;
	unsigned int    pts_end_addr;

	struct gxav_channel* channel;
	int             direction;
	int             pin_id;
};

struct audioout_hal_ops {
	int (*init)         (void);
	int (*uninit)       (void);
	int (*open)         (struct gxav_audioout *audioout);
	int (*close)        (struct gxav_audioout *audioout);
	int (*link)         (struct gxav_audioout *audioout, struct audioout_fifo *fifo);
	int (*unlink)       (struct gxav_audioout *audioout, struct audioout_fifo *fifo);
	int (*config_port)  (struct gxav_audioout *audioout, GxAudioOutProperty_ConfigPort   *config);
	int (*config_source)(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigSource *source);
	int (*config_sync)  (struct gxav_audioout *audioout, GxAudioOutProperty_ConfigSync   *sync);
	int (*power_mute)   (struct gxav_audioout *audioout, GxAudioOutProperty_PowerMute    *mute);
	int (*run)          (struct gxav_audioout *audioout);
	int (*stop)         (struct gxav_audioout *audioout);
	int (*pause)        (struct gxav_audioout *audioout);
	int (*resume)       (struct gxav_audioout *audioout);
	int (*set_mute)     (struct gxav_audioout *audioout, GxAudioOutProperty_Mute *mute);
	int (*get_mute)     (struct gxav_audioout *audioout, GxAudioOutProperty_Mute *mute);
	int (*set_voltable) (struct gxav_audioout *audioout, GxAudioOutProperty_VolumeTable *table);
	int (*set_volume)   (struct gxav_audioout *audioout, GxAudioOutProperty_Volume *volume);
	int (*get_volume)   (struct gxav_audioout *audioout, GxAudioOutProperty_Volume *volume);
	int (*set_channel)  (struct gxav_audioout *audioout, GxAudioOutProperty_Channel *channel);
	int (*set_port)     (struct gxav_audioout *audioout, GxAudioOutProperty_SetPort *port);
	int (*get_state)    (struct gxav_audioout *audioout, GxAudioOutProperty_State *state);
	int (*mix_pcm)      (struct gxav_audioout *audioout, GxAudioOutProperty_PcmMix *mix);
	int (*set_speed)    (struct gxav_audioout *audioout, GxAudioOutProperty_Speed *speed);
	int (*get_pts)      (struct gxav_audioout *audioout, GxAudioOutProperty_Pts *pts);
	int (*set_ptsoffset)(struct gxav_audioout *audioout, GxAudioOutProperty_PtsOffset *pts);
	int (*set_event)    (struct gxav_audioout *audioout, GxAvAudioDataEvent data_event);
	int (*irq)(struct gxav_audioout *audioout);
	int (*set_dump)     (struct gxav_audioout *audioout, int enable);
	int (*get_dump)     (struct gxav_audioout *audioout, GxAvDebugDump *dump);
	int (*set_debug)    (struct gxav_audioout *audioout, GxAudioOutProperty_Debug *debug);
	int (*turn_port)    (struct gxav_audioout *audioout, GxAudioOutProperty_TurnPort *port);
};

extern struct gxav_audioout* gxav_audioout_find_instance(int sub);

extern int gxav_audioout_init  (struct gxav_module_ops* ops);
extern int gxav_audioout_uninit(void);
extern int gxav_audioout_open  (int id);
extern int gxav_audioout_close (int id);
extern int gxav_audioout_link  (int id, struct audioout_fifo *fifo);
extern int gxav_audioout_unlink(int id, struct audioout_fifo *fifo);
extern int gxav_audioout_config_source(int id, GxAudioOutProperty_ConfigSource *source);
extern int gxav_audioout_config_port  (int id, GxAudioOutProperty_ConfigPort *port);
extern int gxav_audioout_config_sync  (int id, GxAudioOutProperty_ConfigSync *sync);
extern int gxav_audioout_power_mute   (int id, GxAudioOutProperty_PowerMute  *mute);
extern int gxav_audioout_set_voltable (int id, GxAudioOutProperty_VolumeTable *table);
extern int gxav_audioout_set_volume   (int id, GxAudioOutProperty_Volume *volume);
extern int gxav_audioout_run      (int id);
extern int gxav_audioout_stop     (int id);
extern int gxav_audioout_pause    (int id);
extern int gxav_audioout_resume   (int id);
extern int gxav_audioout_mute     (int id, GxAudioOutProperty_Mute *mute);
extern int gxav_audioout_channel  (int id, GxAudioOutProperty_Channel *channel);
extern int gxav_audioout_set_port (int id, GxAudioOutProperty_SetPort *port);
extern int gxav_audioout_turn_port(int id, GxAudioOutProperty_TurnPort *port);
extern int gxav_audioout_update   (int id);
extern int gxav_audioout_state    (int id, GxAudioOutProperty_State *state);
extern int gxav_audioout_mixpcm   (int id, GxAudioOutProperty_PcmMix *mix);
extern int gxav_audioout_speed    (int id, GxAudioOutProperty_Speed *speed);
extern int gxav_audioout_pts      (int id, GxAudioOutProperty_Pts *pts);
extern int gxav_audioout_ptsoffset(int id, GxAudioOutProperty_PtsOffset *pts);
extern int gxav_audioout_get_mute (int id, GxAudioOutProperty_Mute *mute);
extern int gxav_audioout_spdif_irq(int irq,int (*callback)(unsigned int event,void *priv),void *priv);
extern int gxav_audioout_i2s_irq  (int irq,int (*callback)(unsigned int event,void *priv),void *priv);
extern int gxav_audioout_set_dump (int id, int enable);
extern int gxav_audioout_get_dump (int id, GxAvDebugDump *dump);
extern int gxav_audioout_set_debug(int id, GxAudioOutProperty_Debug *debug);

#endif

