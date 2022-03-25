#ifndef __VOUT_HAL_H__
#define __VOUT_HAL_H__

#include "gxav.h"
#include "hdmi_hal.h"
#include "kernelcalls.h"

//#define GX_VIDEOOUT_DEBUG (1)
#ifdef GX_VIDEOOUT_DEBUG
#define VIDEOOUT_PRINTF(fmt, args...) \
	do { \
		gx_printf("\n[VIDEOOUT][%s():%d]: ", __func__, __LINE__); \
		gx_printf(fmt, ##args); \
	} while(0)
#else
#define VIDEOOUT_PRINTF(fmt, args...)   ((void)0)
#endif

#ifdef GX_VIDEOOUT_DEBUG
#define VIDEOOUT_DBG(fmt, args...)   ((void)0)
#else
#define VIDEOOUT_DBG(fmt, args...)   ((void)0)
#endif

#define VIDEOOUT_MAX (1)

#define IS_INTERLACE_MODE(mode) \
	(  (mode == GXAV_VOUT_PAL)\
	   ||(mode == GXAV_VOUT_PAL_M)\
	   ||(mode == GXAV_VOUT_PAL_N)\
	   ||(mode == GXAV_VOUT_PAL_NC)\
	   ||(mode == GXAV_VOUT_576I)\
	   ||(mode == GXAV_VOUT_576I)\
	   ||(mode == GXAV_VOUT_NTSC_M)\
	   ||(mode == GXAV_VOUT_NTSC_443)\
	   ||(mode == GXAV_VOUT_480I)\
	   ||(mode == GXAV_VOUT_480I)\
	   ||(mode == GXAV_VOUT_1080I_50HZ)\
	   ||(mode == GXAV_VOUT_1080I_60HZ)\
	   ||(mode == GXAV_VOUT_1080I_50HZ)\
	   ||(mode == GXAV_VOUT_1080I_60HZ))

#define IS_PROGRESSIVE_MODE(mode) \
	(  (mode == GXAV_VOUT_480P)\
	   ||(mode == GXAV_VOUT_576P)\
	   ||(mode == GXAV_VOUT_720P_50HZ)\
	   ||(mode == GXAV_VOUT_720P_60HZ)\
	   ||(mode == GXAV_VOUT_1080P_50HZ)\
	   ||(mode == GXAV_VOUT_1080P_60HZ)\
	   ||(mode == GXAV_VOUT_480P)\
	   ||(mode == GXAV_VOUT_576P)\
	   ||(mode == GXAV_VOUT_720P_50HZ)\
	   ||(mode == GXAV_VOUT_720P_60HZ)\
	   ||(mode == GXAV_VOUT_1080P_50HZ)\
	   ||(mode == GXAV_VOUT_1080P_60HZ))

#define IS_P_MODE(mode) \
	(  (mode == GXAV_VOUT_PAL)\
	   ||(mode == GXAV_VOUT_PAL_N)\
	   ||(mode == GXAV_VOUT_PAL_NC)\
	   ||(mode == GXAV_VOUT_576I)\
	   ||(mode == GXAV_VOUT_576I)\
	   ||(mode == GXAV_VOUT_576P)\
	   ||(mode == GXAV_VOUT_576P)\
	   ||(mode == GXAV_VOUT_720P_50HZ)\
	   ||(mode == GXAV_VOUT_720P_50HZ)\
	   ||(mode == GXAV_VOUT_1080I_50HZ)\
	   ||(mode == GXAV_VOUT_1080I_50HZ)\
	   ||(mode == GXAV_VOUT_1080P_50HZ)\
	   ||(mode == GXAV_VOUT_1080P_50HZ))


#define IS_N_MODE(mode) \
	(  (mode == GXAV_VOUT_PAL_M)\
	   ||(mode == GXAV_VOUT_NTSC_M)\
	   ||(mode == GXAV_VOUT_NTSC_443)\
	   ||(mode == GXAV_VOUT_480I)\
	   ||(mode == GXAV_VOUT_480I)\
	   ||(mode == GXAV_VOUT_480P)\
	   ||(mode == GXAV_VOUT_480P)\
	   ||(mode == GXAV_VOUT_720P_60HZ)\
	   ||(mode == GXAV_VOUT_720P_60HZ)\
	   ||(mode == GXAV_VOUT_1080I_60HZ)\
	   ||(mode == GXAV_VOUT_1080I_60HZ)\
	   ||(mode == GXAV_VOUT_1080P_60HZ)\
	   ||(mode == GXAV_VOUT_1080P_60HZ))

enum {
	GX_VOUT0,
	GX_VOUT1,
};

enum dac_out_mode {
	CVBS_YC_CVBS = 0,
	Y_Cb_Cr_CVBS = 1,
	G_B_R_CVBS   = 2,
	Y_Pb_Pr_Y    = 3,
	G_B_R_Y      = 4,
	HDTV_R_G_B_R = 5,
	NOT_CARE
};

enum dac_full_case {
	G_B_R_X = 0 ,
	G_B_X_R = 1 ,
	G_R_B_X = 2 ,
	G_R_X_B = 3 ,
	G_X_B_R = 4 ,
	G_X_R_B = 5 ,
	B_G_R_X = 6 ,
	B_G_X_R = 7 ,
	B_X_R_G = 8 ,
	B_X_G_R = 9 ,
	B_R_X_G = 10,
	B_R_G_X = 11,
	R_B_G_X = 12,
	R_B_X_G = 13,
	R_G_B_X = 14,
	R_G_X_B = 15,
	R_X_B_G = 16,
	R_X_G_B = 17,
	X_B_R_G = 18,
	X_B_G_R = 19,
	X_G_R_B = 20,
	X_G_B_R = 21,
	X_R_B_G = 22,
	X_R_G_B = 23
};

enum hdtv_out_mode {
	SMPTE_YUV = 0,
	EIA_770_YUV = 1,
	RGB_SYNC   = 2,
	RGB_SYNC_OUT_SIGNAL    = 3,
};

typedef enum {
	IFACE_ID_RCA ,
	IFACE_ID_RCA1 ,
	IFACE_ID_YUV ,
	IFACE_ID_SCART ,
	IFACE_ID_SVIDEO ,
	IFACE_ID_HDMI ,
	IFACE_ID_MAX ,
} IfaceID;

struct bcs_level {
	unsigned int brightness;
	unsigned int contrast;
	unsigned int saturation;
};

struct gxav_resolution {
	GxVideoOutProperty_Interface iface;
	GxVideoOutProperty_Mode      mode;
	unsigned int                 same_config;
	unsigned int                 hdmi_hdcp_on;
};

struct vout_ccinfo{
	short cc_type;
	short cc_data;
};


struct vout_dvemode{
	int id;
	GxVideoOutProperty_Mode      mode;
};

enum {
	ID_VPU  = 0,
	ID_SVPU = 1, //svpu
	SRC_ID_MAX,
};

struct reg_iface {
	int select;
	int vpu_dac_case;
	int vpu_dac_mode;
	int svpu_dac_case;
	int svpu_dac_mode;
	int dac_src0;
	int dac_src1;
	int dac_src2;
	int dac_src3;
	struct{
		GxVideoOutProperty_Interface iface;
		int dac;
	}ifaces_dac[IFACE_ID_MAX];
};

struct vout_interface;
struct interface_ops {
	int (*set_mode)(struct vout_interface *viface, GxVideoOutProperty_Mode mode);
	int (*set_brightness)(struct vout_interface *viface, unsigned int value);
	int (*set_saturation)(struct vout_interface *viface, unsigned int value);
	int (*set_contrast)(struct vout_interface *viface, unsigned int value);

	GxVideoOutProperty_Mode (*get_mode)(struct vout_interface *viface);
	unsigned int (*get_brightness)(struct vout_interface *viface);
	unsigned int (*get_saturation)(struct vout_interface *viface);
	unsigned int (*get_contrast)(struct vout_interface *viface);
};


struct vout_src {
	unsigned loader_inited;
	GxVideoOutProperty_Mode mode;
};

struct vout_interface {
	unsigned int loader_inited;

	GxVideoOutProperty_Interface iface;
	GxVideoOutProperty_Mode      mode;

	unsigned int brightness;
	unsigned int contrast;
	unsigned int saturation;
	unsigned int sharpness;
	unsigned int hue;

	struct gxav_videoout *vout;
	struct interface_ops *ops;
};

struct vout_info {
	int select;
	GxVideoOutProperty_ScartConfig scart_config;
	GxVideoOutCGMSACopyPermission analog_copy_permission;
	GxVideoOutCGMSACopyPermission digital_copy_permission;

	struct vout_src       src[SRC_ID_MAX];
	struct vout_interface interfaces[IFACE_ID_MAX];
};

struct gxav_videoout {
	int id;
#define UNUSED  0
#define USED    1
	int used;

	int svpu_configed;
	int hdcp_enable;
	int macrovision_enable;
	int cec_enable;
	int select;
	int enhancement_enable;

	GxVideoOutProperty_Auto vout1_auto;
	unsigned int svpu_delayms;
	unsigned int hdmi_delayms;
	struct gxav_module_inode* inode;

	GxVideoOutProperty_DCS dcs;
	GxVideoOutProperty_Macrovision macrovision;
};

struct vout_ops {
	int (*init)(void);
	int (*uninit)(void);
	int (*open)(struct gxav_videoout *vout);
	int (*close)(struct gxav_videoout* vout);
	int (*config)(struct gxav_videoout* vout, GxVideoOutProperty_OutputConfig *param);
	int (*set_interface)(struct gxav_videoout* vout, GxVideoOutProperty_OutputSelect *output_select);
	int (*set_resolution)(struct gxav_videoout* vout, GxVideoOutProperty_Resolution *resolution);
	int (*set_brightness)(struct gxav_videoout* vout, GxVideoOutProperty_Brightness *brightness);
	int (*set_saturation)(struct gxav_videoout* vout, GxVideoOutProperty_Saturation *saturation);
	int (*set_contrast)(struct gxav_videoout* vout, GxVideoOutProperty_Contrast *contrast);
	int (*set_sharpness)(struct gxav_videoout* vout, GxVideoOutProperty_Sharpness *sharpness);
	int (*set_hue)      (struct gxav_videoout* vout, GxVideoOutProperty_Hue *hue);
	int (*set_aspratio)(struct gxav_videoout* vout, GxVideoOutProperty_AspectRatio *aspect_ratio);
	int (*set_tvscreen)(struct gxav_videoout* vout, GxVideoOutProperty_TvScreen *TvScreen);
	int (*get_resolution)(struct gxav_videoout* vout, GxVideoOutProperty_Resolution *resolution);
	int (*get_dvemode)(struct gxav_videoout* vout, struct vout_dvemode *dvemode);
	int (*get_brightness)(struct gxav_videoout* vout, GxVideoOutProperty_Brightness *brightness);
	int (*get_contrast)(struct gxav_videoout* vout, GxVideoOutProperty_Contrast *contrast);
	int (*get_sharpness)(struct gxav_videoout* vout, GxVideoOutProperty_Sharpness *sharpness);
	int (*get_hue)      (struct gxav_videoout* vout, GxVideoOutProperty_Hue *hue);
	int (*get_saturation)(struct gxav_videoout* vout, GxVideoOutProperty_Saturation *saturation);
	int (*play_cc)(struct gxav_videoout* vout, struct vout_ccinfo *ccinfo);
	int (*set_default)(struct gxav_videoout* vout, GxVideoOutProperty_OutDefault *outdefault);
	int (*set_PowerOff)(struct gxav_videoout* vout, int selection);
	int (*set_PowerOn)(struct gxav_videoout* vout, int selection);
	int (*hdcp_enable)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpEnable* auth);
	int (*hdcp_config)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpConfig* config);
	int (*hdcp_start)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiHdcpEnable* auth);
	int (*get_hdmi_status)(struct gxav_videoout* vout, GxVideoOutProperty_OutHdmiStatus *status);
	int (*get_edid_info)(struct gxav_videoout* vout, GxVideoOutProperty_EdidInfo *edid_info);
	int (*set_macrovision)(struct gxav_videoout* vout, GxVideoOutProperty_Macrovision *macrovision);
	int (*set_dcs)(struct gxav_videoout* vout, GxVideoOutProperty_DCS *dcs);
	int (*get_hdmi_version)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiVersion *version);
	int (*set_cgms_enable)(GxVideoOutProperty_CGMSEnable *param);
	int (*set_cgms_config)(GxVideoOutProperty_CGMSConfig *param);
	int (*scart_config)(struct gxav_videoout* vout, GxVideoOutProperty_ScartConfig *config);
	int (*white_screen)(int enable);
	int (*cec_enable)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiCecEnable* param);
	int (*cec_send_cmd)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiCecCmd* param);
	int (*cec_recv_cmd)(struct gxav_videoout* vout, GxVideoOutProperty_HdmiCecCmd* param);
	int (*set_vbi_wss_enable)(GxVideoOutProperty_WSSEnable *param);
	int (*set_vbi_wss_config)(GxVideoOutProperty_WSSConfig *param);
};


