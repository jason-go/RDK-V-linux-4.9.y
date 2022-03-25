#include "gxse_core.h"
#include "gxse_hwobj_tfm.h"

int32_t gx_tfm_encrypt(GxTfmCrypto *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, param->module_sub))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d, sub = %d\n", param->module, param->module_sub);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_ENCRYPT, param, sizeof(GxTfmCrypto));

	return ret;
}

int32_t gx_tfm_decrypt(GxTfmCrypto *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, param->module_sub))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d, sub = %d\n", param->module, param->module_sub);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_DECRYPT, param, sizeof(GxTfmCrypto));

	return ret;
}

int32_t gx_tfm_set_pvr_key(GxTfmPVRKey *param)
{
	int32_t ret = GXSE_ERR_GENERIC;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", param->module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_SET_PVRK, param, sizeof(GxTfmPVRKey));

	return ret;
}

int32_t gx_tfm_enable_key(GxTfmModule module, uint32_t sub)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == (mod = gxse_module_find_by_tfmid(module, sub))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d, sub = %d\n", module, sub);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_ENABLE_KEY, NULL, 0);

	return ret;
}

int32_t gx_tfm_disable_key(GxTfmModule module, uint32_t sub)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == (mod = gxse_module_find_by_tfmid(module, sub))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d, sub = %d\n", module, sub);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_DISABLE_KEY, NULL, 0);

	return ret;
}

int32_t gx_tfm_clear_key(GxTfmModule module, uint32_t sub)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == (mod = gxse_module_find_by_tfmid(module, sub))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d, sub = %d\n", module, sub);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_CLEAR_KEY, NULL, 0);

	return ret;
}

int32_t gx_tfm_klm_select_rootkey(GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", param->module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_KLM_SELECT_ROOTKEY, param, sizeof(GxTfmKeyLadder));

	return ret;
}

int32_t gx_tfm_klm_set_kn(GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", param->module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_KLM_SET_KN, param, sizeof(GxTfmKeyLadder));

	return ret;
}

int32_t gx_tfm_klm_set_cw(GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", param->module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_KLM_SET_CW, param, sizeof(GxTfmKeyLadder));

	return ret;
}

int32_t gx_tfm_klm_update_TD_param(GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", param->module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_KLM_UPDATE_TD_PARAM, param, sizeof(GxTfmKeyLadder));

	return ret;
}

