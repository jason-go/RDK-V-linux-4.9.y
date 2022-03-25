#include "kernelcalls.h"
#include "gxav_event_type.h"
#include "gxav_common.h"
#include "include/audio_common.h"
#include "aout.h"
#include "log_printf.h"

#define CHECK_OPS_NULL(aout, ops) do {                                     \
	if ((aout == NULL) || (ops == NULL)) {                                 \
		gxlog_e(LOG_AOUT, "%s %d\n", __func__, __LINE__);  \
		return -1;                                                         \
	}                                                                      \
} while(0)

static struct audioout_hal_ops* audioout_ops = NULL;
static struct gxav_audioout gx3201_audioout[AUDIOOUT_MAX] = {{-1}};
static struct gxav_audioout* gxav_audioout_create_instance(int sub)
{
	unsigned int i = 0;
	struct gxav_audioout *audioout = gx3201_audioout;

	if ((sub >= AUDIOOUT_MAX) || (audioout == NULL)) {
		return NULL;
	}

	while (i < AUDIOOUT_MAX) {
		if ((audioout->out_id == sub) && (audioout->out_used == USED)) {
			return NULL;
		}
		i++;
		audioout++;
	}

	i = 0;
	audioout = gx3201_audioout;

	while (i < AUDIOOUT_MAX) {
		if (audioout->out_used == UNUSED)
		{
			audioout->out_id = sub;
			audioout->out_used = USED;
			break;
		}
		i++;
		audioout++;
	}

	return (AUDIOOUT_MAX == i) ? (NULL) : (audioout);
}

struct gxav_audioout* gxav_audioout_find_instance(int sub)
{
	unsigned int i = 0;

	struct gxav_audioout *audioout = gx3201_audioout;

	if ((sub >= AUDIOOUT_MAX) || (audioout == NULL)) {
		return NULL;
	}

	while (i < AUDIOOUT_MAX) {
		if (audioout->out_id == sub) {
			break;
		}
		i++;
		audioout++;
	}

	return (AUDIOOUT_MAX == i) ? (NULL) : (audioout);
}

static int gxav_audioout_delete_instance(struct gxav_audioout* audioout)
{
	if (audioout == NULL){
		return -1;
	}

	audioout->out_id = -1;
	audioout->out_used = UNUSED;
	audioout->i2s_pcm[0].pcm_in_id = -1;
	audioout->i2s_pcm[1].pcm_in_id = -1;
	audioout->spd_src2.enc_in_id    = -1;
	audioout->spd_src3.enc_in_id   = -1;
	audioout->ops = NULL;

	return 0;
}

int gxav_audioout_init(struct gxav_module_ops* ops)
{
	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout_ops = ops->priv;
	audioout_ops->init();

	return 0;
}

int gxav_audioout_uninit(void)
{
	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	if (audioout_ops) {
		audioout_ops->uninit();
		audioout_ops = NULL;
	}

	return 0;
}

