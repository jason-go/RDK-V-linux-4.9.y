#include "gx3211_hdmi.h"
#include "hdmi_module.h"
#include "secure_hal.h"
#include "log_printf.h"

struct gxav_module_ops gx3211_hdmi_module;
extern volatile struct clock_regs *gx3211_clock_reg;
static volatile struct gxav_hdmidev g_hdmidev;
static int debug_hdmi_flow = 0;
static const u16 FC_BASE_ADDR = 0x1000;

#define NOS_READ_EDID (1)

#define HDMI_GET_PDEV() ((struct gxav_hdmidev *)(&g_hdmidev))
#define HDMI_GET_PRODUCT(pdev) ((productParams_t *)(&pdev->product))
#define HDMI_GET_AUDIO(pdev) ((audioParams_t *)(&pdev->audio))
#define HDMI_GET_VIDEO(pdev) ((videoParams_t *)(&pdev->video))
#define HDMI_GET_HDCP(pdev) ((hdcpParams_t *)(&pdev->hdcp))

#define API_AUDIOMUTE(enable) \
	do {\
		if (enable == 1)\
		api_AudioMute(1);\
		else {\
			if (pdev->audio_mute_require)\
			api_AudioMute(1);\
			else\
			api_AudioMute(0);\
		}\
	} while(0)

#if (defined(NO_OS) || defined(TEE_OS))
#define HDMI_LOCK_INIT()    (void)0
#define HDMI_LOCK()         (void)0
#define HDMI_UNLOCK()       (void)0
#define HDMI_LOCK_UNINIT()  (void)0
#else
#define HDMI_LOCK_INIT()    gx_mutex_init(&pdev->cfg_mutex)
#define HDMI_LOCK()         gx_mutex_lock(&pdev->cfg_mutex)
#define HDMI_UNLOCK()       gx_mutex_unlock(&pdev->cfg_mutex)
#define HDMI_LOCK_UNINIT()  gx_mutex_destroy(&pdev->cfg_mutex)
#endif

#define HDMI_THREAD_STACK (8 * 1024)
static unsigned char *thread_stack;

static u8 ksvListBuffer[60] = {0};
static u8 ksvDevices = 6;
static const int hdcp_len = 7 + 2 + 280;

static const unsigned cn_hdmi_sfr_clk = 2700;
static u8 cc_hdmi_phy_model = 102;
static const u32 cn_hdmi_base_addr = 0x0;
unsigned repeater_enabled = 0;

static int gn_hdmi_open = 0;
static int gn_hdmi_thread_running = 0;
static struct hdmi_dtd_map gn_dtd_map[] = {
	{2  ,GXAV_VOUT_480P       },
	{6  ,GXAV_VOUT_480I       },
	{21 ,GXAV_VOUT_576I       },
	{17 ,GXAV_VOUT_576P       },
	{19 ,GXAV_VOUT_720P_50HZ  },
	{4  ,GXAV_VOUT_720P_60HZ  },
	{20 ,GXAV_VOUT_1080I_50HZ },
	{5  ,GXAV_VOUT_1080I_60HZ },
	{31 ,GXAV_VOUT_1080P_50HZ },
	{16 ,GXAV_VOUT_1080P_60HZ }
};

static unsigned int GX3211_HDMI_INT_SRC = 54;
static unsigned int GX3211_HDMI_BASE_ADDR = 0x04f00000;

#define gx3211_hdmi_interrupt_mask()   gx_interrupt_mask(GX3211_HDMI_INT_SRC)
#define gx3211_hdmi_interrupt_unmask() gx_interrupt_unmask(GX3211_HDMI_INT_SRC)

static int gx3211_hdmi_open(unsigned delay_ms);
static int gx3211_hdmi_set_brightness(int b, int c, int s);
static int gx3211_hdmi_set_saturation(int b, int c, int s);
static int gx3211_hdmi_set_contrast(int b, int c, int s);
static void _hdmi_delay_configure(struct gxav_hdmidev *pdev);
extern int api_InitializeAudio(u16 address, u8 dataEnablePolarity, u16 sfrClock, u8 phy_model, u16 ceacode, u8 force);
static void gx3211_hdmi_audio_open(void)
{
	if (gn_hdmi_open == 0) {
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
		audioParams_t *audio = HDMI_GET_AUDIO(HDMI_GET_PDEV());
		videoParams_t *video = HDMI_GET_VIDEO(HDMI_GET_PDEV());

		api_InitializeAudio(cn_hdmi_base_addr, 1, cn_hdmi_sfr_clk, cc_hdmi_phy_model, 20, 1);
		audioParams_Reset(audio);
		audioParams_SetGpaSamplePacketInfoSource(audio, 1);

		gx3211_hdmi_interrupt_mask();
		audioParams_SetInterfaceType(audio, I2S);
		audioParams_SetCodingType(audio, PCM);
		if (audio_Configure(cn_hdmi_base_addr, audio,
					videoParams_GetPixelClock(video), videoParams_GetRatioClock(video)) != TRUE) {
			gx3211_hdmi_interrupt_unmask();
			LOG_ERROR2("audio configure", error_Get());
			return;
		}
		packets_AudioInfoFrame(cn_hdmi_base_addr, audio);
		if (pdev->audio_mute_require)
			API_AUDIOMUTE(1);
		gx3211_hdmi_interrupt_unmask();
	}

	return;
}

/*
 * enable ACR packet insertion
 */
static void gx3211_hdmi_acr_enable(int enable)
{
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	if (enable)
		access_Write(access_Read(0x10b7) | 0x1, 0x10b7);
	else
		access_Write(access_Read(0x10b7) & 0xfe, 0x10b7);
}

/*
 * set black screen for hdmi out
 */
static void gx3211_hdmi_black_enable(int enable)
{
	gxlog_d(LOG_HDMI, "%s: %d [black: %d]\n", __func__, __LINE__, enable);
	if (enable)
		access_Write(access_Read(0x3000) | 0x10, 0x3000);
	else
		access_Write(access_Read(0x3000) & 0xef, 0x3000);
}

void gx3211_hdmi_soft_reset_set(void)
{
	access_Write(0x0, 0x4002);
	access_Write(0x0, 0x400a);
	access_Write(0x20, 0x1005);
}

void gx3211_hdmi_soft_reset_clr(void)
{
	access_Write(0xc7, 0x4002);
	access_Write(0x0, 0x400a);
}

#define TOKEN_SIZE  (64)
static int _hdmi_init(struct gxav_hdmidev *pdev);
static int _hdmi_configure(struct gxav_hdmidev *pdev, int force);
static void gx3211_hdmi_thread(void* p);
static void gx3211_hdmi_tigger(struct gxav_hdmidev *pdev);

static int _hdmi_init(struct gxav_hdmidev *pdev)
{
	const u8 vName[] = "Synopsys";
	const u8 pName[] = "HDMI Tx";
	speakerAllocationDataBlock_t allocation;

	productParams_t *product = HDMI_GET_PRODUCT(pdev);
	audioParams_t *audio = HDMI_GET_AUDIO(pdev);
	videoParams_t *video = HDMI_GET_VIDEO(pdev);
	hdcpParams_t *hdcp = HDMI_GET_HDCP(pdev);

	LOG_MENUSTR("HDMI Tx demo: ", __DATE__);
	productParams_Reset(product);
	productParams_SetVendorName(product, vName, sizeof(vName) - 1);
	productParams_SetProductName(product, pName, sizeof(pName) - 1);
	productParams_SetSourceType(product, 0x0A);
	/* only when 3D is disabled, which is the case upon initialisation */

	audioParams_Reset(audio);
#if GPA_ENABLE
	LOG_NOTICE("GPA interface");
	audioParams_SetInterfaceType(audio, GPA);
	audioParams_SetCodingType(audio, PCM);
	audioParams_SetChannelAllocation(audio, 0);
	audioParams_SetPacketType(audio, AUDIO_SAMPLE);
	audioParams_SetSampleSize(audio, 16);
	audioParams_SetSamplingFrequency(audio, 44100);
	audioParams_SetLevelShiftValue(audio, 0);
	audioParams_SetDownMixInhibitFlag(audio, 0);
	audioParams_SetClockFsFactor(audio, 128);
#endif

	videoParams_Reset(video);
	videoParams_SetEncodingIn(video, YCC444);
	videoParams_SetEncodingOut(video, pdev->encoding_out);

	hdcpParams_Reset(hdcp);
	hdcpParams_SetNoOfSupportedDevices(hdcp, ksvDevices);
	hdcpParams_SetKsvListBuffer(hdcp, ksvListBuffer);

	if (api_EdidSpeakerAllocationDataBlock(&allocation))
	{
		audioParams_SetChannelAllocation(audio, speakerAllocationDataBlock_GetChannelAllocationCode(&allocation));
	}
	videoParams_SetColorResolution(video, 0);

	return TRUE;
}

