#include "gxklm_virdev.h"
#include "gx3211_klm_reg.h"
#include "gx3211_klm.h"
#include "taurus_m2m.h"
#include "gx3211_m2m.h"

int32_t gx3211_klm_get_alg(GxTfmAlg alg)
{
	switch(alg) {
	case TFM_ALG_AES128:
		return GX3211_KLM_ALG_AES128;
	case TFM_ALG_AES192:
		return GX3211_KLM_ALG_AES192;
	case TFM_ALG_AES256:
		return GX3211_KLM_ALG_AES256;
	case TFM_ALG_DES:
		return GX3211_KLM_ALG_DES;
	case TFM_ALG_TDES:
		return GX3211_KLM_ALG_TDES;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t gx3211_klm_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_CWUK:
		param->sub &= 0x1;
		return GX3211_KLM_KEY_CDCAS_AES + param->sub;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t gx3211_klm_get_dst(GxTfmDstBuf *param)
{
	switch (param->id) {
	case TFM_DST_TS:
		return GX3211_KLM_DST_TS;

	case TFM_DST_M2M:
		return GX3211_KLM_DST_M2M;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm dst error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t gx3211_klm_check_param(void *vreg, void *priv, GxTfmKlm *param, uint32_t cmd)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	uint8_t check_key = 0, check_stage = 0, check_alg = 0;
	uint8_t check_in = 0, check_dst = 0;
	GxKLMResOps *ops = p->resops;
	(void) vreg;

	switch (cmd) {
	case TFM_KLM_SELECT_ROOTKEY:
		check_key = 1;
		check_stage = 1;
		break;
	case TFM_KLM_SET_KN:
		check_in = 1;
		break;
	case TFM_KLM_SET_CW:
		check_in = 1;
		check_alg = 1;
		if (param->key.id == TFM_KEY_CWUK)
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

	if (check_key) {
		if (ops->get_key(&param->key) < 0)
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

int32_t gx3211_klm_set_rootkey(void *vreg, void *priv, GxTfmKeyBuf key_sel)
{
	int32_t ret = 0;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxKLMResOps *ops = p->resops;

	if ((ret = ops->get_key(&key_sel)) < 0)
		return ret;

	reg->ctrl1.bits.gx3211_key_sel = ret;

	return GXSE_SUCCESS;
}


int32_t gx3211_klm_set_stage(void *vreg, void *priv, uint32_t stage)
{
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
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
    case 4:
        reg->ca_cw_select.bits.cw_round_4 = 1;
        break;
    case 5:
        reg->ca_cw_select.bits.cw_round_5 = 1;
        break;
    default:
        break;
    }

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxKLMResOps *ops = p->resops;

	if ((ret = ops->get_alg(alg)) < 0)
		return ret;

	if (ret<= GX3211_KLM_ALG_TDES) {
		reg->ctrl1.bits.tri_des = ret;
		reg->ctrl1.bits.work_mode = 0;
	} else if (ret <= GX3211_KLM_ALG_AES256) {
		reg->ctrl1.bits.aes_mode = ret - GX3211_KLM_ALG_AES128;
		reg->ctrl1.bits.work_mode = 1;
	}

	return GXSE_SUCCESS;
}

static int32_t gx3211_klm_set_dst(void *vreg, void *priv, GxTfmDstBuf out_sel)
{
	int32_t ret = 0, sub = 0;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
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
			reg->ca_addr |= (0xa0 + sub*2 + 1);
		}
		if (out_sel.sub >= 16) {
			sub = out_sel.sub - 16;
			reg->ca_addr |= (0xa0 + sub*2);
		}
		if (combine_flag) {
			reg->ca_addr = (0xa0 + sub*2 + 1);
			reg->ca_addr |= (0xa0 + sub*2) << 16;
		}
	}

	if (ret == GX3211_KLM_DST_M2M) {
		reg->ca_addr = 0;
		reg->ca_cw_select.bits.multi_key_en = 1;
	}
	return GXSE_SUCCESS;
}

static int _klm_set_data(void *base, unsigned char *buf, int size, uint32_t endian_flag)
{
    volatile unsigned int *reg = (volatile unsigned int *)base;
	int i = 0;
	for (i = 0; i < size/4; i++)
		*(reg + 6-size/4 + i) = gx_u32_get_val(buf+4*i, endian_flag);
    return 0;
}

static void _klm_set_addr(volatile Gx3211KLMReg *reg, struct klm_priv *p , uint32_t pos, uint8_t *in, uint32_t in_len, uint32_t endian_flag)
{

	unsigned int * addr[5] = {(void *)reg->ca_DSK, (void *)reg->ca_DCK,
		(void *)reg->ca_DCW, (void *)reg->ca_VCW, (void *)reg->ca_MCW};

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
    case 4:
        _klm_set_data(addr[pos], in, in_len, endian_flag);
        break;
    case 5:
        _klm_set_data(addr[pos], in, in_len, endian_flag);
        break;
    default:
        break;
	}
}

int32_t gx3211_klm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	GxTfmKlm *param = &p->param;

	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	_klm_set_addr(reg, p, pos, in, in_len, endian_flag);

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	(void) in_len;

	_klm_set_addr(reg, p, p->stage-1, in, in_len, endian_flag);

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_set_ready(void *vreg, void *priv)
{
	int ret = 0;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;
	GxKLMResOps *ops = p->resops;

	if ((ret = ops->get_alg(param->alg)) < 0)
		return ret;

	reg->ca_mode.bits.enable = 1;
	reg->ctrl1.bits.encrypt = 0x0; //dec
	reg->ctrl1.bits.key_rdy = 0x1; //key_rdy

	if (ret <= GX3211_KLM_ALG_TDES)
		reg->ctrl2.value = 0x1 << 0;
	else if (ret <= GX3211_KLM_ALG_AES256)
		reg->ctrl2.value = 0x1 << 1;

	switch (p->stage) {
	case 0:
		reg->ca_mode.bits.DCW_ready = 1;
		break;
	case 1:
		reg->ca_mode.bits.DCW_ready = 1;
		break;
	case 2:
		reg->ca_mode.bits.DCK_ready = 1;
		reg->ca_mode.bits.DCW_ready = 1;
		break;
	case 3:
		reg->ca_mode.bits.DSK_ready = 1;
		reg->ca_mode.bits.DCK_ready = 1;
		reg->ca_mode.bits.DCW_ready = 1;
		break;
	case 4:
		reg->ca_mode.bits.DSK_ready = 1;
		reg->ca_mode.bits.DCK_ready = 1;
		reg->ca_mode.bits.DCW_ready = 1;
		reg->ca_mode.bits.VCW_ready = 1;
		break;
	case 5:
		reg->ca_mode.bits.DSK_ready = 1;
		reg->ca_mode.bits.DCK_ready = 1;
		reg->ca_mode.bits.DCW_ready = 1;
		reg->ca_mode.bits.VCW_ready = 1;
		reg->ca_mode.bits.MCW_ready = 1;
		break;
	default:
		break;
	}

	reg->ca_cw_select.bits.cw_en = 1;
	p->run_flag = 1;

	gxlog_d(GXSE_LOG_MOD_KLM, "[GEN KLM CTRL1] : %x\n", reg->ctrl1.value);
	gxlog_d(GXSE_LOG_MOD_KLM, "[GEN KLM CTRL2] : %x\n", reg->ctrl2.value);
	gxlog_d(GXSE_LOG_MOD_KLM, "[GEN KLM CA_MODE] : %x\n", reg->ca_mode.value);
	gxlog_d(GXSE_LOG_MOD_KLM, "[GEN KLM CA_CW_SEL] : %x\n", reg->ca_cw_select.value);

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_isr(GxSeModuleHwObj *obj)
{
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)obj->reg;
	struct klm_priv *p = (struct klm_priv *)obj->priv;

    if (reg->isr.bits.data_finish) {
        gxlog_d(GXSE_LOG_MOD_KLM, "data finish\n");
		p->status = KLM_STATUS_DONE;
    }

	if (reg->isr.bits.aes_round_finish) {
		gxlog_d(GXSE_LOG_MOD_KLM, "aes round finish\n");
		p->status = KLM_STATUS_DONE;
	}
	if (reg->isr.bits.tdes_round_finish) {
		gxlog_d(GXSE_LOG_MOD_KLM, "tdes round finish\n");
		p->status = KLM_STATUS_DONE;
	}

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	unsigned long long cur = 0;
	unsigned long long tick = 0;
	int32_t ret = GXSE_SUCCESS;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	if (is_query < TFM_ISR_QUERY_UNTIL)
		tick = gx_osdep_current_tick();
	while (1) {
		GxSeModuleHwObj obj = {0};
		obj.reg = vreg;
		obj.priv = priv;
		gx3211_klm_isr(&obj);

		if (is_query < TFM_ISR_QUERY_UNTIL) {
			cur = gx_osdep_current_tick();
			if (gx_time_after_eq(cur, tick + 15)) {
				ret = GXSE_ERR_GENERIC;
				break;
			}
		}

		if (p->status != KLM_STATUS_DONE)
			continue;

		gx_osdep_cache_sync(param->output.buf, param->output.length, DMA_FROM_DEVICE);
		p->status = KLM_STATUS_IDLE;
		break;
	}
	return ret;
}

int32_t gx3211_klm_clr_config(void *vreg, void *priv)
{
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)vreg;
	(void) vreg;
	(void) priv;

	reg->ctrl2.value = 0;
	reg->ctrl1.value = 0;
	reg->ca_mode.value = 0;
	reg->ca_cw_select.value = 0;
	reg->isr.value = 0xff;

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_init(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;
	volatile Gx3211KLMReg *reg = (volatile Gx3211KLMReg *)obj->reg;

	reg->isr_en.value = 0x0;
	gx_mutex_init(p->mutex);
	obj->mutex = p->mutex;

	return GXSE_SUCCESS;
}

int32_t gx3211_klm_deinit(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;

	gx_mutex_destroy(p->mutex);
	obj->mutex = NULL;

	return GXSE_SUCCESS;
}


static int32_t gx3211_klm_capability(void *vreg, void *priv, GxTfmCap *param)
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

GxKLMResOps gx3211_klm_resops = {
	.get_alg = gx3211_klm_get_alg,
	.get_key = gx3211_klm_get_key,
	.get_dst = gx3211_klm_get_dst,
};

static struct klm_priv gx3211_klm_priv = {
	.param = {0},
	.mutex = &gx3211_mtc_mutex,
	.resops = &gx3211_klm_resops,
};

static GxKLMOps gx3211_klm_ops = {
	.check_param     = gx3211_klm_check_param,
	.capability      = gx3211_klm_capability,

	.set_rootkey     = gx3211_klm_set_rootkey,
	.set_stage       = gx3211_klm_set_stage,
	.set_alg         = gx3211_klm_set_alg,
	.set_dst         = gx3211_klm_set_dst,
	.set_ekn         = gx3211_klm_set_ekn,
	.set_ecw         = gx3211_klm_set_ecw,
	.set_ready       = gx3211_klm_set_ready,
	.wait_finish     = gx3211_klm_wait_finish,
	.clr_config      = gx3211_klm_clr_config,
};

GxSeModuleDevOps gx3211_klm_devops = {
	.init            = gx3211_klm_init,
	.deinit          = gx3211_klm_deinit,
	.isr             = gx3211_klm_isr,
};

static GxSeModuleHwObjTfmOps gx3211_klm_tfm_ops = {
	.devops = &gx3211_klm_devops,
	.hwops = &gx3211_klm_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj gx3211_klm_generic_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_GENERAL,
	.ops  = &gx3211_klm_tfm_ops,
	.priv = &gx3211_klm_priv,
};

GxSeModule gx3211_klm_generic_module = {
	.id   = GXSE_MOD_KLM_GENERIC,
	.ops  = &klm_dev_ops,
	.hwobj= &gx3211_klm_generic_tfm,
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"klm"},
	},
};
