#include "gxse_core.h"
#include "../gxmisc_virdev.h"

int32_t gx_misc_chip_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;
	(void) size;

	switch (cmd) {
#ifdef CPU_ACPU
	case SECURE_GET_CSSN:
		{
			GxSecureCSSN *CSSN = (GxSecureCSSN *)param;
			if (ops->misc_chip.get_CSSN)
				ret = ops->misc_chip.get_CSSN(obj, CSSN->value, sizeof(CSSN->value));
		}
		break;

	case SECURE_GET_CHIPNAME:
		{
			GxSecureChipName *chipname = (GxSecureChipName *)param;

			if (ops->misc_chip.get_chipname)
				ret = ops->misc_chip.get_chipname(obj, chipname->value, sizeof(chipname->value));
		}
		break;
	case MISC_CHIP_SWITCH_MULTIPIN:
		{
			GxSeMultipinStatus *status = (GxSeMultipinStatus *)param;
			if (ops->misc_chip.switch_multipin)
				ret = ops->misc_chip.switch_multipin(obj, *status);
		}
		break;
#else
	case MISC_CHIP_SCPU_RESET:
		if (ops->misc_chip.chip_reset)
			ret = ops->misc_chip.chip_reset(obj);
		break;

	case MISC_CHIP_SEND_KEY:
		{
			GxSeScpuKey *p = (GxSeScpuKey *)param;

			if (p->type == MISC_KEY_SCPU_SOFT) {
				if (ops->misc_chip.send_m2m_soft_key)
					ret = ops->misc_chip.send_m2m_soft_key(obj, p->value, p->length);
			} else {
				if (ops->misc_chip.send_secure_key)
					ret = ops->misc_chip.send_secure_key(obj, p->type, p->value, p->length);
			}
		}
		break;

	case MISC_CHIP_GET_BIV:
		{
			GxSecureImageVersion *p = (GxSecureImageVersion *)param;

			if (ops->misc_chip.get_BIV)
				ret = ops->misc_chip.get_BIV(obj, (void *)p->version, sizeof(p->version));
		}
		break;
#endif

	default:
		break;
	}

	return ret;
}
