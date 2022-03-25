#include "taurus.h"

extern GxSeModule taurus_firewall_module;
extern GxSeModule taurus_sci_module;
extern GxSeModule taurus_mbox_module;
extern GxSeModule taurus_otp_module;
extern GxSeModule taurus_rng_module;
extern GxSeModule taurus_chip_module;
extern GxSeModule taurus_m2m_module;
extern GxSeModule taurus_crypto_module;
extern GxSeModule sirius_secure_klm_module;

int gxse_device_register_taurus_misc_otp(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_OTP
	ret = gxse_module_register(&taurus_otp_module);
#endif

	return ret;
}

int gxse_device_register_taurus_misc_firewall(void)
{
	int ret = 0;
	if (gxse_device_register_taurus_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_FIREWALL
	ret |= gxse_module_register(&taurus_firewall_module);
#endif
	return ret;
}

int gxse_device_register_taurus_misc_sci(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SCI
	ret = gxse_module_register(&taurus_sci_module);
#endif
	return ret;
}

int gxse_device_register_taurus_misc_chip(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_CHIP_CFG
	gxse_module_register(&taurus_chip_module);
#endif
	return ret;
}

int gxse_device_register_taurus_misc_rng(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_RNG
	gxse_module_register(&taurus_rng_module);
#endif
	return ret;
}

int gxse_device_register_taurus_secure_generic(void)
{
	int ret = 0;
#ifdef CFG_GXSE_FIRMWARE_MBOX
	ret = gxse_module_register(&taurus_mbox_module);
#endif
	return ret;
}

int gxse_device_register_taurus_crypto_dma(void)
{
	int ret = 0;
#ifdef CFG_GXSE_CRYPTO_DMA
	ret = gxse_module_register(&taurus_m2m_module);
#endif
	return ret;
}

int gxse_device_register_taurus_crypto_fifo(void)
{
	int ret = 0;
#ifdef CFG_GXSE_CRYPTO_FIFO
	ret = gxse_module_register(&taurus_crypto_module);
#endif
	return ret;
}

int gxse_device_register_taurus_klm_scpu(void)
{
	int ret = 0;
	if (gxse_device_register_taurus_secure_generic() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_KLM_SECURE_GENERIC
	ret |= gxse_module_register(&sirius_secure_klm_module);
#endif
	return ret;
}

int gxse_device_register_taurus_misc(void)
{
	if (gxse_device_register_taurus_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_taurus_misc_sci() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_taurus_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_taurus_misc_chip() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_taurus_misc_rng() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_taurus_crypto(void)
{
	if (gxse_device_register_taurus_crypto_dma() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_taurus_crypto_fifo() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_taurus_secure(void)
{
	if (gxse_device_register_taurus_secure_generic() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_taurus_klm(void)
{
	if (gxse_device_register_taurus_klm_scpu() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_chip_register_taurus(void)
{
	if (gxse_device_register_taurus_crypto() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_taurus_klm() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_taurus_secure() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_taurus_misc() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}