int32_t gx_tfm_klm_get_resp(GxTfmKeyLadder *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(param->module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", param->module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_KLM_GET_RESP, param, sizeof(GxTfmKeyLadder));

	return ret;
}

int32_t gx_tfm_dgst(GxTfmDgst *param)
{
	GxSeModule *mod = NULL;
	int32_t ret = GXSE_SUCCESS;
	uint32_t module = TFM_MOD_MISC_DGST;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (param->alg == TFM_ALG_SM2_ZA)
		module = TFM_MOD_MISC_AKCIPHER;

	if (NULL == (mod = gxse_module_find_by_tfmid(module, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", module);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_DGST, param, sizeof(GxTfmDgst));

	return ret;
}

int gx_tfm_verify(GxTfmVerify *param)
{
	int32_t ret = GXSE_SUCCESS;
	GxSeModule *mod = NULL;

	if (NULL == param) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter is NULL.\n");
		return GXSE_ERR_PARAM;
	}

	if (NULL == (mod = gxse_module_find_by_tfmid(TFM_MOD_MISC_AKCIPHER, 0))) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Can't find the module. i = %d\n", TFM_MOD_MISC_AKCIPHER);
		return GXSE_ERR_GENERIC;
	}

	if (mod_ops(mod)->ioctl)
		ret = mod_ops(mod)->ioctl(mod, TFM_VERIFY, param, sizeof(GxTfmVerify));

	return ret;
}

#ifdef CPU_ACPU
int32_t gx_tfm_align_buf(GxTfmBuf *param, GxTfmBuf *align_buf, uint32_t *paddr, uint32_t align, uint32_t flags)
{
	uint32_t align_flag = flags & 0xf;
	uint32_t src_flag = TFM_FLAG_GET(flags, TFM_FLAG_IS_SRC);
	GxTfmBuf tmp = {0};

	if (NULL == align_buf->buf || align_buf->length == 0)
		goto set_addr;

	if (align_flag == TFM_UNFORCE_ALIGN)
		goto set_addr;

	if (param->length > align_buf->length) {
		gxlog_e(GXSE_LOG_MOD_HAL, "The cipher_buflen is bigger than align_buflen.\n");
		return GXSE_ERR_GENERIC;
	}
	gxlog_d(GXSE_LOG_MOD_HAL, "%d %p %x %p %x\n", align_flag, param->buf, param->length, align_buf->buf, align_buf->length);

	memcpy(&tmp, param, sizeof(GxTfmBuf));
	if ((uint32_t)param->buf % align || (align_flag && param->buf != align_buf->buf)) {
		uint32_t buf = (uint32_t)align_buf->buf;
		if (buf % align) {
			if (param->length+align > align_buf->length) {
				gxlog_e(GXSE_LOG_MOD_HAL, "The cipher_buflen is bigger than align_buflen.\n");
				return GXSE_ERR_GENERIC;
			}
			buf = (buf + align - 1) / align * align;
		}

		if (src_flag)
			memcpy((void *)buf, param->buf, param->length);

		param->buf = (void *)buf;
	}

	if (src_flag)
		gx_osdep_cache_sync(param->buf, param->length, DMA_TO_DEVICE);

	memcpy(align_buf, &tmp, sizeof(GxTfmBuf));

set_addr:
	gxlog_d(GXSE_LOG_MOD_HAL, "%p %x %x\n", param->buf, param->length, *paddr);
	if (*paddr == 0 || align_flag == TFM_FORCE_ALIGN)
		*paddr = gx_virt_to_phys(param->buf);

	gxlog_d(GXSE_LOG_MOD_HAL, "%p %x %x\n", param->buf, param->length, *paddr);
	return GXSE_SUCCESS;
}

int32_t gx_tfm_restore_buf(GxTfmBuf *param, GxTfmBuf *old_buf, uint32_t flags)
{
	uint32_t align_flag = flags & 0xf;
	uint32_t src_flag = TFM_FLAG_GET(flags, TFM_FLAG_IS_SRC);

	if (align_flag == TFM_UNFORCE_ALIGN)
		return GXSE_SUCCESS;

	if (!src_flag)
		gx_osdep_cache_sync(param->buf, param->length, DMA_FROM_DEVICE);

	if (param->buf != old_buf->buf) {
		if (!src_flag) {
			memcpy(old_buf->buf, param->buf, param->length);
		}
		param->buf = old_buf->buf;
	}

	return GXSE_SUCCESS;
}

static int gx_dgst_exec(int alg, uint8_t *input, uint32_t length,
		uint8_t *mkey, uint32_t mkeyLen, uint8_t *result, uint8_t dma_en)
{
	GxTfmDgst dgst = {0};

	if (NULL == input || NULL == result) {
		gxlog_e(GXSE_LOG_MOD_HAL, "Parameter error\n");
		return GXSE_ERR_PARAM;
	}

	memset(&dgst, 0, sizeof(dgst));
	dgst.module = TFM_MOD_MISC;
	dgst.alg    = alg;
	dgst.input.buf = input;
	dgst.input.length = length;
	dgst.output.buf = result;
	dgst.output.length = 32;
	dgst.flags |= dma_en ? TFM_FLAG_HASH_DMA_EN : 0;

	if (alg == TFM_ALG_SM2_ZA) {
		dgst.pub_key.buf = mkey;
		dgst.pub_key.length = mkeyLen;

	} else if (alg > TFM_ALG_SM3_HMAC) {
		dgst.hmac_key.buf = mkey;
		dgst.hmac_key.length = mkeyLen;
	}

	return gx_tfm_dgst(&dgst);
}

int gx_sm3_perform (
	unsigned char* dataIn, unsigned int dataInLen,
	unsigned char* result, int dma_en)
{
	return gx_dgst_exec(TFM_ALG_SM3, dataIn, dataInLen, NULL, 0, result, dma_en);
}

int gx_sm3_perform_hmac (
	unsigned char* dataIn, unsigned int dataInLen,
	unsigned char* key, unsigned int keyLen,
	unsigned char* result, int dma_en)
{
	return gx_dgst_exec(TFM_ALG_SM3_HMAC, dataIn, dataInLen, key, keyLen, result, dma_en);
}

int gx_sha256_perform (
	unsigned char *dataIn, unsigned int   dataInLen,
	unsigned char *result, int dma_en)
{
	return gx_dgst_exec(TFM_ALG_SHA256, dataIn, dataInLen, NULL, 0, result, dma_en);
}

int gx_sha256_perform_hmac (
	unsigned char* dataIn, unsigned int dataInLen,
	unsigned char* key, unsigned int keyLen,
	unsigned char* result, int dma_en)
{
	return gx_dgst_exec(TFM_ALG_SHA256_HMAC, dataIn, dataInLen, key, keyLen, result, dma_en);
}

int gx_sha1_perform (
	unsigned char *dataIn, unsigned int   dataInLen,
	unsigned char *result, int dma_en)
{
	return gx_dgst_exec(TFM_ALG_SHA1, dataIn, dataInLen, NULL, 0, result, dma_en);
}

int gx_sm2_cal_za (
	unsigned char *pub_key, unsigned int pub_key_len,
	unsigned char *ida, unsigned int ida_len,
	unsigned char *za, int dma_en)
{
	return gx_dgst_exec(TFM_ALG_SM2_ZA, ida, ida_len, pub_key, pub_key_len, za, dma_en);
}

int gx_sm2_encrypt (
	unsigned char *pub_key, unsigned int pub_key_len,
	unsigned char *inputData, unsigned int inputData_size,
	unsigned char *outputData)
{
	GxTfmCrypto param = {0};

	memset(&param, 0, sizeof(GxTfmCrypto));
	param.module             = TFM_MOD_MISC_AKCIPHER;
	param.alg                = TFM_ALG_SM2;
	param.input.buf          = inputData;
	param.input.length       = inputData_size;
	param.output.buf         = outputData;
	param.output.length      = 256; // TODO
	param.sm2_pub_key.buf    = pub_key;
	param.sm2_pub_key.length = pub_key_len;

	return gx_tfm_encrypt(&param);
}

int gx_sm2_decrypt (
	unsigned char *inputData, unsigned int inputData_size,
	unsigned char *outputData)
{
	GxTfmCrypto param = {0};

	memset(&param, 0, sizeof(GxTfmCrypto));
	param.module             = TFM_MOD_MISC_AKCIPHER;
	param.alg                = TFM_ALG_SM2;
	param.input.buf          = inputData;
	param.input.length       = inputData_size;
	param.output.buf         = outputData;
	param.output.length      = 256; // TODO

	return gx_tfm_decrypt(&param);
}
#endif

#if defined (LINUX_OS)
EXPORT_SYMBOL(gx_tfm_encrypt);
EXPORT_SYMBOL(gx_tfm_decrypt);
#endif
