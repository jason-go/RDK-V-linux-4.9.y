#include "gxse_core.h"
#include "../gxmisc_virdev.h"

int32_t gx_misc_rng_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;
	(void) size;

	switch (cmd) {
	case SECURE_GET_RNG:
		if (ops->misc_rng.rng_request)
			ret = ops->misc_rng.rng_request(obj, param);
		break;

	default:
		break;
	}

	return ret;
}
