#include "kernelcalls.h"
#include "gxav_event_type.h"
#include "gxav_common.h"
#include "avcore.h"
#include "include/audio_common.h"
#include "include/audio_module.h"
#include "aout/aout.h"
#include "sirius_out.h"
#include "sirius_stream.h"
#include "hdmi_hal.h"
#include "sdc_hal.h"
#include "log_printf.h"
#include "sirius_debug.h"

typedef struct {
	AoutStreamSrc     src;
	unsigned int      startUp;
	AoutStreamBody    *body;
	const char        *nameString;
	const char        *dataString;
	GxAvPinId         inputPin[4];
} AoutStreamManager;

static AoutStreamHeader *manager_header = NULL;
//when spdif output pcm, hdmi output ac3/eac3, it will be a problem
static AoutStreamManager manager_body[MAX_STREAM_BODY] = {
	{SRC_R0, 0, NULL, "SRC-0", "pcm", {GXAV_PIN_PCM,   GXAV_PIN_MAX}},
	{SRC_R1, 0, NULL, "SRC-1", "pcm", {GXAV_PIN_ADPCM, GXAV_PIN_MAX}},
	{SRC_R2, 0, NULL, "SRC-2", "ac3", {GXAV_PIN_AC3,   GXAV_PIN_MAX}},
	{SRC_R3, 0, NULL, "SRC-3", "eac3/aac/dts", {GXAV_PIN_EAC3,  GXAV_PIN_AAC, GXAV_PIN_DTS, GXAV_PIN_MAX}}
};
static int aoutStreamOpenCnt = 0;

#ifdef DEBUG_AOUT_RECORD
static unsigned int   record_src         = DEBUG_AOUT_RECORD_SRC;
static unsigned int   record_buffer_size = DEBUG_AOUT_RECORD_SIZE;
static unsigned char *record_buffer      = NULL;
static unsigned int   record_size        = 0;
static unsigned int   record_over_flags  = 1;
#endif

static AoutStreamManager *_search_manager_by_pin(GxAvPinId pinId)
{
	unsigned int i = 0, j = 0;

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		for (j = 0; manager_body[i].inputPin[j] != GXAV_PIN_MAX; j++) {
			if (manager_body[i].inputPin[j] == pinId)
				return &manager_body[i];
		}
	}

	return NULL;
}

static AoutStreamManager *_search_manager_by_id(int id)
{
	return &manager_body[id];
}

static void _printf_aout_hdmi_data(unsigned int outdata_type,
		unsigned indata_type, unsigned int edid_type, unsigned int hdmi_src)
{
	char hdmi_string[128] = {0};

	strcat(hdmi_string, "[HDMI : ");
	switch (outdata_type) {
	case HDMI_SRC_PCM:
		strcat(hdmi_string, "pcm");
		break;
	case HDMI_SRC_AC3:
		strcat(hdmi_string, "dly");
		break;
	case HDMI_SRC_EAC3:
		strcat(hdmi_string, "dly+");
		break;
	case HDMI_SRC_DTS:
		strcat(hdmi_string, "dts");
		break;
	case HDMI_SRC_AAC:
		strcat(hdmi_string, "aac");
		break;
	default:
		strcat(hdmi_string, "not");
		break;
	}
	strcat(hdmi_string, "] (INPUT: ");
	if (indata_type & PCM_TYPE)
		strcat(hdmi_string, "|pcm|");
	if (indata_type & AC3_TYPE)
		strcat(hdmi_string, "|dly|");
	if (indata_type & EAC3_TYPE)
		strcat(hdmi_string, "|dly+|");
	if (indata_type & ADPCM_TYPE)
		strcat(hdmi_string, "|adpcm|");
	if (indata_type & DTS_TYPE)
		strcat(hdmi_string, "|dts|");
	if (indata_type & AAC_TYPE)
		strcat(hdmi_string, "|aac|");
	strcat(hdmi_string, ") (EDID: ");
	if (edid_type & EDID_AUDIO_LINE_PCM)
		strcat(hdmi_string, "|pcm|");
	if (edid_type & EDID_AUDIO_AC3)
		strcat(hdmi_string, "|dly|");
	if (edid_type & EDID_AUDIO_EAC3)
		strcat(hdmi_string, "|dly+|");
	if (edid_type & EDID_AUDIO_DTS)
		strcat(hdmi_string, "|dts|");
	if (edid_type & EDID_AUDIO_AAC)
		strcat(hdmi_string, "|aac|");
	if (edid_type == 0)
		strcat(hdmi_string, "|not|");
	strcat(hdmi_string, ") (MODE: ");
	if ((hdmi_src == SAFE_OUTPUT) ||
			(hdmi_src == PCM_OUTPUT))
		strcat(hdmi_string, "|safe|");
	else
		strcat(hdmi_string, "|norm|");
	strcat(hdmi_string, ")");

	gxlog_i(LOG_AOUT, "%s\n", hdmi_string);
}

