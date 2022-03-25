#include "hdmi_hal.h"

#if defined CONFIG_AV_TEE_MODULE_HDMI && defined CONFIG_OPTEE
#include "hal/property.h"

int gxav_hdmi_open(unsigned int delay_ms)
{
	struct avhal_hdmi_property_open data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_OPEN, &data, sizeof(data));

	return data.ret;
}

int gxav_hdmi_detect_hotplug(void)
{
	struct avhal_hdmi_property_detect_hotplug data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_DETECT_HOTPLUG, &data, sizeof(data));

	return data.ret;
}

int gxav_hdmi_read_edid(struct videoout_hdmi_edid *edid)
{
	struct avhal_hdmi_property_read_edid data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_READ_EDID, &data, sizeof(data));
	memcpy(edid, &data.edid, sizeof(struct videoout_hdmi_edid));

	return data.ret;
}

unsigned int gxav_hdmi_audio_codes_get(void)
{
	struct avhal_hdmi_property_audio_codes_get data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_AUDIO_CODES_GET, &data, sizeof(data));

	return data.ret;
}

void gxav_hdmi_audioout_set(unsigned int audio_source)
{
	struct avhal_hdmi_property_audioout_set data = { .source = audio_source };

	gxav_tee_ioctl(AVHAL_HDMI_AUDIOOUT_SET, &data, sizeof(data));

	return;
}

void gxav_hdmi_audioout_mute(unsigned int enable)
{
	struct avhal_hdmi_property_audioout_mute data = { .enable = enable };

	gxav_tee_ioctl(AVHAL_HDMI_AUDIOOUT_MUTE , &data, sizeof(data));

	return;
}

void gxav_hdmi_av_mute(unsigned int enable)
{
	struct avhal_hdmi_property_av_mute data = { .enable = enable };

	gxav_tee_ioctl(AVHAL_HDMI_AV_MUTE, &data, sizeof(data));

	return;
}

void gxav_hdmi_powerdown(unsigned int enable)
{
	struct avhal_hdmi_property_powerdown data = { .enable = enable };

	gxav_tee_ioctl(AVHAL_HDMI_POWERDOWN, &data, sizeof(data));

	return;
}

void gxav_hdmi_audioout_reset(unsigned int flag)
{
	struct avhal_hdmi_property_audioout_reset data = { .flag = flag };

	gxav_tee_ioctl(AVHAL_HDMI_AUDIOOUT_RESET, &data, sizeof(data));

	return;
}

void gxav_hdmi_ready(unsigned int flag)
{
	struct avhal_hdmi_property_ready data = { .flag = flag };

	gxav_tee_ioctl(AVHAL_HDMI_READY, &data, sizeof(data));

	return;
}

void gxav_hdmi_audiosample_change(GxAudioSampleFre samplefre, unsigned int cnum)
{
	struct avhal_hdmi_property_audiosample_change data = { .samplefre = samplefre, .cnum = cnum };

	gxav_tee_ioctl(AVHAL_HDMI_AUDIOSAMPLE_CHANGE, &data, sizeof(data));

	return;
}

void gxav_hdmi_acr_enable(int enable)
{
	struct avhal_hdmi_property_acr_enable data = { .enable = enable };

	gxav_tee_ioctl(AVHAL_HDMI_ACR_ENABLE, &data, sizeof(data));

	return;
}

void gxav_hdmi_black_enable(int enable)
{
	struct avhal_hdmi_property_black_enable data = { .enable = enable };

	gxav_tee_ioctl(AVHAL_HDMI_BLACK_ENABLE, &data, sizeof(data));

	return;
}

void gxav_hdmi_videoout_set(GxVideoOutProperty_Mode vout_mode)
{
	struct avhal_hdmi_property_videoout_set data = { .mode = vout_mode };

	gxav_tee_ioctl(AVHAL_HDMI_VIDEOOUT_SET, &data, sizeof(data));

	return;
}

GxVideoOutProperty_Mode gxav_hdmi_videoout_get(void)
{
	struct avhal_hdmi_property_videoout_get data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_VIDEOOUT_GET, &data, sizeof(data));

	return data.mode;
}

