#include "sirius_crypto_reg.h"
#include "sirius_crypto.h"

//#define DEBUG_INPUT
//#define DEBUG_OUTPUT

struct crypto_priv sirius_crypto_dy_priv = {
	.param = {0},
};

int32_t sirius_dynamic_crypto_get_alg(GxTfmAlg alg)
{
	switch(alg) {
	case TFM_ALG_AES128:
		return SIRIUS_SKLM_ALG_AES;

	case TFM_ALG_TDES:
		return SIRIUS_SKLM_ALG_TDES;

	case TFM_ALG_SM4:
		return SIRIUS_SKLM_ALG_SM4;

	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_dynamic_crypto_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_CWUK:
		param->sub &= 0x1;
		return SIRIUS_SKLM_KEY_DSK1 + param->sub;

	case TFM_KEY_PVRK:
		param->sub &= 0x1;
		return SIRIUS_SKLM_KEY_RK1 + param->sub;

	case TFM_KEY_ROOTKEY:
		return SIRIUS_SKLM_KEY_KDS;

	case TFM_KEY_SMK:
		return SIRIUS_SKLM_KEY_SMK;

	case TFM_KEY_REG:
		param->sub &= 0x7;
		return SIRIUS_SKLM_KEY_REG0 + param->sub;

	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_dynamic_crypto_get_src(GxTfmSrcBuf *param)
{
	switch (param->id) {
	case TFM_SRC_MEM:
		return SIRIUS_SKLM_SRC_SOFT;

	case TFM_SRC_REG:
		param->sub &= 0x7;
		return SIRIUS_SKLM_SRC_REG0 + param->sub;

	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_dynamic_crypto_get_dst(GxTfmDstBuf *param)
{
	switch (param->id) {
	case TFM_DST_TS:
		return SIRIUS_SKLM_DST_TS;

	case TFM_DST_GP:
		return SIRIUS_SKLM_DST_GP;

	case TFM_DST_M2M:
		return SIRIUS_SKLM_DST_M2M;

	case TFM_DST_MEM:
		return SIRIUS_SKLM_DST_SOFT;

	case TFM_DST_REG:
		param->sub &= 0x7;
		return SIRIUS_SKLM_DST_REG0 + param->sub;

	default:
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_dynamic_crypto_clr_config(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	(void) priv;

	reg->dctrl.bits.di_words = 0;

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	(void) vreg;

	if (cmd == TFM_ENCRYPT)
		return GXSE_ERR_PARAM;

	if (ops->get_alg(param->alg) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_src(&param->src) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_dst(&param->dst) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_key(&param->even_key) < 0)
		return GXSE_ERR_PARAM;

	memcpy(&p->param, param, sizeof(GxTfmCrypto));

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	int32_t ret = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
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

int32_t sirius_dynamic_crypto_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	if ((ret = ops->get_alg(alg)) < 0)
		return ret;

	reg->dctrl.bits.cipher = ret;

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_set_input_buf(void *vreg, void *priv, GxTfmSrcBuf in_sel, uint8_t *in, uint32_t in_len)
{
	int32_t ret = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	(void) in;
	(void) in_len;

	if ((ret = ops->get_src(&in_sel)) < 0)
		return ret;

	reg->dctrl.bits.data_src = ret;

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_set_output_buf(void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len)
{
	int32_t ret = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	GxTfmCrypto *param = &p->param;

	uint32_t inhalf_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_INPUT_HALF);
	uint32_t cwhalf_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_HALF);
	uint32_t cwhigh_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_HIGH_8BYTE);
	uint32_t cwxor_flag  = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_XOR);
	uint32_t cwready_flag= TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_READY);
	(void) out;
	(void) out_len;

	if ((ret = ops->get_dst(&out_sel)) < 0)
		return ret;

	reg->dctrl.bits.cw_ready = 0;
	reg->dctrl.bits.rslt_des = ret;

	if (cwready_flag) {
		reg->dctrl.bits.xor_en  = cwxor_flag;
		reg->dctrl.bits.di_words = inhalf_flag;

		if (ret >= SIRIUS_SKLM_DST_TS) {
			reg->dctrl.bits.cw_ready = 1;
			reg->dctrl.bits.cw_half  = cwhalf_flag;
			reg->dctrl.bits.cw_sel   = cwhigh_flag;
		}
	}

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_get_blocklen(void *vreg, void *priv)
{
	(void) vreg;
	(void) priv;
	return SIRIUS_SKLM_BLOCKLEN;
}

int32_t sirius_dynamic_crypto_fifo_put(void *vreg, void *priv, uint8_t *buf, uint32_t len, uint32_t first_block)
{
	int32_t i = 0, highest = 4;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxTfmCrypto *param = &p->param;

	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	uint32_t inhalf_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_INPUT_HALF);

	if (reg->dctrl.bits.data_src != SIRIUS_SKLM_SRC_SOFT)
		return GXSE_SUCCESS;

	if (inhalf_flag || len < 16) {
		highest = 1;
		reg->data_length = 8;

	} else {
		highest = 3;
		reg->data_length = 16;
	}

#ifdef DEBUG_INPUT
	gx_fw_debug_print(0x1000);
#endif
	for (i = 3; i >= 0; i--) {
		if (i > highest)
			reg->data_in[i] = 0;
		else {
			reg->data_in[i] = gx_u32_get_val(buf+(highest-i)*4, endian_flag);
#ifdef DEBUG_INPUT
			gx_fw_debug_print(reg->data_in[i]);
#endif
		}
	}
#ifdef DEBUG_INPUT
	gx_fw_debug_print(0x1001);
#endif
	if (first_block)
		reg->dctrl.bits.sob = 1;
	reg->dctrl.bits.di_update = 1;

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_fifo_get(void *vreg, void *priv, uint8_t *buf, uint32_t len)
{
	int32_t i = 0;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxTfmCrypto *param = &p->param;

	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);

	if (reg->dctrl.bits.rslt_des != SIRIUS_SKLM_DST_SOFT)
		return GXSE_SUCCESS;

	reg->ctrl.bits.do_update = 1;

#ifdef DEBUG_OUTPUT
	gx_fw_debug_print(0x2000);
#endif
	for (i = 3; i >= 0; i--) {
		if (len >= (i+1)*4) {
			gx_u32_set_val(buf+(3-i)*4, &reg->data_out[i], endian_flag);
#ifdef DEBUG_OUTPUT
			{
				uint32_t *aa = (uint32_t *)buf;
				gx_fw_debug_print(aa[3-i]);
			}
#endif
		}
	}
#ifdef DEBUG_OUTPUT
	gx_fw_debug_print(0x2001);
#endif

	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_clr_key(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	(void) priv;

	reg->dctrl.bits.key_clr = 1;
	reg->dctrl.bits.key_clr = 0;

	return GXSE_SUCCESS;
}

static GxCryptoResOps sirius_dy_crypto_resops = {
	.get_alg = sirius_dynamic_crypto_get_alg,
	.get_key = sirius_dynamic_crypto_get_key,
	.get_src = sirius_dynamic_crypto_get_src,
	.get_dst = sirius_dynamic_crypto_get_dst,
};

int32_t sirius_dynamic_crypto_init(GxSeModuleHwObj *obj)
{
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;

	p->reg = (volatile SiriusCryptoReg *)obj->reg;
	p->reg->crypto_int_en.value = 0x3;
	p->resops = &sirius_dy_crypto_resops;
#ifdef CPU_SCPU
	scpu_crypto_init(obj);
#endif
	return GXSE_SUCCESS;
}

int32_t sirius_dynamic_crypto_scpu_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK | 1 << TFM_KEY_PVRK;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_TDES | 1 << TFM_ALG_SM4;
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->flags = TFM_FLAG_KLM_INPUT_HALF | TFM_FLAG_KLM_CW_HALF | TFM_FLAG_KLM_CW_HIGH_8BYTE | TFM_FLAG_KLM_CW_XOR;

	param->key_sub_num[TFM_KEY_CWUK] = 2;
	param->key_sub_num[TFM_KEY_PVRK] = 2;
	param->dst = 1 << TFM_DST_TS | 1 << TFM_DST_M2M | 1 << TFM_DST_GP;
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_dy_crypto_devops = {
	.init            = sirius_dynamic_crypto_init,
};

static GxCryptoOps sirius_dy_crypto_hwops = {
	.check_param     = sirius_dynamic_crypto_check_param,
//	.capability      = sirius_dynamic_crypto_scpu_capability,

    .get_blocklen    = sirius_dynamic_crypto_get_blocklen,
	.set_key         = sirius_dynamic_crypto_set_key,
	.set_alg         = sirius_dynamic_crypto_set_alg,
    .fifo_put        = sirius_dynamic_crypto_fifo_put,
    .fifo_get        = sirius_dynamic_crypto_fifo_get,
	.set_output_buf  = sirius_dynamic_crypto_set_output_buf,
	.clr_config      = sirius_dynamic_crypto_clr_config,
	.clr_key         = sirius_dynamic_crypto_clr_key,
	.set_input_buf   = sirius_dynamic_crypto_set_input_buf,
	.set_ready       = scpu_crypto_set_ready,
	.wait_finish     = scpu_crypto_wait_finish,
};

static GxSeModuleHwObjTfmOps crypto_tfm_ops = {
	.devops = &sirius_dy_crypto_devops,
	.hwops = &sirius_dy_crypto_hwops,

	.tfm_cipher = {
		.decrypt         = gx_tfm_object_crypto_fifo_decrypt,
		.clr_key         = gx_tfm_object_crypto_clr_key,
	//	.capability      = gx_tfm_object_crypto_capability,
	},
};


static GxSeModuleHwObj sirius_dy_crypto_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_CRYPTO,
	.ops  = &crypto_tfm_ops,
	.priv = &sirius_crypto_dy_priv,
};

GxSeModule sirius_crypto_scpu_dynamic_module = {
	.id   = GXSE_MOD_CRYPTO_DYNAMIC,
	.ops  = &crypto_dev_ops,
	.hwobj= &sirius_dy_crypto_tfm,
	.res  = {
		.reg_base  = GXSCPU_BASE_ADDR_CRYPTO,
		.reg_len   = sizeof(SiriusCryptoReg),
	},
};
