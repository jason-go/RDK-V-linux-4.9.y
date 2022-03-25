#include "gxmisc_virdev.h"
#include "sirius_otp.h"
#include "sirius_rng.h"
#include "sirius_chip.h"
#include "sirius_chip_reg.h"
#include "sirius_timer.h"
#include "sirius_sensor.h"

int32_t gx_misc_sub_init(GxSeModuleHwObj *obj)
{
	obj->reg = (void *)GXSCPU_BASE_ADDR_MISC;
#ifdef CFG_GXSE_MISC_CHIP_CFG
	sirius_misc_chip_init(obj);
#endif

#ifdef CFG_GXSE_MISC_TIMER
	obj->reg = (void *)GXSCPU_BASE_ADDR_TIMER;
	sirius_timer_init(obj);
#endif

#ifdef CFG_GXSE_MISC_SENSOR
	obj->reg = (void *)GXSCPU_BASE_ADDR_SENSOR;
	sirius_sensor_init(obj);
#endif

	return GXSE_SUCCESS;
}

static int32_t gx_misc_sub_ioctl(GxSeModuleHwObj *obj, uint32_t cmd, void *param, uint32_t size)
{
	obj->reg = (void *)GXSCPU_BASE_ADDR_MISC;
#ifdef CFG_GXSE_MISC_OTP
	if (gx_misc_otp_ioctl(obj, cmd, param, size) == GXSE_SUCCESS)
		return GXSE_SUCCESS;
#endif
#ifdef CFG_GXSE_MISC_RNG
	if (gx_misc_rng_ioctl(obj, cmd, param, size) == GXSE_SUCCESS)
		return GXSE_SUCCESS;
#endif
#ifdef CFG_GXSE_MISC_CHIP_CFG
	if (gx_misc_chip_ioctl(obj, cmd, param, size) == GXSE_SUCCESS)
		return GXSE_SUCCESS;
#endif
#ifdef CFG_GXSE_MISC_TIMER
	if (gx_misc_timer_ioctl(obj, cmd, param, size) == GXSE_SUCCESS)
		return GXSE_SUCCESS;
#endif
#ifdef CFG_GXSE_MISC_SENSOR
	if (gx_misc_sensor_ioctl(obj, cmd, param, size) == GXSE_SUCCESS)
		return GXSE_SUCCESS;
#endif

	return GXSE_ERR_GENERIC;
}

GxSeModuleDevOps sirius_misc_scpu_devops = {
	.init  = gx_misc_sub_init,
	.ioctl = gx_misc_sub_ioctl,
};

static GxSeModuleHwObjMiscOps sirius_misc_scpu_ops = {
	.devops = &sirius_misc_scpu_devops,
	.misc_otp = {
		.otp_read  = sirius_misc_otp_read,
		.otp_write = sirius_misc_otp_write,
	},

#ifdef CFG_GXSE_MISC_CHIP_CFG
	.misc_chip = {
		.chip_reset        = sirius_misc_scpu_reset,
		.get_BIV           = sirius_misc_get_BIV,
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

#ifdef CFG_GXSE_MISC_SENSOR
	.misc_sensor = {
		.get_err_status = sirius_sensor_get_err_status,
		.get_err_value  = sirius_sensor_get_err_value,
	},
#endif
};

static GxSeModuleHwObj sirius_misc_scpu_hwobj = {
	.type = GXSE_HWOBJ_TYPE_MISC,
	.ops  = &sirius_misc_scpu_ops,
};

GxSeModule sirius_misc_scpu_module = {
	.id   = GXSE_MOD_MISC_ALL,
	.ops  = &misc_dev_ops,
	.hwobj= &sirius_misc_scpu_hwobj,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_MISC,
		.reg_len   = SIRIUS_MISC_REG_LEN,
	},
};