int gxav_hdmi_hdcp_enable_auth(void)
{
	//gxav_hdmi_videoout_set(mode);
	return 0;
#if 0
	struct avhal_hdmi_property_hdcp_enable_auth data = { .mode = mode };

	gxav_tee_ioctl(AVHAL_HDMI_HDCP_ENABLE_AUTH, &data, sizeof(data));

	return data.ret;
#endif
}

int gxav_hdmi_hdcp_config(GxVideoOutProperty_HdmiHdcpConfig *config)
{
	return 0;
}

int gxav_hdmi_hdcp_disable_auth(void)
{
	struct avhal_hdmi_property_hdcp_disable_auth data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_HDCP_DISABLE_AUTH, &data, sizeof(data));

	return data.ret;
}

void gxav_hdmi_enable(int enable)
{
	struct avhal_hdmi_property_enable data = { .enable = enable };

	gxav_tee_ioctl(AVHAL_HDMI_ENABLE, &data, sizeof(data));

	return;
}

int gxav_hdmi_set_brightness(int b, int c, int s)
{
	struct avhal_hdmi_property_set_brightness data = { .b = b, .c = c, .s = s };

	gxav_tee_ioctl(AVHAL_HDMI_SET_BRIGHTNESS, &data, sizeof(data));

	return data.ret;
}

int gxav_hdmi_set_saturation(int b, int c, int s)
{
	struct avhal_hdmi_property_set_saturation data = { .b = b, .c = c, .s = s };

	gxav_tee_ioctl(AVHAL_HDMI_SET_SATURATION, &data, sizeof(data));

	return data.ret;
}

int gxav_hdmi_set_contrast(int b, int c, int s)
{
	struct avhal_hdmi_property_set_contrast data = { .b = b, .c = c, .s = s };

	gxav_tee_ioctl(AVHAL_HDMI_SET_CONTRAST, &data, sizeof(data));

	return data.ret;
}

void gxav_hdmi_get_version(GxVideoOutProperty_HdmiVersion *version)
{
	struct avhal_hdmi_property_open data = { 0 };

	gxav_tee_ioctl(AVHAL_HDMI_OPEN, &data, sizeof(data));

	return;
}

int  gxav_hdmi_set_encoding_out(GxAvHdmiEncodingOut encoding)
{
	struct avhal_hdmi_property_set_encoding_out data = { .encoding = encoding };

	gxav_tee_ioctl(AVHAL_HDMI_SET_ENCODING_OUT, &data, sizeof(data));

	return data.ret;
}

int gxav_hdmi_set_cgmsa_permission(int permission)
{
	struct avhal_hdmi_property_set_cgmsa_permission data = { .permission = permission};

	gxav_tee_ioctl(AVHAL_HDMI_SET_CGMSA_PERMISSION, &data, sizeof(data));

	return data.ret;
}

#else

static struct gxav_hdmi_module* _hdmi_ops = NULL;

int gxav_hdmi_init(struct gxav_module_ops* ops)
{
	_hdmi_ops = ops->priv;

	if (_hdmi_ops && _hdmi_ops->init)
		return _hdmi_ops->init();

	return 0;
}

int gxav_hdmi_uninit(void)
{
	if (_hdmi_ops && _hdmi_ops->uninit)
		return _hdmi_ops->uninit();
	_hdmi_ops = NULL;

	return 0;
}

int gxav_hdmi_open(unsigned delay_ms)
{
	if (_hdmi_ops && _hdmi_ops->open) {
		return _hdmi_ops->open(delay_ms);
	}
	return 0;
}

int gxav_hdmi_detect_hotplug(void)
{
	if (_hdmi_ops && _hdmi_ops->detect_hotplug) {
		return _hdmi_ops->detect_hotplug();
	}
	return 0;
}

int gxav_hdmi_read_edid(struct videoout_hdmi_edid *edid)
{
	if (_hdmi_ops && _hdmi_ops->read_edid) {
		return _hdmi_ops->read_edid(edid);
	}
	return 0;
}

unsigned int gxav_hdmi_audio_codes_get(void)
{
	if (_hdmi_ops && _hdmi_ops->audio_codes_get) {
		return _hdmi_ops->audio_codes_get();
	}
	return 0;
}

