#include "gxse_core.h"
#include "../gxmisc_virdev.h"

int32_t gx_misc_sensor_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;

	switch (cmd) {
	case MISC_SENSOR_GET_STATUS:
		if (ops->misc_sensor.get_err_status)
			ret = ops->misc_sensor.get_err_status(obj, param);
		break;

	case MISC_SENSOR_GET_VALUE:
		if (ops->misc_sensor.get_err_value)
			ret = ops->misc_sensor.get_err_value(obj, param);
		break;

	default:
		break;
	}

	return ret;
}
