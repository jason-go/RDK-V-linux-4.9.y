#include "gxav.h"
#include "hdmi_hal.h"
#include "vout_hal.h"
#include "kernelcalls.h"

int gx_videoout_init(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_videoout_init(inode->interface);
}

int gx_videoout_uninit(struct gxav_device *dev, struct gxav_module_inode *inode)
{
	return gxav_videoout_uninit();
}

int gx_videoout_open(struct gxav_module *module)
{
	return gxav_videoout_open(module->sub, module->inode);
}

int gx_videoout_close(struct gxav_module *module)
{
	return gxav_videoout_close(module->sub);
}

int gx_videoout_set_property(struct gxav_module *module, int property_id, void *property, int size)
{
	switch (property_id) {
	case GxVideoOutPropertyID_OutputConfig:
		{
			GxVideoOutProperty_OutputConfig *output_config = (GxVideoOutProperty_OutputConfig *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out OutputConfig)\n", property_id);

			if (output_config == NULL || size != sizeof(GxVideoOutProperty_OutputConfig)) {
				VIDEOOUT_PRINTF("video out output config error!\n");
				return -1;
			}

			return gxav_videoout_config(module->sub, output_config);
		}
		break;

	case GxVideoOutPropertyID_OutputSelect:
		{
			GxVideoOutProperty_OutputSelect *output_select = (GxVideoOutProperty_OutputSelect *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out OutputSelect)\n", property_id);

			if (output_select == NULL || size != sizeof(GxVideoOutProperty_OutputSelect)) {
				VIDEOOUT_PRINTF("video out output select error!\n");
				return -1;
			}

			return gxav_videoout_interface(module->sub, output_select);
		}
		break;

	case GxVideoOutPropertyID_Resolution:
		{
			GxVideoOutProperty_Resolution *resolution = (GxVideoOutProperty_Resolution *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Resolution)\n", property_id);

			if (resolution == NULL || size != sizeof(GxVideoOutProperty_Resolution)) {
				VIDEOOUT_PRINTF("video out output resolution error!\n");
				return -1;
			}

			return gxav_videoout_resolution(module->sub, resolution);
		}
		break;

	case GxVideoOutPropertyID_Brightness:
		{
			GxVideoOutProperty_Brightness *brightness = (GxVideoOutProperty_Brightness *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Brightness)\n", property_id);

			if (brightness == NULL || size != sizeof(GxVideoOutProperty_Brightness)) {
				VIDEOOUT_PRINTF("video out brightness error!\n");
				return -1;
			}

			return gxav_videoout_brightness(module->sub, brightness);
		}
		break;

	case GxVideoOutPropertyID_Contrast:
		{
			GxVideoOutProperty_Contrast *contrast = (GxVideoOutProperty_Contrast *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Contrast)\n", property_id);

			if (contrast == NULL || size != sizeof(GxVideoOutProperty_Contrast)) {
				VIDEOOUT_PRINTF("video out contrast error!\n");
				return -1;
			}

			return gxav_videoout_contrast(module->sub, contrast);
		}
		break;

	case GxVideoOutPropertyID_Saturation:
		{
			GxVideoOutProperty_Saturation *saturation = (GxVideoOutProperty_Saturation *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Saturation)\n", property_id);

			if (saturation == NULL || size != sizeof(GxVideoOutProperty_Saturation)) {
				VIDEOOUT_PRINTF("video out saturation error!\n");
				return -1;
			}

			return gxav_videoout_saturation(module->sub, saturation);
		}
		break;

	case GxVideoOutPropertyID_Sharpness:
		{
			GxVideoOutProperty_Sharpness *sharpness = (GxVideoOutProperty_Sharpness *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Sharpness)\n", property_id);

			if (sharpness == NULL || size != sizeof(GxVideoOutProperty_Sharpness)) {
				VIDEOOUT_PRINTF("video out sharpness error!\n");
				return -1;
			}

			return gxav_videoout_sharpness(module->sub, sharpness);
		}
		break;

	case GxVideoOutPropertyID_Hue:
		{
			GxVideoOutProperty_Hue *hue= (GxVideoOutProperty_Hue*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hue)\n", property_id);

			if (hue== NULL || size != sizeof(GxVideoOutProperty_Hue)) {
				VIDEOOUT_PRINTF("video out sharpness error!\n");
				return -1;
			}

			return gxav_videoout_hue(module->sub, hue);
		}
		break;
	case GxVideoOutPropertyID_AspectRatio:
		{
			GxVideoOutProperty_AspectRatio *aspect_ratio = (GxVideoOutProperty_AspectRatio *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out AspectRatio)\n", property_id);

			if (aspect_ratio == NULL || size != sizeof(GxVideoOutProperty_AspectRatio)) {
				VIDEOOUT_PRINTF("aspect_ratio error!\n");
				return -1;
			}

			return gxav_videoout_aspratio(module->sub, aspect_ratio);
		}
		break;

	case GxVideoOutPropertyID_TvScreen:
		{
			GxVideoOutProperty_TvScreen *TvScreen = (GxVideoOutProperty_TvScreen *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out TvScreen)\n", property_id);

			if (TvScreen == NULL || size != sizeof(GxVideoOutProperty_TvScreen)) {
				VIDEOOUT_PRINTF("TVScreen error!\n");
				return -1;
			}

			return gxav_videoout_tvscreen(module->sub, TvScreen);
		}
		break;

	case GxVideoOutPropertyID_PowerOn:
		{
			GxVideoOutProperty_PowerOn * power = (GxVideoOutProperty_PowerOn*) property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out On)\n", property_id);
			return gxav_videoout_PowerOn(module->sub, power->selection);
		}
		break;

	case GxVideoOutPropertyID_PowerOff:
		{
			GxVideoOutProperty_PowerOff * power = (GxVideoOutProperty_PowerOff*) property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Off)\n", property_id);
			return gxav_videoout_PowerOff(module->sub, power->selection);
		}
		break;

	case GxVideoOutPropertyID_OutDefault:
		{
			GxVideoOutProperty_OutDefault *def = (GxVideoOutProperty_OutDefault*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out OutDefault)\n", property_id);
			return gxav_videoout_out_default(module->sub, def);
		}
		break;

	case GxVideoOutPropertyID_HDCPEnable:
		{
			GxVideoOutProperty_HdmiHdcpEnable *auth = (GxVideoOutProperty_HdmiHdcpEnable*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Hdcp Auth)\n", property_id);
			return gxav_videoout_hdcp_enable(module->sub, auth);
		}
		break;

	case GxVideoOutPropertyID_CecEnable:
		{
			GxVideoOutProperty_HdmiCecEnable *param = (GxVideoOutProperty_HdmiCecEnable*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Hdcp Auth)\n", property_id);
			return gxav_videoout_cec_enable(module->sub, param);
		}
		break;
	case GxVideoOutPropertyID_CecCmd:
		{
			GxVideoOutProperty_HdmiCecCmd *param = (GxVideoOutProperty_HdmiCecCmd*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Hdcp Auth)\n", property_id);
			return gxav_videoout_cec_send_cmd(module->sub, param);
		}
		break;
	case GxVideoOutPropertyID_HDCPConfig:
		{
			GxVideoOutProperty_HdmiHdcpConfig *config = (GxVideoOutProperty_HdmiHdcpConfig*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Hdcp Auth)\n", property_id);
			return gxav_videoout_hdcp_config(module->sub, config);
		}
		break;
	case GxVideoOutPropertyID_HDCPStart:
		{
			GxVideoOutProperty_HdmiHdcpEnable *auth = (GxVideoOutProperty_HdmiHdcpEnable*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Hdcp Start)\n", property_id);
			return gxav_videoout_hdcp_start(module->sub, auth);
		}
		break;
	case GxVideoOutPropertyID_HdmiEncodingOut:
		{
			GxVideoOutProperty_HdmiEncodingOut *mode = (GxVideoOutProperty_HdmiEncodingOut *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out HdmiEncodingOut)\n", property_id);
			return gxav_videoout_set_hdmi_encoding_out(mode->encoding);
		}
		break;
	case GxVideoOutPropertyID_HdmiColorResolution:
		{
			GxVideoOutProperty_HdmiColorDepth *color = (GxVideoOutProperty_HdmiColorDepth *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out HdmiColorResolution)\n", property_id);
			return gxav_videoout_set_hdmi_color_resolution(color->color_depth);
		}
		break;
	case GxVideoOutPropertyID_Macrovision:
		{
			GxVideoOutProperty_Macrovision *macrovision = (GxVideoOutProperty_Macrovision*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Macrovision)\n", property_id);
			return gxav_videoout_set_macrovision(module->sub, macrovision);
		}
		break;
	case GxVideoOutPropertyID_DCS:
		{
			GxVideoOutProperty_DCS *dcs = (GxVideoOutProperty_DCS *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out DCS)\n", property_id);
			return (gxav_videoout_set_dcs(module->sub, dcs));
		}
		break;
	case GxVideoOutPropertyID_CGMSEnable:
		{
			GxVideoOutProperty_CGMSEnable *cgms_enable = (GxVideoOutProperty_CGMSEnable *) property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out CGMS Enable)\n", property_id);
			return (gxav_videoout_set_CGMS_enable(module->sub, cgms_enable));
		}
		break;
	case GxVideoOutPropertyID_CGMSConfig:
		{
			GxVideoOutProperty_CGMSConfig *cgms_config = (GxVideoOutProperty_CGMSConfig *) property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out CGMS Config)\n", property_id);
			return (gxav_videoout_set_CGMS_config(module->sub, cgms_config));
		}
		break;
	case GxVideoOutPropertyID_WSSEnable:
		{
			GxVideoOutProperty_WSSEnable *param = (GxVideoOutProperty_WSSEnable *) property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out wss Enable)\n", property_id);
			return (gxav_videoout_set_WSS_enable(module->sub, param));
		}
		break;
	case GxVideoOutPropertyID_WSSConfig:
		{
			GxVideoOutProperty_WSSConfig *param = (GxVideoOutProperty_WSSConfig *) property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out wss Config)\n", property_id);
			return (gxav_videoout_set_WSS_config(module->sub, param));
		}
		break;
	case GxVideoOutPropertyID_ScartConfig:
		{
			GxVideoOutProperty_ScartConfig *config = (GxVideoOutProperty_ScartConfig*)property;
			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Scart Mode)\n", property_id);
			return gxav_videoout_scart_config(module->sub, config);
		}
		break;
	case GxVideoOutPropertyID_HdmiAudioMute:
		{
			GxVideoOutProperty_HdmiAudioMute *param = (GxVideoOutProperty_HdmiAudioMute *)property;
			return gxav_videoout_set_hdmi_audiomute(param);
		}
		break;
	default:
		VIDEOOUT_PRINTF("there is something wrong with the parameter property_id,"
				"please set the right parameter %d\n", property_id);
		return -2;
	}

	return 0;
}

