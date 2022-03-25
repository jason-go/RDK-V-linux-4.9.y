#include "vout_hal.h"
#include "video.h"

static struct vout_ops* _vout_ops;
static struct gxav_videoout _videoout[VIDEOOUT_MAX];
#define VOUT_CHIP_CHECK_ID(vout_id) \
	do { \
		if (vout_id != 0) \
		return -1; \
	}while(0)

static struct gxav_videoout* gxav_videoout_create_instance(int sub)
{
	unsigned int i = 0;

	struct gxav_videoout *videoout = _videoout;

	if ((sub >= VIDEOOUT_MAX) || (videoout == NULL)) {
		return NULL;
	}

	while (i < VIDEOOUT_MAX) {
		if ((videoout->id == sub) && (videoout->used == USED)) {
			return NULL;
		}
		i++;
		videoout++;
	}

	i = 0;
	videoout = _videoout;

	while (i < VIDEOOUT_MAX) {
		if (videoout->used == UNUSED) {
			videoout->id = sub;
			videoout->used = USED;
			break;
		}
		i++;
		videoout++;
	}

	return (VIDEOOUT_MAX == i) ? (NULL) : (videoout);
}

struct gxav_videoout* gxav_videoout_find_instance(int sub)
{
	unsigned int i = 0;

	struct gxav_videoout* videoout = _videoout;

	if ((sub >= VIDEOOUT_MAX) || (videoout == NULL)) {
		return NULL;
	}

	while (i < VIDEOOUT_MAX) {
		if (videoout->id == sub && videoout->used == USED) {
			break;
		}
		i++;
		videoout++;
	}

	return (VIDEOOUT_MAX == i) ? (NULL) : (videoout);
}

int gxav_videoout_init(struct gxav_module_ops* ops)
{
	_vout_ops = ops->priv;
	memset(_videoout, 0, sizeof(_videoout));

	if (_vout_ops && _vout_ops->init)
		return _vout_ops->init();

	return 0;
}

int gxav_videoout_uninit(void)
{
	if (_vout_ops && _vout_ops->uninit)
		return _vout_ops->uninit();
	_vout_ops = NULL;

	return 0;
}

int gxav_videoout_open(int id, struct gxav_module_inode* inode)
{
	struct gxav_videoout *vout = gxav_videoout_create_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	vout->inode = inode;

	if (_vout_ops && _vout_ops->open) {
		return _vout_ops->open(vout);
	}

	return 0;
}

int gxav_videoout_close(int id)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->close) {
		_vout_ops->close(vout);
	}

	memset(vout, 0, sizeof(struct gxav_videoout));

	return 0;
}

int gxav_videoout_config(int id, GxVideoOutProperty_OutputConfig *param)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->config) {
		return _vout_ops->config(vout, param);
	}

	return 0;
}

int gxav_videoout_interface(int id, GxVideoOutProperty_OutputSelect *output_select)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_interface) {
		return _vout_ops->set_interface(vout, output_select);
	}

	return 0;
}

int gxav_videoout_resolution(int id, GxVideoOutProperty_Resolution *resolution)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_resolution) {
		return _vout_ops->set_resolution(vout, resolution);
	}

	return 0;
}

int gxav_videoout_brightness(int id, GxVideoOutProperty_Brightness *brightness)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_brightness) {
		return _vout_ops->set_brightness(vout, brightness);
	}

	return 0;
}

int gxav_videoout_saturation(int id, GxVideoOutProperty_Saturation *saturation)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_saturation) {
		return _vout_ops->set_saturation(vout, saturation);
	}

	return 0;
}

int gxav_videoout_contrast(int id, GxVideoOutProperty_Contrast *contrast)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_contrast) {
		return _vout_ops->set_contrast(vout, contrast);
	}

	return 0;
}

int gxav_videoout_sharpness(int id, GxVideoOutProperty_Sharpness *sharpness)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_sharpness) {
		return _vout_ops->set_sharpness(vout, sharpness);
	}

	return 0;
}

int gxav_videoout_hue(int id, GxVideoOutProperty_Hue *hue)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_hue) {
		return _vout_ops->set_hue(vout, hue);
	}

	return 0;
}

int gxav_videoout_aspratio(int id, GxVideoOutProperty_AspectRatio *aspect_ratio)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_aspratio) {
		return _vout_ops->set_aspratio(vout, aspect_ratio);
	}

	return 0;
}

int gxav_videoout_tvscreen(int id, GxVideoOutProperty_TvScreen *tvscreen)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_tvscreen) {
		return _vout_ops->set_tvscreen(vout, tvscreen);
	}

	return 0;
}