static void _printf_aout_spdif_data(unsigned int outdata_type, unsigned indata_type)
{
	char spdif_string[128] = {0};

	strcat(spdif_string, "[SPDIF: ");
	switch (outdata_type) {
	case SPDIF_SRC_PCM:
		strcat(spdif_string, "pcm");
		break;
	case SPDIF_SRC_AC3:
		strcat(spdif_string, "dly");
		break;
	case SPDIF_SRC_EAC3:
		strcat(spdif_string, "dly+");
		break;
	case SPDIF_SRC_DTS:
		strcat(spdif_string, "dts");
		break;
	case SPDIF_SRC_AAC:
		strcat(spdif_string, "aac");
		break;
	default:
		strcat(spdif_string, "not");
		break;
	}
	strcat(spdif_string, "] (INPUT: ");
	if (indata_type & PCM_TYPE)
		strcat(spdif_string, "|pcm|");
	if (indata_type & AC3_TYPE)
		strcat(spdif_string, "|dly|");
	if (indata_type & EAC3_TYPE)
		strcat(spdif_string, "|dly+|");
	if (indata_type & ADPCM_TYPE)
		strcat(spdif_string, "|adpcm|");
	if (indata_type & DTS_TYPE)
		strcat(spdif_string, "|dts|");
	if (indata_type & AAC_TYPE)
		strcat(spdif_string, "|aac|");
	strcat(spdif_string, ")");

	gxlog_i(LOG_AOUT, "%s\n", spdif_string);
}

