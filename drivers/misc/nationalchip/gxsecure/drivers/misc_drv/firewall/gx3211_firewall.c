#include "../gxmisc_virdev.h"
#include "taurus_firewall.h"

#define FW_SECURE_VALUE 0x6
#define FW_OTP_DDR_CONTENT_LIMIT 0x300
#define FW_OTP_DDR_SCRAMBLE      0x310

static struct mutex_static_priv gx3211_firewall_priv;
static int32_t gx3211_fw_get_protect_buffer(GxSeModuleHwObj *obj, uint32_t *param)
{
	uint32_t value = gxse_fuse_read(FW_OTP_DDR_CONTENT_LIMIT);
	(void) obj;

	if ((value & FW_SECURE_VALUE) == FW_SECURE_VALUE)
		*param= (GXFW_BUFFER_DEMUX_ES |
				GXFW_BUFFER_DEMUX_TSW |
				GXFW_BUFFER_DEMUX_TSR |
				GXFW_BUFFER_AUDIO_FIRMWARE |
				GXFW_BUFFER_VIDEO_FIRMWARE);
	else
		*param = 0;
	return GXSE_SUCCESS;
}

static int32_t gx3211_fw_query_access_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	uint32_t value = gxse_fuse_read(FW_OTP_DDR_SCRAMBLE);
	(void) obj;

	if (((value>>3) & FW_SECURE_VALUE) == FW_SECURE_VALUE)
		*param = 4;
	else
		*param = 1;

	return GXSE_SUCCESS;
}

static int32_t gx3211_fw_query_protect_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;
	*param = 64;
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps gx3211_firewall_devops = {
	.init  = gxse_hwobj_mutex_static_init,
	.deinit= gxse_hwobj_mutex_static_deinit,
	.ioctl = gx_misc_fw_ioctl,
};

static GxSeModuleHwObjMiscOps gx3211_firewall_ops = {
	.devops = &gx3211_firewall_devops,
	.misc_fw = {
		.get_protect_buffer  = gx3211_fw_get_protect_buffer,
		.query_access_align  = gx3211_fw_query_access_align,
		.query_protect_align = gx3211_fw_query_protect_align,
	},
};

static GxSeModuleHwObj gx3211_firewall_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &gx3211_firewall_ops,
	.priv = &gx3211_firewall_priv,
};

GxSeModule gx3211_firewall_module = {
	.id   = GXSE_MOD_MISC_FIREWALL,
	.ops  = &misc_dev_ops,
	.hwobj= &gx3211_firewall_hwobj,
	.res  = {.irqs = {-1},},
};