void gxav_hdmi_clock_set(GxVideoOutProperty_Mode mode)
{
	if (_hdmi_ops && _hdmi_ops->clock_set) {
		_hdmi_ops->clock_set(mode);
	}
}

void gxav_hdmi_audioout_set(unsigned int audio_source)
{
	if (_hdmi_ops && _hdmi_ops->audioout_set) {
		_hdmi_ops->audioout_set(audio_source);
	}
}

void gxav_hdmi_audioout_mute(unsigned int enable)
{
	if (_hdmi_ops && _hdmi_ops->audioout_mute) {
		_hdmi_ops->audioout_mute(enable);
	}
}

void gxav_hdmi_av_mute(unsigned int enable)
{
	if (_hdmi_ops && _hdmi_ops->av_mute) {
		_hdmi_ops->av_mute(enable);
	}
}

void gxav_hdmi_powerdown(unsigned int enable)
{
	if (_hdmi_ops && _hdmi_ops->powerdown) {
		_hdmi_ops->powerdown(enable);
	}
}

void gxav_hdmi_audioout_reset(unsigned int flg)
{
	if (_hdmi_ops && _hdmi_ops->audioout_reset) {
		_hdmi_ops->audioout_reset(flg);
	}
}

void gxav_hdmi_ready(unsigned int flg)
{
	if (_hdmi_ops && _hdmi_ops->ready) {
		_hdmi_ops->ready(flg);
	}
}

void gxav_hdmi_audiosample_change(GxAudioSampleFre samplefre, unsigned int cnum)
{
	if (_hdmi_ops && _hdmi_ops->audiosample_change) {
		_hdmi_ops->audiosample_change(samplefre, cnum);
	}
}

void gxav_hdmi_acr_enable(int enable)
{
	if (_hdmi_ops && _hdmi_ops->acr_enable) {
		_hdmi_ops->acr_enable(enable);
	}
}

void gxav_hdmi_black_enable(int enable)
{
	if (_hdmi_ops && _hdmi_ops->black_enable) {
		_hdmi_ops->black_enable(enable);
	}
}

void gxav_hdmi_videoout_set(GxVideoOutProperty_Mode vout_mode)
{
	if (_hdmi_ops && _hdmi_ops->videoout_set) {
		_hdmi_ops->videoout_set(vout_mode);
	}
}

GxVideoOutProperty_Mode gxav_hdmi_videoout_get(void)
{
	if (_hdmi_ops && _hdmi_ops->videoout_get) {
		return _hdmi_ops->videoout_get();
	}

	return 0;
}

int gxav_hdmi_hdcp_enable_auth(void)
{
	if (_hdmi_ops && _hdmi_ops->hdcp_enable) {
		return _hdmi_ops->hdcp_enable();
	}
	return 0;
}

int gxav_hdmi_hdcp_config(GxVideoOutProperty_HdmiHdcpConfig *config)
{
	if (_hdmi_ops && _hdmi_ops->hdcp_config) {
		return _hdmi_ops->hdcp_config(config);
	}
	return 0;
}

int gxav_hdmi_hdcp_disable_auth(void)
{
	if (_hdmi_ops && _hdmi_ops->hdcp_disable) {
		return _hdmi_ops->hdcp_disable();
	}
	return 0;
}

int gxav_hdmi_hdcp_start_auth(GxVideoOutProperty_Mode mode)
{
	if (_hdmi_ops && _hdmi_ops->hdcp_start) {
		return _hdmi_ops->hdcp_start(mode);
	}
	return 0;
}

int gxav_hdmi_cec_enable(int enable)
{
	if (_hdmi_ops && _hdmi_ops->cec_enable) {
		return _hdmi_ops->cec_enable(enable);
	}
	return 0;
}

int gxav_hdmi_cec_send_cmd(GxVideoOutProperty_HdmiCecCmd *cmd)
{
	if (_hdmi_ops && _hdmi_ops->cec_send_cmd) {
		return _hdmi_ops->cec_send_cmd(cmd);
	}
	return 0;
}