static int _hdmi_configure(struct gxav_hdmidev *pdev, int force)
{
	dtd_t tmp_dtd;
	u8 hdmi_flag = TRUE;
	encoding_t encoding_out;
	color_depth_t color_depth;

	HDMI_LOCK();
	LOG_DEBUG("start");
	/* needed to make sure it doesnt change while in this function*/
	api_AvMute(TRUE);
	phy_Gen2PDDQ(0x0, 1);
	if(TRUE)
	{
		productParams_t *product = HDMI_GET_PRODUCT(pdev);
		audioParams_t *audio = HDMI_GET_AUDIO(pdev);
		videoParams_t *video = HDMI_GET_VIDEO(pdev);
		hdcpParams_t *hdcp = HDMI_GET_HDCP(pdev);

		dtd_Fill(&tmp_dtd, pdev->ceacode, board_SupportedRefreshRate(pdev->ceacode));
		videoParams_SetDtd(video, &tmp_dtd);

		if (pdev->edid_info.data_len != 0) {
			if (edid_IsHdmi() == 1) {
				hdmi_flag    = TRUE;
				encoding_out = pdev->encoding_out;
				color_depth  = pdev->color_depth;
			} else {
				hdmi_flag    = FALSE;
				encoding_out = RGB;
				color_depth  = HDMI_24BPP;
			}
		} else {
			hdmi_flag    = TRUE;
			encoding_out = RGB;
			color_depth  = HDMI_24BPP;
		}

		videoParams_SetHdmi(video, hdmi_flag);
		videoParams_SetColorResolution(video, color_depth);

		videoParams_SetEncodingIn(video,  YCC444);
		videoParams_SetEncodingOut(video, encoding_out);

		if (pdev->ceacode == 2 || pdev->ceacode == 6 || pdev->ceacode == 21 || pdev->ceacode == 17){
			videoParams_SetColorimetry(video, ITU601);
		}else{
			videoParams_SetColorimetry(video, ITU709);
		}

		hdcpParams_Reset(hdcp);
		if (pdev->fetched_key&&pdev->hdcp_en){
			hdcpParams_SetNoOfSupportedDevices(hdcp, ksvDevices);
			hdcpParams_SetKsvListBuffer(hdcp, ksvListBuffer);
			/* For Tx 1.40a HDCP encryption */
			hdcpParams_SetDpkInfo(hdcp, pdev->hdcp_para.aksv, pdev->hdcp_para.enc_keys, pdev->hdcp_para.dpk_keys);
		} else {
			hdcp = NULL;
		}

		if(force)
			api_ConfigureExt(video, audio, product, hdcp, cc_hdmi_phy_model, pdev->ceacode);
		else
			api_Configure(video, audio, product, hdcp, cc_hdmi_phy_model, pdev->ceacode, pdev->cec_enable);

		if (pdev->hdcp_en)
			api_HdcpBypassEncryption(!pdev->data_encrypt);
		else
			api_HdcpBypassEncryption(1);
	}

	if (pdev->hdcp_en == 0) {
		gx3211_hdmi_acr_enable(1);
		gx_mdelay(20);
		api_AvMute(FALSE);
	}

	if (pdev->audio_mute_require)
		API_AUDIOMUTE(1);
	LOG_DEBUG("end");
	HDMI_UNLOCK();

	return TRUE;
}

static int gx3211_hdmi_read_edid(struct videoout_hdmi_edid *edid)
{
	int ret = -1;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (edid && pdev->edid_info.data_len) {
		gx_memcpy((void*)edid, (void*)&pdev->edid_info, sizeof(struct videoout_hdmi_edid));
		ret  = 0;
	}

	return ret;
}

static unsigned int gx3211_hdmi_audio_codes_get(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	return pdev->edid_info.audio_codes;
}

static int hdmi_hotplug_detect(void)
{
	if (!gx3211_hdmi_open(0)) {
		if(TRUE == api_HotPlugDetected()){
			return 0;
		}
	}
	return -1;
}

static int gx3211_hdmi_detect_hotplug(void)
{
	return hdmi_hotplug_detect();
}

static int resolution_to_ceacode(GxVideoOutProperty_Mode vout_mode)
{
	int i;
	int ret = 0;

	for (i = 0; i < (sizeof(gn_dtd_map) / sizeof(gn_dtd_map[0])); i++) {
		if (gn_dtd_map[i].mode == vout_mode){
			ret = gn_dtd_map[i].code;
			break;
		}
	}

	return ret;
}

static GxVideoOutProperty_Mode ceacode_to_resolution(u8 code)
{
	int i;
	int ret = GXAV_VOUT_NULL_MAX;

	for (i = 0; i < (sizeof(gn_dtd_map) / sizeof(gn_dtd_map[0])); i++) {
		if (gn_dtd_map[i].code == code) {
			ret = gn_dtd_map[i].mode;
			break;
		}
	}

	return ret;
}

static void gx3211_hdmi_clock_set(GxVideoOutProperty_Mode mode)
{

}

static void gx3211_hdmi_hdcp_set_key(struct hdmi_hdcp_para *para)
{
	if (gn_hdmi_open && para){
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

		gx_memcpy(pdev->hdcp_para.aksv, para->aksv, HDCP_AKSV_LEN);
		gx_memcpy(pdev->hdcp_para.enc_keys, para->enc_keys, HDCP_ENC_KEY_LEN);
		gx_memcpy(pdev->hdcp_para.dpk_keys, para->dpk_keys, HDCP_DPK_KEYS_LEN);
	}
}

extern int* get_csc_params(int colorprimaries, HdmiFormat_t outFormat, int contrast, int brightness, int saturation, int rgb_flag);
static int gx3211_hdmi_set_colorimetry(int c)
{
	int i, *params = NULL;
	int src, dst;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	videoParams_t *video = HDMI_GET_VIDEO(pdev);
	encoding_t encoding_out = videoParams_GetEncodingOut(video);

	src = c;
	if (pdev->ceacode == 2 || pdev->ceacode == 6)
		dst = SDTV_525;
	else if (pdev->ceacode == 21 || pdev->ceacode == 17)
		dst = SDTV_625;
	else
		dst = HDTV;

	//gx_printf("\nsrc = %d, dst = %d\n", src, dst);
	params = get_csc_params(src, dst, pdev->c, pdev->b, pdev->s, (encoding_out == RGB));
	if (params != NULL) {
		access_Write((4<<4)|params[0], 0x4101);
		for (i = 1; i <= 24; i++)
			access_Write(params[i], (0x4100 + 1 + i));
	};

	return 0;
}

static void hdmi_set_resolution(GxVideoOutProperty_Mode resolution)
{
	u8 code;
	dtd_t dtd;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	videoParams_t *video = HDMI_GET_VIDEO(pdev);

	if (pdev->post_type == POST_PLUG_OUT)
		return;

	gx3211_hdmi_interrupt_mask();
	if ((code = resolution_to_ceacode(resolution)) == 0) {
		gx3211_hdmi_interrupt_unmask();
		gx_printf("%s, resolution error = %d", __func__, resolution);
		return;
	}
	pdev->ceacode    = code;
	pdev->resolution = resolution;
	dtd_Fill(&dtd, code, board_SupportedRefreshRate(code));
	videoParams_SetDtd(video, &dtd);
	gx3211_hdmi_interrupt_unmask();

	_hdmi_delay_configure(pdev);
	gx3211_hdmi_set_colorimetry(pdev->src_colorimetry);
}

static void gx3211_hdmi_set_resolution(GxVideoOutProperty_Mode resolution)
{
	gx3211_hdmi_open(0);

	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (gn_hdmi_open) {
	gxlog_d(LOG_HDMI, "%s: %d [resolution: %d]\n", __func__, __LINE__, resolution);
#if ((defined(NO_OS) || defined(TEE_OS)) && (NOS_READ_EDID == 0))
		hdmi_set_resolution(resolution);
#else
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
		gx3211_hdmi_interrupt_mask();
		switch (pdev->post_type) {
			case POST_READ_EDID_DONE:
			case POST_READ_EDID_ERR:
			case POST_HDCP_SUCCESS:
				gx3211_hdmi_interrupt_unmask();
				hdmi_set_resolution(resolution);
				break;
			default:
				gx3211_hdmi_interrupt_unmask();
				pdev->resolution_waiting = resolution;
				return;
		}
#endif
	}

	return;
}

