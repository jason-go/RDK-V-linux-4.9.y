#include "kernelcalls.h"
#include "gxav_event_type.h"
#include "gxav_common.h"
#include "gxav_firmware.h"
#include "avcore.h"
#include "include/audio_common.h"
#include "gxav_audiodec_propertytypes.h"
#include "adec.h"
#include "log_printf.h"

#define CHECK_OPS_NULL(adec, ops) do {                                     \
	if ((adec == NULL) || (ops == NULL)) {                                 \
		gxlog_e(LOG_ADEC, "%s %d\n", __func__, __LINE__);  \
		return -1;                                                         \
	}                                                                      \
} while(0)

static struct audiodec_hal_ops* audiodec_ops = NULL;
static struct gxav_audiodec gx32xx_audiodec[AUDIO_DECODER_MAX] = { {-1},{-1}};

static struct gxav_audiodec* gxav_audiodec_create_instance(int sub)
{
	unsigned int i = 0;
	struct gxav_audiodec *audiodec = gx32xx_audiodec;

	if ((sub >= AUDIO_DECODER_MAX) || (audiodec == NULL)) {
		return NULL;
	}

	while (i < AUDIO_DECODER_MAX) {
		if ((audiodec->dec_id == sub) && (audiodec->dec_used == USED)) {
			return NULL;
		}
		i++;
		audiodec++;
	}

	i = 0;
	audiodec = gx32xx_audiodec;

	while (i < AUDIO_DECODER_MAX) {
		if (audiodec->dec_used == UNUSED) {

			audiodec->dec_id = sub;
			audiodec->dec_used = USED;
			break;
		}
		i++;
		audiodec++;
	}

	return (AUDIO_DECODER_MAX == i) ? (NULL) : (audiodec);
}

struct gxav_audiodec* gxav_audiodec_find_instance(int sub)
{
	unsigned int i = 0;
	struct gxav_audiodec *audiodec = gx32xx_audiodec;

	if ((sub >= AUDIO_DECODER_MAX) || (audiodec == NULL)) {
		return NULL;
	}

	while (i < AUDIO_DECODER_MAX) {
		if (audiodec->dec_id == sub) {
			break;
		}
		i++;
		audiodec++;
	}

	return (AUDIO_DECODER_MAX == i) ? (NULL) : (audiodec);
}

static int gxav_audiodec_delete_instance(struct gxav_audiodec* audiodec)
{
	if (audiodec == NULL) {
		return -1;
	}

	audiodec->dec_id = -1;
	audiodec->dec_used = UNUSED;
	audiodec->dec_type = -1;
	audiodec->ops = NULL;
	audiodec->dec_state = AUDIODEC_STOPED;

	return 0;
}

int gxav_audiodec_init(struct gxav_module_ops* ops)
{
	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec_ops = ops->priv;
	audiodec_ops->init();

	return 0;
}

int gxav_audiodec_uninit( void )
{
	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	if (audiodec_ops) {
		audiodec_ops->uninit();
		audiodec_ops = NULL;
	}

	return 0;
}