static int _check_aout_hdmi_data(struct gxav_audioout *audioout)
{
	HDMI_OUTPUT_TYPE outdata_type = HDMI_SRC_NOTANY;
	int edid_audio_codes = gxav_hdmi_audio_codes_get();

	// Already config hdmi, and hdmi plug out(edid unkown)
	if (audioout->hdmi_outdata_type != (unsigned int)(-1)) {
		if (edid_audio_codes == 0)
			return 0;
	}

	if ((edid_audio_codes & EDID_AUDIO_EAC3) &&
			((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)) {
		outdata_type = HDMI_SRC_EAC3;
	} else if ((edid_audio_codes & EDID_AUDIO_AC3) &&
			((audioout->indata_type & AC3_TYPE) == AC3_TYPE)) {
		outdata_type = HDMI_SRC_AC3;
	} else if ((edid_audio_codes & EDID_AUDIO_LINE_PCM) &&
			(audioout->indata_type & PCM_TYPE) == PCM_TYPE) {
		outdata_type = HDMI_SRC_PCM;
	} else if ((edid_audio_codes & EDID_AUDIO_DTS) &&
			(audioout->indata_type & DTS_TYPE) == DTS_TYPE) {
		outdata_type = HDMI_SRC_DTS;
	} else if ((edid_audio_codes & EDID_AUDIO_AAC) &&
			((audioout->indata_type & AAC_TYPE) == AAC_TYPE)) {
		outdata_type = HDMI_SRC_AAC;
	} else if (edid_audio_codes == 0) {
		if (audioout->hdmisrc == SAFE_OUTPUT) {
			if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
				outdata_type = HDMI_SRC_PCM;
		} else {
			if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
				outdata_type = HDMI_SRC_PCM;
			else if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
				outdata_type = HDMI_SRC_EAC3;
			else if ((audioout->indata_type & AC3_TYPE ) == AC3_TYPE)
				outdata_type = HDMI_SRC_AC3;
			else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
				outdata_type = HDMI_SRC_DTS;
			else if ((audioout->indata_type & AAC_TYPE ) == AAC_TYPE)
				outdata_type = HDMI_SRC_AAC;
		}
	}

	if (audioout->hdmi_outdata_type != outdata_type) {
		audioout->hdmi_outdata_type = outdata_type;
		_printf_aout_hdmi_data(audioout->hdmi_outdata_type,
				audioout->indata_type, edid_audio_codes, audioout->hdmisrc);

		if (audioout->hdmi_outdata_type == HDMI_SRC_PCM)
			stream_port_select_src(manager_header, HDMI_PORT, (SRC_R0 | SRC_R1));
		else if (audioout->hdmi_outdata_type == HDMI_SRC_AC3)
			stream_port_select_src(manager_header, HDMI_PORT, SRC_R2);
		else if ((audioout->hdmi_outdata_type == HDMI_SRC_EAC3) ||
				(audioout->hdmi_outdata_type == HDMI_SRC_AAC)   ||
				(audioout->hdmi_outdata_type == HDMI_SRC_DTS))
			stream_port_select_src(manager_header, HDMI_PORT, SRC_R3);
	}

	return 0;
}

static int _check_aout_spdif_data(struct gxav_audioout *audioout)
{
	SPDIF_OUTPUT_TYPE outdata_type = SPDIF_SRC_NOTANY;

	if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE)
		outdata_type = SPDIF_SRC_AC3;
	else if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE)
		outdata_type = SPDIF_SRC_PCM;
	else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE)
		outdata_type = SPDIF_SRC_DTS;
	else if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE)
		outdata_type = SPDIF_SRC_AAC;
	else if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE)
		outdata_type = SPDIF_SRC_EAC3;

	if (audioout->spdif_outdata_type != outdata_type) {
		audioout->spdif_outdata_type = outdata_type;
		_printf_aout_spdif_data(audioout->spdif_outdata_type, audioout->indata_type);

		if (audioout->spdif_outdata_type == SPDIF_SRC_PCM)
			stream_port_select_src(manager_header, SPDIF_PORT, (SRC_R0 | SRC_R1));
		else if (audioout->spdif_outdata_type == SPDIF_SRC_AC3)
			stream_port_select_src(manager_header, SPDIF_PORT, SRC_R2);
		else if ((audioout->spdif_outdata_type == SPDIF_SRC_EAC3) ||
				(audioout->spdif_outdata_type == SPDIF_SRC_AAC)   ||
				(audioout->spdif_outdata_type == SPDIF_SRC_DTS))
			stream_port_select_src(manager_header, SPDIF_PORT, SRC_R3);
	}

	return 0;
}

AoutStreamChannel _check_aout_channel(GxAudioOutProperty_OutData data)
{
	if ((data & AUDIO_CHANNEL_0) == AUDIO_CHANNEL_0)
		return SRC_CHANNEL_0;

	if ((data & AUDIO_CHANNEL_1) == AUDIO_CHANNEL_1)
		return SRC_CHANNEL_1;

	if ((data & AUDIO_CHANNEL_2) == AUDIO_CHANNEL_2)
		return SRC_CHANNEL_2;

	if ((data & AUDIO_CHANNEL_3) == AUDIO_CHANNEL_3)
		return SRC_CHANNEL_2;

	if ((data & AUDIO_CHANNEL_4) == AUDIO_CHANNEL_4)
		return SRC_CHANNEL_4;

	if ((data & AUDIO_CHANNEL_5) == AUDIO_CHANNEL_5)
		return SRC_CHANNEL_5;

	if ((data & AUDIO_CHANNEL_6) == AUDIO_CHANNEL_6)
		return SRC_CHANNEL_6;

	if ((data & AUDIO_CHANNEL_7) == AUDIO_CHANNEL_7)
		return SRC_CHANNEL_7;

	if ((data & AUDIO_CHANNEL_M) == AUDIO_CHANNEL_M)
		return SRC_CHANNEL_M;

	return SRC_CHANNEL_0;
}

static int aout_init(void)
{
	manager_header = stream_init();

	stream_set_i2s_work(manager_header);

	return ((manager_header == NULL) ? -1 : 0);
}

static int aout_uninit(void)
{
	stream_uninit(manager_header);
	manager_header = NULL;

	return 0;
}