extern int gxav_videoout_init(struct gxav_module_ops* ops);
extern int gxav_videoout_uninit(void);
extern int gxav_videoout_open(int id, struct gxav_module_inode* inode);
extern int gxav_videoout_close(int id);
extern int gxav_videoout_config(int id, GxVideoOutProperty_OutputConfig *param);
extern int gxav_videoout_interface(int id, GxVideoOutProperty_OutputSelect *output_select);
extern int gxav_videoout_resolution(int id, GxVideoOutProperty_Resolution *resolution);
extern int gxav_videoout_brightness(int id, GxVideoOutProperty_Brightness *brightness);
extern int gxav_videoout_saturation(int id, GxVideoOutProperty_Saturation *saturation);
extern int gxav_videoout_contrast(int id, GxVideoOutProperty_Contrast *contrast);
extern int gxav_videoout_sharpness(int id, GxVideoOutProperty_Sharpness *sharpness);
extern int gxav_videoout_hue      (int id, GxVideoOutProperty_Hue       *hue);
extern int gxav_videoout_aspratio(int id, GxVideoOutProperty_AspectRatio *aspect_ratio);
extern int gxav_videoout_tvscreen(int id, GxVideoOutProperty_TvScreen *TvScreen);
extern int gxav_videoout_resolution_information(int id, GxVideoOutProperty_Resolution *resolution);
extern int gxav_videoout_brightness_information(int id, GxVideoOutProperty_Brightness *brightness);
extern int gxav_videoout_contrast_information(int id, GxVideoOutProperty_Contrast *contrast);
extern int gxav_videoout_saturation_information(int id, GxVideoOutProperty_Saturation *saturation);
extern int gxav_videoout_sharpness_information(int id, GxVideoOutProperty_Sharpness *sharpness);
extern int gxav_videoout_hue_information(int id, GxVideoOutProperty_Hue *hue);
extern int gxav_videoout_play_cc(int id, struct vout_ccinfo *ccinfo);
extern int gxav_videoout_get_dvemode(int id, struct vout_dvemode *dvemode);
extern int gxav_videoout_out_default(int id, GxVideoOutProperty_OutDefault *outdefault);
extern int gxav_videoout_PowerOff(int id, unsigned int selection);
extern int gxav_videoout_PowerOn(int id, unsigned int selection);
extern int gxav_videoout_hdcp_enable(int id, GxVideoOutProperty_HdmiHdcpEnable *auth);
extern int gxav_videoout_hdcp_config(int id, GxVideoOutProperty_HdmiHdcpConfig *config);
extern int gxav_videoout_hdcp_start(int id, GxVideoOutProperty_HdmiHdcpEnable *auth);
extern int gxav_videoout_get_hdmi_status(int id, GxVideoOutProperty_OutHdmiStatus *status);
extern int gxav_videoout_get_edid_info(int id, GxVideoOutProperty_EdidInfo *edid_info);
extern int gxav_videoout_set_macrovision(int id, GxVideoOutProperty_Macrovision *macrovision);
extern int gxav_videoout_set_dcs(int id, GxVideoOutProperty_DCS *dcs);
extern int gxav_videoout_set_CGMS_enable(int id, GxVideoOutProperty_CGMSEnable *property);
extern int gxav_videoout_set_CGMS_config(int id, GxVideoOutProperty_CGMSConfig *property);
extern int gxav_videoout_set_WSS_enable (int id, GxVideoOutProperty_WSSEnable  *property);
extern int gxav_videoout_set_WSS_config (int id, GxVideoOutProperty_WSSConfig  *property);
extern int gxav_videoout_scart_config(int id, GxVideoOutProperty_ScartConfig *config);
extern struct gxav_videoout* gxav_videoout_find_instance(int sub);
extern int gxav_videoout_get_hdmi_version(int id, GxVideoOutProperty_HdmiVersion *version);
extern int gxav_videoout_set_hdmi_encoding_out(GxAvHdmiEncodingOut encoding);
extern int gxav_videoout_set_hdmi_color_resolution(GxAvHdmiColorDepth color_depth);
extern int gxav_videoout_white_screen(int id, int enable);
extern int gxav_videoout_cec_enable(int id, GxVideoOutProperty_HdmiCecEnable *param);
extern int gxav_videoout_cec_send_cmd(int id, GxVideoOutProperty_HdmiCecCmd *param);
extern int gxav_videoout_cec_recv_cmd(int id, GxVideoOutProperty_HdmiCecCmd *param);
extern int gxav_videoout_set_hdmi_audiomute(GxVideoOutProperty_HdmiAudioMute *param);
#endif
