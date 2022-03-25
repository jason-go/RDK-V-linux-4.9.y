#include "../gxmisc_virdev.h"
#include "taurus_firewall.h"

static struct mutex_static_priv gx6605s_firewall_priv;
static int32_t gx6605s_fw_get_protect_buffer(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;
	*param = 0;
	return GXSE_SUCCESS;
}

static int32_t gx6605s_fw_query_access_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;
	*param = 1;
	return GXSE_SUCCESS;
}

static int32_t gx6605s_fw_query_protect_align(GxSeModuleHwObj *obj, uint32_t *param)
{
	(void) obj;
	*param = 64;
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps gx6605s_firewall_devops = {
	.init  = gxse_hwobj_mutex_static_init,
	.deinit= gxse_hwobj_mutex_static_deinit,
    .ioctl = gx_misc_fw_ioctl,
};

static GxSeModuleHwObjMiscOps gx6605s_firewall_ops = {
	.devops = &gx6605s_firewall_devops,
	.misc_fw = {
		.get_protect_buffer  = gx6605s_fw_get_protect_buffer,
		.query_access_align  = gx6605s_fw_query_access_align,
		.query_protect_align = gx6605s_fw_query_protect_align,
	},
};

static GxSeModuleHwObj gx6605s_firewall_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &gx6605s_firewall_ops,
	.priv = &gx6605s_firewall_priv,
};

GxSeModule gx6605s_firewall_module = {
	.id   = GXSE_MOD_MISC_FIREWALL,
	.ops  = &misc_dev_ops,
	.hwobj= &gx6605s_firewall_hwobj,
	.res  = {.irqs = {-1},},
};
