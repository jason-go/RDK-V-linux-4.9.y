#include "gxse_core.h"
#include "../gxmisc_virdev.h"
#include "sirius_firewall.h"

int32_t _fw_get_otp_flag(uint32_t addr, uint32_t high_4_bit)
{
	int32_t ret = gxse_fuse_read(addr);
	return (ret == GXSE_ERR_GENERIC) ? 0 : (ret & (0xf << (high_4_bit ? 4 : 0)));
}

int32_t gx_misc_fw_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModuleHwObjMiscOps *ops = (GxSeModuleHwObjMiscOps *)obj->ops;

	switch (cmd) {
	case FIREWALL_GET_PROTECT_BUFFER:
		{
			if (size != 4)
                break;

			if (ops->misc_fw.get_protect_buffer)
				ret = ops->misc_fw.get_protect_buffer(obj, param);

			if (*(int *)param == 0)
				ret = gx_mem_protect_type_get(param);
		}
		break;

	case FIREWALL_SET_PROTECT_BUFFER:
		{
			if (size != sizeof(GxSeFirewallBuffer))
                break;

			if (ops->misc_fw.set_protect_buffer)
				ret = ops->misc_fw.set_protect_buffer(obj, param);
		}
		break;

	case FIREWALL_QUERY_ACCESS_ALIGN:
		{
			if (size != 4)
                break;

			if (ops->misc_fw.query_access_align)
				ret = ops->misc_fw.query_access_align(obj, param);
		}
		break;

	case FIREWALL_QUERY_PROTECT_ALIGN:
		{
			if (size != 4)
                break;

			if (ops->misc_fw.query_protect_align)
				ret = ops->misc_fw.query_protect_align(obj, param);
		}
		break;

	default:
		break;
	}

	return ret;
}