static int aout_open(struct gxav_audioout *audioout)
{
	unsigned int i = 0;

	if (aoutStreamOpenCnt == 0) {
		for (i = 0; i < MAX_STREAM_BODY; i++) {
			AoutStreamManager *manager = _search_manager_by_id(i);

			manager->body = stream_open(manager->src);
			manager->body->header = (void *)manager_header;
			manager->body->debug  = 0;
			manager_header->body[i] = manager->body;
		}
	}
	aoutStreamOpenCnt++;
	audioout->indata_type  = 0;
	audioout->out_state = AUDIOOUT_READY;

	return 0;
}

static int aout_stop(struct gxav_audioout *audioout);
static int aout_close(struct gxav_audioout *audioout)
{
	unsigned int i = 0;

	if (audioout->out_state == AUDIOOUT_RUNNING)
		aout_stop(audioout);

	aoutStreamOpenCnt--;
	if (aoutStreamOpenCnt == 0) {
		for (i = 0; i < MAX_STREAM_BODY; i++) {
			AoutStreamManager *manager = _search_manager_by_id(i);

			stream_close(manager->body);
			manager->body = NULL;
			manager_header->body[i] = NULL;
		}
	} else if (aoutStreamOpenCnt < 0) {
		gxlog_w(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		aoutStreamOpenCnt = 0;
	}

	return 0;
}

static int aout_write_callbak(unsigned int id, unsigned int lenx, unsigned int overflow, void *arg)
{
#ifdef DEBUG_AOUT_RECORD
	AoutStreamBody *body = (AoutStreamBody *)arg;

	if (body->src == record_src) {
		if (record_over_flags == 0) {
			if ((record_size + lenx) <= record_buffer_size) {
				unsigned int w_ptr = 0;
				unsigned int virt_s_addr = (unsigned int)gx_phys_to_virt(body->buf.bufferAddr);
				unsigned int virt_e_addr = virt_s_addr + body->buf.bufferSize;

				gx_dcache_inv_range(virt_s_addr, virt_e_addr);
				gxav_sdc_rwaddr_get(body->buf.bufferId, NULL, &w_ptr);
				w_ptr = (w_ptr + body->buf.bufferSize - lenx) % body->buf.bufferSize;

				if ((w_ptr + lenx) > body->buf.bufferSize) {
					unsigned int tmp_addr = w_ptr;
					unsigned int tmp_size = (body->buf.bufferSize - w_ptr);

					gx_memcpy((unsigned char*)record_buffer + record_size,
							(unsigned char *)virt_s_addr + tmp_addr, tmp_size);
					record_size += tmp_size;
					tmp_addr = 0;
					tmp_size = lenx - tmp_size;
					gx_memcpy((unsigned char*)record_buffer + record_size,
							(unsigned char *)virt_s_addr + tmp_addr, tmp_size);
					record_size += tmp_size;
				} else {
					gx_memcpy((unsigned char*)record_buffer + record_size,
							(unsigned char *)virt_s_addr + w_ptr, lenx);
					record_size += lenx;
				}
				gxlog_i(LOG_AOUT, "size: %d\n", record_size);
			} else {
				gxlog_i(LOG_AOUT, "=======> AOUT: addr 0x%x, size %d\n",
						gx_virt_to_phys((unsigned int)record_buffer), record_size);
				record_over_flags = 1;
			}
		}
	}
#endif
	return stream_write_callback((AoutStreamBody *)arg, overflow);
}

static int aout_link_fifo(struct gxav_audioout *audioout, struct audioout_fifo *fifo)
{
	int ret = -1;
	AoutStreamBuffer buf;
	AoutStreamManager *manager = NULL;
	unsigned int max_channel_num = 1;

	if (!aoutStreamOpenCnt) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (fifo == NULL) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	manager = _search_manager_by_pin(fifo->pin_id);
	if (!manager) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ret = gxav_audiodec_get_fifo(((fifo->pin_id == GXAV_PIN_ADPCM) ? 1 : 0), fifo->pin_id, (void**)&buf.nodeFifo);
	if (ret != 0) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if ((fifo->pin_id == GXAV_PIN_ADPCM) || (fifo->pin_id == GXAV_PIN_PCM)) {
		ret = gxav_audiodec_get_max_channel(((fifo->pin_id == GXAV_PIN_ADPCM) ? 1 : 0), &max_channel_num);
		if (ret != 0) {
			gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
			return -1;
		}
	}

	if (buf.nodeFifo == NULL) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	buf.bufferId   = fifo->channel_id;
	buf.bufferAddr = fifo->buffer_start_addr;
	buf.bufferSize = (fifo->buffer_end_addr - fifo->buffer_start_addr + 1)/max_channel_num;

	stream_config_buffer(manager->body, &buf);
	fifo->channel->indata     = (void *)manager->body;
	fifo->channel->incallback = aout_write_callbak;

	return 0;
}

static int aout_unlink_fifo(struct gxav_audioout *audioout, struct audioout_fifo *fifo)
{
	AoutStreamManager *manager = NULL;
	AoutStreamBuffer buf;

	if (!aoutStreamOpenCnt) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (fifo == NULL) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	manager = _search_manager_by_pin(fifo->pin_id);
	if (!manager) {
		gxlog_e(LOG_AOUT, "%s %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	buf.nodeFifo   = NULL;
	buf.bufferId   = -1;
	buf.bufferAddr = 0;
	buf.bufferSize = 0;
	stream_config_buffer(manager->body, &buf);
	fifo->channel->incallback = NULL;
	fifo->channel->indata	  = NULL;

	return 0;
}

static int aout_config_port(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigPort *config)
{
	if ((config->port_type & AUDIO_OUT_I2S_IN_DAC) == AUDIO_OUT_I2S_IN_DAC)
		stream_config_port(manager_header, DAC_PORT);

	if ((config->port_type & AUDIO_OUT_HDMI) == AUDIO_OUT_HDMI) {
		//edid读取不到，hdmi输出机制的选择
		audioout->hdmisrc = config->data_type;
	}

	audioout->spdif_outdata_type = (unsigned int)(-1);
	audioout->hdmi_outdata_type  = (unsigned int)(-1);

	return 0;
}

static int aout_config_source(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigSource *config)
{
	AoutStreamSrc src = 0;

	if (config->audiodec_id == 0) {
		audioout->indata_type &= ADPCM_TYPE;
		audioout->indata_type |= config->indata_type;
	} else
		audioout->indata_type |= ADPCM_TYPE;

	if ((audioout->indata_type & PCM_TYPE) == PCM_TYPE) {
		AoutStreamManager *manager = _search_manager_by_pin(GXAV_PIN_PCM);
		src |= manager->src;
		manager->startUp = 1;
	}

	if ((audioout->indata_type & ADPCM_TYPE) == ADPCM_TYPE) {
		AoutStreamManager *manager = _search_manager_by_pin(GXAV_PIN_ADPCM);
		src |= manager->src;
		manager->startUp = 1;
	}

	if ((audioout->indata_type & AC3_TYPE) == AC3_TYPE) {
		AoutStreamManager *manager = _search_manager_by_pin(GXAV_PIN_AC3);
		src |= manager->src;
		manager->startUp = 1;
	}

	if ((audioout->indata_type & EAC3_TYPE) == EAC3_TYPE) {
		AoutStreamManager *manager = _search_manager_by_pin(GXAV_PIN_EAC3);
		src |= manager->src;
		manager->startUp = 1;
	} else if ((audioout->indata_type & DTS_TYPE) == DTS_TYPE) {
		AoutStreamManager *manager = _search_manager_by_pin(GXAV_PIN_DTS);
		src |= manager->src;
		manager->startUp = 1;
	} else if ((audioout->indata_type & AAC_TYPE) == AAC_TYPE) {
		AoutStreamManager *manager = _search_manager_by_pin(GXAV_PIN_AAC);
		src |= manager->src;
		manager->startUp = 1;
	}

	//sirius hardware bug
	//when src = (SRC_R0 | SRC_R1), and SRC_R1 hasn't data, and  contorl stop mute,
	//it will not sound output
	stream_config_src(manager_header, src);

	return 0;
}

static int aout_config_sync(struct gxav_audioout *audioout, GxAudioOutProperty_ConfigSync *config)
{
	unsigned int i = 0;
	AoutStreamSync sync;

	sync.syncStc         = 1;//config->sync;
	sync.lowToleranceMs  = config->low_tolerance;
	sync.highToleranceMs = config->high_tolerance;

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->body)
			stream_set_sync(manager->body, &sync);
	}

	return 0;
}

static int aout_power_mute(struct gxav_audioout* audioout, GxAudioOutProperty_PowerMute *mute)
{
	if (audioout->out_state == AUDIOOUT_RUNNING)
		aout_stop(audioout);

	stream_power_mute(manager_header, mute->enable);

	return 0;
}

static int aout_turn_port(struct gxav_audioout *audioout, GxAudioOutProperty_TurnPort *port)
{
	if ((port->port_type & AUDIO_OUT_I2S_IN_DAC) == AUDIO_OUT_I2S_IN_DAC) {
		stream_turn_port(manager_header, DAC_PORT, port->onoff);
	}
	if ((port->port_type & AUDIO_OUT_HDMI) == AUDIO_OUT_HDMI) {
		stream_turn_port(manager_header, HDMI_PORT, port->onoff);
	}
	if ((port->port_type & AUDIO_OUT_SPDIF) == AUDIO_OUT_SPDIF) {
		stream_turn_port(manager_header, SPDIF_PORT, port->onoff);
	}

	return 0;
}

static int aout_start(struct gxav_audioout *audioout)
{
	unsigned int i = 0;
	AoutStreamSync sync;

	_check_aout_hdmi_data (audioout);
	_check_aout_spdif_data(audioout);

	sync.recoveryStc = 1;
	stream_set_mute_now(manager_header, 0);

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->startUp) {
			stream_set_recovery(manager->body, &sync);
			stream_start(manager->body);
			sync.recoveryStc = 0;
		}
	}
	audioout->out_state = AUDIOOUT_RUNNING;

	return 0;
}

