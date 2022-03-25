#include "gemini.h"
#include "chip_all.h"

DEFINE_DEV_REGISTER(gemini, taurus, klm)
DEFINE_DEV_REGISTER(gemini, taurus, crypto)
DEFINE_DEV_REGISTER(gemini, taurus, secure)

DEFINE_MODULE_REGISTER(gemini, taurus, klm, scpu)
DEFINE_MODULE_REGISTER(gemini, taurus, misc, otp)
DEFINE_MODULE_REGISTER(gemini, taurus, misc, rng)
DEFINE_MODULE_REGISTER(gemini, taurus, misc, chip)
DEFINE_MODULE_REGISTER(gemini, taurus, misc, firewall)
DEFINE_MODULE_REGISTER(gemini, taurus, crypto, dma)
DEFINE_MODULE_REGISTER(gemini, taurus, crypto, fifo)
DEFINE_MODULE_REGISTER(gemini, taurus, secure, generic)

extern GxSeModule gemini_sci_module;

int gxse_device_register_gemini_misc_sci(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SCI
	ret = gxse_module_register(&gemini_sci_module);
#endif
	return ret;
}

int gxse_device_register_gemini_misc(void)
{
	if (gxse_device_register_gemini_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gemini_misc_sci() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gemini_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gemini_misc_chip() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_gemini_misc_rng() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_chip_register_gemini(void)
{
	if (gxse_device_register_gemini_crypto() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_gemini_klm() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_gemini_secure() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_gemini_misc() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}
