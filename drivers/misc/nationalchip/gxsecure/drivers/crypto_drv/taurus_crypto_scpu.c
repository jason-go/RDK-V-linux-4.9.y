#include "sirius_crypto.h"
#include "taurus_crypto.h"

int32_t taurus_boot_crypto_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_ACPU_SOFT:
		return TAURUS_CRYPTO_KEY_SOFT;

	case TFM_KEY_OTP_SOFT:
		return TAURUS_CRYPTO_KEY_SOFT;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t taurus_boot_crypto_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->max_sub_num = 1;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_SM4;
	param->opt = 1 << TFM_OPT_ECB | 1 << TFM_OPT_CBC;
	param->key = 1 << TFM_KEY_OTP_SOFT | 1 << TFM_KEY_ACPU_SOFT;

	return GXSE_SUCCESS;
}

static GxCryptoResOps taurus_scpu_crypto_resops = {
	.get_alg = sirius_crypto_get_alg,
	.get_key = taurus_boot_crypto_get_key,
	.get_opt = sirius_crypto_get_opt,
};

int32_t taurus_crypto_init(GxSeModuleHwObj *obj)
{
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;

	p->reg = (volatile SiriusCryptoReg *)obj->reg;
	p->reg->crypto_int_en.value = 0x3;
	p->resops = &taurus_scpu_crypto_resops;

#ifdef CPU_SCPU
	scpu_crypto_init(obj);
#endif
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps taurus_crypto_devops = {
	.init            = taurus_crypto_init,
};

static GxCryptoOps taurus_crypto_ops = {
	.check_param     = sirius_crypto_check_param,
//	.capability      = taurus_boot_crypto_capability,
    .set_alg         = sirius_crypto_set_alg,
    .set_key         = sirius_crypto_set_key,
    .set_opt         = sirius_crypto_set_opt,
    .get_blocklen    = sirius_crypto_get_blocklen,
    .fifo_put        = sirius_crypto_fifo_put,
    .fifo_get        = sirius_crypto_fifo_get,
	.clr_key         = sirius_crypto_clr_key,
	.set_ready       = scpu_crypto_set_ready,
	.wait_finish     = scpu_crypto_wait_finish,
};

static GxSeModuleHwObjTfmOps taurus_scpu_crypto_fifo_tfm_ops = {
	.devops = &taurus_crypto_devops,
	.hwops = &taurus_crypto_ops,

	.tfm_cipher = {
		.set_pvr_key   = gx_tfm_object_crypto_set_pvr_key,
		.clr_key       = gx_tfm_object_crypto_clr_key,
		.enable_key    = gx_tfm_object_crypto_enable_key,
		.disable_key   = gx_tfm_object_crypto_disable_key,
		.decrypt       = gx_tfm_object_crypto_fifo_decrypt,
	//	.capability    = gx_tfm_object_crypto_capability,
	},
};

static struct crypto_priv taurus_scpu_crypto_priv = {
	.param = {0},
	.resops = &taurus_scpu_crypto_resops,
};

static GxSeModuleHwObj taurus_scpu_crypto_fifo_obj = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_CRYPTO,
	.ops  = &taurus_scpu_crypto_fifo_tfm_ops,
	.priv = &taurus_scpu_crypto_priv,
};

GxSeModule taurus_crypto_module = {
	.id    = GXSE_MOD_CRYPTO_FIFO,
	.ops   = &crypto_dev_ops,
	.hwobj = &taurus_scpu_crypto_fifo_obj,
	.res   = {
		.reg_base  = GXSCPU_BASE_ADDR_CRYPTO,
		.reg_len   = sizeof(SiriusCryptoReg),
	},
};