int gxav_audioout_open(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_create_instance(id);
	CHECK_OPS_NULL(audioout, audioout_ops);

	audioout->debug.value = 0;
	audioout->i2s_pcm[0].pcm_in_id = -1;
	audioout->i2s_pcm[1].pcm_in_id = -1;
	audioout->spd_src2.enc_in_id   = -1;
	audioout->spd_src3.enc_in_id   = -1;
	audioout->indata_type = 0;
	audioout->source_type = 0;
	audioout->update = 0;
	audioout->out_state = AUDIOOUT_STOPED;
	audioout->ops = audioout_ops;

	if ((audioout->ops)&&(audioout->ops->open)) {
		return audioout->ops->open(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_close(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->close) {
		audioout->ops->close(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	gxav_audioout_delete_instance(audioout);

	return 0;
}

int gxav_audioout_link(int id, struct audioout_fifo *fifo)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->link) {
		return audioout->ops->link(audioout, fifo);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_unlink(int id, struct audioout_fifo *fifo)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->unlink) {
		return audioout->ops->unlink(audioout, fifo);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_config_port(int id, GxAudioOutProperty_ConfigPort *config)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->config_port) {
		return audioout->ops->config_port(audioout, config);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_config_source(int id, GxAudioOutProperty_ConfigSource *source)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->config_source) {
		return audioout->ops->config_source(audioout, source);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_config_sync(int id, GxAudioOutProperty_ConfigSync *sync)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->config_sync) {
		return audioout->ops->config_sync(audioout, sync);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_run(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->run) {
		return audioout->ops->run(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_stop(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->stop) {
		return audioout->ops->stop(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_pause(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->pause) {
		return audioout->ops->pause(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_resume(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->resume) {
		return audioout->ops->resume(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_mute(int id, GxAudioOutProperty_Mute *mute)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d [mute: %d]\n", __func__, __LINE__, mute->mute);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_mute) {
		return audioout->ops->set_mute(audioout, mute);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}


int gxav_audioout_get_mute(int id, GxAudioOutProperty_Mute *mute)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->get_mute) {
		return audioout->ops->get_mute(audioout, mute);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}


int gxav_audioout_channel(int id,  GxAudioOutProperty_Channel *channel)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_channel) {
		return audioout->ops->set_channel(audioout, channel);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_set_port(int id, GxAudioOutProperty_SetPort *port)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_port) {
		return audioout->ops->set_port(audioout, port);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_turn_port(int id, GxAudioOutProperty_TurnPort *port)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->turn_port) {
		return audioout->ops->turn_port(audioout, port);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_set_voltable(int id,  GxAudioOutProperty_VolumeTable *table)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_voltable) {
		return audioout->ops->set_voltable(audioout, table);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_set_volume(int id,  GxAudioOutProperty_Volume *volume)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_volume) {
		return audioout->ops->set_volume(audioout, volume);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_update(int id)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	audioout->update = 1;

	return 0;
}

int gxav_audioout_state(int id,   GxAudioOutProperty_State *state)
{
	struct gxav_audioout *audioout = NULL;

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->get_state) {
		return audioout->ops->get_state(audioout, state);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_mixpcm(int id,   GxAudioOutProperty_PcmMix *config)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->mix_pcm) {
		return audioout->ops->mix_pcm(audioout, config);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_speed(int id,   GxAudioOutProperty_Speed *speed)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_speed) {
		return audioout->ops->set_speed(audioout, speed);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_pts(int id,   GxAudioOutProperty_Pts *pts)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->get_pts) {
		return audioout->ops->get_pts(audioout, pts);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_ptsoffset(int id,   GxAudioOutProperty_PtsOffset *pts)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_ptsoffset) {
		return audioout->ops->set_ptsoffset(audioout, pts);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_i2s_irq(int irq,int (*callback)(unsigned int event,void *priv),void *priv)
{
	int event_mask = 0;
	struct gxav_audioout *audioout = gxav_audioout_find_instance(0);

	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->irq) {
		event_mask = audioout->ops->irq(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (event_mask & EVENT_AUDIOOUT_FINDSYNC) {
		if(callback)
			callback(EVENT_AUDIOOUT_FINDSYNC, priv);
	}
	return 0;
}

int gxav_audioout_spdif_irq(int irq,int (*callback)(unsigned int event,void *priv),void *priv)
{
	int event_mask = 0;
	struct gxav_audioout *audioout = gxav_audioout_find_instance(0);

	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops && audioout->ops->irq) {
		event_mask = audioout->ops->irq(audioout);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if (event_mask & EVENT_AUDIOOUT_FINDSYNC) {
		if(callback)
			callback(EVENT_AUDIOOUT_FINDSYNC,priv);
	}
	return 0;
}

int gxav_audioout_samplefre_index(unsigned int samplefre)
{
	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	switch(samplefre)
	{
	case 44100:
		return AUDIO_SAMPLE_FRE_44KDOT1HZ;
	case 48000:
		return AUDIO_SAMPLE_FRE_48KHZ;
	case 32000:
		return AUDIO_SAMPLE_FRE_32KHZ;
	case 22050:
		return AUDIO_SAMPLE_FRE_22KDOT05HZ;
	case 24000:
		return AUDIO_SAMPLE_FRE_24KHZ;
	case 16000:
		return AUDIO_SAMPLE_FRE_16KHZ;
	case 96000:
		return AUDIO_SAMPLE_FRE_96KHZ;
	case 88200:
		return AUDIO_SAMPLE_FRE_88KDOT2HZ;
	case 128000:
		return AUDIO_SAMPLE_FRE_128KHZ;
	case 176400:
		return AUDIO_SAMPLE_FRE_176KDOT4HZ;
	case 192000:
		return AUDIO_SAMPLE_FRE_192KHZ;
	case 64000:
		return AUDIO_SAMPLE_FRE_64KHZ;
	case 12000:
		return AUDIO_SAMPLE_FRE_12KHZ;
	case 11025:
		return AUDIO_SAMPLE_FRE_11KDOT025HZ;
	case 9600:
		return AUDIO_SAMPLE_FRE_9KDOT6HZ;
	case 8000:
		return AUDIO_SAMPLE_FRE_8KHZ;
	default:
		gxlog_w(LOG_AOUT, "%s %d (samplefre %d)\n", __func__, __LINE__, samplefre);
		return AUDIO_SAMPLE_FRE_44KDOT1HZ;
	}
}

int gxav_audioout_set_event(int id, GxAvAudioDataEvent data_event)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_event) {
		return audioout->ops->set_event(audioout, data_event);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_power_mute(int id, GxAudioOutProperty_PowerMute *mute)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->power_mute) {
		return audioout->ops->power_mute(audioout, mute);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_set_dump(int id, int enable)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->set_dump) {
		return audioout->ops->set_dump(audioout, enable);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_get_dump(int id, GxAvDebugDump *dump)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	if (audioout->ops->get_dump) {
		return audioout->ops->get_dump(audioout, dump);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audioout_set_debug(int id, GxAudioOutProperty_Debug *debug)
{
	struct gxav_audioout *audioout = NULL;

	gxlog_d(LOG_AOUT, "%s %d\n", __func__, __LINE__);

	audioout = gxav_audioout_find_instance(id);
	CHECK_OPS_NULL(audioout, audioout->ops);

	audioout->debug.value = debug->value;
	if (audioout->ops->set_debug) {
		return audioout->ops->set_debug(audioout, debug);
	} else {
		gxlog_w(LOG_AOUT, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	return 0;
}