int gxav_hdmi_cec_recv_cmd(GxVideoOutProperty_HdmiCecCmd *cmd)
{
	if (_hdmi_ops && _hdmi_ops->cec_recv_cmd) {
		return _hdmi_ops->cec_recv_cmd(cmd);
	}
	return 0;
}

int gxav_hdmi_interrupt(void)
{
	if (_hdmi_ops && _hdmi_ops->interrupt) {
		return _hdmi_ops->interrupt();
	}
	return 0;
}

void gxav_hdmi_enable(int enable)
{
	if (_hdmi_ops && _hdmi_ops->enable) {
		_hdmi_ops->enable(enable);
	}
}

unsigned char gxav_hdmi_read(unsigned char addr)
{
	if (_hdmi_ops && _hdmi_ops->read) {
		return _hdmi_ops->read(addr);
	}
	return 0;
}

void gxav_hdmi_write(unsigned char addr, unsigned char data)
{
	if (_hdmi_ops && _hdmi_ops->write) {
		_hdmi_ops->write(addr, data);
	}
}

void gxav_hdmi_cold_reset_set(void)
{
	if (_hdmi_ops && _hdmi_ops->cold_reset_set) {
		_hdmi_ops->cold_reset_set();
	}
}

void gxav_hdmi_cold_reset_clr(void)
{
	if (_hdmi_ops && _hdmi_ops->cold_reset_clr) {
		_hdmi_ops->cold_reset_clr();
	}
}

int gxav_hdmi_set_brightness(int b, int c, int s)
{
    if (_hdmi_ops && _hdmi_ops->set_brightness) {
        return _hdmi_ops->set_brightness(b, c, s);
    }

	return 0;
}

int gxav_hdmi_set_saturation(int b, int c, int s)
{
    if (_hdmi_ops && _hdmi_ops->set_saturation) {
        return _hdmi_ops->set_saturation(b, c, s);
    }

	return 0;
}

int gxav_hdmi_set_contrast(int b, int c, int s)
{
    if (_hdmi_ops && _hdmi_ops->set_contrast) {
        return _hdmi_ops->set_contrast(b, c, s);
    }

	return 0;
}

int gxav_hdmi_set_colorimetry(int c)
{
    if (_hdmi_ops && _hdmi_ops->set_colorimetry) {
        return _hdmi_ops->set_colorimetry(c);
    }

	return 0;
}


void gxav_hdmi_get_version(GxVideoOutProperty_HdmiVersion *version)
{
	if (_hdmi_ops && _hdmi_ops->get_version) {
		_hdmi_ops->get_version(version);
	}
}

int  gxav_hdmi_set_encoding_out(GxAvHdmiEncodingOut encoding)
{
	if (_hdmi_ops && _hdmi_ops->set_encoding_out) {
		return _hdmi_ops->set_encoding_out(encoding);
	}

	return -1;
}

int gxav_hdmi_set_cgmsa_permission(int permission)
{
	if (_hdmi_ops && _hdmi_ops->set_cgmsa_copy_permission) {
		return _hdmi_ops->set_cgmsa_copy_permission(permission);
	}

	return -1;
}

int gxav_hdmi_support_hdr10(void)
{
	if (_hdmi_ops && _hdmi_ops->support_hdr10) {
		return _hdmi_ops->support_hdr10();
	}

	return 0;
}

int gxav_hdmi_support_hlg(void)
{
	if (_hdmi_ops && _hdmi_ops->support_hlg) {
		return _hdmi_ops->support_hlg();
	}

	return 0;
}

int gxav_hdmi_set_hdr10(struct frame *frame)
{
	if (_hdmi_ops && _hdmi_ops->set_hdr10) {
		return _hdmi_ops->set_hdr10(frame);
	}

	return -1;
}

int gxav_hdmi_set_hlg(struct frame *frame)
{
	if (_hdmi_ops && _hdmi_ops->set_hlg) {
		return _hdmi_ops->set_hlg(frame);
	}

	return -1;
}

int gxav_hdmi_set_color_resolution(GxAvHdmiColorDepth color_depth)
{
	if (_hdmi_ops && _hdmi_ops->set_color_resolution) {
		return _hdmi_ops->set_color_resolution(color_depth);
	}

	return -1;
}
#endif
