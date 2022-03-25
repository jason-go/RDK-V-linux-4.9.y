#include "gxcrypto_virdev.h"
#include "taurus_m2m_reg.h"
#include "taurus_m2m.h"
#include "gx3211_m2m.h"
#include "gx6605s_m2m.h"

int32_t gx6605s_m2m_get_alg(GxTfmAlg alg)
{
	switch (alg) {

	case TFM_ALG_DES:
		return TAURUS_M2M_ALG_DES;

	case TFM_ALG_TDES:
		return TAURUS_M2M_ALG_TDES3K;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t gx6605s_m2m_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_OTP_SOFT:
		return GX6605S_M2M_KEY_OTP_SOFT;

	case TFM_KEY_ACPU_SOFT:
		return TAURUS_M2M_KEY_ACPU_SOFT;

	case TFM_KEY_MULTI:
		return TAURUS_M2M_KEY_MULTI;
	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t gx6605s_m2m_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	int32_t key = 0, alg = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	(void) vreg;
	(void) cmd;

	if ((alg = ops->get_alg(param->alg)) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_opt(param->opt) < 0)
		return GXSE_ERR_PARAM;

	if ((key = ops->get_key(&param->even_key)) < 0)
		return GXSE_ERR_PARAM;

	if (param->src.id < TFM_SRC_MEM || param->src.id > TFM_SRC_REG)
		return GXSE_ERR_PARAM;

	if (param->dst.id < TFM_DST_TS || param->dst.id > TFM_DST_REG)
		return GXSE_ERR_PARAM;

	if (NULL == param->input.buf || param->input.length == 0 ||
		NULL == param->output.buf || param->output.length < param->input.length) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m buf error\n");
		return GXSE_ERR_PARAM;
	}

	memcpy(&p->param, param, sizeof(GxTfmCrypto));
	return GXSE_SUCCESS;
}

static int32_t gx6605s_m2m_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	volatile TaurusM2MReg *reg = NULL;
	int32_t alg = 0, evenkey = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;
	GxCryptoResOps *ops = p->resops;
	uint32_t key_big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	(void) oddkey_sel;
	(void) key_len;

	if ((reg = (volatile TaurusM2MReg *)vreg) == NULL) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "reg addr is NULL\n");
		return GXSE_ERR_GENERIC;
	}

	if ((alg = ops->get_alg(param->alg)) < 0)
		return alg;

	if ((evenkey = ops->get_key(&key_sel)) < 0)
		return GXSE_ERR_GENERIC;

	reg->ctrl1.bits.gx3211_key_sel = evenkey;

	if (evenkey == TAURUS_M2M_KEY_ACPU_SOFT)
		taurus_set_soft_key(vreg, alg, key, key_big_endian);

	return GXSE_SUCCESS;
}

static int32_t gx6605s_m2m_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) priv;
	(void) vreg;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));

	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_ACPU_SOFT |
				 1 << TFM_KEY_OTP_SOFT;
	param->alg = 1 << TFM_ALG_TDES |
				 1 << TFM_ALG_DES;
	param->opt = 1 << TFM_OPT_ECB |
				 1 << TFM_OPT_CBC |
				 1 << TFM_OPT_CFB |
				 1 << TFM_OPT_OFB |
				 1 << TFM_OPT_CTR;
	param->flags = TFM_FLAG_BIG_ENDIAN |
				   TFM_FLAG_CRYPT_DES_IV_SHIFT |
				   TFM_FLAG_CRYPT_SHORT_MSG_IV1 |
				   TFM_FLAG_CRYPT_SHORT_MSG_IV2;
	return 0;
}

static GxCryptoResOps gx6605s_mtc_resops = {
	.get_opt = taurus_m2m_get_opt,
	.get_alg = gx6605s_m2m_get_alg,
	.get_key = gx6605s_m2m_get_key,
};

static struct mtc_priv gx6605s_mtc_priv = {
	.mutex = &gx3211_mtc_mutex,
	.resops = &gx6605s_mtc_resops,
};

GxCryptoOps gx6605s_m2m_ops = {
	.check_param      = gx6605s_m2m_check_param,
	.get_align_param  = taurus_m2m_get_align_param,
	.capability       = gx6605s_m2m_capability,

	.set_endian       = taurus_m2m_set_endian,
	.set_work_mode    = taurus_m2m_set_work_mode,
	.set_alg          = taurus_m2m_set_alg,
	.set_key          = gx6605s_m2m_set_key,
	.set_opt          = taurus_m2m_set_opt,
	.set_input_buf    = taurus_m2m_set_input_buf,
	.set_output_buf   = taurus_m2m_set_output_buf,
	.set_residue_mode = taurus_m2m_set_residue_mode,
	.set_shortmsg_mode= taurus_m2m_set_shortmsg_mode,
	.wait_finish      = taurus_m2m_wait_finish,
	.clr_config       = taurus_m2m_clr_config,
	.set_ready        = taurus_m2m_set_ready,
	.set_isr_en       = taurus_m2m_set_isr_en,
};

static GxSeModuleHwObjTfmOps gx6605s_crypto_dma_tfm_ops = {
	.hwops = &gx6605s_m2m_ops,
	.devops = &taurus_m2m_devops,

	.tfm_cipher = {
		.set_pvr_key   = gx_tfm_object_crypto_set_pvr_key,
		.clr_key       = gx_tfm_object_crypto_clr_key,
		.enable_key    = gx_tfm_object_crypto_enable_key,
		.disable_key   = gx_tfm_object_crypto_disable_key,
		.get_align_buf = gx_tfm_object_crypto_get_align_buf,
		.encrypt       = gx_tfm_object_crypto_dma_encrypt,
		.decrypt       = gx_tfm_object_crypto_dma_decrypt,
		.capability    = gx_tfm_object_crypto_capability,
	},
};

static GxSeModuleHwObj gx6605s_m2m_dma_obj = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub = TFM_MOD_M2M,
	.ops = &gx6605s_crypto_dma_tfm_ops,
	.priv = &gx6605s_mtc_priv,
};

GxSeModule gx6605s_m2m_module = {
	.id = GXSE_MOD_CRYPTO_DMA0,
	.ops  = &crypto_dev_ops,
	.hwobj = &gx6605s_m2m_dma_obj,
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"m2m"},
	},
};
