#include "gxklm_virdev.h"
#include "gxse_kapi_secure.h"

struct klm_priv {
	GxTfmKlm param;
	gx_mutex_t mutex;
};

static int32_t sirius_secure_klm_check_param(void *vreg, void *priv, GxTfmKlm *param, uint32_t cmd)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	uint8_t check_key = 0, check_stage = 0, check_alg = 0;
	uint8_t check_in = 0, cw_half = 0, check_src = 0, check_dst = 0;
	(void) vreg;

	switch (cmd) {
	case TFM_KLM_SELECT_ROOTKEY:
		check_key = 1;
		check_stage = 1;
		break;
	case TFM_KLM_SET_KN:
		check_in = 1;
		check_alg = 1;
		check_src = 1;
		break;
	case TFM_KLM_SET_CW:
		check_in = 1;
		check_alg = 1;
		check_src = 1;
		check_dst = 1;
		break;
	case TFM_KLM_GET_RESP:
		check_key = 1;
		check_in = 1;
		check_alg = 1;
		check_src = 2;
		check_dst = 1;
		break;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "Not support the cmd.\n");
		return GXSE_ERR_PARAM;
	}

	if (check_alg) {
		if (param->alg != TFM_ALG_SM4 && param->alg != TFM_ALG_AES128 && param->alg != TFM_ALG_TDES)
			return GXSE_ERR_PARAM;
	}

	if (check_src) {
		if (param->src.id < TFM_SRC_MEM || param->src.id > TFM_SRC_REG)
			return GXSE_ERR_PARAM;
		if (check_src == 2) {
			if (param->src.id != TFM_SRC_MEM)
				return GXSE_ERR_PARAM;
		}
	}

	if (check_dst) {
		if (cmd == TFM_KLM_GET_RESP) {
			if (param->dst.id != TFM_DST_MEM)
				return GXSE_ERR_PARAM;
		} else {
			if (param->dst.id >= TFM_DST_MEM)
				return GXSE_ERR_PARAM;
		}
	}

	if (check_key) {
		if (param->key.id != TFM_KEY_CWUK && param->key.id != TFM_KEY_PVRK)
			return GXSE_ERR_PARAM;
	}

	if (check_in) {
		if (cmd == TFM_KLM_GET_RESP) {
			if (param->alg == TFM_ALG_SM4 || param->alg == TFM_ALG_AES128) {
				if (param->output.length != 16 || param->input.length != 16)
					return GXSE_ERR_PARAM;
			} else if (param->alg == TFM_ALG_TDES) {
				if (param->input.length > param->output.length)
					return GXSE_ERR_PARAM;

				if (param->input.length != 16 && param->input.length != 8)
					return GXSE_ERR_PARAM;

				if (param->output.length != 16 && param->output.length != 8)
					return GXSE_ERR_PARAM;
			} else
				return GXSE_ERR_PARAM;

		} else {
			if (param->alg == TFM_ALG_SM4 || param->alg == TFM_ALG_AES128) {
				if (param->input.length < 16) {
					gxlog_e(GXSE_LOG_MOD_KLM, "buf length error\n");
					return GXSE_ERR_PARAM;
				}
			}
			if (NULL == param->input.buf || param->input.length < 8) {
				gxlog_e(GXSE_LOG_MOD_KLM, "buf error\n");
				return GXSE_ERR_PARAM;
			}
			if (cmd == TFM_KLM_SET_CW && param->input.length < 16)
				cw_half = 1;
		}
	}

	if (check_stage) {
		if (param->stage < 2 || param->stage > 8) {
			gxlog_e(GXSE_LOG_MOD_KLM, "stage error\n");
			return GXSE_ERR_PARAM;
		}
	}

	memcpy(&p->param, param, sizeof(GxTfmKlm));
	if (cw_half)
		p->param.flags |= TFM_FLAG_KLM_INPUT_HALF | TFM_FLAG_KLM_CW_HALF;

	return GXSE_SUCCESS;
}

static int32_t sirius_secure_klm_set_rootkey(void *vreg, void *priv, GxTfmKeyBuf key_sel)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	(void) vreg;
	(void) key_sel;

	return gxse_secure_send_root_key(&p->param);
}

static int32_t sirius_secure_klm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	(void) vreg;
	(void) in;
	(void) in_len;
	(void) pos;

	return gxse_secure_send_kn(&p->param, 0);
}

static int32_t sirius_secure_klm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	(void) vreg;
	(void) in;
	(void) in_len;

	return gxse_secure_send_kn(&p->param, 1);
}

static int32_t sirius_secure_klm_get_resp(void *vreg, void *priv, uint8_t *nonce, uint32_t nonce_len, uint8_t *out, uint32_t out_len)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	(void) vreg;
	(void) out;
	(void) out_len;
	(void) nonce;
	(void) nonce_len;

	return gxse_secure_get_resp(&p->param);
}

static int32_t sirius_secure_klm_init(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;
	memset(p, 0, sizeof(struct klm_priv));

	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;

	return GXSE_SUCCESS;
}

static int32_t sirius_secure_klm_deinit(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;

	gx_mutex_destroy(&p->mutex);

	return GXSE_SUCCESS;
}

static int32_t sirius_secure_klm_capability(void *vreg, void *priv, GxTfmCap *param)
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

static struct klm_priv sirius_klm_secure_priv = {.param = {0},};

static GxSeModuleDevOps sirius_secure_klm_devops = {
	.init            = sirius_secure_klm_init,
	.deinit          = sirius_secure_klm_deinit,
};

static GxKLMOps sirius_secure_klm_ops = {
	.check_param     = sirius_secure_klm_check_param,
	.capability      = sirius_secure_klm_capability,

	.set_rootkey     = sirius_secure_klm_set_rootkey,
	.set_ekn         = sirius_secure_klm_set_ekn,
	.set_ecw         = sirius_secure_klm_set_ecw,
	.get_resp        = sirius_secure_klm_get_resp,
};

static GxSeModuleHwObjTfmOps sirius_klm_secure_tfm_ops = {
	.devops = &sirius_secure_klm_devops,
	.hwops = &sirius_secure_klm_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.get_resp        = gx_tfm_object_klm_get_resp,
		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj sirius_secure_klm_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_SCPU_NC_DYNAMIC,
	.ops  = &sirius_klm_secure_tfm_ops,
	.priv = &sirius_klm_secure_priv,
};

GxSeModule sirius_secure_klm_module = {
	.id   = GXSE_MOD_KLM_SCPU_GENERIC,
	.ops  = &klm_dev_ops,
	.hwobj= &sirius_secure_klm_tfm,
	.res  = {
		.irqs      = {-1},
	},
};