static int aout_stop(struct gxav_audioout *audioout)
{
	unsigned int i = 0;

	stream_set_mute_now(manager_header, 1);
	stream_drop_volume(manager_header);

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->startUp) {
			stream_stop(manager->body);
			manager->startUp = 0;
		}
	}

	stream_reset       (manager_header);
	stream_set_i2s_work(manager_header);

	gxav_stc_invaild_apts(0);

	audioout->indata_type  = 0;
	audioout->out_state = AUDIOOUT_STOPED;

	return 0;
}

static int aout_pause(struct gxav_audioout *audioout)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->startUp)
			stream_pause(manager->body);
	}
	audioout->out_state = AUDIOOUT_PAUSED;

	return 0;
}

static int aout_resume(struct gxav_audioout *audioout)
{
	unsigned int i = 0;

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->startUp)
			stream_resume(manager->body);
	}
	audioout->out_state = AUDIOOUT_RUNNING;

	return 0;
}

static int aout_set_mute(struct gxav_audioout *audioout, GxAudioOutProperty_Mute *mute)
{
	stream_set_mute(manager_header, mute->mute);

	return 0;
}

static int aout_get_mute(struct gxav_audioout *audioout, GxAudioOutProperty_Mute *mute)
{
	stream_get_mute(manager_header, &mute->mute);

	return 0;
}

