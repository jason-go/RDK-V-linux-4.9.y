#include "gxse_core.h"
#include "../gxmisc_virdev.h"

int32_t gx_misc_sci_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;
	(void) size;

	switch (cmd) {
	case SCI_PARAM_SET:
		if (ops->misc_sci.set_param)
			ret = ops->misc_sci.set_param(obj, param);
		break;

	case SCI_PARAM_GET:
		if (ops->misc_sci.get_param)
			ret = ops->misc_sci.get_param(obj, param);
		break;

	case SCI_STATUS_GET:
		if (ops->misc_sci.get_status)
			ret = ops->misc_sci.get_status(obj, param);
		break;

	case SCI_RESET:
		if (ops->misc_sci.ICC_reset)
			ret = ops->misc_sci.ICC_reset(obj);
		break;

	case SCI_DEACT:
		if (ops->misc_sci.ICC_poweroff)
			ret = ops->misc_sci.ICC_poweroff(obj);
		break;

	case SCI_PRINT_REG:
		if (ops->misc_sci.print_reg)
			ret = ops->misc_sci.print_reg(obj);
		break;

	default:
		break;
	}

	return ret;
}
