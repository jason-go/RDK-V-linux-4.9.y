#include "../gxmisc_virdev.h"
#include "sirius_firewall.h"
#include "taurus_firewall.h"

#define FW_OTP_DDR_CONTENT_LIMIT 0x13E
#define FW_OTP_DDR_SCRAMBLE      0x13E
#define FW_OTP_DEMUX_TSW_BUF     0x13F
#define FW_OTP_DEMUX_ES_BUF      0x140
#define FW_OTP_VIDEO_FW_BUF      0x149
#define FW_OTP_AUDIO_FW_BUF      0x149
#define FW_OTP_SCPU_BUF          0x149

static struct mutex_static_priv taurus_firewall_priv;
static int taurus_fw_get_protect_buffer(GxSeModuleHwObj *obj, uint32_t *param)
{
	unsigned int protect_buffer = 0;
	(void) obj;

	// main switch
	if (!_fw_get_otp_flag(FW_OTP_DDR_CONTENT_LIMIT, 1)) {
		*param = protect_buffer;
		return GXSE_SUCCESS;
	}

	// DEMUX TSW buff switch
	if (_fw_get_otp_flag(FW_OTP_DEMUX_TSW_BUF, 1))
		protect_buffer |= (GXFW_BUFFER_DEMUX_TSW | GXFW_BUFFER_DEMUX_TSR);

	// DEMUX ES buff switch
	if (_fw_get_otp_flag(FW_OTP_DEMUX_ES_BUF, 0))
		protect_buffer |= GXFW_BUFFER_DEMUX_ES;

	// VIDEO/AUDIO FIRMWARE buff switch
	if (_fw_get_otp_flag(FW_OTP_VIDEO_FW_BUF, 0))
		protect_buffer |= (GXFW_BUFFER_AUDIO_FIRMWARE | GXFW_BUFFER_VIDEO_FIRMWARE);

	// SCPU buff switch
	if (_fw_get_otp_flag(FW_OTP_SCPU_BUF, 1))
		protect_buffer |= GXFW_BUFFER_SCPU;

	*param = protect_buffer;
	return GXSE_SUCCESS;
}

static int taurus_fw_query_access_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;
	*param = _fw_get_otp_flag(FW_OTP_DDR_SCRAMBLE, 0) ? 4 : 1;

	return GXSE_SUCCESS;
}

static int taurus_fw_query_protect_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;
	*param = _fw_get_otp_flag(FW_OTP_DDR_CONTENT_LIMIT, 1) ? 1024 : 64;

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps taurus_firewall_devops = {
	.init  = gxse_hwobj_mutex_static_init,
	.deinit= gxse_hwobj_mutex_static_deinit,
    .ioctl = gx_misc_fw_ioctl,
};

static GxSeModuleHwObjMiscOps taurus_firewall_ops = {
	.devops = &taurus_firewall_devops,
	.misc_fw = {
		.get_protect_buffer  = taurus_fw_get_protect_buffer,
		.query_access_align  = taurus_fw_query_access_align,
		.query_protect_align = taurus_fw_query_protect_align,
	},
};

static GxSeModuleHwObj taurus_firewall_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &taurus_firewall_ops,
	.priv = &taurus_firewall_priv,
};

GxSeModule taurus_firewall_module = {
	.id   = GXSE_MOD_MISC_FIREWALL,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_firewall_hwobj,
	.res  = {.irqs = {-1},},
};