static GxVideoOutProperty_Mode gx3211_hdmi_get_resolution(void)
{
	u8 code = access_Read(0x101c);

	if (code) {
		int i;
		for(i = 0; i < (sizeof(gn_dtd_map) / sizeof(gn_dtd_map[0])); i++){
			if(gn_dtd_map[i].code == code){
				return gn_dtd_map[i].mode;
			}
		}
	}

	return 0;
}


static void gx3211_hdmi_audioout_set(unsigned int audio_source)
{
	interfaceType_t type;
	codingType_t code_type;

	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (gn_hdmi_open) {
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
		videoParams_t *video = HDMI_GET_VIDEO(pdev);
		audioParams_t *audio = HDMI_GET_AUDIO(pdev);

		switch (audio_source) {
		case 0:
			type = I2S;
			code_type = PCM;
			break;
		case 1:
			type = SPDIF;
			code_type = AC3;
			break;
		default:
			return;
		}
		gxlog_d(LOG_HDMI, "%s: %d [%s]\n", __func__, __LINE__, (type == I2S)?"I2S":"SPDIF");
		/*
		 * the same interface type
		 */
		if (audioParams_GetInterfaceType(audio) == type)
			return;

		gx3211_hdmi_interrupt_mask();
		API_AUDIOMUTE(1);
		audioParams_SetInterfaceType(audio, type);
		audioParams_SetCodingType(audio, code_type);
		if (audio_Configure(0, audio, videoParams_GetPixelClock(video), videoParams_GetRatioClock(video)) != TRUE) {
			gx3211_hdmi_interrupt_unmask();
			LOG_ERROR2("audio configure", error_Get());
			API_AUDIOMUTE(0);
			return;
		}
		packets_AudioInfoFrame(0, audio);
		API_AUDIOMUTE(0);
		gx3211_hdmi_interrupt_unmask();
	}
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);

	return;
}

static void gx3211_hdmi_audioout_mute(int enable)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d [mute %d]\n", __func__, __LINE__, enable);
	pdev->audio_mute_require = enable;
	API_AUDIOMUTE(enable);
	//if (enable)
	//	access_Write(access_Read(0x4001) | 0x08 , 0x4001);
	//else
	//	access_Write(access_Read(0x4001) & 0xf7 , 0x4001);

	return;
}

static void gx3211_hdmi_av_mute(int enable)
{
	gxlog_d(LOG_HDMI, "%s: %d [avmute: %d]\n", __func__, __LINE__, enable);
	api_AudioMute(enable);

	if (enable) {
		//halAudioI2s_DataEnable(0x3100,0x0);
		api_AvMute(TRUE);
	} else {
		//halAudioI2s_DataEnable(0x3100,0xf);
		api_AvMute(FALSE);
	}
	gx_mdelay(20);
}

static void gx3211_hdmi_powerdown(int enable)
{
	gxlog_d(LOG_HDMI, "%s: %d [powerdown: %d]\n", __func__, __LINE__, enable);
	if (enable)
		phy_Gen2PDDQ(0x0, 1);
	else
		phy_Gen2PDDQ(0x0, 0);
}

static void gx3211_hdmi_cold_reset_set(void)
{
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	gxav_clock_hot_rst_set(MODULE_TYPE_HDMI);
}

static void gx3211_hdmi_cold_reset_clr(void)
{
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	gxav_clock_hot_rst_clr(MODULE_TYPE_HDMI);
}

static u8* gx3211_hdmi_reg = NULL;
static void gx3211_HdmiIOUnmap(void)
{
	if(gx3211_hdmi_reg) {
		gx_iounmap(gx3211_hdmi_reg);
		gx_release_mem_region(GX3211_HDMI_BASE_ADDR, 0x20000);
		gx3211_hdmi_reg = NULL;
	}

	return ;
}

static int gx3211_HdmiIORemap(void)
{
	if(!gx_request_mem_region(GX3211_HDMI_BASE_ADDR, 0x20000)) {
		gx_printf("request_mem_region failed");
		goto HDMI_IOMAP_ERROR;
	}
	gx3211_hdmi_reg = gx_ioremap(GX3211_HDMI_BASE_ADDR, 0x20000);
	if(!gx3211_hdmi_reg) {
		goto HDMI_IOMAP_ERROR;
	}

	return 0;

HDMI_IOMAP_ERROR:
	gx3211_HdmiIOUnmap();
	return -1;
}

static int gx3211_hdmi_init(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (debug_hdmi_flow)
		gxav_config(LOG_DEBUG, LOG_HDMI);

	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	if(gx3211_HdmiIORemap() != 0)
		return -1;
	access_Initialize(gx3211_hdmi_reg);

	memset((void *)pdev, 0, sizeof(struct gxav_hdmidev));
	pdev->edid_info.data_len = 0;
	pdev->cec_enable   = 0;
	pdev->data_encrypt = 1;
	pdev->mute_av_when_failed = 1;
	pdev->encoding_out = RGB;
	pdev->color_depth  = HDMI_24BPP;
	pdev->state = HDMI_NOT_INIT;
	pdev->hdcp_state = HDCP_IDLE;
	pdev->post_type = POST_NO_THING;
	pdev->ceacode = 2;
	pdev->src_colorimetry = 0;
	pdev->b = pdev->c = pdev->s = 50;
	pdev->resolution = pdev->resolution_waiting = GXAV_VOUT_NULL_MAX;
	pdev->hdcp_en = 0;
	pdev->audio_mute_require = 0;
	log_SetVerbose(VERBOSE_ERROR);

	HDMI_LOCK_INIT();

	thread_stack = gx_malloc(HDMI_THREAD_STACK);
	gx_sem_create((gx_sem_id *)&pdev->sem_id, 0);
	gn_hdmi_thread_running = 1;

	gx_thread_create("gx3211_hdmi_thread", (void *)&pdev->thread_id,
			gx3211_hdmi_thread,
			(void *)pdev, thread_stack,
			HDMI_THREAD_STACK, 9, (gx_thread_info *)&pdev->thread_info);

	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);

	return 0;
}

static int gx3211_hdmi_uninit(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	gn_hdmi_thread_running = 0;
	gx_sem_post((gx_sem_id *)&pdev->sem_id);
	gx_thread_delete(pdev->thread_id);
	gx_sem_delete((gx_sem_id *)&pdev->sem_id);
	HDMI_LOCK_UNINIT();
	if (thread_stack) {
		gx_free(thread_stack);
		thread_stack = NULL;
	}
	gn_hdmi_open = 0;

	gx3211_HdmiIOUnmap();
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	return 0;
}

static int gx3211_hdmi_open(unsigned delay_ms)
{
	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (!gn_hdmi_open) {
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

        //halAudioI2s_DataEnable(0x3100,0x0);
		api_AudioMute(TRUE);
		gx_mdelay(20);
		phy_Gen2PDDQ(0x0, 1);

		gx_mdelay(delay_ms);

		gx3211_hdmi_cold_reset_set();
		gx_mdelay(2);
		gx3211_hdmi_interrupt_mask();
		gx3211_hdmi_cold_reset_clr();
		if (!api_Initialize(cn_hdmi_base_addr, 1, cn_hdmi_sfr_clk, 0, cc_hdmi_phy_model, pdev->ceacode)){
			pdev->state = HDMI_ERROR;
			LOG_ERROR2("API initialize", error_Get());
			gx3211_hdmi_interrupt_unmask();
			return -1;
		}

		gn_hdmi_open = 1;
		pdev->state = HDMI_INIT;
		_hdmi_init(HDMI_GET_PDEV());
		_hdmi_configure(HDMI_GET_PDEV(), 1);
		gx3211_hdmi_interrupt_unmask();
	}
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);

	return 0;
}

static int gx3211_hdmi_interrupt(void)
{
	int event;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	pdev->stat.cnt_irq++;
	api_EventHandler(0);

	if (pdev->event && gn_hdmi_open) {
		event = pdev->event;
		pdev->event = 0;
		return event;
	}

	return 0;
}

static void gx3211_hdmi_audioout_reset(unsigned int flg)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d [reset %d]\n", __func__, __LINE__, flg);
	if (flg) {
		API_AUDIOMUTE(1);
	} else {
		API_AUDIOMUTE(0);
	}
}

void gx3211_hdmi_pluged_in(void);
static void gx3211_hdmi_ready(unsigned int flg)
{
	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (gn_hdmi_open == 0) {
		dtd_t dtd;
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
		videoParams_t *video = HDMI_GET_VIDEO(pdev);

		u8 code = access_Read(0x101c);
		pdev->ceacode = code;
		pdev->resolution = ceacode_to_resolution(code);
		dtd_Fill(&dtd, code, board_SupportedRefreshRate(code));
		videoParams_SetDtd(video, &dtd);
		gx3211_hdmi_audio_open();
		gn_hdmi_open = 1;

#if 0
		gx3211_hdmi_pluged_in();
#else
		pdev->state = HDMI_HOT_PLUG;
		pdev->hdcp_state = HDCP_IDLE;
		pdev->edid_err_cnt = 0;
		if (!api_EdidRead()) {
			LOG_ERROR2("cannot read E-EDID", error_Get());
			pdev->state = HDMI_ERROR;
			return;
		}
#endif
		gx3211_hdmi_acr_enable(1);
	}
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
}

