#include "gxse_core.h"
#include "../gxmisc_virdev.h"

int32_t gx_misc_timer_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;

	switch (cmd) {
	case MISC_UDELAY:
		if (ops->misc_timer.udelay)
			ret = ops->misc_timer.udelay(obj, *(uint32_t *)param);
		break;

	default:
		break;
	}

	return ret;
}