static int aout_set_volume_table(struct gxav_audioout *audioout,
		GxAudioOutProperty_VolumeTable *table)
{
	return stream_set_voltable(manager_header, table->table_idx);
}

static int aout_set_volume(struct gxav_audioout *audioout, GxAudioOutProperty_Volume *volume)
{
	AoutStreamVol vol;

	vol.vol      = volume->volume;
	vol.rightNow = volume->right_now;
	vol.step     = (volume->step == 0) ? 1 : volume->step;
	stream_set_volume(manager_header, &vol);

	return 0;
}

static int aout_get_volume(struct gxav_audioout *audioout, GxAudioOutProperty_Volume *volume)
{
	AoutStreamVol vol;

	stream_get_volume(manager_header, &vol);
	volume->volume    = vol.vol;
	volume->right_now = vol.rightNow;
	volume->step      = vol.step;

	return 0;
}

static int aout_set_channel(struct gxav_audioout *audioout, GxAudioOutProperty_Channel *channel)
{
	AoutStreamTrack track;

	if (channel->mono) {
		track.lChannel   = SRC_CHANNEL_0;
		track.rChannel   = SRC_CHANNEL_0;
		track.cChannel   = SRC_CHANNEL_0;
		track.ls1Channel = SRC_CHANNEL_0;
		track.rs1Channel = SRC_CHANNEL_0;
		track.ls2Channel = SRC_CHANNEL_0;
		track.rs2Channel = SRC_CHANNEL_0;
		track.csChannel  = SRC_CHANNEL_0;
		gxav_audiodec_set_mono(0, 1);
	} else {
		track.lChannel   = _check_aout_channel(channel->left);
		track.rChannel   = _check_aout_channel(channel->right);
		track.cChannel   = _check_aout_channel(channel->center);
		track.ls1Channel = _check_aout_channel(channel->left_surround);
		track.rs1Channel = _check_aout_channel(channel->right_surround);
		track.ls2Channel = _check_aout_channel(channel->left_backsurround);
		track.rs2Channel = _check_aout_channel(channel->right_backsurround);
		track.csChannel  = _check_aout_channel(channel->lfe);
		gxav_audiodec_set_mono(0, 0);
	}

	stream_set_track(manager_header, &track);
	return 0;
}

