#include "gx3113c.h"
#include "taurus.h"

extern GxSeModule gx6605s_firewall_module;
extern GxSeModule gx3211_sci_module;
extern GxSeModule taurus_otp_module;
extern GxSeModule taurus_rng_module;
extern GxSeModule taurus_chip_module;
extern GxSeModule gx3113c_klm_generic_module;

int gxse_device_register_gx3113c_misc_otp(void)
{
	int ret = 0;

#ifdef CFG_GXSE_MISC_OTP
	ret |= gxse_module_register(&taurus_otp_module);
#endif
	return ret;
}

int gxse_device_register_gx3113c_misc_firewall(void)
{
	int ret = 0;
	if (gxse_device_register_gx3113c_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_FIREWALL
	ret |= gxse_module_register(&gx6605s_firewall_module);
#endif
	return ret;
}

int gxse_device_register_gx3113c_misc_sci(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SCI
	ret = gxse_module_register(&gx3211_sci_module);
#endif
	return ret;
}

int gxse_device_register_gx3113c_misc_rng(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_RNG
	gxse_module_register(&taurus_rng_module);
#endif
	return ret;
}

int gxse_device_register_gx3113c_misc_chip(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_CHIP_CFG
	gxse_module_register(&taurus_chip_module);
#endif
	return ret;
}

int gxse_device_register_gx3113c_klm_generic(void)
{
	int ret = 0;
#ifdef CFG_GXSE_KLM_GENERIC
	ret = gxse_module_register(&gx3113c_klm_generic_module);
#endif
	return ret;
}

int gxse_device_register_gx3113c_misc(void)
{
	if (gxse_device_register_gx3113c_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx3113c_misc_sci() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx3113c_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx3113c_misc_rng() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx3113c_misc_chip() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_gx3113c_crypto(void)
{
	if (gxse_device_register_taurus_crypto_dma() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_gx3113c_klm(void)
{
	if (gxse_device_register_gx3113c_klm_generic() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_chip_register_gx3113c(void)
{
	if (gxse_device_register_gx3113c_misc() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx3113c_klm() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx3113c_crypto() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