int gxav_audiodec_open(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_create_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec_ops);

	audiodec->debug.value = 0;
	audiodec->dec_type = -1;
	audiodec->update = 0;
	audiodec->dec_state = AUDIODEC_STOPED;
	audiodec->ops = audiodec_ops;
	audiodec->decode_fifo     = NULL;
	audiodec->decode_fifo_num = 0;
	if (audiodec->ops->open) {
		return audiodec->ops->open(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_close(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->close) {
		audiodec->ops->close(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	return gxav_audiodec_delete_instance(audiodec);
}

int gxav_audiodec_link(int id, struct audiodec_fifo *fifo)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->link) {
		return audiodec->ops->link(audiodec, fifo);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_unlink(int id, struct audiodec_fifo *fifo)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->unlink) {
		return audiodec->ops->unlink(audiodec, fifo);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_config(int id, GxAudioDecProperty_Config *config)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->config) {
		return audiodec->ops->config(audiodec, config);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_outinfo(int id, GxAudioDecProperty_OutInfo* outinfo)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_outinfo) {
		return audiodec->ops->get_outinfo(audiodec, outinfo);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_run(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->run) {
		return audiodec->ops->run(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_stop(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->stop) {
		return audiodec->ops->stop(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_pause(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->pause) {
		return audiodec->ops->pause(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_resume(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->resume) {
		return audiodec->ops->resume(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_update(int id)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->update) {
		return audiodec->ops->update(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_write_ctrlinfo(int id, GxAudioDecProperty_ContrlInfo *ctrlinfo)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->write_ctrlinfo) {
		return audiodec->ops->write_ctrlinfo(audiodec, ctrlinfo);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_frameinfo(int id, GxAudioDecProperty_FrameInfo *frame_info)
{
	struct gxav_audiodec *audiodec = NULL;

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_frameinfo) {
		return audiodec->ops->get_frameinfo(audiodec, frame_info);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_state(int id,GxAudioDecProperty_State *state)
{
	struct gxav_audiodec *audiodec = NULL;

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_state) {
		return audiodec->ops->get_state(audiodec, state);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_pts(int id,GxAudioDecProperty_Pts *pts)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_pts) {
		return audiodec->ops->get_pts(audiodec, pts);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_err(int id,GxAudioDecProperty_Errno *err_no)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_error) {
		return audiodec->ops->get_error(audiodec, err_no);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_cap(int id,GxAudioDecProperty_Capability *cap)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_cap) {
		return audiodec->ops->get_cap(cap);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_support_ad(int id,GxAudioDecProperty_SupportAD *sup)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->support_ad) {
		return audiodec->ops->support_ad(sup);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_decode_key(int id, GxAudioDecProperty_DecodeKey *key)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->decode_key) {
		return audiodec->ops->decode_key(audiodec, key);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_get_key(int id, GxAudioDecProperty_GetKey *key)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_key) {
		return audiodec->ops->get_key(audiodec, key);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_boost_volume(int id, GxAudioDecProperty_BoostVolume *boost)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance( id );
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->boost_volume) {
		return audiodec->ops->boost_volume(audiodec, boost);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_irq(int irq,int (*callback)(unsigned int event,void *priv),void *priv)
{
	int event_mask = 0;
	struct gxav_audiodec *audiodec = NULL;

	audiodec = gxav_audiodec_find_instance(0);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->irq) {
		audiodec->inode = priv;
		event_mask = audiodec->ops->irq(audiodec);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}

	if(callback)
		callback(event_mask, priv);

	return 0;
}

char gxav_audiodec_get_dolby_type(void)
{
	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	if (audiodec_ops && audiodec_ops->get_dolbytype) {
		return audiodec_ops->get_dolbytype();
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_get_fifo(int id, GxAvPinId pin_id, void **fifo)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_fifo) {
		return audiodec->ops->get_fifo(audiodec, pin_id, (struct gxfifo **)fifo);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_get_max_channel(int id, unsigned int *max_channel)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_max_channel) {
		return audiodec->ops->get_max_channel(audiodec, max_channel);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_set_volume(int id, int volumedb)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->set_volume) {
		return audiodec->ops->set_volume(audiodec, volumedb);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int  gxav_audiodec_set_ad_ctrlinfo(int id, int enable)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->set_ad_ctrlinfo) {
		return audiodec->ops->set_ad_ctrlinfo(audiodec, enable);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_set_mono(int id, int mono_en)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->set_mono) {
		return audiodec->ops->set_mono(audiodec, mono_en);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_set_pcminfo(int id, GxAudioDecProperty_PcmInfo *info)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->set_pcminfo) {
		return audiodec->ops->set_pcminfo(audiodec, info);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_set_debug(int id, GxAudioDecProperty_Debug *debug)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);

	audiodec->debug.value = debug->value;

	return 0;
}

int gxav_audiodec_set_dump(int id, int enable)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->set_dump) {
		return audiodec->ops->set_dump(audiodec, enable);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}

int gxav_audiodec_get_dump(int id, GxAvDebugDump *dump)
{
	struct gxav_audiodec *audiodec = NULL;

	gxlog_d(LOG_ADEC, "%s %d\n", __func__, __LINE__);

	audiodec = gxav_audiodec_find_instance(id);
	CHECK_OPS_NULL(audiodec, audiodec->ops);

	if (audiodec->ops->get_dump) {
		return audiodec->ops->get_dump(audiodec, dump);
	} else {
		gxlog_w(LOG_ADEC, "%s %d\n", __func__, __LINE__);
		return -1;
	}
}