static int aout_set_port(struct gxav_audioout *audioout, GxAudioOutProperty_SetPort *port)
{
	if (port->port_type == AUDIO_OUT_HDMI) {
		unsigned int samplefre = port->hdmi.samplefre;
		unsigned int channels  = port->hdmi.channel;

		samplefre = (samplefre == 0) ? 4800 : samplefre;
		channels  = (channels  == 0) ? 2 : channels;
		stream_set_hdmi_port(manager_header, samplefre, channels);
	}

	return 0;
}

static int aout_get_state(struct gxav_audioout *audioout, GxAudioOutProperty_State *state)
{
	unsigned int i = 0;
	unsigned int playFrameCnt = 0;
	unsigned int loseFrameCnt = 0;
	AoutStreamFrame frame;

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->startUp) {
			stream_get_frame(manager->body, &frame);
			if (frame.playFrameCnt > playFrameCnt) {
				playFrameCnt = frame.playFrameCnt;
				loseFrameCnt = frame.loseFrameCnt;
			}
		}
	}
	state->state = audioout->out_state;
	state->frame_stat.play_frame_cnt = (unsigned long long)playFrameCnt;
	state->frame_stat.lose_sync_cnt  = (unsigned long long)loseFrameCnt;

	return 0;
}

static int aout_mix_pcm(struct gxav_audioout *audioout, GxAudioOutProperty_PcmMix *config)
{
	AoutStreamI2SSource i2s_source;

	if (config->pcm_mix == (MIX_PCM | MIX_ADPCM))
		i2s_source = MIX_0_1_I2S_SEL_SRC_R0_R1;
	else if (config->pcm_mix == MIX_PCM)
		i2s_source = MIX_0_1_I2S_SEL_SRC_R0;
	else if (config->pcm_mix == MIX_ADPCM)
		i2s_source = MIX_0_1_I2S_SEL_SRC_R1;
	else
		i2s_source = MIX_0_1_I2S_SEL_OFF;

	stream_set_source(manager_header, i2s_source);

	return 0;
}

static int aout_set_speed(struct gxav_audioout *audioout, GxAudioOutProperty_Speed *speed)
{
	return 0;
}

static int aout_get_pts(struct gxav_audioout* audioout, GxAudioOutProperty_Pts *pts)
{
	unsigned int i = 0;
	AoutStreamSync sync;

	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->startUp) {
			stream_get_recovery(manager->body, &sync);
			if (sync.recoveryStc) {
				stream_get_pts(manager->body, &sync);
				pts->pts = sync.pts;
				break;
			}
		}
	}

	return 0;
}

static int aout_set_ptsoffset(struct gxav_audioout* audioout, GxAudioOutProperty_PtsOffset *pts)
{
	unsigned int i = 0;
	AoutStreamSync sync;

	sync.delay_ms = pts->offset_ms;
	for (i = 0; i < MAX_STREAM_BODY; i++) {
		AoutStreamManager *manager = _search_manager_by_id(i);

		if (manager->body)
			stream_set_delay(manager->body, &sync);
	}

	return 0;
}

static int aout_interrupt(struct gxav_audioout *audioout)
{
	_check_aout_hdmi_data (audioout);
	_check_aout_spdif_data(audioout);

	stream_interrupt(manager_header);

	return 0;
}

static int aout_set_dump(struct gxav_audioout *audioout, int enable)
{
#ifdef DEBUG_AOUT_RECORD
	if (enable) {
		if (!record_buffer)
			record_buffer = gx_page_malloc(record_buffer_size);
		record_size       = 0;
		record_over_flags = 0;
	} else {
		record_over_flags = 1;
		if (record_buffer) {
			gx_page_free(record_buffer, record_size);
			record_buffer = NULL;
		}
	}
#endif
	return 0;
}

