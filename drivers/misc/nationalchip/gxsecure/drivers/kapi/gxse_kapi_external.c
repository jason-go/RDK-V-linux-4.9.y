#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"
#include "gxse_hwobj_misc.h"

#ifdef TEE_OS

#ifdef CFG_GXSE_CRYPTO
int gx_ta_decrypt(unsigned char *src, unsigned char*dst, unsigned int len)
{
	int ret = 0, pos = 0, size = 0;
	unsigned char ta_iv[] = {
		0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
		0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
	};

	GxTfmCrypto chan = {0};

	memset(&chan,0,sizeof(chan));
	chan.module = TFM_MOD_M2M;
	chan.opt  = TFM_OPT_CBC;
	chan.alg  = TFM_ALG_AES128;
	chan.src.id = TFM_SRC_MEM;
	chan.dst.id = TFM_DST_MEM;
	chan.even_key.id = TFM_KEY_OTP_SOFT;
	chan.flags = TFM_FLAG_CRYPT_EVEN_KEY_VALID;

	gxlog_i(GXSE_LOG_MOD_TFM, "gxsecure tfm decrypt ta.\n");

	while (pos < len) {
		size = len - pos;
		if (size > TFM_MAX_CIPHER_UNIT)
			size = TFM_MAX_CIPHER_UNIT;

		memcpy(chan.iv,ta_iv,16);
		chan.input.buf = src + pos;
		chan.input.length = size;
		chan.output.buf = dst + pos;
		chan.output.length = size;
		memcpy(ta_iv, src+pos+size-16, 16);
		if ((ret = gx_tfm_decrypt(&chan)) < 0)
			break;
		pos += size;
	}
	gxlog_i(GXSE_LOG_MOD_TFM, "finish gxsecure tfm decrypt ta. ret = %d\n", ret);
	return ret;
}

TEE_Result tee_otp_get_hw_unique_key(struct tee_hw_unique_key *hwkey)
{
	GxTfmCrypto param = {0};
	unsigned char die_id[16] = {0};

	firewall_dev_probe();
	crypto_dev_probe();

	if (gxse_fuse_get_publicid(die_id+16-GXSE_MISC_CSSN_LEN, GXSE_MISC_CSSN_LEN) < 0)
		return GXSE_ERR_GENERIC;

	param.module        = TFM_MOD_M2M;
	param.module_sub    = 0;
	param.alg           = TFM_ALG_AES128;
	param.opt           = TFM_OPT_ECB;
	param.src.id        = TFM_SRC_MEM;
	param.dst.id        = TFM_DST_MEM;
	param.even_key.id   = TFM_KEY_OTP_SOFT;
	param.input.buf     = die_id;
	param.input.length  = sizeof(die_id);
	param.output.buf    = hwkey->data;
	param.output.length = sizeof(hwkey->data);
	param.flags         = TFM_FLAG_CRYPT_EVEN_KEY_VALID | TFM_FLAG_ISR_QUERY_UNTIL;

	return gx_tfm_decrypt(&param);
}

int gx_ltc_crypt(int alg, char enc, const unsigned char *src, unsigned char *dst,
        unsigned long blocks, unsigned char *key, unsigned int keyLen, unsigned char *IV)
{
	unsigned int block_len = 0;
	GxTfmCrypto param  = {0};

	if (keyLen > 32)
		return GXSE_ERR_GENERIC;

	switch (alg) {
	case TEE_ALG_AES_ECB_NOPAD:
		param.alg = TFM_ALG_AES128;
		param.opt = TFM_OPT_ECB;
		block_len = 16;
		break;

	case TEE_ALG_AES_CBC_NOPAD:
		param.alg = TFM_ALG_AES128;
		param.opt = TFM_OPT_CBC;
		block_len = 16;
		break;

	case TEE_ALG_DES_ECB_NOPAD:
		param.alg = TFM_ALG_DES;
		param.opt = TFM_OPT_ECB;
		block_len = 8;
		break;

	case TEE_ALG_DES_CBC_NOPAD:
		param.alg = TFM_ALG_DES;
		param.opt = TFM_OPT_CBC;
		block_len = 8;
		break;

	case TEE_ALG_DES3_ECB_NOPAD:
		param.alg = TFM_ALG_TDES;
		param.opt = TFM_OPT_ECB;
		block_len = 8;
		break;

	case TEE_ALG_DES3_CBC_NOPAD:
		param.alg = TFM_ALG_TDES;
		param.opt = TFM_OPT_CBC;
		block_len = 8;
		break;

	default:
		return GXSE_ERR_PARAM;
	}
	param.module       = TFM_MOD_M2M;
	param.module_sub   = 0;
	param.src.id       = TFM_SRC_MEM;
	param.dst.id       = TFM_DST_MEM;
	param.input.buf    = (void *)src;
	param.output.buf   = (void *)dst;
	param.input.length = param.output.length = blocks * block_len;
	param.even_key.id  = TFM_KEY_ACPU_SOFT;
	param.flags       |= TFM_FLAG_CRYPT_EVEN_KEY_VALID;

	if (alg == TEE_ALG_AES_ECB_NOPAD || alg == TEE_ALG_AES_CBC_NOPAD) {
		if (keyLen == 32)
			param.alg = TFM_ALG_AES256;
		else if (keyLen == 24)
			param.alg = TFM_ALG_AES192;
	}
	memcpy(param.soft_key, key, keyLen);

	if (IV)
		memcpy(param.iv, IV, block_len);
	else
		param.opt = TFM_OPT_ECB;

	if (enc)
		return gx_tfm_encrypt(&param);
	return gx_tfm_decrypt(&param);
}
#endif // end of CFG_GXSE_CRYPTO

#ifdef CFG_GXSE_MISC
unsigned int gx_ta_marketid(void)
{
	unsigned char mark_id[4]={0};
	unsigned int otp_mark = 0;

	gxse_fuse_get_marketid(mark_id,4);

	otp_mark = (mark_id[0]<<24) | (mark_id[1]<<16) | (mark_id[2]<<8) | (mark_id[3]);
	gxlog_i(GXSE_LOG_MOD_HAL, "otp market %x\n", otp_mark);

	return otp_mark;
}

int tee_otp_get_die_id(unsigned char *buffer, size_t len)
{
	if (len < GXSE_MISC_CSSN_LEN) {
		gxlog_e(GXSE_LOG_MOD_HAL, "die_id buffer length error\n");
		return GXSE_ERR_GENERIC;
	}

	return gxse_fuse_get_publicid(buffer, len);
}

/* porting: OPTEE get rng function */
unsigned char hw_get_random_byte(void)
{
	unsigned char ret;
	static unsigned char pos = 0;
	static unsigned int  val = 0;
	if (pos == 0)
		val = gxse_rng_request();

	ret = (val>>(24-8*pos)) & 0xff;
	pos++;
	if (pos == 4)
		pos = 0;
	return ret;
}
#endif // end of CFG_GXSE_MISC
#endif // end of TEE_OS

#ifdef CPU_SCPU
void udelay(unsigned int us)
{
	GxSeModule *mod = gxse_module_find_by_id(GXSE_MOD_MISC_TIMER);
	if (NULL == mod) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. id = %x\n", GXSE_MOD_MISC_TIMER);
		return;
	}

	if (mod_ops(mod)->ioctl)
		mod_ops(mod)->ioctl(mod, MISC_UDELAY, &us, 4);
}

void mdelay(unsigned int ms)
{
	udelay(ms*1000);
}

void delay(unsigned int s)
{
	udelay(s*1000*1000);
}
#endif // end of CPU_SCPU
