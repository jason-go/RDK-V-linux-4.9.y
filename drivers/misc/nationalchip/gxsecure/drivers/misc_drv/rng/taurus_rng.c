#include "../gxmisc_virdev.h"
#include "sirius_rng_reg.h"
#include "sirius_rng.h"

GxSeModule taurus_rng_module = {
	.id   = GXSE_MOD_MISC_RNG,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_rng_hwobj,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_CHIP_CFG,
		.reg_len   = SIRIUS_MISC_REG_LEN,
		.irqs      = {-1},
	},
};