static int aout_get_dump(struct gxav_audioout *audioout, GxAvDebugDump *dump)
{
#ifdef DEBUG_AOUT_RECORD
	dump->status = record_over_flags;
	dump->buffer = record_buffer;
	dump->size   = record_size;
#endif
	return 0;
}

static int aout_set_debug(struct gxav_audioout *audioout, GxAudioOutProperty_Debug *debug)
{
	AoutStreamManager *manager = NULL;

	manager = _search_manager_by_id(0);
	if (manager && manager->body)
		manager->body->debug = (debug->value & AUDIO_OUT_I2S0_PTS_SYNC);

	manager = _search_manager_by_id(1);
	if (manager && manager->body)
		manager->body->debug = (debug->value & AUDIO_OUT_I2S1_PTS_SYNC);

	manager = _search_manager_by_id(2);
	if (manager && manager->body)
		manager->body->debug = (debug->value & AUDIO_OUT_SPD0_PTS_SYNC);

	manager = _search_manager_by_id(3);
	if (manager && manager->body)
		manager->body->debug = (debug->value & AUDIO_OUT_SPD1_PTS_SYNC);

	return 0;
}

static struct audioout_hal_ops aout_ops = {
	.init          = aout_init,
	.uninit        = aout_uninit,
	.open          = aout_open,
	.close         = aout_close,
	.link          = aout_link_fifo,
	.unlink        = aout_unlink_fifo,
	.config_port   = aout_config_port,
	.config_source = aout_config_source,
	.config_sync   = aout_config_sync,
	.power_mute    = aout_power_mute,
	.run           = aout_start,
	.stop          = aout_stop,
	.pause         = aout_pause,
	.resume        = aout_resume,
	.set_mute      = aout_set_mute,
	.get_mute      = aout_get_mute,
	.set_voltable  = aout_set_volume_table,
	.set_volume    = aout_set_volume,
	.get_volume    = aout_get_volume,
	.set_channel   = aout_set_channel,
	.set_port      = aout_set_port,
	.get_state     = aout_get_state,
	.mix_pcm       = aout_mix_pcm,
	.set_speed     = aout_set_speed,
	.get_pts       = aout_get_pts,
	.set_ptsoffset = aout_set_ptsoffset,
	.irq           = aout_interrupt,
	.set_dump      = aout_set_dump,
	.get_dump      = aout_get_dump,
	.set_debug     = aout_set_debug,
	.turn_port     = aout_turn_port,
};

struct gxav_module_ops sirius_audioout_module;
static int aout_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	AoutStreamResource resource;

	resource.i2sAddr_0 = res->regs[0];
	resource.i2sAddr_1 = res->regs[1];
	resource.sdcAddr_0 = res->regs[2];
	resource.sdcAddr_1 = res->regs[3];
	resource.spdAddr_0 = res->regs[4];
	resource.spdAddr_1 = res->regs[5];
	resource.runClock  = res->clks[0];
	stream_set_resource(&resource);

	sirius_audioout_module.irqs[0] = res->irqs[0];
	sirius_audioout_module.irqs[1] = res->irqs[1];
	sirius_audioout_module.irqs[2] = res->irqs[2];
	sirius_audioout_module.interrupts[res->irqs[0]] = gx_audioout_i2s_interrupt;
	sirius_audioout_module.interrupts[res->irqs[1]] = gx_audioout_spdif_interrupt;
	sirius_audioout_module.interrupts[res->irqs[2]] = gx_audioout_spdif_interrupt;

	return 0;
}

struct gxav_module_ops sirius_audioout_module = {
	.module_type  = GXAV_MOD_AUDIO_OUT,
	.count        = 1,
	.irqs         = {6, 7, 8, -1},
	.irq_names    = {"aout_srl", "aout_spd", "aout_ac3"},
	.event_mask   = 0xffffffff,
	.setup        = aout_setup,
	.init         = gx_audioout_init,
	.cleanup      = gx_audioout_uninit,
	.open         = gx_audioout_open,
	.close        = gx_audioout_close,
	.set_property = gx_audioout_set_property,
	.get_property = gx_audioout_get_property,
	.priv = &aout_ops,
};
