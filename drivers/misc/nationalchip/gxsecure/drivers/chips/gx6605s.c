#include "gx6605s.h"

extern GxSeModule gx6605s_firewall_module;
extern GxSeModule gx6605s_m2m_module;
extern GxSeModule gx6605s_sci_module;
extern GxSeModule pegasus_sci_module;
extern GxSeModule gx6605s_klm_generic_module;
extern GxSeModule gx6605s_otp_module;
extern GxSeModule gx6605s_chip_module;

int gxse_device_register_gx6605s_misc_otp(void)
{
	int ret = 0;

#ifdef CFG_GXSE_MISC_OTP
	ret |= gxse_module_register(&gx6605s_otp_module);
#endif
	return ret;
}

int gxse_device_register_gx6605s_crypto_dma(void)
{
	int ret = 0;
#ifdef CFG_GXSE_CRYPTO_DMA
	ret = gxse_module_register(&gx6605s_m2m_module);
#endif
	return ret;
}

int gxse_device_register_gx6605s_klm_generic(void)
{
	int ret = 0;
#ifdef CFG_GXSE_KLM_GENERIC
	ret = gxse_module_register(&gx6605s_klm_generic_module);
#endif
	return ret;
}

int gxse_device_register_gx6605s_misc_firewall(void)
{
	int ret = 0;
	if (gxse_device_register_gx6605s_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_FIREWALL
	ret |= gxse_module_register(&gx6605s_firewall_module);
#endif
	return ret;
}

int gxse_device_register_gx6605s_misc_sci(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SCI
	if (gx_osdep_chip_sub_probe() == 1)
		ret = gxse_module_register(&pegasus_sci_module);
	else
		ret = gxse_module_register(&gx6605s_sci_module);
#endif
	return ret;
}

int gxse_device_register_gx6605s_misc_chip(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_CHIP_CFG
	gxse_module_register(&gx6605s_chip_module);
#endif
	return ret;
}

int gxse_device_register_gx6605s_klm(void)
{
	if (gxse_device_register_gx6605s_klm_generic() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_gx6605s_crypto(void)
{
	if (gxse_device_register_gx6605s_crypto_dma() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_gx6605s_misc(void)
{
	if (gxse_device_register_gx6605s_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx6605s_misc_sci() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx6605s_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx6605s_misc_chip() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_chip_register_gx6605s(void)
{
	if (gxse_device_register_gx6605s_crypto() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx6605s_klm() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gx6605s_misc() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}
