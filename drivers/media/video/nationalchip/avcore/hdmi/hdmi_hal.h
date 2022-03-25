#ifndef __HDMI_HAL_H__
#define __HDMI_HAL_H__

#include "gxav_common.h"
#include "avcore.h"
#include "vpu_hal.h"
#include "include/audio_common.h"
#include "gxav_vout_propertytypes.h"

#ifdef GX_DEBUG
#define HDMI_PRINTF(fmt, args...) do{ \
    gx_printf("\n[COMMON][%s():%d]: ", __func__, __LINE__);\
    gx_printf(fmt, ##args); \
} while(0)
#define HDMI_DBG(fmt, args...)   do{\
    gx_printf("\n[COMMON][%s():%d]: ", __func__, __LINE__);\
    gx_printf(fmt, ##args);\
}while(0)
#else
#define HDMI_PRINTF(fmt, args...)   ((void)0)
#define HDMI_DBG(fmt, args...)      ((void)0)
#endif

struct videoout_hdmi_edid {
	/*
	 * tv_cap is the mask of all resolutions TV support, which is get from TV EDID
	 *
	 * tv_cap = (1<<GXAV_VOUT_480I) | (1<<GXAV_VOUT_576I) | ... | (GXAV_VOUT_1080I_50HZ);
	 *
	 **/
	unsigned tv_cap;
	unsigned is_hdmi;

	GxAvHdmiEdid audio_codes;

	/* raw edid data */
	unsigned int  data_len;
	unsigned char data[256];
};

typedef enum {
	GX_ITU601  = 1,
	GX_ITU709  = 2,
	GX_ITU2020 = 3,
	GX_EXTENDED_COLORIMETRY,
} GxColorimetry;

struct gxav_hdmi_module {
	int (*init)(void);
	int (*uninit)(void);
	int (*open)(unsigned delay_ms);
	int (*detect_hotplug)(void);
	int (*read_edid)(struct videoout_hdmi_edid *edid);
	unsigned int (*audio_codes_get)(void);
	void (*clock_set)(GxVideoOutProperty_Mode mode);
	void (*audioout_set)(unsigned int audio_source);
	void (*audioout_mute)(int enable);
	void (*audioout_reset)(unsigned int flg);
	void (*av_mute)(int enable);
	void (*powerdown)(int enable);
	void (*ready)(unsigned int flg);
	void (*audiosample_change)(GxAudioSampleFre samplefre, unsigned int cnum);
	void (*acr_enable)(int enable);
	void (*black_enable)(int enable);//设置黑屏
	void (*videoout_set)(GxVideoOutProperty_Mode vout_mode);
	GxVideoOutProperty_Mode (*videoout_get)(void);
	int (*hdcp_enable)(void);
	int (*hdcp_disable)(void);
	int (*hdcp_start)(GxVideoOutProperty_Mode vout_mode);
	int (*hdcp_config)(GxVideoOutProperty_HdmiHdcpConfig *config);
	int (*cec_enable)(int enable);
	int (*cec_send_cmd)(GxVideoOutProperty_HdmiCecCmd *cmd);
	int (*cec_recv_cmd)(GxVideoOutProperty_HdmiCecCmd *cmd);
	int (*interrupt)(void);
	void (*enable)(int enable);
	unsigned char (*read)(unsigned char addr);
	void (*write)(unsigned char addr, unsigned char data);
	void (*cold_reset_set)(void);
	void (*cold_reset_clr)(void);
	int (*set_brightness)(int b, int c, int s);
	int (*set_saturation)(int b, int c, int s);
	int (*set_contrast)(int b, int c, int s);
	int (*set_colorimetry)(int c);
	int (*get_version)(GxVideoOutProperty_HdmiVersion *version);
	int (*set_encoding_out)(GxAvHdmiEncodingOut encoding);
	int (*set_cgmsa_copy_permission)(int permission);
	int (*support_hdr10)(void);
	int (*support_hlg)(void);
	int (*set_hdr10)(struct frame *frame);
	int (*set_hlg)(struct frame *frame);
	int (*set_color_resolution)(GxAvHdmiColorDepth color_depth);
};

extern int gxav_hdmi_init(struct gxav_module_ops* ops);
extern int gxav_hdmi_uninit(void);
extern int gxav_hdmi_open(unsigned delay_ms);
extern int gxav_hdmi_detect_hotplug(void);
extern int gxav_hdmi_read_edid(struct videoout_hdmi_edid *edid);
extern unsigned int gxav_hdmi_audio_codes_get(void);
extern void gxav_hdmi_clock_set(GxVideoOutProperty_Mode mode);
extern void gxav_hdmi_audioout_set(unsigned int audio_source);
extern void gxav_hdmi_audioout_mute(unsigned int enable);
extern void gxav_hdmi_audioout_reset(unsigned int flg);
extern void gxav_hdmi_audiosample_change(GxAudioSampleFre samplefre, unsigned int cnum);
extern void gxav_hdmi_av_mute(unsigned int enable);
extern void gxav_hdmi_powerdown(unsigned int enable);
extern void gxav_hdmi_acr_enable(int enable);
extern void gxav_hdmi_black_enable(int enable);
extern void gxav_hdmi_videoout_set(GxVideoOutProperty_Mode vout_mode);
extern GxVideoOutProperty_Mode gxav_hdmi_videoout_get(void);
extern int gxav_hdmi_hdcp_enable_auth(void);
extern int gxav_hdmi_hdcp_config(GxVideoOutProperty_HdmiHdcpConfig *config);
extern int gxav_hdmi_hdcp_disable_auth(void);
extern int gxav_hdmi_hdcp_start_auth(GxVideoOutProperty_Mode vout_mode);
extern int gxav_hdmi_interrupt(void);
extern void gxav_hdmi_enable(int enable);
extern unsigned char gxav_hdmi_read(unsigned char addr);
extern void gxav_hdmi_write(unsigned char addr, unsigned char data);
extern void gxav_hdmi_cold_reset_set(void);
extern void gxav_hdmi_cold_reset_clr(void);
extern int  gxav_hdmi_set_brightness(int b, int c, int s);
extern int  gxav_hdmi_set_colorimetry(int c);
extern int  gxav_hdmi_set_saturation(int b, int c, int s);
extern int  gxav_hdmi_set_contrast(int b, int c, int s);
extern void gxav_hdmi_get_version(GxVideoOutProperty_HdmiVersion *version);
extern void gxav_hdmi_ready(unsigned int flg);
extern int  gxav_hdmi_set_encoding_out(GxAvHdmiEncodingOut encoding);
extern int  gxav_hdmi_set_cgmsa_permission(int permission);

extern int gxav_hdmi_cec_enable(int enable);
extern int gxav_hdmi_cec_send_cmd(GxVideoOutProperty_HdmiCecCmd *cmd);
extern int gxav_hdmi_cec_recv_cmd(GxVideoOutProperty_HdmiCecCmd *cmd);

extern int gxav_hdmi_support_hdr10(void);
extern int gxav_hdmi_support_hlg(void);

extern int gxav_hdmi_set_hdr10(struct frame *frame);
extern int gxav_hdmi_set_hlg(struct frame *frame);
extern int gxav_hdmi_set_color_resolution(GxAvHdmiColorDepth color_depth);

#endif
