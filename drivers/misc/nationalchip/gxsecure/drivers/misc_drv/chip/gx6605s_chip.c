#include "../gxmisc_virdev.h"
#include "sirius_chip.h"
#include "taurus_chip.h"
#include "gx6605s_chip_reg.h"

static struct mutex_static_priv gx6605s_chip_priv;
static int32_t gx6605s_switch_multipin(GxSeModuleHwObj *obj, GxSeMultipinStatus status)
{
	volatile Gx6605sChipReg *reg = (volatile Gx6605sChipReg *)(obj->reg);
	static char port_20 = 0, port_11 = 0;

	switch(status) {
	case GXSE_MULTIPIN_SWITCH:
		{
			port_20 = reg->pinmax_port_a & (1 << 20);
			port_11 = reg->pinmax_port_b & (1 << 11);
			/* switch to otp */
			reg->pinmax_port_a &= ~(1 << 20);
			reg->pinmax_port_b |= (1 << 11);
		}
		break;
	case GXSE_MULTIPIN_RECOVER:
		{
			if (port_20)
				reg->pinmax_port_a |= (1 << 20);
			else
				reg->pinmax_port_a &= ~(1 << 20);

			if (port_11)
				reg->pinmax_port_b |= (1 << 11);
			else
				reg->pinmax_port_b &= ~(1 << 11);
		}
		break;
	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps gx6605s_chip_devops = {
	.init  = gxse_hwobj_mutex_static_init,
	.deinit= gxse_hwobj_mutex_static_deinit,
	.ioctl = gx_misc_chip_ioctl,
};

static GxSeModuleHwObjMiscOps gx6605s_chip_ops = {
	.devops = &gx6605s_chip_devops,
	.misc_chip = {
		.get_CSSN         = taurus_misc_get_CSSN,
		.get_chipname     = taurus_misc_get_chipname,
		.switch_multipin  = gx6605s_switch_multipin,
	},
};

static GxSeModuleHwObj gx6605s_chip_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &gx6605s_chip_ops,
	.priv = &gx6605s_chip_priv,
};

GxSeModule gx6605s_chip_module = {
	.id   = GXSE_MOD_MISC_CHIP_CFG,
	.ops  = &misc_dev_ops,
	.hwobj= &gx6605s_chip_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_CHIP_CFG,
		.reg_len   = SIRIUS_MISC_REG_LEN,
		.irqs      = {-1},
	},
};