static int _sample_enum_to_int(GxAudioSampleFre samplefre, u32 *freq)
{
	switch(samplefre){
	case AUDIO_SAMPLE_FRE_44KDOT1HZ:
		*freq = 44100;
		break;
	case AUDIO_SAMPLE_FRE_48KHZ:
		*freq = 48000;
		break;
	case AUDIO_SAMPLE_FRE_32KHZ:
		*freq = 32000;
		break;
	case AUDIO_SAMPLE_FRE_22KDOT05HZ:
		*freq = 22050;
		break;
	case AUDIO_SAMPLE_FRE_24KHZ:
		*freq = 24000;
		break;
	case AUDIO_SAMPLE_FRE_16KHZ:
		*freq = 16000;
		break;
	case AUDIO_SAMPLE_FRE_96KHZ:
		*freq = 96000;
		break;
	case AUDIO_SAMPLE_FRE_88KDOT2HZ:
		*freq = 88200;
		break;
	case AUDIO_SAMPLE_FRE_128KHZ:
		*freq = 128000;
		break;
	case AUDIO_SAMPLE_FRE_176KDOT4HZ:
		*freq = 176400;
		break;
	case AUDIO_SAMPLE_FRE_192KHZ:
		*freq = 192000;
		break;
	case AUDIO_SAMPLE_FRE_64KHZ:
		*freq = 64000;
		break;
	case AUDIO_SAMPLE_FRE_12KHZ:
		*freq = 12000;
		break;
	case AUDIO_SAMPLE_FRE_11KDOT025HZ:
		*freq = 11025;
		break;
	case AUDIO_SAMPLE_FRE_9KDOT6HZ:
		*freq = 9600;
		break;
	case AUDIO_SAMPLE_FRE_8KHZ:
		*freq = 8000;
		break;
	default:
		return -1;
	}

	return 0;
}

// called by adec interrupt
static void gx3211_hdmi_audiosample_change(GxAudioSampleFre samplefre, unsigned int cnum)
{
	u8  channel_value = 0;
	u32 freq;

	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (gn_hdmi_open) {
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
		videoParams_t *video = HDMI_GET_VIDEO(pdev);
		audioParams_t *audio = HDMI_GET_AUDIO(pdev);

		if(_sample_enum_to_int(samplefre, &freq))
			return;

		if (cnum == 2)
			channel_value = 0;
		else if (cnum == 8)
			channel_value = 0x13;

		gxlog_d(LOG_HDMI, "%s: %d [samplefre:%d, cnum:%d]\n", __func__, __LINE__, freq, cnum);
		audioParams_SetChannelAllocation(audio, channel_value);
		audioParams_SetSamplingFrequency(audio, freq);
		if (audio_Configure(0, audio, videoParams_GetPixelClock(video), videoParams_GetRatioClock(video)) != TRUE) {
			LOG_ERROR2("audio configure", error_Get());
			gx3211_hdmi_interrupt_unmask();
		} else {
			packets_AudioInfoFrame(0, audio);
		}
		if (pdev->audio_mute_require)
			API_AUDIOMUTE(1);
	}
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);

	return;
}

static int gx3211_hdmi_hdcp_disable(void);
static void gx3211_hdmi_enable(int enable)
{
	static int hdcp_on = 0;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (gn_hdmi_open == 0)
		return;

	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, enable);
	if (enable == 1){
		gx3211_hdmi_interrupt_mask();
		if (pdev->state != HDMI_NOT_INIT) {
			gx3211_hdmi_interrupt_unmask();
			return;
		}
		pdev->state = HDMI_INIT;

		access_Standby(1);
		gx3211_hdmi_cold_reset_set();
		gx_mdelay(2);
		gx3211_hdmi_cold_reset_clr();
		access_Standby(0);

		if (!api_Initialize(0, 1, cn_hdmi_sfr_clk, FALSE, cc_hdmi_phy_model, pdev->ceacode)){
			LOG_ERROR2("API initialize", error_Get());
			pdev->state = HDMI_ERROR;
			return;
		}
		_hdmi_init(pdev);
		if (hdcp_on == 0)
			_hdmi_configure(pdev, 1);
		gx3211_hdmi_interrupt_unmask();
	} else {
		gx3211_hdmi_interrupt_mask();
		//if (pdev->state == HDMI_NOT_INIT) {
		//	gx3211_hdmi_interrupt_unmask();
		//	return;
		//}

		pdev->state = HDMI_NOT_INIT;
		pdev->post_type = POST_PLUG_OUT;
		pdev->stat.cnt_disconn++;
		hdcp_on = pdev->hdcp_en;

		if (hdcp_on) {
			api_AvMute(TRUE);
			gx3211_hdmi_hdcp_disable();
		}
		if (!api_Standby()){
			LOG_ERROR2("API standby", error_Get());
			pdev->state = HDMI_ERROR;
		}
		gx3211_hdmi_interrupt_unmask();
	}
	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);

	return;
}

void gx3211_hdmi_pluged_out(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	LOG_DEBUG(" --- PLUGED OUT --- ");
	pdev->state = HDMI_NOT_INIT;
	pdev->stat.cnt_disconn++;
	pdev->hdcp_state = HDCP_IDLE;
	pdev->edid_info.audio_codes = 0;
	pdev->event = EVENT_HDMI_HOTPLUG_OUT;
	pdev->edid_err_cnt = 0;
	pdev->edid_info.data_len = 0;
	gxav_secure_set_hdcp_state(AV_HDCP_IDLE);

	api_HdcpMask();
	api_EdidMask();
#ifdef SET_RESOLUTION_WHEN_PLUGOUT
	pdev->post_type = POST_PLUG_OUT;
	gx3211_hdmi_tigger(pdev);
#endif
}

#ifdef SET_RESOLUTION_WHEN_PLUGOUT
static void hdmi_plug_out(struct gxav_hdmidev *pdev)
{
	gx3211_hdmi_open(0);

	if(!api_Initialize(0, 1, cn_hdmi_sfr_clk, FALSE, cc_hdmi_phy_model, pdev->ceacode)){
		LOG_ERROR2("API initialize", error_Get());
		pdev->state = HDMI_ERROR;
		return;
	}

	gx3211_hdmi_interrupt_mask();
	pdev->state = HDMI_INIT;
	pdev->post_type = POST_NO_THING;
	gx3211_hdmi_interrupt_unmask();
	_hdmi_delay_configure(pdev);

	return;
}
#endif

static void hdmi_fetch_key(void)
{
	unsigned int  hdcp_key = 0;
	unsigned int  hdmi_hdcp_key_len = 0;
	unsigned char *hdmi_hdcp_key = NULL;

	struct hdmi_hdcp_para para;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxav_hdcp_key_fecth(&hdcp_key, &hdmi_hdcp_key_len);
	hdmi_hdcp_key = (unsigned char*)hdcp_key;
	if ((hdmi_hdcp_key_len != hdcp_len) || (hdmi_hdcp_key == 0)) {
		pdev->fetched_key = 0;
		LOG_DEBUG(" --- HDCP FETCH KEY FAILED --- ");
	} else {
		LOG_DEBUG(" --- HDCP FETCH KEY SUCCESS --- ");
		gx_memcpy(para.aksv, hdmi_hdcp_key, HDCP_AKSV_LEN);
		gx_memcpy(para.enc_keys, hdmi_hdcp_key + HDCP_AKSV_LEN, HDCP_ENC_KEY_LEN);
		gx_memcpy(para.dpk_keys, hdmi_hdcp_key + HDCP_AKSV_LEN + HDCP_ENC_KEY_LEN, HDCP_DPK_KEYS_LEN);
		gx3211_hdmi_hdcp_set_key(&para);
		pdev->fetched_key = 1;
	}
}