int gx_videoout_get_property(struct gxav_module *module, int property_id, void *property, int size)
{
	switch (property_id) {
	case GxVideoOutPropertyID_Resolution:
		{
			GxVideoOutProperty_Resolution *resolution = (GxVideoOutProperty_Resolution *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Resolution)\n", property_id);

			if (resolution == NULL || size != sizeof(GxVideoOutProperty_Resolution)) {
				VIDEOOUT_PRINTF("video out output resolution error!\n");
				return -1;
			}

			return gxav_videoout_resolution_information(module->sub, resolution);
		}
		break;

	case GxVideoOutPropertyID_Brightness:
		{
			GxVideoOutProperty_Brightness *brightness = (GxVideoOutProperty_Brightness *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Brightness)\n", property_id);

			return gxav_videoout_brightness_information(module->sub, brightness);
		}
		break;

	case GxVideoOutPropertyID_Contrast:
		{
			GxVideoOutProperty_Contrast *contrast = (GxVideoOutProperty_Contrast *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Contrast)\n", property_id);

			return gxav_videoout_contrast_information(module->sub, contrast);
		}
		break;

	case GxVideoOutPropertyID_Saturation:
		{
			GxVideoOutProperty_Saturation *saturation = (GxVideoOutProperty_Saturation *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Saturation)\n", property_id);

			return gxav_videoout_saturation_information(module->sub, saturation);
		}
		break;

	case GxVideoOutPropertyID_Sharpness:
		{
			GxVideoOutProperty_Sharpness *sharpness = (GxVideoOutProperty_Sharpness *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Sharpness)\n", property_id);

			return gxav_videoout_sharpness_information(module->sub, sharpness);
		}
		break;

	case GxVideoOutPropertyID_Hue:
		{
			GxVideoOutProperty_Hue *hue = (GxVideoOutProperty_Hue*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hue)\n", property_id);

			return gxav_videoout_hue_information(module->sub, hue);
		}
		break;

	case GxVideoOutPropertyID_HdmiStatus:
		{
			GxVideoOutProperty_OutHdmiStatus *hdmi_status= (GxVideoOutProperty_OutHdmiStatus *) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Status)\n", property_id);

			return gxav_videoout_get_hdmi_status(module->sub, hdmi_status);
		}
		break;

	case GxVideoOutPropertyID_EdidInfo:
		{
			GxVideoOutProperty_EdidInfo *edid_info = (GxVideoOutProperty_EdidInfo*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Edid Info)\n", property_id);

			return gxav_videoout_get_edid_info(module->sub, edid_info);
		}
		break;
	case GxVideoOutPropertyID_HdmiVersion:
		{
			GxVideoOutProperty_HdmiVersion *version = (GxVideoOutProperty_HdmiVersion*) property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi version Info)\n", property_id);

			return gxav_videoout_get_hdmi_version(module->sub, version);
		}
	case GxVideoOutPropertyID_CecCmd:
		{
			GxVideoOutProperty_HdmiCecCmd *param = (GxVideoOutProperty_HdmiCecCmd*)property;

			VIDEOOUT_DBG("[property_id] : 0x%x (Video Out Hdmi Hdcp Auth)\n", property_id);
			return gxav_videoout_cec_recv_cmd(module->sub, param);
		}
		break;
	default:
		VIDEOOUT_PRINTF("there is something wrong with the parameter property_id,"
				"please set the right parameter\n");
		return -2;
	}

	return 0;
}

struct gxav_module_inode *gx_video_out_interrupt(struct gxav_module_inode *inode, int irq)
{
	return inode;
}

