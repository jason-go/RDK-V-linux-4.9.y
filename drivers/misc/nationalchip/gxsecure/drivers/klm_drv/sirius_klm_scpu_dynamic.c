#include "gxklm_virdev.h"
#include "sirius_crypto.h"

//#define DEBUG_KN
//#define DEBUG_CW

struct dy_klm {
	struct crypto_isr_info *isr_info;
	GxTfmKeyLadder klm;
};

static struct dy_klm sirius_dy_klm = {.klm = {0},};

static int32_t sirius_scpu_dklm_check_param(void *vreg, void *priv, GxTfmKeyLadder *param, uint32_t cmd)
{
	struct dy_klm *p = (struct dy_klm *)priv;
	GxTfmKeyLadder *klm = &p->klm;

	switch (cmd) {
	case TFM_KLM_SELECT_ROOTKEY:
		if (param->stage < 2 || param->stage > 8)
			return GXSE_ERR_PARAM;
		memset(klm, 0, sizeof(GxTfmKeyLadder));
		break;

	case TFM_KLM_GET_RESP:
		memset(klm, 0, sizeof(GxTfmKeyLadder));

	case TFM_KLM_SET_CW:
		memcpy(&klm->dst, &param->dst, sizeof(GxTfmDstBuf));
		klm->flags = param->flags;

	case TFM_KLM_SET_KN:
		klm->alg = param->alg;
		memcpy(klm->input, param->input, sizeof(klm->input));
		break;

	default:
		return GXSE_ERR_PARAM;
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_scpu_dklm_set_rootkey(void *vreg, void *priv, GxTfmKeyBuf key_sel)
{
	struct dy_klm *p = (struct dy_klm *)priv;
	GxTfmKeyLadder *klm = &p->klm;
	(void) vreg;

	memcpy(&klm->key, &key_sel, sizeof(GxTfmKeyBuf));

	return GXSE_SUCCESS;
}

static int32_t sirius_scpu_dklm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos)
{
	struct dy_klm *p = (struct dy_klm *)priv;
	GxTfmKeyLadder *klm = &p->klm;
	GxTfmCrypto param = {0};
	(void) vreg;

	param.module = TFM_MOD_CRYPTO;
	param.module_sub = 1;
	param.alg = klm->alg;
	param.opt = TFM_OPT_ECB;
	param.src.id = TFM_SRC_MEM;
	param.input.buf = in;
	param.input.length = in_len;
	if (pos == 0) {
		memcpy(&param.even_key, &klm->key, sizeof(param.even_key));
	} else {
		param.even_key.id = TFM_KEY_REG;
		param.even_key.sub = 0;
	}

#ifdef DEBUG_KN
	{
		int32_t ret = 0;
		param.dst.id = TFM_DST_MEM;
		param.output.buf = (void *)klm->output;
		param.output.length = sizeof(klm->output);
		ret = gx_tfm_decrypt(&param);
		gx_fw_debug_print(0xc100 | (ret == 0 ? 0 : 1));
		gx_fw_debug_print(klm->output[0]);
		gx_fw_debug_print(klm->output[1]);
		gx_fw_debug_print(klm->output[2]);
		gx_fw_debug_print(klm->output[3]);
		gx_fw_debug_print(0xc100);
	}
#endif

	param.dst.id = TFM_DST_REG;
	param.dst.sub = 0;

	return gx_tfm_decrypt(&param);
}

static int32_t sirius_scpu_dklm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	int32_t ret = 0;
	GxTfmCrypto param = {0};
	GxTfmKeyLadder *klm = NULL;
	struct dy_klm *p = (struct dy_klm *)priv;
	(void) vreg;

	klm = &p->klm;
	param.module = TFM_MOD_CRYPTO;
	param.module_sub = 1;
	param.alg = klm->alg;
	param.opt = TFM_OPT_ECB;
	param.src.id = TFM_SRC_MEM;
	param.even_key.id = TFM_KEY_REG;
	param.even_key.sub = 0;
	param.input.buf = in;
	param.input.length = in_len;
	param.flags = klm->flags | TFM_FLAG_KLM_CW_READY;
	memcpy(&param.dst, &klm->dst, sizeof(param.dst));

	ret = gx_tfm_decrypt(&param);

#ifdef DEBUG_CW
	{
		param.dst.id = TFM_DST_MEM;
		param.output.buf = (void *)klm->output;
		param.output.length = sizeof(klm->output);
		param.flags = klm->flags;
		ret = gx_tfm_decrypt(&param);
		gx_fw_debug_print(0xc200 | (ret == 0 ? 0 : 1));
		gx_fw_debug_print(klm->output[0]);
		gx_fw_debug_print(klm->output[1]);
		gx_fw_debug_print(klm->output[2]);
		gx_fw_debug_print(klm->output[3]);
		gx_fw_debug_print(0xc200);
	}
#endif

	return ret;
}