int HdmiSet_Main(void)
{
	u8 edid_blk_num = 0;
	unsigned int tag_code=0,len1=0,len2=0,offset=0,d_offset=0,acode=0,audio_codes=0;
	u8 *edid_data = NULL;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (gn_hdmi_open == 0)
		return 0;

	hdmi_fetch_key();

	/* application needs EDID read to start functioning */
	edid_data = pdev->edid_info.data;
	edid_GetRawEdid(&edid_blk_num, edid_data);
	pdev->edid_info.data_len = edid_blk_num*128;

	if ((edid_blk_num > 1) &&
			(edid_data[(edid_blk_num - 1) * 128 + 0] == 0x02) &&
			(edid_data[(edid_blk_num - 1) * 128 + 1] == 0x03)) {
		edid_blk_num = edid_blk_num - 1;
		d_offset = edid_data[edid_blk_num * 128 + 2];

		for (offset = 4;offset < d_offset;) {
			tag_code = edid_data[edid_blk_num * 128 + offset]&0xe0;
			len1 = edid_data[edid_blk_num * 128 + offset]&0x1f;
			HDMI_DBG("%d,%d,tag_code=%x,len1=%d======\n",offset,d_offset,tag_code,len1);
			if ((edid_data[edid_blk_num * 128 + offset]&0xe0) == 0x20) {
				offset+=1;
				len2 = len1 + offset;
				for (;offset<len2;offset+=3) {
					acode = (edid_data[edid_blk_num * 128 + offset]>>3)&0xf;
					if (acode==0x1)
						audio_codes |= EDID_AUDIO_LINE_PCM;
					else if (acode==0x2)
						audio_codes |= EDID_AUDIO_AC3;
					else if (acode==0xa)
						audio_codes |= EDID_AUDIO_EAC3;
					else if (acode==0x6)
						audio_codes |= EDID_AUDIO_AAC;
					else if (acode==0x7)
						audio_codes |= EDID_AUDIO_DTS;
				}

				if (len1==0) {
					audio_codes |= EDID_AUDIO_LINE_PCM;
					HDMI_DBG("EDID read audio, len1 = zero !!!!! \n");
				}
				pdev->edid_info.audio_codes = audio_codes;
				HDMI_DBG("EDID read audio, %d ###### \n",audio_codes);
				break;
			} else
				offset +=(len1+1);
		}
	}

	pdev->stat.edid_error = 0;
	pdev->state = HDMI_EDID_READ;
	pdev->event = EVENT_HDMI_READ_EDID|EVENT_HDMI_START_AUTH;
	pdev->post_type = POST_READ_EDID_DONE;
	pdev->edid_err_cnt = 0;
	pdev->hdcp_err_cnt = 0;
	gx3211_hdmi_tigger(pdev);

	return 0;
}

static void _hdmi_delay_configure(struct gxav_hdmidev *pdev)
{
	if (gn_hdmi_open) {
		gx3211_hdmi_interrupt_mask();
		if (pdev->state >= HDMI_EDID_READ) {
			pdev->state = HDMI_CONFIGURE;
			gx3211_hdmi_interrupt_unmask();
			_hdmi_configure(HDMI_GET_PDEV(), 0);
		} else {
			gx3211_hdmi_interrupt_unmask();
			_hdmi_configure(HDMI_GET_PDEV(), 1);
		}

		gx3211_hdmi_set_brightness(pdev->b, pdev->c, pdev->s);
		gx3211_hdmi_set_saturation(pdev->b, pdev->c, pdev->s);
		gx3211_hdmi_set_contrast(pdev->b, pdev->c, pdev->s);
	}
}

