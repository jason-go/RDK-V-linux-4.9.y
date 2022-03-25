#include "sirius_crypto.h"
#include "taurus_crypto.h"
#include "sirius_crypto_reg.h"
#include "taurus_crypto_reg.h"

int32_t taurus_dynamic_crypto_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_CWUK:
		param->sub &= 0x3;
		return TAURUS_SKLM_KEY_DSK1 + param->sub;

	case TFM_KEY_PVRK:
		param->sub &= 0x1;
		return TAURUS_SKLM_KEY_RK1 + param->sub;

	case TFM_KEY_ROOTKEY:
		return TAURUS_SKLM_KEY_KDS;

	case TFM_KEY_SMK:
		return TAURUS_SKLM_KEY_SMK;

	case TFM_KEY_SCPU_SOFT:
		return TAURUS_SKLM_KEY_SOFT;

	case TFM_KEY_TK:
		return TAURUS_SKLM_KEY_TK1 + param->sub;

	case TFM_KEY_REG:
		param->sub &= 0x7;
		return TAURUS_SKLM_KEY_REG0 + param->sub;

	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t taurus_dynamic_crypto_get_dst(GxTfmDstBuf *param)
{
	switch (param->id) {
	case TFM_DST_TS:
		return TAURUS_SKLM_DST_TS;

	case TFM_DST_M2M:
		return TAURUS_SKLM_DST_M2M;

	case TFM_DST_MEM:
		return TAURUS_SKLM_DST_SOFT;

	case TFM_DST_REG:
		param->sub &= 0x7;
		return TAURUS_SKLM_DST_REG0 + param->sub;

	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t taurus_dynamic_crypto_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	int32_t ret = 0;
	volatile TaurusCryptoReg *reg = (volatile TaurusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	(void) oddkey_sel;
	(void) key;
	(void) key_len;

	if ((ret = ops->get_key(&key_sel)) < 0)
		return ret;

	reg->dctrl.bits.key_sel = ret;

	return GXSE_SUCCESS;
}

int32_t taurus_dynamic_crypto_clr_key(void *vreg, void *priv)
{
	volatile TaurusCryptoReg *reg = (volatile TaurusCryptoReg *)vreg;
	(void) priv;

	reg->dctrl.bits.key_clr = 1;
	reg->dctrl.bits.key_clr = 0;

	return GXSE_SUCCESS;
}

static GxCryptoResOps taurus_dy_crypto_resops = {
	.get_alg = sirius_dynamic_crypto_get_alg,
	.get_key = taurus_dynamic_crypto_get_key,
	.get_src = sirius_dynamic_crypto_get_src,
	.get_dst = taurus_dynamic_crypto_get_dst,
};

int32_t taurus_dynamic_crypto_init(GxSeModuleHwObj *obj)
{
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;

	p->reg = (volatile SiriusCryptoReg *)obj->reg;
	p->reg->crypto_int_en.value = 0x3;
	p->resops = &taurus_dy_crypto_resops;
#ifdef CPU_SCPU
	scpu_crypto_init(obj);
#endif
	return GXSE_SUCCESS;
}

int32_t taurus_dynamic_crypto_scpu_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK | 1 << TFM_KEY_PVRK;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_TDES | 1 << TFM_ALG_SM4;
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->flags = TFM_FLAG_KLM_INPUT_HALF | TFM_FLAG_KLM_CW_HALF | TFM_FLAG_KLM_CW_HIGH_8BYTE | TFM_FLAG_KLM_CW_XOR;

	param->key_sub_num[TFM_KEY_CWUK] = 4;
	param->key_sub_num[TFM_KEY_PVRK] = 2;
	param->dst = 1 << TFM_DST_TS | 1 << TFM_DST_M2M;
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps taurus_dy_crypto_devops = {
	.init            = taurus_dynamic_crypto_init,
};

static GxCryptoOps taurus_dy_crypto_hwops = {
	.check_param     = sirius_dynamic_crypto_check_param,
//	.capability      = taurus_dynamic_crypto_scpu_capability,

    .get_blocklen    = sirius_dynamic_crypto_get_blocklen,
	.set_key         = taurus_dynamic_crypto_set_key,
	.set_alg         = sirius_dynamic_crypto_set_alg,
    .fifo_put        = sirius_dynamic_crypto_fifo_put,
    .fifo_get        = sirius_dynamic_crypto_fifo_get,
	.set_input_buf   = sirius_dynamic_crypto_set_input_buf,
	.set_output_buf  = sirius_dynamic_crypto_set_output_buf,
	.clr_config      = sirius_dynamic_crypto_clr_config,
	.clr_key         = taurus_dynamic_crypto_clr_key,
	.set_ready       = scpu_crypto_set_ready,
	.wait_finish     = scpu_crypto_wait_finish,
};

static GxSeModuleHwObjTfmOps crypto_tfm_ops = {
	.devops = &taurus_dy_crypto_devops,
	.hwops = &taurus_dy_crypto_hwops,

	.tfm_cipher = {
		.decrypt         = gx_tfm_object_crypto_fifo_decrypt,
		.clr_key         = gx_tfm_object_crypto_clr_key,
	//	.capability      = gx_tfm_object_crypto_capability,
	},
};

static struct crypto_priv taurus_crypto_priv = {
	.param = {0},
};

static GxSeModuleHwObj taurus_dy_crypto_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_CRYPTO,
	.ops  = &crypto_tfm_ops,
	.priv = &taurus_crypto_priv,
};

GxSeModule taurus_crypto_scpu_dynamic_module = {
	.id   = GXSE_MOD_CRYPTO_DYNAMIC,
	.ops  = &crypto_dev_ops,
	.hwobj= &taurus_dy_crypto_tfm,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_CRYPTO,
		.reg_len   = sizeof(TaurusCryptoReg),
	},
};