static int32_t sirius_scpu_dklm_get_resp(void *vreg, void *priv, uint8_t *nonce, uint32_t nonce_len, uint8_t *out, uint32_t out_len)
{
	GxTfmCrypto param = {0};
	GxTfmKeyLadder *klm = NULL;
	struct dy_klm *p = (struct dy_klm *)priv;

	klm = &p->klm;
	param.module = TFM_MOD_CRYPTO;
	param.module_sub = 1;
	param.alg = klm->alg;
	param.opt = TFM_OPT_ECB;
	param.src.id = TFM_SRC_REG;
	param.src.sub = 0;
	param.dst.id = TFM_DST_REG;
	param.dst.sub = 0;
	param.even_key.id = TFM_KEY_REG;
	param.even_key.sub = 0;
	param.input.buf = NULL;
	param.input.length = 0;
	if (gx_tfm_decrypt(&param) < 0)
		return GXSE_ERR_GENERIC;

	param.src.id = TFM_SRC_MEM;
   	param.dst.id = TFM_DST_MEM;
	param.input.buf = nonce;
	param.input.length = nonce_len;
	param.output.buf = (void *)klm->output;
	param.output.length = sizeof(klm->output);
	if (gx_tfm_decrypt(&param) < 0)
		return GXSE_ERR_GENERIC;

	memcpy(out, klm->output, out_len);

	return GXSE_SUCCESS;
}

int32_t sirius_scpu_dklm_scpu_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK | 1 << TFM_KEY_PVRK;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_TDES | 1 << TFM_ALG_SM4;
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->flags = TFM_FLAG_KLM_INPUT_HALF | TFM_FLAG_KLM_CW_HALF | TFM_FLAG_KLM_CW_HIGH_8BYTE | TFM_FLAG_KLM_CW_XOR;

	if (CHIP_IS_SIRIUS) {
		param->key_sub_num[TFM_KEY_CWUK] = 2;
		param->key_sub_num[TFM_KEY_PVRK] = 2;
		param->dst = 1 << TFM_DST_TS | 1 << TFM_DST_M2M | 1 << TFM_DST_GP;

	} else {
		param->key_sub_num[TFM_KEY_CWUK] = 4;
		param->key_sub_num[TFM_KEY_PVRK] = 2;
		param->dst = 1 << TFM_DST_TS | 1 << TFM_DST_M2M;
	}
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_scpu_dklm_devops = {};

static GxKLMOps sirius_scpu_dklm_ops = {
	.check_param     = sirius_scpu_dklm_check_param,
//	.capability      = sirius_scpu_dklm_scpu_capability,

	.set_rootkey     = sirius_scpu_dklm_set_rootkey,
	.set_ekn         = sirius_scpu_dklm_set_ekn,
	.set_ecw         = sirius_scpu_dklm_set_ecw,
	.set_resp_rootkey= sirius_scpu_dklm_set_rootkey,
	.set_resp_kn     = sirius_scpu_dklm_set_ekn,
	.get_resp        = sirius_scpu_dklm_get_resp,
};

static GxSeModuleHwObjTfmOps sirius_klm_scpu_tfm_ops = {
	.devops = &sirius_scpu_dklm_devops,
	.hwops = &sirius_scpu_dklm_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.get_resp        = gx_tfm_object_klm_get_resp,
//		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj sirius_scpu_klm_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_SCPU_NC_DYNAMIC,
	.ops  = &sirius_klm_scpu_tfm_ops,
	.priv = &sirius_dy_klm,
};

GxSeModule sirius_klm_scpu_dynamic_module = {
	.id   = GXSE_MOD_KLM_SCPU_GENERIC,
	.ops  = &klm_dev_ops,
	.hwobj= &sirius_scpu_klm_tfm,
};
