#include "gxklm_virdev.h"
#include "sirius_klm_reg.h"
#include "sirius_klm.h"

struct klm_priv {
	GxTfmKlm param;
	unsigned int total_run_time;
	unsigned int stage;

	volatile uint32_t klm_done;

	gx_event_t queue;
	gx_spin_lock_t spin_lock;
	gx_mutex_t *mutex;
};

static struct klm_priv sirius_klm_generic_priv = {
	.param = {0},
	.klm_done = 0,
	.mutex = &sirius_klm_mutex,
};

static int _get_alg(GxTfmAlg alg)
{
	switch(alg) {
	case TFM_ALG_AES128:
		return SIRIUS_KLM_ALG_AES;

	case TFM_ALG_TDES:
		return SIRIUS_KLM_ALG_TDES;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int _get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_CWUK:
		param->sub &= 0x1;
		return SIRIUS_KLM_KEY_DSK1 + param->sub;

	case TFM_KEY_PVRK:
		param->sub &= 0x1;
		return SIRIUS_KLM_KEY_RK1 + param->sub;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int _get_dst(GxTfmDstBuf *param)
{
	switch (param->id) {
	case TFM_DST_TS:
		return SIRIUS_KLM_DST_TS;

	case TFM_DST_GP:
		return SIRIUS_KLM_DST_GP;

	default:
		gxlog_e(GXSE_LOG_MOD_KLM, "klm dst error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t sirius_generic_klm_check_param(void *vreg, void *priv, GxTfmKlm *param, uint32_t cmd)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	uint8_t check_key = 0, check_stage = 0, check_alg = 0;
	uint8_t check_in = 0, check_dst = 0;
	unsigned long spin_flag = 0;
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
		if (_get_alg(param->alg) < 0)
			return GXSE_ERR_PARAM;
	}
	if (check_dst) {
		if (_get_dst(&param->dst) < 0)
			return GXSE_ERR_PARAM;
	}

	if (check_key) {
		if (_get_key(&param->key) < 0)
			return GXSE_ERR_PARAM;
	}

	if (check_in) {
		if (NULL == param->input.buf || param->input.length < 8) {
			gxlog_e(GXSE_LOG_MOD_KLM, "buf error\n");
			return GXSE_ERR_PARAM;
		}
	}

	if (check_stage) {
		if (param->stage < 2 || param->stage > 8) {
			gxlog_e(GXSE_LOG_MOD_KLM, "stage error\n");
			return GXSE_ERR_PARAM;
		}
	}

	memcpy(&p->param, param, sizeof(GxTfmKlm));
	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->klm_done = 0;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_rootkey(void *vreg, void *priv, GxTfmKeyBuf key_sel)
{
	int32_t ret = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) priv;

	if ((ret = _get_key(&key_sel)) < 0)
		return ret;

	reg->gklm_ctrl.bits.mode = ret;

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_stage(void *vreg, void *priv, uint32_t stage)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	p->stage = stage;
	if (param->key.id == TFM_KEY_CWUK)
		reg->gklm_ctrl.bits.cw_stage = SIRIUS_KLM_STAGE(stage);
	else
		reg->gklm_ctrl.bits.pvr_stage = SIRIUS_KLM_STAGE(stage);
	reg->gklm_stage_sel.value = (stage == SIRIUS_KLM_STAGE(5)) ? 0xf : 0x0;

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;

	(void) priv;
	if ((ret = _get_alg(alg)) < 0)
		return ret;

	reg->gklm_ctrl.bits.alg = ret;

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_dst(void *vreg, void *priv, GxTfmDstBuf out_sel)
{
	int32_t ret = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	uint32_t cwhalf_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_HALF);
	uint32_t cwhigh_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_HIGH_8BYTE);

	if (param->key.id == TFM_KEY_CWUK) {
		if ((ret = _get_dst(&out_sel)) < 0)
			return ret;
		reg->gklm_ctrl.bits.dst = ret;
	}

	if (p->total_run_time == 0) {
		reg->gklm_ctrl.bits.dst_cw_half = cwhalf_flag;
		reg->gklm_ctrl.bits.aes_cw_high = cwhigh_flag;
		if (param->alg == TFM_ALG_TDES || param->alg == TFM_ALG_T3DES)
			p->total_run_time = 1 - (cwhalf_flag&0x1);
	} else {
		if (cwhalf_flag && param->alg%2 == SIRIUS_KLM_ALG_TDES) {
			reg->gklm_ctrl.bits.tdes_klm_half_clear = 1;
			reg->gklm_ctrl.bits.tdes_klm_half_clear = 0;
			reg->gklm_ctrl.bits.dst_cw_half = cwhalf_flag;
			reg->gklm_ctrl.bits.aes_cw_high = cwhigh_flag;
		}
		p->total_run_time--;
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	uint32_t i = 0;
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) in_len;

	for (i = 0; i < 4; i++) {
		reg->gklm_EK[pos][3-i] = gx_u32_get_val(in+i*4, endian_flag);
		gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->gklm_EK[pos][3-i]);
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	int32_t i = 0;
	uint32_t highest = 4;
	uint32_t inhalf_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_INPUT_HALF);
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;

	highest = (inhalf_flag || in_len < 16) ? 1 : 3;

	gxlog_d(GXSE_LOG_MOD_KLM, "%x %x %x\n", highest, inhalf_flag, in_len);
	for (i = 3; i >= 0; i--) {
		if (i > highest) {
			reg->gklm_EK[p->stage-1][i] = 0;
			gxlog_d(GXSE_LOG_MOD_KLM, "0\n");
		} else {
			reg->gklm_EK[p->stage-1][i] = gx_u32_get_val(in+(highest-i)*4, endian_flag);
			gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->gklm_EK[p->stage-1][i]);
		}
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_set_ready(void *vreg, void *priv)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) priv;

	reg->gklm_start = 1;

	gxlog_d(GXSE_LOG_MOD_KLM, "[GEN KLM CTRL] : %x\n", reg->gklm_ctrl.value);

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	(void) vreg;
	(void) is_query;

	CHECK_ISR_TIMEOUT(GXSE_LOG_MOD_KLM, p->klm_done, TFM_MASK_DONE, 50, p->queue);

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_init(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)obj->reg;
	memset(p, 0, sizeof(struct klm_priv)-4);

	reg->klm_int_en.value |= 0x1f00;
	gx_spin_lock_init(&p->spin_lock);
	gx_event_init(&p->queue);
	gx_mutex_init(p->mutex);
	obj->mutex = p->mutex;

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_deinit(GxSeModuleHwObj *obj)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)obj->reg;
	struct klm_priv *p = (struct klm_priv *)obj->priv;

	reg->klm_int_en.value &= ~(0x1f00);
	gx_mutex_destroy(p->mutex);
	obj->mutex = NULL;

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_dsr(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;
	if (p->klm_done)
		gx_wake_event(&p->queue, p->klm_done);

	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_isr(GxSeModuleHwObj *obj)
{
	unsigned long spin_flag;
	int gx_finish = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)obj->reg;
	struct klm_priv *p = (struct klm_priv *)obj->priv;

    if (reg->klm_int.bits.gklm_rcv_key_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "gklm_rcv_key_err!\n");

    if (reg->klm_int.bits.gklm_tdes_key_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "gklm_tdes_key_err!\n");

    if (reg->klm_int.bits.gklm_half_finish) {
		gx_finish = 1;
		gxlog_d(GXSE_LOG_MOD_KLM, "gklm_half_finish!\n");
	}

    if (reg->klm_int.bits.gklm_tdes_finish) {
		gx_finish = 1;
		gxlog_d(GXSE_LOG_MOD_KLM, "gklm_tdes_finish!\n");
	}

    if (reg->klm_int.bits.gklm_finish) {
		gx_finish = 1;
		gxlog_d(GXSE_LOG_MOD_KLM, "gklm_finish!\n");
	}

	if (gx_finish) {
		reg->gklm_start = 0;
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->klm_done = 1;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}
	reg->klm_int.value = 0x1f00;

#ifndef ECOS_OS
	sirius_generic_klm_dsr(obj);
#endif
	return GXSE_SUCCESS;
}