static int gx3211_hdmi_get_tv_cap(u8 *edid_data)
{
	unsigned ret = 0;
	u8  i, len = 0;
	u8  video_id_code = 0;
	u8 *cea_861d = NULL, *video_data_block = NULL;

	static struct {
		char                   *video_id_code_desc;
		GxVideoOutProperty_Mode gx_mode;
	} vic_tab[] = {
		{""                                  , GXAV_VOUT_NULL_MAX  },
		{"640x480p    60hz  4:3  1:1"        , GXAV_VOUT_NULL_MAX  },
		{"720x480p    60hz  4:3  8:9"        , GXAV_VOUT_NULL_MAX  },
		{"720x480p    60hz 16:9 32:27"       , GXAV_VOUT_480P      },
		{"1280x720p   60hz 16:9  1:1"        , GXAV_VOUT_720P_60HZ },
		{"1920x1080i  60hz 16:9  1:1"        , GXAV_VOUT_1080I_60HZ},
		{"720x480i    60hz  4:3  8:9"        , GXAV_VOUT_NULL_MAX  },
		{"720x480i    60hz 16:9 32:27"       , GXAV_VOUT_480I      },
		{"720x240p    60hz  4:3  4:9"        , GXAV_VOUT_NULL_MAX  },
		{"720x240p    60hz 16:9 16:27"       , GXAV_VOUT_NULL_MAX  },
		{"2880x480i   60hz  4:3  2:9-20:9"   , GXAV_VOUT_NULL_MAX  },
		{"2880x480i   60hz 16:9  8:27-80:27" , GXAV_VOUT_NULL_MAX  },
		{"2880x240p   60hz  4:3  1:9-10:9"   , GXAV_VOUT_NULL_MAX  },
		{"2880x240p   60hz 16:9  4:27-40:27" , GXAV_VOUT_NULL_MAX  },
		{"1440x480p   60hz  4:3  4:9"        , GXAV_VOUT_NULL_MAX  },
		{"1440x480p   60hz 16:9 16:27"       , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  60hz 16:9  1:1"        , GXAV_VOUT_1080P_60HZ},

		{"720x576p    50hz  4:3 16:15"       , GXAV_VOUT_NULL_MAX  },
		{"720x576p    50hz 16:9 64:45"       , GXAV_VOUT_576P      },
		{"1280x720p   50hz 16:9  1:1"        , GXAV_VOUT_720P_50HZ },
		{"1920x1080i  50hz 16:9  1:1"        , GXAV_VOUT_1080I_50HZ},
		{"720x576i    50hz  4:3 16:15"       , GXAV_VOUT_NULL_MAX  },
		{"720x576i    50hz 16:9 64:45"       , GXAV_VOUT_576I      },
		{"720x288p    50hz  4:3  8:15"       , GXAV_VOUT_NULL_MAX  },
		{"720x288p    50hz 16:9 32:45"       , GXAV_VOUT_NULL_MAX  },
		{"2880x576i   50hz  4:3  2:15-20:15" , GXAV_VOUT_NULL_MAX  },
		{"2880x576i   50hz 16:9 16:45-160:45", GXAV_VOUT_NULL_MAX  },
		{"2880x288p   50hz  4:3  1:15-10:15" , GXAV_VOUT_NULL_MAX  },
		{"2880x288p   50hz 16:9  8:45-80:45" , GXAV_VOUT_NULL_MAX  },
		{"1440x576p   50hz  4:3  8:15"       , GXAV_VOUT_NULL_MAX  },
		{"1440x576p   50hz 16:9 32:45"       , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  50hz 16:9  1:1"        , GXAV_VOUT_1080P_50HZ},

		{"1920x1080p  24hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  25hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },
		{"1920x1080p  30hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },

		{"2880x480p   60hz  4:3  2:9"        , GXAV_VOUT_NULL_MAX  },
		{"2880x480p   60hz 16:9  8:27"       , GXAV_VOUT_NULL_MAX  },
		{"2880x576p   50hz  4:3  4:15"       , GXAV_VOUT_NULL_MAX  },
		{"2880x576p   50hz 16:9 16:45"       , GXAV_VOUT_NULL_MAX  },
		{"1920x1080i  50hz 16:9  1:1"        , GXAV_VOUT_NULL_MAX  },
	};

	if (edid_data != NULL) {
		cea_861d = edid_data;
		if (cea_861d[0] != 0x02 || cea_861d[1] != 0x03)
			goto out;
		video_data_block = &cea_861d[4];
		if (video_data_block[0] >> 5 != 0x02)
			goto out;
		len = video_data_block[0] & 0x1f;
		for (i = 0; i < len; i++) {
			video_id_code = video_data_block[i+1] & 0x7f;
			if (video_id_code < sizeof(vic_tab)/sizeof(vic_tab[0])) {
				//gx_printf("[%d] %s\n", video_id_code, vic_tab[video_id_code].video_id_code_desc);
				if (vic_tab[video_id_code].gx_mode != GXAV_VOUT_NULL_MAX)
					ret |= (1<<vic_tab[video_id_code].gx_mode);
			}
		}
	}

out:
	//gx_printf("\nsupport code = 0x%x\n", ret);
	return ret;
}

static int gx3211_hdmi_hdcp_start(GxVideoOutProperty_Mode resolution);
static void hdmi_edid_over(struct gxav_hdmidev *pdev)
{
	if (gn_hdmi_open) {
		gx3211_hdmi_set_brightness(pdev->b, pdev->c, pdev->s);
		gx3211_hdmi_set_saturation(pdev->b, pdev->c, pdev->s);
		gx3211_hdmi_set_contrast(pdev->b, pdev->c, pdev->s);
	}

	if (pdev->resolution_waiting != GXAV_VOUT_NULL_MAX) {
		hdmi_set_resolution(pdev->resolution_waiting);
		pdev->resolution_waiting = GXAV_VOUT_NULL_MAX;
	} else {
		if (pdev->resolution != GXAV_VOUT_NULL_MAX) {
			if (pdev->hdcp_en == 0)
				hdmi_set_resolution(pdev->resolution);
#if 0
			else
				phy_Gen2TxPowerOn(0x0, 1);
#endif
		}
	}
	if (pdev->i2c_bad)
		gxav_hdmi_wake_event(EVENT_HDMI_READ_EDID_ERR|EVENT_HDMI_START_AUTH);

#if (defined(NO_OS) || defined(TEE_OS))
	if (pdev->hdcp_en == 1 && pdev->resolution != GXAV_VOUT_NULL_MAX)
		gx3211_hdmi_hdcp_start(pdev->resolution);
#endif

	if (pdev->edid_info.data_len >= 128)
		pdev->edid_info.tv_cap = gx3211_hdmi_get_tv_cap(&pdev->edid_info.data[128]);
}

void gx3211_hdmi_pluged_in(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (gn_hdmi_open == 0)
		return;

	LOG_DEBUG(" --- PLUGED IN --- ");
	pdev->stat.cnt_conn++;
	pdev->event = EVENT_HDMI_HOTPLUG_IN;
	pdev->state = HDMI_HOT_PLUG;
	pdev->hdcp_state = HDCP_IDLE;
	pdev->edid_err_cnt = 0;
	pdev->edid_info.data_len = 0;
	phy_Gen2TxPowerOn(0x0, 0);
	gx3211_hdmi_acr_enable(1);

#if (defined(NO_OS) || defined(TEE_OS))
	#if (NOS_READ_EDID == 1)
	if (api_EdidProbe() == FALSE) {
		phy_Gen2TxPowerOn(0x0, 1);
		LOG_DEBUG(" --- I2C BAD--- ");
		pdev->i2c_bad = 1;
		pdev->event = EVENT_HDMI_READ_EDID_ERR|EVENT_HDMI_START_AUTH;
		pdev->state = HDMI_EDID_ERR;
		pdev->post_type = POST_READ_EDID_ERR;
		gx3211_hdmi_tigger(pdev);
		return;
	}

	if (!api_EdidRead()) {
		LOG_ERROR2("cannot read E-EDID", error_Get());
		pdev->state = HDMI_ERROR;
		return;
	}
	#endif
#else
	if (!api_EdidRead()) {
		LOG_ERROR2("cannot read E-EDID", error_Get());
		pdev->state = HDMI_ERROR;
		return;
	}
	pdev->post_type = POST_PLUG_IN;
	gx3211_hdmi_tigger(pdev);
#endif
}

int HdmiSet_Main_Patch(void)
{
	int edid_try_times = 2;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (gn_hdmi_open == 0)
		return 0;

	/* application needs EDID read to start functioning */
	if (pdev->state == HDMI_HOT_PLUG){
		if (pdev->edid_err_cnt < edid_try_times) {
			if (!api_EdidRead()){
				pdev->state = HDMI_ERROR;
				return -1;
			}
			pdev->edid_err_cnt++;
			gx_mdelay(2);
		} else {
			pdev->stat.edid_error = 1;
			pdev->edid_info.audio_codes = 0;
			pdev->event = EVENT_HDMI_READ_EDID_ERR|EVENT_HDMI_START_AUTH;
			pdev->edid_err_cnt = 0;
			pdev->state = HDMI_EDID_ERR;
			pdev->post_type = POST_READ_EDID_ERR;
			gx3211_hdmi_tigger(pdev);
		}
	}

	return 0;
}

int HdmiSet_Main_ReadingI2C(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (gn_hdmi_open == 0)
		return 0;

	pdev->post_type = POST_READ_EDID_ING;
	gx3211_hdmi_tigger(pdev);
	return 0;
}

int HdmiGetAuthState(hdcp_status_t status)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	pdev->hdcp_state = status;
	switch (status) {
	case HDCP_ENGAGED:
		access_Write(access_Read(0x5008) | 0xc0, 0x5008);
		//access_Write(access_Read(0x1018) | 0x01, 0x1018);
		pdev->hdcp_err_cnt = 0;
		api_HdcpDisableEncryption(0);
		pdev->event = EVENT_HDMI_HDCP_SUCCESS;
		pdev->post_type = POST_HDCP_SUCCESS;
		gx3211_hdmi_tigger(pdev);
		LOG_DEBUG(" --- HDCP SUCCESS --- ");
		gxav_secure_set_hdcp_state(AV_HDCP_SUCCESS);
		break;
	case HDCP_FAILED:
		if (pdev->hdcp_err_cnt > 5) {
			access_Write(access_Read(0x5008) | 0x40, 0x5008);
			if (pdev->event != EVENT_HDMI_HOTPLUG_OUT) {
				api_AvMute(FALSE);
				api_HdcpDisableEncryption(0);
				access_Write(access_Read(0x1018) | 0x01, 0x1018);
				pdev->event = EVENT_HDMI_HDCP_FAIL;
				pdev->post_type = POST_HDCP_FAILED;
				gx3211_hdmi_tigger(pdev);
				LOG_DEBUG(" --- HDCP FAILED --- ");
				gxav_secure_set_hdcp_state(AV_HDCP_FAILED);
			}
			pdev->hdcp_err_cnt = 0;
		} else {
			pdev->hdcp_err_cnt++;
			gx_mdelay(2);
		}
		break;
	default:
		break;
	}

	return 0;
}

static int gx3211_hdmi_hdcp_disable(void)
{
	gxlog_d(LOG_HDMI, "%s: %d [%d]\n", __func__, __LINE__, gn_hdmi_open);
	if (gn_hdmi_open) {
		struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

		if (!pdev->hdcp_en || !pdev->fetched_key)
			return 0;

		gx3211_hdmi_interrupt_mask();
		api_HdcpDisableEncryption(1);
		access_Write(access_Read(0x5008) | 0xc0, 0x5008);
		gx_mdelay(20);
		pdev->hdcp_en = 0;
		pdev->hdcp_state = HDCP_IDLE;
		gx3211_hdmi_interrupt_unmask();
	}

	return 0;
}

static int gx3211_hdmi_hdcp_enable(void)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
#if 0
	gx3211_hdmi_open(0);
	hdmi_fetch_key();

	if (pdev->hdcp_state == HDCP_FAILED || pdev->hdcp_state == HDCP_ENGAGED)
		return 0;

	if ((pdev->state >= HDMI_EDID_READ || pdev->state == HDMI_EDID_ERR) && pdev->fetched_key) {
		LOG_DEBUG(" --- HDCP START --- ");
		access_Write(access_Read(0x5008) & 0x3f, 0x5008);
		access_Write(access_Read(0x1018) | 0x02, 0x1018);
		pdev->hdcp_encrypt = 1;
		hdmi_set_resolution(resolution);
		pdev->last_hdcp_state = 88;
		pdev->hdcp_start_time = gx_current_tick();
		return 0;
	} else {
		LOG_DEBUG(" --- HDCP START FAILED (No key OR Did not read edid)--- ");
		pdev->event = EVENT_HDMI_HDCP_FAIL;
		pdev->hdcp_encrypt = 0;
		hdmi_set_resolution(resolution);
		gxav_hdmi_wake_event(pdev->event);
		return pdev->event;
	}
#endif
	pdev->hdcp_en = 1;
	return 0;
}

static int gx3211_hdmi_hdcp_config(GxVideoOutProperty_HdmiHdcpConfig *config)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	pdev->data_encrypt        = config->data_encrypt;
	pdev->mute_av_when_failed = config->mute_av_when_failed;

	return 0;
}

#define HDCP_TIMEOUT_GATE_MS (50000)
static int gx3211_hdmi_hdcp_start(GxVideoOutProperty_Mode resolution)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d\n", __func__, __LINE__);
	gx3211_hdmi_open(0);
	hdmi_fetch_key();

	if (pdev->hdcp_state == HDCP_FAILED || pdev->hdcp_state == HDCP_ENGAGED)
		return 0;

	if ((pdev->state >= HDMI_EDID_READ || pdev->state == HDMI_EDID_ERR) && pdev->fetched_key) {
		LOG_DEBUG(" --- HDCP START --- ");
		gx3211_hdmi_black_enable(0);
		access_Write(access_Read(0x5008) & 0x3f, 0x5008);
		access_Write(access_Read(0x1018) | 0x02, 0x1018);
		pdev->hdcp_encrypt = 1;
		hdmi_set_resolution(resolution);
		pdev->last_hdcp_state = 88;
		pdev->hdcp_start_time = gx_tick_start(HDCP_TIMEOUT_GATE_MS);
		return 0;
	} else {
		LOG_DEBUG(" --- HDCP START FAILED (No key OR Did not read edid)--- ");
		pdev->event = EVENT_HDMI_HDCP_FAIL;
		gx3211_hdmi_black_enable(1);
		phy_Gen2TxPowerOn(0x0, 1);
		//pdev->hdcp_encrypt = 0;
		//hdmi_set_resolution(resolution);
		gxav_hdmi_wake_event(pdev->event);
		return pdev->event;
	}
}

