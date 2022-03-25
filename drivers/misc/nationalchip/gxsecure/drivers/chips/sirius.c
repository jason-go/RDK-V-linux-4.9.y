#include "sirius.h"

extern GxSeModule sirius_otp_module;
extern GxSeModule sirius_chip_module;
extern GxSeModule sirius_sci_module;
extern GxSeModule sirius_rng_module;
extern GxSeModule sirius_mbox_module;
extern GxSeModule sirius_klm_generic_module;
extern GxSeModule sirius_klm_irdeto_module;
extern GxSeModule sirius_secure_klm_module;
extern GxSeModule sirius_crypto_module;
extern GxSeModule sirius_m2m_module[CFG_CRYPTO_DMA_CHANNEL_NUM];
extern GxSeModule sirius_firewall_module;
extern GxSeModule sirius_hash_module;
extern GxSeModule sirius_sm2_module;

int gxse_device_register_sirius_secure_generic(void)
{
	int ret = 0;
#ifdef CFG_GXSE_FIRMWARE_MBOX
	ret = gxse_module_register(&sirius_mbox_module);
#endif
	return ret;
}

int gxse_device_register_sirius_secure_tee(void)
{
	return GXSE_SUCCESS;
}

int gxse_device_register_sirius_klm_generic(void)
{
	int ret = 0;
#ifdef CFG_GXSE_KLM_GENERIC
	ret = gxse_module_register(&sirius_klm_generic_module);
#endif
	return ret;
}

int gxse_device_register_sirius_klm_irdeto(void)
{
	int ret = 0;
#ifdef CFG_GXSE_KLM_IRDETO
	ret = gxse_module_register(&sirius_klm_irdeto_module);
#endif
	return ret;
}

int gxse_device_register_sirius_klm_scpu(void)
{
	int ret = 0;
	if (gxse_device_register_sirius_secure_generic() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_KLM_SECURE_GENERIC
	ret |= gxse_module_register(&sirius_secure_klm_module);
#endif
	return ret;
}

int gxse_device_register_sirius_crypto_fifo(void)
{
	int ret = 0;
#ifdef CFG_GXSE_CRYPTO_FIFO
	ret = gxse_module_register(&sirius_crypto_module);
#endif
	return ret;
}

int gxse_device_register_sirius_crypto_dma(void)
{
	int ret = 0;
	if (gxse_device_register_sirius_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_CRYPTO_DMA
	uint32_t i = 0;
	for (i = 0; i < CFG_CRYPTO_DMA_CHANNEL_NUM; i++)
		if ((ret |= gxse_module_register(&sirius_m2m_module[i])) < 0)
			break;
#endif

	return ret;
}

int gxse_device_register_sirius_misc_otp(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_OTP
	ret |= gxse_module_register(&sirius_otp_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc_firewall(void)
{
	int ret = 0;
	if (gxse_device_register_sirius_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

#ifdef CFG_GXSE_FIREWALL
	ret |= gxse_module_register(&sirius_firewall_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc_sci(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SCI
	ret = gxse_module_register(&sirius_sci_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc_chip(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_CHIP_CFG
	ret = gxse_module_register(&sirius_chip_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc_rng(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_RNG
	ret = gxse_module_register(&sirius_rng_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc_dgst(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_HASH
	ret = gxse_module_register(&sirius_hash_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc_akcipher(void)
{
	int ret = 0;
#ifdef CFG_GXSE_MISC_SM2
	ret = gxse_module_register(&sirius_sm2_module);
#endif
	return ret;
}

int gxse_device_register_sirius_misc(void)
{
	if (gxse_device_register_sirius_misc_firewall() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc_otp() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc_chip() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc_rng() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc_dgst() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc_akcipher() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc_sci() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_sirius_crypto(void)
{
	if (gxse_device_register_sirius_crypto_fifo() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_crypto_dma() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_sirius_klm(void)
{
	if (gxse_device_register_sirius_klm_generic() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_klm_irdeto() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_klm_scpu() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_device_register_sirius_secure(void)
{
	if (gxse_device_register_sirius_secure_generic() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_secure_tee() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}

int gxse_chip_register_sirius(void)
{
	if (gxse_device_register_sirius_secure() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_klm() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_crypto() < 0)
		return GXSE_ERR_GENERIC;

	if (gxse_device_register_sirius_misc() < 0)
		return GXSE_ERR_GENERIC;

	return GXSE_SUCCESS;
}
