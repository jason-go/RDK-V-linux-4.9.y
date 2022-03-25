#include "gxse_core.h"
#include "gxmisc_virdev.h"

static int32_t gx_misc_ioctl(GxSeModule *module, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModuleHwObj *obj = NULL;
	GxSeModuleDevOps *ops = NULL;

	if (gxse_module_ioctl_check(module, GXSE_HWOBJ_TYPE_MISC, cmd, size) < 0)
		return GXSE_ERR_GENERIC;

	obj = module->hwobj;
    ops = (GxSeModuleDevOps *)GXSE_OBJ_DEVOPS(obj);
	gx_mutex_lock(obj->mutex);
	if (ops->ioctl)
		ret = ops->ioctl(obj, cmd, param, size);
	gx_mutex_unlock(obj->mutex);

	return ret;
}

GxSeModuleOps misc_dev_ops = {
	.ioctl  = gx_misc_ioctl,
	.init   = gxse_module_init,
#ifdef CPU_ACPU
	.deinit = gxse_module_deinit,
	.open   = gxse_module_open,
	.close  = gxse_module_close,
	.read   = gxse_module_read,
	.write  = gxse_module_write,
	.isr    = gxse_module_isr,
	.dsr    = gxse_module_dsr,
	.poll   = gxse_module_poll,
#endif
};
