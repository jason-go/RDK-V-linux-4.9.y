#include "cygnus.h"
#include "chip_all.h"

DEFINE_DEV_REGISTER(cygnus, taurus, klm)
DEFINE_DEV_REGISTER(cygnus, taurus, crypto)
DEFINE_DEV_REGISTER(cygnus, taurus, secure)

DEFINE_MODULE_REGISTER(cygnus, taurus, klm, scpu)
DEFINE_MODULE_REGISTER(cygnus, taurus, misc, otp)
DEFINE_MODULE_REGISTER(cygnus, taurus, misc, rng)
DEFINE_MODULE_REGISTER(cygnus, taurus, misc, chip)
DEFINE_MODULE_REGISTER(cygnus, taurus, misc, firewall)
DEFINE_MODULE_REGISTER(cygnus, taurus, crypto, dma)
DEFINE_MODULE_REGISTER(cygnus, taurus, crypto, fifo)
DEFINE_MODULE_REGISTER(cygnus, taurus, secure, generic)

extern GxSeModule cygnus_sci_module;

int gxse_device_register_cygnus_misc_sci(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SCI
	ret = gxse_module_register(&cygnus_sci_module);
#endif
	return ret;
}

int gxse_device_register_cygnus_misc(void)
{
	if (gxse_device_register_cygnus_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_cygnus_misc_sci() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_cygnus_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_cygnus_misc_chip() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_cygnus_misc_rng() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_chip_register_cygnus(void)
{
	if (gxse_device_register_cygnus_crypto() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_cygnus_klm() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_cygnus_secure() < 0)
		return GXSE_ERR_GENERIC;
	if (gxse_device_register_cygnus_misc() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

