#ifndef __GX3211_HDMI_H__
#define __GX3211_HDMI_H__

#include "gxav_common.h"
#include "hdmi_hal.h"
#include "include/audio_common.h"
#include "clock_hal.h"
#include "kernelcalls.h"
#include "gxav_bitops.h"
#include "gxav_event_type.h"
#include "gxav_hdcp_key.h"
#include "gxav_vout_propertytypes.h"
#include "hdmi_api_tx/util/types.h"
#include "hdmi_api_tx/util/log.h"
#include "hdmi_api_tx/core/audioParams.h"
#include "hdmi_api_tx/core/videoParams.h"
#include "hdmi_api_tx/core/productParams.h"
#include "hdmi_api_tx/hdcp/hdcpParams.h"
#include "hdmi_api_tx/hdcp/hdcp.h"
#include "hdmi_api_tx/api/api.h"
#include "hdmi_api_tx/cec/cec.h"
#include "hdmi_api_tx/edid/edid.h"
#include "hdmi_api_tx/core/audio.h"
#include "hdmi_api_tx/core/packets.h"
#include "hdmi_api_tx/core/control.h"
#include "hdmi_api_tx/phy/hdmi_phy_hal.h"
#include "hdmi_api_tx/core/halAudioI2s.h"
#include "hdmi_api_tx/core/halFrameComposerDrm.h"
#include "csc.h"

#define GPA_ENABLE          (0)
/* options: hdcp, 3d X, next(mode), modes(all), mode X*/

enum hdmi_state{
	HDMI_NOT_INIT = 1,
	HDMI_INIT,
	HDMI_HOT_PLUG,
	HDMI_EDID_READ,
	HDMI_EDID_ERR,
	HDMI_CONFIGURE,
	HDMI_ERROR,
};

struct hdmi_hdcp_para{
#define HDCP_AKSV_LEN (7)
#define HDCP_ENC_KEY_LEN (2)
#define HDCP_DPK_KEYS_LEN (280)
	u8 aksv[HDCP_AKSV_LEN];
	u8 enc_keys[HDCP_ENC_KEY_LEN];
	u8 dpk_keys[HDCP_DPK_KEYS_LEN];
};

struct gxav_hdmidev{
	enum hdmi_state               state;
	int                           event;
	videoParams_t                 video;
	audioParams_t                 audio;

	int                           hdcp_en;
	hdcpParams_t                  hdcp;
	hdcp_status_t                 hdcp_state;
	int                           hdcp_encrypt;    //1: hdcp encrypt 0: no hdcp encrypt
	int                           edid_err_cnt;
	int                           hdcp_err_cnt;
	struct hdmi_hdcp_para         hdcp_para;
	int                           fetched_key;
#define POST_NO_THING           (0)
#define POST_PLUG_OUT           (1)
#define POST_READ_EDID_ING      (2)
#define POST_READ_EDID_DONE     (3)
#define POST_READ_EDID_ERR      (4)
#define POST_SET_VOUT_MODE      (5)
#define POST_HDCP_SUCCESS       (6)
#define POST_HDCP_FAILED        (7)
#define POST_PLUG_IN            (8)
	volatile int                  post_type;
	productParams_t               product;
	u16                           ceacode;
	struct{
		int cnt_irq;
		int cnt_conn;
		int cnt_disconn;
		int edid_error;
	}stat;
	gx_thread_id thread_id;
	gx_thread_info thread_info;
	gx_sem_id sem_id;
	int b, c, s;
	int src_colorimetry;

	int cec_enable;
	int i2c_bad;
	int data_encrypt;
	int mute_av_when_failed;
	int last_hdcp_state;
	unsigned long long hdcp_start_time;

	int audio_mute_require;

	struct videoout_hdmi_edid edid_info;
	gx_mutex_t cfg_mutex;

	encoding_t encoding_out;
	GxVideoOutProperty_Mode resolution;
	GxVideoOutProperty_Mode resolution_waiting;
	color_depth_t color_depth;
};

struct hdmi_dtd_map{
	u8                      code;
	GxVideoOutProperty_Mode mode;
};

#endif
