#include "../gxmisc_virdev.h"
#include "sirius_rng.h"
#include "sirius_rng_reg.h"

int32_t sirius_misc_rng_request(GxSeModuleHwObj *obj, uint32_t *val)
{
	volatile SiriusRNGReg  *reg = (volatile SiriusRNGReg *)(obj->reg);

	reg->rng_ctrl.bits.req = 1;
	while(!reg->rng_ctrl.bits.valid);
	reg->rng_ctrl.bits.req = 0;
	*val = reg->rng_data;

	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static struct mutex_static_priv sirius_rng_priv;
static GxSeModuleDevOps sirius_rng_devops = {
	.init = gxse_hwobj_mutex_static_init,
	.ioctl = gx_misc_rng_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_rng_ops = {
	.devops = &sirius_rng_devops,
	.misc_rng = {
		.rng_request  = sirius_misc_rng_request,
	},
};

GxSeModuleHwObj sirius_rng_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &sirius_rng_ops,
	.priv = &sirius_rng_priv,
};

GxSeModule sirius_rng_module = {
	.id   = GXSE_MOD_MISC_RNG,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_rng_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_CHIP_CFG,
		.reg_len   = SIRIUS_MISC_REG_LEN,
		.irqs      = {-1},
	},
};
#endif
