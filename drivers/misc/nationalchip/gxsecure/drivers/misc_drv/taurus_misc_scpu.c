#include "gxmisc_virdev.h"
#include "taurus_otp.h"
#include "sirius_rng.h"
#include "sirius_chip.h"
#include "sirius_timer.h"
#include "sirius_chip_reg.h"

extern GxSeModuleDevOps sirius_misc_scpu_devops;

static GxSeModuleHwObjMiscOps taurus_misc_scpu_ops = {
	.devops = &sirius_misc_scpu_devops,
	.misc_otp = {
		.otp_read  = taurus_misc_otp_read,
		.otp_write = taurus_misc_otp_write,
	},

#ifdef CFG_GXSE_MISC_CHIP_CFG
	.misc_chip = {
		.send_m2m_soft_key = sirius_misc_send_m2m_soft_key,
		.send_secure_key   = sirius_misc_send_secure_key,
	},
#endif

#ifdef CFG_GXSE_MISC_RNG
	.misc_rng = {
		.rng_request = sirius_misc_rng_request,
	},
#endif

#ifdef CFG_GXSE_MISC_TIMER
	.misc_timer = {
		.udelay  = sirius_timer_udelay,
	},
#endif
};

static GxSeModuleHwObj taurus_misc_scpu_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &taurus_misc_scpu_ops,
};

GxSeModule taurus_misc_scpu_module = {
	.id   = GXSE_MOD_MISC_ALL,
	.ops  = &misc_dev_ops,
	.hwobj= &taurus_misc_scpu_hwobj,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_MISC,
		.reg_len   = SIRIUS_MISC_REG_LEN,
	},
};
