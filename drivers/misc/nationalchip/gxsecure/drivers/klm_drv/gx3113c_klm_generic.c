#include "gxklm_virdev.h"
#include "gx3113c_klm_reg.h"
#include "gx3211_klm.h"
#include "taurus_m2m.h"

static int _klm_set_data(void *base, unsigned char *buf, int size, uint32_t endian_flag)
{
	volatile unsigned int *reg = (volatile unsigned int *)base;
	int i = 0;
	for (i = 0; i < size/4; i++)
		*(reg + 6-size/4 + i) = gx_u32_get_val(buf+4*i, endian_flag);
	return 0;
}

static void _klm_set_addr(volatile Gx3113CKLMReg *reg, struct klm_priv *p , uint32_t pos, uint8_t *in, uint32_t in_len, uint32_t endian_flag)
{
	unsigned int * addr[3] = {(void *)reg->ca_DSK, (void *)reg->ca_DCK, (void *)reg->ca_DCW};

	switch (p->stage) {
		break;
	case 1:
		_klm_set_data(addr[pos+2], in, in_len, endian_flag);
		break;
	case 2:
		_klm_set_data(addr[pos+1], in, in_len, endian_flag);
		break;
	case 3:
		_klm_set_data(addr[pos], in, in_len, endian_flag);
		break;
	default:
		break;
	}
}

static int32_t gx3113c_klm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	volatile Gx3113CKLMReg *reg = (volatile Gx3113CKLMReg *)vreg;
	GxTfmKlm *param = &p->param;

	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	_klm_set_addr(reg, p, pos, in, in_len, endian_flag);

	return GXSE_SUCCESS;
}

static int32_t gx3113c_klm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	volatile Gx3113CKLMReg *reg = (volatile Gx3113CKLMReg *)vreg;
	(void) in_len;

	_klm_set_addr(reg, p, p->stage-1, in, in_len, endian_flag);

	return GXSE_SUCCESS;
}

static int32_t gx3113c_klm_set_stage(void *vreg, void *priv, uint32_t stage)
{
	volatile Gx3113CKLMReg *reg = (volatile Gx3113CKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;

	p->stage = stage;
	switch (stage) {
	case 0:
		reg->ca_cw_select.bits.cw_round_0 = 1;
		reg->ca_cw_select.bits.cw_round_0_valid = 1;
		break;
	case 1:
		reg->ca_cw_select.bits.cw_round_1 = 1;
		break;
	case 2:
		reg->ca_cw_select.bits.cw_round_2 = 1;
		break;
	case 3:
		reg->ca_cw_select.bits.cw_round_3 = 1;
		break;
	default:
		break;
	}

	return GXSE_SUCCESS;
}

static int32_t gx3113c_klm_set_dst(void *vreg, void *priv, GxTfmDstBuf out_sel)
{
	int32_t ret = 0, sub = 0;
	volatile Gx3113CKLMReg *reg = (volatile Gx3113CKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	uint32_t combine_flag = TFM_FLAG_GET(p->param.flags, TFM_FLAG_KLM_COMBINE_CW);
	GxKLMResOps *ops = p->resops;

	if ((ret = ops->get_dst(&out_sel)) < 0)
		return ret;

	if (out_sel.sub >= 32)
		return GXSE_ERR_PARAM;

	if (ret == GX3211_KLM_DST_TS) {
		reg->ca_addr = 0;
		if (out_sel.sub < 16) {
			sub = out_sel.sub;
			reg->ca_addr |= (0xa0 + sub*2 + 2);
		}
		if (out_sel.sub >= 16) {
			sub = out_sel.sub - 16;
			reg->ca_addr |= (0xa0 + sub*2);
		}
		if (combine_flag) {
			reg->ca_addr = (0xa0 + sub*2 + 2);
			reg->ca_addr |= (0xa0 + sub*2) << 16;
		}
	}

	if (ret == GX3211_KLM_DST_M2M) {
		reg->ca_addr = 0;
		reg->ca_cw_select.bits.multi_key_en = 1;
	}
	return GXSE_SUCCESS;
}

static int32_t gx3113c_klm_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_TDES;
	param->dst = 1 << TFM_DST_TS;
	param->key_sub_num[TFM_KEY_CWUK] = 2;

	return GXSE_SUCCESS;
}

static struct klm_priv gx3113c_klm_priv = {
	.param = {0},
	.mutex = &gx3211_mtc_mutex,
	.resops = &gx3211_klm_resops,
};

static GxKLMOps gx3113c_klm_ops = {
	.check_param     = gx3211_klm_check_param,
	.capability      = gx3113c_klm_capability,

	.set_rootkey     = gx3211_klm_set_rootkey,
	.set_stage       = gx3113c_klm_set_stage,
	.set_alg         = gx3211_klm_set_alg,
	.set_dst         = gx3113c_klm_set_dst,
	.set_ekn         = gx3113c_klm_set_ekn,
	.set_ecw         = gx3113c_klm_set_ecw,
	.set_ready       = gx3211_klm_set_ready,
	.wait_finish     = gx3211_klm_wait_finish,
	.clr_config      = gx3211_klm_clr_config,
};

static GxSeModuleHwObjTfmOps klm_tfm_ops = {
	.devops = &gx3211_klm_devops,
	.hwops = &gx3113c_klm_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj gx3113c_klm_generic_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_GENERAL,
	.ops  = &klm_tfm_ops,
	.priv = &gx3113c_klm_priv,
};

GxSeModule gx3113c_klm_generic_module = {
	.id   = GXSE_MOD_KLM_GENERIC,
	.ops  = &klm_dev_ops,
	.hwobj= &gx3113c_klm_generic_tfm,
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"klm"},
	},
};