static int gx3211_cec_enable(int enable)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	pdev->cec_enable = enable;
	return cec_enable(enable);
}

static int gx3211_cec_send_cmd(GxVideoOutProperty_HdmiCecCmd *cmd)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (pdev->cec_enable)
		return cec_send_cmd(cmd->code);

	return -1;
}

static int gx3211_cec_recv_cmd(GxVideoOutProperty_HdmiCecCmd *cmd)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (pdev->cec_enable)
		return cec_recv_cmd(&cmd->code, -1);

	return -1;
}

int gx3211_cec_event_handler(unsigned state)
{
	CecRet ret = cec_parse_state(state);
	if (ret == RET_CEC_EOM)
		gxav_hdmi_wake_event(EVENT_HDMI_CEC_EOM);

	return 0;
}

static unsigned char gx3211_hdmi_read(unsigned char addr)
{
	return 0;
}

static void gx3211_hdmi_write(unsigned char addr, unsigned char data)
{
}

static void gx3211_hdmi_event_process(struct gxav_hdmidev *pdev)
{
	switch (pdev->post_type) {
#ifdef SET_RESOLUTION_WHEN_PLUGOUT
	case POST_PLUG_OUT:
		hdmi_plug_out(pdev);
		break;
#endif
	case POST_PLUG_IN:
		gx_mdelay(200);
		if (pdev->post_type == POST_PLUG_IN) {
		    if (api_EdidProbe() == FALSE) {
		        phy_Gen2TxPowerOn(0x0, 1);
		        LOG_DEBUG(" --- I2C BAD--- ");
				pdev->i2c_bad = 1;
		        pdev->event = EVENT_HDMI_READ_EDID_ERR|EVENT_HDMI_START_AUTH;
		        pdev->state = HDMI_EDID_ERR;
		        pdev->post_type = POST_READ_EDID_ERR;
		        gx3211_hdmi_tigger(pdev);
		    } else {
				pdev->i2c_bad = 0;
		        LOG_DEBUG(" --- NO EDID but I2C is OK --- ");
		    }
		}
		break;
	case POST_READ_EDID_DONE:
	case POST_READ_EDID_ERR:
		hdmi_edid_over(pdev);
		break;
	case POST_READ_EDID_ING:
		api_EdidReadRequstPath();
		gx_mdelay(4);
		break;
	case POST_HDCP_SUCCESS:
		gx_mdelay(20);
		api_AvMute(FALSE);
		break;
	default:
		break;
	}
}


static void gx3211_hdmi_thread(void* p)
{
	u8 hdcp_state;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	while (gn_hdmi_thread_running) {
		int ret;

		if (gx_thread_should_stop())
			break;

		if(pdev->thread_id == 0)
			break;

		if (pdev->hdcp_en) {
			if(gx_tick_end(pdev->hdcp_start_time)) {
				/* 0x20 is ok */
				hdcp_state = api_HdcpAuthenticationState();
				//if (pdev->last_hdcp_state != hdcp_state && hdcp_state != 0x20) {
				if (pdev->last_hdcp_state != hdcp_state)
					gx_printf("hdcp_state -> %x\n", hdcp_state);

				if (pdev->last_hdcp_state != hdcp_state && hdcp_state == 0) {
					if (pdev->mute_av_when_failed) {
						API_AUDIOMUTE(TRUE);
						api_AvMute(TRUE);
					}
					gxav_hdmi_wake_event(EVENT_HDMI_HDCP_FAIL);
					gx_printf("hdcp_state error,%x\n", hdcp_state);
				}
				if (pdev->last_hdcp_state != 0x20 && hdcp_state == 0x20) {
					if (pdev->mute_av_when_failed) {
						API_AUDIOMUTE(FALSE);
						api_AvMute(FALSE);
					}
					gxav_hdmi_wake_event(EVENT_HDMI_HDCP_SUCCESS);
					gx_printf("hdcp_state ok\n");
				}
				pdev->last_hdcp_state = hdcp_state;
			};
		}

		ret = gx_sem_wait_timeout(&(pdev->sem_id), 2000);
		if (ret != 0)
			continue;
		gx3211_hdmi_event_process(pdev);
	}
}

static void gx3211_hdmi_tigger(struct gxav_hdmidev *pdev)
{
#if (defined(NO_OS) || defined(TEE_OS))
	gx3211_hdmi_event_process(pdev);
#else
	gx_sem_post(&(pdev->sem_id));
#endif
}

static int gx3211_hdmi_set_brightness(int b, int c, int s)
{
	unsigned int brightness_value_hdmi = 0;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	videoParams_t *video = HDMI_GET_VIDEO(pdev);
	encoding_t encoding_out = videoParams_GetEncodingOut(video);

	gxlog_d(LOG_HDMI, "%s: %d [encoding: %d]\n", __func__, __LINE__, encoding_out);
	if (encoding_out == RGB)
		return -1;

	pdev->b = b;
	pdev->c = c;
	pdev->s = s;

	brightness_value_hdmi = (32 *((c << 1) * (b << 1))) / (100 * 100) + 4;
	brightness_value_hdmi = brightness_value_hdmi >= 63 ? 63 : brightness_value_hdmi;

	access_Write(brightness_value_hdmi, 0x4102);
	access_Write(0xff, 0x4108);
	access_Write(0xda, 0x4109);
	//access_Write(0xea, 0x4109);

	return 0;
}

static int gx3211_hdmi_set_saturation(int b, int c, int s)
{
	unsigned int hdmi_value = 0;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	videoParams_t *video = HDMI_GET_VIDEO(pdev);
	encoding_t encoding_out = videoParams_GetEncodingOut(video);

	gxlog_d(LOG_HDMI, "%s: %d [encoding: %d]\n", __func__, __LINE__, encoding_out);
	if (encoding_out == RGB)
		return -1;

	pdev->b = b;
	pdev->c = c;
	pdev->s = s;

	hdmi_value = (32*((c << 1)*(s << 1))) / (100 * 100);
	hdmi_value = hdmi_value  >= 63 ? 63 : hdmi_value;

	access_Write(hdmi_value, 0x410c);
	access_Write(hdmi_value, 0x4116);

	if (hdmi_value > 32) {
		hdmi_value =  (hdmi_value  - 32) * 8;
		hdmi_value = 0x8000 - hdmi_value;
	} else {
		hdmi_value =  (32 - hdmi_value) * 8;
	}

	access_Write(hdmi_value & 0xff, 0x4111);
	access_Write(hdmi_value & 0xff, 0x4119);
	hdmi_value =  hdmi_value >> 8;
	access_Write(hdmi_value & 0xff, 0x4110);
	access_Write(hdmi_value & 0xff, 0x4118);

	return 0;
}

static int gx3211_hdmi_set_contrast(int b, int c, int s)
{
	unsigned int hdmi_value = 0;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	videoParams_t *video = HDMI_GET_VIDEO(pdev);
	encoding_t encoding_out = videoParams_GetEncodingOut(video);

	gxlog_d(LOG_HDMI, "%s: %d [encoding: %d]\n", __func__, __LINE__, encoding_out);
	if (encoding_out == RGB)
		return -1;

	pdev->b = b;
	pdev->c = c;
	pdev->s = s;

	hdmi_value = (32 *((c << 1) * (b << 1))) / (100 * 100) + 4 ;
	hdmi_value = hdmi_value  >= 63 ? 63 : hdmi_value;
	access_Write(hdmi_value, 0x4102);

	hdmi_value = (32*((c << 1) * (s << 1))) / (100 * 100);
	hdmi_value = hdmi_value  >= 63 ? 63 : hdmi_value;
	access_Write(hdmi_value, 0x410c);
	access_Write(hdmi_value, 0x4116);

	if (hdmi_value > 32) {
		hdmi_value =  (hdmi_value  - 32) * 8;
		hdmi_value = 0x8000 - hdmi_value;
	} else {
		hdmi_value =  (32 - hdmi_value) * 8;
	}

	access_Write(hdmi_value & 0xff, 0x4111);
	access_Write(hdmi_value & 0xff, 0x4119);
	hdmi_value =  hdmi_value >> 8;
	access_Write(hdmi_value & 0xff, 0x4110);
	access_Write(hdmi_value & 0xff, 0x4118);

	return 0;
}