int gxav_videoout_resolution_information(int id, GxVideoOutProperty_Resolution *resolution)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_resolution) {
		return _vout_ops->get_resolution(vout, resolution);
	}

	return 0;
}

int gxav_videoout_brightness_information(int id, GxVideoOutProperty_Brightness *brightness)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_brightness) {
		return _vout_ops->get_brightness(vout, brightness);
	}

	return 0;
}

int gxav_videoout_contrast_information(int id, GxVideoOutProperty_Contrast *contrast)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_contrast) {
		return _vout_ops->get_contrast(vout, contrast);
	}

	return 0;
}

int gxav_videoout_saturation_information(int id, GxVideoOutProperty_Saturation *saturation)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_saturation) {
		return _vout_ops->get_saturation(vout, saturation);
	}

	return 0;
}

int gxav_videoout_sharpness_information(int id, GxVideoOutProperty_Sharpness *sharpness)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_sharpness) {
		return _vout_ops->get_sharpness(vout, sharpness);
	}

	return 0;
}

int gxav_videoout_hue_information(int id, GxVideoOutProperty_Hue *hue)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_hue) {
		return _vout_ops->get_hue(vout, hue);
	}

	return 0;
}

int gxav_videoout_play_cc(int id, struct vout_ccinfo *ccinfo)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->play_cc) {
		return _vout_ops->play_cc(vout, ccinfo);
	}

	return 0;
}

int gxav_videoout_get_dvemode(int id, struct vout_dvemode *dvemode)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_dvemode) {
		return _vout_ops->get_dvemode(vout, dvemode);
	}

	return 0;
}

int gxav_videoout_out_default(int id, GxVideoOutProperty_OutDefault *outdefault)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_default) {
		return _vout_ops->set_default(vout, outdefault);
	}

	return 0;
}

int gxav_videoout_PowerOff(int id, unsigned int selection)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_PowerOff) {
		return _vout_ops->set_PowerOff(vout, selection);
	}

	return 0;
}

int gxav_videoout_PowerOn(int id, unsigned int selection)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_PowerOn) {
		return _vout_ops->set_PowerOn(vout, selection);
	}

	return 0;
}

int gxav_videoout_hdcp_enable(int id, GxVideoOutProperty_HdmiHdcpEnable *auth)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->hdcp_enable) {
		return _vout_ops->hdcp_enable(vout, auth);
	}

	return 0;
}

int gxav_videoout_cec_enable(int id, GxVideoOutProperty_HdmiCecEnable *param)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->cec_enable) {
		return _vout_ops->cec_enable(vout, param);
	}

	return 0;
}

int gxav_videoout_cec_send_cmd(int id, GxVideoOutProperty_HdmiCecCmd *param)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->cec_send_cmd) {
		return _vout_ops->cec_send_cmd(vout, param);
	}

	return 0;
}

int gxav_videoout_cec_recv_cmd(int id, GxVideoOutProperty_HdmiCecCmd *param)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->cec_recv_cmd) {
		return _vout_ops->cec_recv_cmd(vout, param);
	}

	return 0;
}

int gxav_videoout_hdcp_config(int id, GxVideoOutProperty_HdmiHdcpConfig* config)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->hdcp_config) {
		return _vout_ops->hdcp_config(vout, config);
	}

	return 0;
}

int gxav_videoout_hdcp_start(int id, GxVideoOutProperty_HdmiHdcpEnable* auth)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->hdcp_start) {
		return _vout_ops->hdcp_start(vout, auth);
	}

	return 0;
}

int gxav_videoout_get_hdmi_status(int id, GxVideoOutProperty_OutHdmiStatus *status)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_hdmi_status) {
		return _vout_ops->get_hdmi_status(vout, status);
	}

	return 0;
}

int gxav_videoout_get_edid_info(int id, GxVideoOutProperty_EdidInfo *edid_info)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_edid_info) {
		return _vout_ops->get_edid_info(vout, edid_info);
	}

	return 0;
}

int gxav_videoout_set_macrovision(int id, GxVideoOutProperty_Macrovision *macrovision)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_macrovision) {
		return _vout_ops->set_macrovision(vout, macrovision);
	}

	return 0;
}

int gxav_videoout_set_dcs(int id, GxVideoOutProperty_DCS *dcs)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_dcs) {
		return _vout_ops->set_dcs(vout, dcs);
	}

	return 0;
}

int gxav_videoout_set_CGMS_enable(int id, GxVideoOutProperty_CGMSEnable *property)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_cgms_enable) {
		return _vout_ops->set_cgms_enable(property);
	}

	return 0;
}