static int32_t sirius_generic_klm_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK | 1 << TFM_KEY_PVRK;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_TDES;
	param->dst = 1 << TFM_DST_TS | 1 << TFM_DST_M2M | 1 << TFM_DST_GP;
	param->key_sub_num[TFM_KEY_CWUK] = 2;
	param->key_sub_num[TFM_KEY_PVRK] = 2;
	param->flags = TFM_FLAG_KLM_INPUT_HALF | TFM_FLAG_KLM_CW_HALF | TFM_FLAG_KLM_CW_HIGH_8BYTE;

	return GXSE_SUCCESS;
}


static GxSeModuleDevOps sirius_klm_generic_devops = {
	.init            = sirius_generic_klm_init,
	.deinit          = sirius_generic_klm_deinit,
	.isr             = sirius_generic_klm_isr,
	.dsr             = sirius_generic_klm_dsr,
};

static GxKLMOps sirius_klm_generic_ops = {
	.check_param     = sirius_generic_klm_check_param,
	.capability      = sirius_generic_klm_capability,

	.set_rootkey     = sirius_generic_klm_set_rootkey,
	.set_stage       = sirius_generic_klm_set_stage,
	.set_alg         = sirius_generic_klm_set_alg,
	.set_dst         = sirius_generic_klm_set_dst,
	.set_ekn         = sirius_generic_klm_set_ekn,
	.set_ecw         = sirius_generic_klm_set_ecw,
	.set_ready       = sirius_generic_klm_set_ready,
	.wait_finish     = sirius_generic_klm_wait_finish,
};

static GxSeModuleHwObjTfmOps sirius_klm_generic_tfm_ops = {
	.devops = &sirius_klm_generic_devops,
	.hwops = &sirius_klm_generic_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj sirius_klm_generic_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_GENERAL,
	.ops  = &sirius_klm_generic_tfm_ops,
	.priv = &sirius_klm_generic_priv,
};

GxSeModule sirius_klm_generic_module = {
	.id   = GXSE_MOD_KLM_GENERIC,
	.ops  = &klm_dev_ops,
	.hwobj= &sirius_klm_generic_tfm,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_KLM,
		.reg_len   = sizeof(SiriusKLMReg),
		.irqs      = {GXACPU_IRQ_KLM, -1},
		.irq_names = {"klm"},
	},
};
