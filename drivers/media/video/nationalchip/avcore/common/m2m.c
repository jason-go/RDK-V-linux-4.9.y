#include "kernelcalls.h"
#include "profile.h"
#include "secure_hal.h"
#include "gxav_common.h"
#include "gxav_bitops.h"
#include "gxav_dvr_propertytypes.h"

#ifdef CONFIG_AV_MODULE_FIRMWARE
int gxm2m_firmware_copy(unsigned int dst, unsigned int src, unsigned int size, GxAvFirmwareType type)
{
	int ret;
	unsigned char *temp_src = NULL;
	unsigned int align_size = 0, align = gxav_firewall_protect_align();

	GxTfmCrypto param;
	memset(&param, 0, sizeof(GxTfmCrypto));

	if (CHIP_IS_SIRIUS || CHIP_IS_TAURUS || CHIP_IS_GEMINI || CHIP_IS_CYGNUS) {
		param.module      = TFM_MOD_M2M;
		param.alg         = TFM_ALG_AES128;
		param.opt         = TFM_OPT_CBC;
		param.even_key.id = TFM_KEY_OTP_FIRM;
		param.odd_key.id  = TFM_KEY_OTP_FIRM;
		param.flags       = TFM_FLAG_CRYPT_EVEN_KEY_VALID | TFM_FLAG_CRYPT_ODD_KEY_VALID;

	} else {
		gx_dcache_clean_range(0, 0);
		gx_memcpy((void *)dst, (void *)src, size);
		gx_dcache_clean_range(0, 0);
		return 0;
	}

	if (size % align)
		align_size = size/align*align+align;
	else
		align_size = size;

	if (type == GXAV_FIRMWARE_AUDIO) {
		temp_src = (unsigned char *)(dst + align_size);
	}
	else {
		temp_src = gx_page_malloc(align_size);
	}

	memcpy(temp_src, (void *)src, size);
	gx_dcache_clean_range(0, 0);

	param.input.length  = align_size;
	param.output.length = align_size;
	param.input.buf     = temp_src;
	param.output.buf    = (unsigned char *)dst;
	param.input_paddr   = gx_virt_to_phys((unsigned int)temp_src);
	param.output_paddr  = gx_virt_to_phys(dst);

	ret = gxav_secure_decrypt(&param);
	if (temp_src && type != GXAV_FIRMWARE_AUDIO)
		gx_page_free(temp_src, size);

	return ret;
}
#endif

static int _get_gx3211_nds_chipid(unsigned char *buf, unsigned int len)
{
#define CHIP_ID_ADDR 0x00f00018
#define CHIP_ID_LEN  0x8
	static volatile unsigned int *reg_chipid = NULL;
	if (NULL == reg_chipid) {
		reg_chipid = gx_ioremap(CHIP_ID_ADDR, CHIP_ID_LEN);
		if (!reg_chipid) {
			gx_printf("ioremap chipid space failed!\n");
			return -1;
		}
	}
	memset(buf, 0, len);
	((unsigned int *)buf)[0] = REG_GET_VAL(reg_chipid);
	((unsigned int *)buf)[1] = REG_GET_VAL(reg_chipid+1);
	return 0;
}

/****************************************************
 * if DDR content limit enable :
 *      The encrypt function can copy internal buf to outter buf.
 *      The decrypt function can copy outter buf to internal buf.
 *
 ****************************************************/

static GxTfmCrypto data = {0};
static int gxm2m_normal_copy(unsigned int dst, unsigned int src, unsigned int size, int enc)
{
	int ret;
	GxTfmCrypto param;
	memset(&param, 0, sizeof(GxTfmCrypto));

	param.module        = TFM_MOD_M2M,
	param.input.length  = size;
	param.output.length = size;
	param.input.buf     = (unsigned char *)src;
	param.output.buf    = (unsigned char *)dst;
	param.opt         = TFM_OPT_ECB;

	if (CHIP_IS_SIRIUS) {
		param.alg         = TFM_ALG_AES128;
		param.even_key.id = TFM_KEY_SSUK;
		param.flags     |= TFM_FLAG_CRYPT_EVEN_KEY_VALID |
						 TFM_FLAG_CRYPT_ODD_KEY_VALID  |
						 TFM_FLAG_CRYPT_TS_PACKET_MODE |
						 TFM_FLAG_CRYPT_SWITCH_CLR |
						 TFM_FLAG_CRYPT_HW_PVR_MODE;

		if (data.flags & TFM_FLAG_PARAM_CONFIG) {
			param.alg           = data.alg;
			param.even_key.id   = data.even_key.id;
			param.even_key.sub  = data.even_key.sub;
		}

		param.odd_key.id    = param.even_key.id;
		param.odd_key.sub   = param.even_key.sub;

	} else if (CHIP_IS_TAURUS || CHIP_IS_GEMINI || CHIP_IS_CYGNUS) {
		param.alg         = TFM_ALG_DES;
		param.even_key.id = TFM_KEY_SCPU_KLM;
		if (data.flags & TFM_FLAG_PARAM_CONFIG) {
			param.alg           = data.alg;
		}

	} else if (CHIP_IS_GX3211) {
		param.alg         = TFM_ALG_DES;
		//param.even_key.id = TFM_KEY_NDS;
		param.even_key.id = TFM_KEY_ACPU_SOFT;
		_get_gx3211_nds_chipid(param.soft_key, sizeof(param.soft_key));

	} else
		return -1;

	param.input_paddr  = gx_virt_to_phys(src);
	param.output_paddr = gx_virt_to_phys(dst);

	if (enc)
		ret = gxav_secure_encrypt(&param);
	else
		ret = gxav_secure_decrypt(&param);

	return ret;
}

int gxm2m_dvr_config(GxDvrProperty_CryptConfig *config)
{
	data.alg           = config->alg;
	data.even_key.id   = TFM_KEY_RK;
	data.even_key.sub  = config->key_sub;
	data.flags        |= TFM_FLAG_PARAM_CONFIG;

	return 0;
}

int gxm2m_decrypt(void *dst, const void *src, int size)
{
	return gxm2m_normal_copy((unsigned int)dst, (unsigned int)src, size, 0);
}

int gxm2m_encrypt(void *dst, const void *src, int size)
{
	return gxm2m_normal_copy((unsigned int)dst, (unsigned int)src, size, 1);
}