int gxav_videoout_set_CGMS_config(int id, GxVideoOutProperty_CGMSConfig *property)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_cgms_config) {
		return _vout_ops->set_cgms_config(property);
	}

	return 0;
}

int gxav_videoout_set_WSS_enable(int id, GxVideoOutProperty_WSSEnable *property)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_vbi_wss_enable) {
		return _vout_ops->set_vbi_wss_enable(property);
	}

	return 0;
}

int gxav_videoout_set_WSS_config(int id, GxVideoOutProperty_WSSConfig *property)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->set_vbi_wss_config) {
		return _vout_ops->set_vbi_wss_config(property);
	}

	return 0;
}

int gxav_videoout_scart_config(int id, GxVideoOutProperty_ScartConfig *config)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);

	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->scart_config) {
		return _vout_ops->scart_config(vout, config);
	}

	return 0;
}

int gxav_videoout_get_hdmi_version(int id, GxVideoOutProperty_HdmiVersion *version)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->get_hdmi_version) {
		return _vout_ops->get_hdmi_version(vout, version);
	}

	return 0;
}

int gxav_videoout_white_screen(int id, int enable)
{
	struct gxav_videoout *vout = gxav_videoout_find_instance(id);
	VOUT_CHIP_CHECK_ID(id);
	if (vout == NULL) {
		VIDEOOUT_PRINTF("Instantiation fail!\n");
		return -1;
	}

	if (_vout_ops && _vout_ops->white_screen) {
		return _vout_ops->white_screen(enable);
	}

	return 0;
}

int gxav_videoout_set_hdmi_encoding_out(GxAvHdmiEncodingOut encoding)
{
	GxVideoOutProperty_Brightness brightness = {GXAV_VOUT_HDMI, 0};
	GxVideoOutProperty_Saturation saturation = {GXAV_VOUT_HDMI, 0};
	GxVideoOutProperty_Contrast contrast     = {GXAV_VOUT_HDMI, 0};
	gxav_videoout_brightness_information(0, &brightness);
	gxav_videoout_contrast_information(0, &contrast);
	gxav_videoout_saturation_information(0, &saturation);
	gxav_hdmi_set_encoding_out(encoding);
	gxav_videoout_brightness(0, &brightness);
	gxav_videoout_contrast(0, &contrast);
	gxav_videoout_saturation(0, &saturation);
	return 0;
}

int gxav_videoout_set_hdmi_color_resolution(GxAvHdmiColorDepth color_depth)
{
	gxav_hdmi_set_color_resolution(color_depth);

	return 0;
}

int gxav_videoout_set_hdmi_audiomute(GxVideoOutProperty_HdmiAudioMute *param)
{
	gxav_hdmi_audioout_mute(param->mute);
	return 0;
}

EXPORT_SYMBOL(gxav_videoout_config);
EXPORT_SYMBOL(gxav_videoout_interface);
EXPORT_SYMBOL(gxav_videoout_resolution);
EXPORT_SYMBOL(gxav_videoout_brightness);
EXPORT_SYMBOL(gxav_videoout_saturation);
EXPORT_SYMBOL(gxav_videoout_contrast);
EXPORT_SYMBOL(gxav_videoout_sharpness);
EXPORT_SYMBOL(gxav_videoout_hue);
EXPORT_SYMBOL(gxav_videoout_aspratio);
EXPORT_SYMBOL(gxav_videoout_tvscreen);
EXPORT_SYMBOL(gxav_videoout_resolution_information);
EXPORT_SYMBOL(gxav_videoout_brightness_information);
EXPORT_SYMBOL(gxav_videoout_contrast_information);
EXPORT_SYMBOL(gxav_videoout_saturation_information);
EXPORT_SYMBOL(gxav_videoout_sharpness_information);
EXPORT_SYMBOL(gxav_videoout_hue_information);
EXPORT_SYMBOL(gxav_videoout_play_cc);
EXPORT_SYMBOL(gxav_videoout_get_dvemode);
EXPORT_SYMBOL(gxav_videoout_out_default);
EXPORT_SYMBOL(gxav_videoout_PowerOff);
EXPORT_SYMBOL(gxav_videoout_PowerOn);
EXPORT_SYMBOL(gxav_videoout_get_hdmi_status);
EXPORT_SYMBOL(gxav_videoout_get_edid_info);
EXPORT_SYMBOL(gxav_videoout_set_macrovision);
EXPORT_SYMBOL(gxav_videoout_set_dcs);
EXPORT_SYMBOL(gxav_videoout_get_hdmi_version);
EXPORT_SYMBOL(gxav_videoout_white_screen);