static int gx3211_hdmi_get_version(GxVideoOutProperty_HdmiVersion *version)
{
	u8 value;

	value = access_Read(0x0000);
	version->hdmi_major = (value>>4)&0xf;
	version->hdmi_minor = value&0xf;
	value = access_Read(0x0001);
	version->hdmi_revision = value;
	value = access_Read(0x5014);
	version->hdcp_major = (value>>4)&0xf;
	version->hdcp_minor = value&0xf;
	value = access_Read(0x5015);
	version->hdcp_revision = value;

	return 0;
}

static int gx3211_hdmi_set_encoding_out(GxAvHdmiEncodingOut encoding)
{
	encoding_t encnew;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	gxlog_d(LOG_HDMI, "%s: %d [encoding: %d]\n", __func__, __LINE__, encoding);
	switch (encoding) {
	case HDMI_YUV_422:
		encnew = YCC422;
		break;
	case HDMI_YUV_444:
		encnew = YCC444;
		break;
	default:
		encnew = RGB;
		break;
	}

	if (pdev->encoding_out != encnew) {
		pdev->encoding_out = encnew;
#if 0
		if (gx3211_vmode_save != GXAV_VOUT_NULL_MAX) {
			gn_hdmi_open = 0;
			gx3211_hdmi_black_enable(1);
			gx3211_hdmi_videoout_set(gx3211_vmode_save);
			gx3211_hdmi_black_enable(0);
		}
#endif
	}

	return 0;
}

static int gx3211_hdmi_setup(struct gxav_device *dev, struct gxav_module_resource *res)
{
	GX3211_HDMI_INT_SRC = res->irqs[0];
	GX3211_HDMI_BASE_ADDR = res->regs[0];

	gx3211_hdmi_module.irqs[0] = res->irqs[0];
	gx3211_hdmi_module.interrupts[res->irqs[0]] = gx_hdmi_interrupt;

	return 0;
}

static int gx3211_hdmi_set_cgmsa_copy_permission(int permission)
{
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();
	videoParams_t *video = HDMI_GET_VIDEO(pdev);
	audioParams_t *audio = HDMI_GET_AUDIO(pdev);

	audioParams_SetIecCgmsA(audio, permission);
	gx3211_hdmi_interrupt_mask();
	if (audio_Configure(0, audio, videoParams_GetPixelClock(video), videoParams_GetRatioClock(video)) != TRUE) {
		gx3211_hdmi_interrupt_unmask();
		LOG_ERROR2("audio configure", error_Get());
		return -1;
	}
	if (pdev->audio_mute_require)
		API_AUDIOMUTE(1);
	gx3211_hdmi_interrupt_unmask();

	return 0;
}

static int gx3211_hdmi_support_hdr10(void)
{
	int ret = 0;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (pdev->edid_info.data_len != 0)
		ret = api_EdidSupportsHDR10();

	return ret;
}

static int gx3211_hdmi_support_hlg(void)
{
	int ret = 0;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	if (pdev->edid_info.data_len != 0)
		ret = api_EdidSupportsHLG();

	return ret;
}

static int gx3211_hdmi_set_hdr10(struct frame *frame)
{
	HDRCurve_e curve = HDMI_HDR10;
	GxAVHDMIMasteringDisplayMetadata mdmd_now;
	GxAVHDMIContentLightMetadata clmd_now;

	gx_memcpy(&mdmd_now, &frame->mdmd, sizeof(GxAVHDMIMasteringDisplayMetadata));
	gx_memcpy(&clmd_now, &frame->clmd, sizeof(GxAVHDMIContentLightMetadata));

	halFrameComposerDrm_Configure(0 + FC_BASE_ADDR, curve, mdmd_now, clmd_now);

	return 0;
}

static int gx3211_hdmi_set_hlg(struct frame *frame)
{
	HDRCurve_e curve = HDMI_HLG;
	GxAVHDMIMasteringDisplayMetadata mdmd_now;
	GxAVHDMIContentLightMetadata clmd_now;

	halFrameComposerDrm_Configure(0 + FC_BASE_ADDR, curve, mdmd_now, clmd_now);

	return 0;
}

static int gx3211_hdmi_set_color_resolution(GxAvHdmiColorDepth color_depth)
{
	color_depth_t color_depth_new;
	struct gxav_hdmidev *pdev = HDMI_GET_PDEV();

	switch (color_depth) {
		case HDMI_COLOR_DEPTH_8BIT:
			color_depth_new = HDMI_24BPP;
			break;
		case HDMI_COLOR_DEPTH_10BIT:
			color_depth_new = HDMI_30BPP;
			break;
		case HDMI_COLOR_DEPTH_12BIT:
			color_depth_new = HDMI_36BPP;
			break;
		default:
			color_depth_new = HDMI_24BPP;
			break;
	}

	if (pdev->color_depth != color_depth_new) {
		pdev->color_depth = color_depth_new;
	}

	return 0;
}

static struct gxav_hdmi_module gx3211_hdmi_ops = {
	.init               = gx3211_hdmi_init ,
	.uninit             = gx3211_hdmi_uninit ,
	.open               = gx3211_hdmi_open ,
	.ready              = gx3211_hdmi_ready,
	.enable             = gx3211_hdmi_enable ,
	.detect_hotplug     = gx3211_hdmi_detect_hotplug ,
	.read_edid          = gx3211_hdmi_read_edid ,
	.audio_codes_get    = gx3211_hdmi_audio_codes_get ,
	.clock_set          = gx3211_hdmi_clock_set ,
	.audioout_set       = gx3211_hdmi_audioout_set ,
	.audioout_mute      = gx3211_hdmi_audioout_mute ,
	.audioout_reset     = gx3211_hdmi_audioout_reset ,
	.audiosample_change = gx3211_hdmi_audiosample_change ,
	.av_mute            = gx3211_hdmi_av_mute ,
	.powerdown          = gx3211_hdmi_powerdown,
	.acr_enable         = gx3211_hdmi_acr_enable ,
	.black_enable       = gx3211_hdmi_black_enable ,
	.videoout_set       = gx3211_hdmi_set_resolution,
	.videoout_get       = gx3211_hdmi_get_resolution,
	.hdcp_enable        = gx3211_hdmi_hdcp_enable,
	.hdcp_config        = gx3211_hdmi_hdcp_config,
	.hdcp_start         = gx3211_hdmi_hdcp_start,
	.hdcp_disable       = gx3211_hdmi_hdcp_disable,
	.read               = gx3211_hdmi_read ,
	.write              = gx3211_hdmi_write ,
	.cold_reset_set     = gx3211_hdmi_cold_reset_set ,
	.cold_reset_clr     = gx3211_hdmi_cold_reset_clr ,
	.set_brightness     = gx3211_hdmi_set_brightness,
	.set_saturation     = gx3211_hdmi_set_saturation,
	.set_contrast       = gx3211_hdmi_set_contrast,
	.set_colorimetry    = gx3211_hdmi_set_colorimetry,
	.get_version        = gx3211_hdmi_get_version,
	.set_encoding_out   = gx3211_hdmi_set_encoding_out,
	.set_cgmsa_copy_permission = gx3211_hdmi_set_cgmsa_copy_permission,
	.cec_enable         = gx3211_cec_enable,
	.cec_send_cmd       = gx3211_cec_send_cmd,
	.cec_recv_cmd       = gx3211_cec_recv_cmd,
	.interrupt          = gx3211_hdmi_interrupt ,

	.support_hdr10      = gx3211_hdmi_support_hdr10,
	.support_hlg        = gx3211_hdmi_support_hlg,
	.set_hdr10          = gx3211_hdmi_set_hdr10,
	.set_hlg            = gx3211_hdmi_set_hlg,
	.set_color_resolution = gx3211_hdmi_set_color_resolution,
};

struct gxav_module_ops gx3211_hdmi_module = {
	.module_type = GXAV_MOD_HDMI,
	.count = 1,
	.irqs = {54, -1},
	.irq_names = {"hdmi"},
	.event_mask = 0xffffffff,
	.setup = gx3211_hdmi_setup,
	.init = gx_hdmi_init,
	.cleanup = gx_hdmi_uninit,
	.open = gx_hdmi_open,
	.close = gx_hdmi_close,
	.set_property = gx_hdmi_set_property,
	.get_property = gx_hdmi_get_property,
	.interrupts[54] = gx_hdmi_interrupt,
	.priv = &gx3211_hdmi_ops,
};

