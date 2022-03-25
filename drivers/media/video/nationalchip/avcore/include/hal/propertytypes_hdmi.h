#ifndef __AV_HAL_PROPERTYTYPES_HDMI_H__
#define __AV_HAL_PROPERTYTYPES_HDMI_H__

#include "hdmi_hal.h"

struct avhal_hdmi_property_open {
	int res;
	int ret;
};

struct avhal_hdmi_property_detect_hotplug {
	int res;
	int ret;
};

struct avhal_hdmi_property_read_edid {
	int res;
	struct videoout_hdmi_edid edid;
	int ret;
};

struct avhal_hdmi_property_audio_codes_get {
	int res;
	unsigned int ret;
};

struct avhal_hdmi_property_audioout_set {
	int res;
	unsigned int source;
};

struct avhal_hdmi_property_audioout_mute {
	int res;
	int enable;
};

struct avhal_hdmi_property_audioout_reset {
	int res;
	unsigned int flag;
};

struct avhal_hdmi_property_av_mute {
	int res;
	int enable;
};

struct avhal_hdmi_property_powerdown {
	int res;
	int enable;
};

struct avhal_hdmi_property_ready {
	int res;
	unsigned int flag;
};

struct avhal_hdmi_property_audiosample_change {
	int res;
	GxAudioSampleFre samplefre;
	unsigned int cnum;
};

struct avhal_hdmi_property_acr_enable {
	int res;
	int enable;
};

struct avhal_hdmi_property_black_enable {
	int res;
	int enable;
};

struct avhal_hdmi_property_videoout_set {
	int res;
	GxVideoOutProperty_Mode mode;
};

struct avhal_hdmi_property_videoout_get {
	int res;
	GxVideoOutProperty_Mode mode;
};

struct avhal_hdmi_property_hdcp_enable_auth {
	int res;
	GxVideoOutProperty_Mode mode;
	int ret;
};

struct avhal_hdmi_property_hdcp_disable_auth {
	int res;
	int ret;
};

struct avhal_hdmi_property_enable {
	int res;
	int enable;
};

struct avhal_hdmi_property_set_brightness {
	int res;
	int b, c, s;
	int ret;
};

struct avhal_hdmi_property_set_saturation {
	int res;
	int b, c, s;
	int ret;
};

struct avhal_hdmi_property_set_contrast {
	int res;
	int b, c, s;
	int ret;
};

struct avhal_hdmi_property_get_version {
	int res;
	GxVideoOutProperty_HdmiVersion version;
	int ret;
};

struct avhal_hdmi_property_set_encoding_out {
	int res;
	GxAvHdmiEncodingOut encoding;
	int ret;
};

struct avhal_hdmi_property_set_cgmsa_permission {
	int res;
	int permission;
	int ret;
};

#endif

