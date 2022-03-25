#include "gxklm_virdev.h"
#include "gx3211_klm_reg.h"
#include "gx3211_klm.h"
#include "taurus_m2m.h"
#include "gx6605s_m2m.h"

static int32_t gx6605s_klm_get_alg(GxTfmAlg alg)
{
	switch(alg) {
	case TFM_ALG_TDES:
		return GX3211_KLM_ALG_TDES;
	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t gx6605s_klm_check_param(void *vreg, void *priv, GxTfmKlm *param, uint32_t cmd)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	uint8_t check_stage = 0, check_alg = 0;
	uint8_t check_in = 0, check_dst = 0;
	GxKLMResOps *ops = p->resops;
	(void) vreg;

	switch (cmd) {
	case TFM_KLM_SELECT_ROOTKEY:
		check_stage = 1;
		break;
	case TFM_KLM_SET_KN:
		check_in = 1;
		break;
	case TFM_KLM_SET_CW:
		check_in = 1;
		check_alg = 1;
		check_dst = 1;
		break;
	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "Not support the cmd.\n");
		return GXSE_ERR_PARAM;
	}

	if (check_alg) {
		if (ops->get_alg(param->alg) < 0)
			return GXSE_ERR_PARAM;
	}
	if (check_dst) {
		if (ops->get_dst(&param->dst) < 0)
			return GXSE_ERR_PARAM;
	}

	if (check_in) {
		if (NULL == param->input.buf || param->input.length < 16) {
			gxlog_e(GXSE_LOG_MOD_KLM, "buf error\n");
			return GXSE_ERR_PARAM;
		}
	}

	if (check_stage) {
		if (param->stage > 5) {
			gxlog_e(GXSE_LOG_MOD_KLM, "stage error\n");
			return GXSE_ERR_PARAM;
		}
	}

	memcpy(&p->param, param, sizeof(GxTfmKlm));

	return GXSE_SUCCESS;
}

static int32_t gx6605s_klm_set_dst(void *vreg, void *priv, GxTfmDstBuf out_sel)
{
	int32_t ret = 0, sub = 0;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	uint32_t combine_flag = TFM_FLAG_GET(p->param.flags, TFM_FLAG_KLM_COMBINE_CW);
	GxKLMResOps *ops = p->resops;

	if ((ret = ops->get_dst(&out_sel)) < 0)
		return ret;

	if (out_sel.sub >= 32)
		return -2;

	if (ret == GX3211_KLM_DST_TS) {
		reg->ca_addr = 0;
		if (out_sel.sub < 16) {
			sub = out_sel.sub;
			reg->ca_addr |= (0x29 + sub*4 + 2);
		}
		if (out_sel.sub >= 16) {
			sub = out_sel.sub - 16;
			reg->ca_addr |= (0x29 + sub * 4);
		}
		if (combine_flag) {
			reg->ca_addr = (0x29 + sub*4 + 2);
			reg->ca_addr |= (0x29 + sub * 4) << 16;
		}
	}

	if (ret == GX3211_KLM_DST_M2M) {
		reg->ca_addr = 0;
		reg->ca_cw_select.bits.multi_key_en = 1;
	}
	return GXSE_SUCCESS;
}

static int32_t gx6605s_klm_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK;
	param->alg = TFM_ALG_TDES;
	param->dst = 1 << TFM_DST_TS;
	return GXSE_SUCCESS;
}

static GxKLMResOps gx6605s_klm_resops = {
	.get_alg = gx6605s_klm_get_alg,
	.get_key = gx3211_klm_get_key,
	.get_dst = gx3211_klm_get_dst,
};

static struct klm_priv gx6605s_klm_priv = {
	.param = {0},
	.mutex = &gx3211_mtc_mutex,
	.resops = &gx6605s_klm_resops,
};

static GxKLMOps gx6605s_klm_ops = {
	.check_param     = gx6605s_klm_check_param,
	.capability      = gx6605s_klm_capability,

	.set_rootkey     = gx3211_klm_set_rootkey,
	.set_stage       = gx3211_klm_set_stage,
	.set_alg         = gx3211_klm_set_alg,
	.set_dst         = gx6605s_klm_set_dst,
	.set_ekn         = gx3211_klm_set_ekn,
	.set_ecw         = gx3211_klm_set_ecw,
	.set_ready       = gx3211_klm_set_ready,
	.wait_finish     = gx3211_klm_wait_finish,
	.clr_config      = gx3211_klm_clr_config,
};

static GxSeModuleDevOps gx6605s_klm_devops = {
	.init            = gx3211_klm_init,
	.deinit          = gx3211_klm_deinit,
	.isr             = gx3211_klm_isr,
};

static GxSeModuleHwObjTfmOps gx6605s_klm_tfm_ops = {
	.devops = &gx6605s_klm_devops,
	.hwops = &gx6605s_klm_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj gx6605s_klm_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_GENERAL,
	.ops  = &gx6605s_klm_tfm_ops,
	.priv = &gx6605s_klm_priv,
};

GxSeModule gx6605s_klm_generic_module = {
	.id   = GXSE_MOD_KLM_GENERIC,
	.ops  = &klm_dev_ops,
	.hwobj= &gx6605s_klm_tfm,
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"klm"},
	},
};
