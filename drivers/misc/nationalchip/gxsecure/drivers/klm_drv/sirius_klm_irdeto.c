#include "gxklm_virdev.h"
#include "sirius_klm_reg.h"
#include "sirius_klm.h"

struct klm_priv {
	GxTfmKlm param;
	unsigned int total_run_time;
	unsigned int stage;

	volatile int klm_done;
	volatile int tdc_klm_run;

	gx_event_t queue;
	gx_event_t tdc_queue;
	gx_spin_lock_t spin_lock;
	gx_mutex_t *mutex;
};


static struct klm_priv sirius_klm_irdeto_priv = {
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

	case TFM_ALG_T3DES:
		return SIRIUS_KLM_ALG_T3DES;

	case TFM_ALG_TAES:
		return SIRIUS_KLM_ALG_TAES;

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
		return SIRIUS_KLM_KEY_CWUK;

	case TFM_KEY_PVRK:
		return SIRIUS_KLM_KEY_CPUK;

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

static int32_t sirius_irdeto_klm_check_param(void *vreg, void *priv, GxTfmKlm *param, uint32_t cmd)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	uint8_t check_key = 0, check_stage = 0, check_alg = 0;
	uint8_t check_in = 0, check_dst = 0, check_td = 0;
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
	case TFM_KLM_UPDATE_TD_PARAM:
		check_in = 1;
		check_td = 1;
		check_key = 1;
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
		if (param->stage < 2 || param->stage > 3) {
			gxlog_e(GXSE_LOG_MOD_KLM, "stage error\n");
			return GXSE_ERR_PARAM;
		}
	}

	if (check_td) {
		if (NULL == param->TD.buf || param->TD.length == 0 || param->input.length != 16) {
			gxlog_e(GXSE_LOG_MOD_KLM, "TD buf error\n");
			return GXSE_ERR_PARAM;
		}
	}
	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->klm_done = 0;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	memcpy(&p->param, param, sizeof(GxTfmKlm));

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_rootkey(void *vreg, void *priv, GxTfmKeyBuf key_sel)
{
	int32_t ret = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) priv;

	if ((ret = _get_key(&key_sel)) < 0)
		return ret;

	reg->ctrl.bits.mode = ret;

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_stage(void *vreg, void *priv, uint32_t stage)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	p->stage = stage;
	if (param->key.id == TFM_KEY_CWUK)
		reg->ctrl.bits.cw_stage = SIRIUS_KLM_STAGE(stage);
	else
		reg->ctrl.bits.pvr_stage = SIRIUS_KLM_STAGE(stage);

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;

	(void) priv;
	if ((ret = _get_alg(alg)) < 0)
		return ret;

	reg->ctrl.bits.alg = ret;

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_dst(void *vreg, void *priv, GxTfmDstBuf out_sel)
{
	int32_t ret = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;

	GxTfmKlm *param = &p->param;
	uint32_t cwhalf_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_KLM_CW_HALF);

	if (param->key.id == TFM_KEY_CWUK) {
		if ((ret = _get_dst(&out_sel)) < 0)
			return ret;
		reg->ctrl.bits.dst = ret;
	}

	if (p->total_run_time == 0) {
			reg->ctrl.bits.dst_cw_half = cwhalf_flag;
		if (param->alg == TFM_ALG_TDES || param->alg == TFM_ALG_T3DES)
			p->total_run_time = 1 - (cwhalf_flag&0x1);
	} else {
		if (cwhalf_flag && param->alg%2 == SIRIUS_KLM_ALG_TDES) {
			reg->ctrl.bits.tdes_klm_half_clear = 1;
			reg->ctrl.bits.tdes_klm_half_clear = 0;
			reg->ctrl.bits.dst_cw_half = cwhalf_flag;
		}
		p->total_run_time--;
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_ekn(void *vreg, void *priv, uint8_t *in, uint32_t in_len, uint32_t pos)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;

	uint32_t i = 0;
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) in_len;

	for (i = 0; i < 4; i++) {
		if (p->stage == 3) {
			if (pos == 0) {
				reg->EK1_AK[3-i] = gx_u32_get_val(in+i*4, endian_flag);
				gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->EK1_AK[3-i]);

			} else if (pos == 1) {
				reg->EK2_SK[3-i] = gx_u32_get_val(in+i*4, endian_flag);
				gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->EK2_SK[3-i]);
			}
		} else {
			reg->EK2_SK[3-i] = gx_u32_get_val(in+i*4, endian_flag);
			gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->EK2_SK[3-i]);
		}
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_ecw(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
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
			reg->EK3_CW[i] = 0;
			gxlog_d(GXSE_LOG_MOD_KLM, "0\n");

		} else {
			reg->EK3_CW[i] = gx_u32_get_val(in+(highest-i)*4, endian_flag);
			gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->EK3_CW[i]);
		}
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_ready(void *vreg, void *priv)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) priv;

	reg->start = 1;
	gxlog_d(GXSE_LOG_MOD_KLM, "[IRDETO KLM CTRL] : %x\n", reg->ctrl.value);

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	(void) vreg;
	(void) is_query;

	CHECK_ISR_TIMEOUT(GXSE_LOG_MOD_KLM, p->klm_done, TFM_MASK_DONE, 50, p->queue);

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_tdc_data(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	uint32_t i = 0;
	struct klm_priv *p = (struct klm_priv *)priv;
	GxTfmKlm *param = &p->param;
	uint32_t endian_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_BIG_ENDIAN);
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	(void) in_len;

	for (i = 0; i < 4; i++) {
		reg->tdc_in[3-i] = gx_endian_set(in+i*4, endian_flag);
		gxlog_d(GXSE_LOG_MOD_KLM, "%x\n", reg->tdc_in[3-i]);
	}
	reg->tdc_ctrl.bits.tdc_write_en = 1;
	reg->tdc_ctrl.bits.tdc_write_en = 0;

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_set_tdc_ready(void *vreg, void *priv, uint32_t tdc_size)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;
	struct klm_priv *p = (struct klm_priv *)priv;
	unsigned long spin_flag = 0;

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->tdc_klm_run = 1;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	reg->ctrl.bits.mode = SIRIUS_KLM_KEY_TAUK;
	reg->ctrl.bits.alg = SIRIUS_KLM_ALG_AES;
	reg->tdc_ctrl.bits.tdc_size = tdc_size / 16;
	reg->start = 1;

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_wait_tdc_idle(void *vreg, void *priv, uint32_t is_query)
{
	struct klm_priv *p = (struct klm_priv *)priv;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)vreg;

	if (is_query == TFM_ISR_QUERY_UNTIL) {
		while(reg->tdc_ctrl.bits.tdc_busy);
		return GXSE_SUCCESS;
	}

	CHECK_ISR_TIMEOUT(GXSE_LOG_MOD_KLM, (!(p->tdc_klm_run)), TFM_MASK_DONE, 100, p->tdc_queue);

	return (p->tdc_klm_run == 0) ? GXSE_ERR_GENERIC : GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_init(GxSeModuleHwObj *obj)
{
	struct klm_priv *p = (struct klm_priv *)obj->priv;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)obj->reg;
	memset(p, 0, sizeof(struct klm_priv)-4);

	reg->klm_int_en.value |= 0xff;
	gx_spin_lock_init(&p->spin_lock);
	gx_event_init(&p->queue);
	gx_event_init(&p->tdc_queue);
	gx_mutex_init(p->mutex);
	obj->mutex = p->mutex;

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_deinit(GxSeModuleHwObj *obj)
{
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)obj->reg;
	struct klm_priv *p = (struct klm_priv *)obj->priv;

	reg->klm_int_en.value &= ~(0xff);
	gx_mutex_destroy(p->mutex);
	obj->mutex = NULL;

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_dsr(GxSeModuleHwObj *obj)
{
	unsigned long spin_flag = 0;
	struct klm_priv *p = (struct klm_priv *)obj->priv;
	if (p->tdc_klm_run) {
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->tdc_klm_run = 0;
		gx_wake_event(&p->tdc_queue, (!(p->tdc_klm_run)));
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}
	if (p->klm_done)
		gx_wake_event(&p->queue, p->klm_done);

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_isr(GxSeModuleHwObj *obj)
{
	int ir_finish = 0;
	unsigned long spin_flag = 0;
	volatile SiriusKLMReg *reg = (volatile SiriusKLMReg *)obj->reg;
	struct klm_priv *p = (struct klm_priv *)obj->priv;

    if (reg->klm_int.bits.rcv_key_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "receive key err!\n");

    if (reg->klm_int.bits.cw_gen_key_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "cw gen key err\n");

    if (reg->klm_int.bits.tdes_key_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "tdes key err!\n");

    if (reg->klm_int.bits.tdc_hash_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "tdc hash err!\n");

    if (reg->klm_int.bits.tdc_cmd_size_err)
		gxlog_e(GXSE_LOG_MOD_KLM, "tdc cmd size err!\n");

    if (reg->klm_int.bits.tdes_half_klm_finish) {
		ir_finish = 1;
		gxlog_d(GXSE_LOG_MOD_KLM, "half klm finish!\n");
    }

    if (reg->klm_int.bits.tdes_klm_finish) {
		ir_finish = 1;
		gxlog_d(GXSE_LOG_MOD_KLM, "tdes klm finish!\n");
    }

	if (reg->klm_int.bits.klm_finish) {
		ir_finish = 1;
		gxlog_d(GXSE_LOG_MOD_KLM, "klm finish\n");
	}

	if (ir_finish) {
		reg->start = 0;
		if (p->tdc_klm_run == 0) {
			gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
			p->klm_done = 1;
			gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
		}
	}

	reg->klm_int.value = 0xff;

#ifndef ECOS_OS
	sirius_irdeto_klm_dsr(obj);
#endif

	return GXSE_SUCCESS;
}

static int32_t sirius_irdeto_klm_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_CWUK | 1 << TFM_KEY_PVRK;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_TDES | 1 << TFM_ALG_T3DES | 1 << TFM_ALG_TAES;
	param->dst = 1 << TFM_DST_TS | 1 << TFM_DST_M2M | 1 << TFM_DST_GP;
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->flags = TFM_FLAG_KLM_INPUT_HALF | TFM_FLAG_KLM_CW_HALF | TFM_FLAG_KLM_CW_HIGH_8BYTE;

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_klm_irdeto_devops = {
	.init            = sirius_irdeto_klm_init,
	.deinit          = sirius_irdeto_klm_deinit,
	.isr             = sirius_irdeto_klm_isr,
	.dsr             = sirius_irdeto_klm_dsr,
};

static GxKLMOps sirius_klm_irdeto_ops = {
	.check_param     = sirius_irdeto_klm_check_param,
	.capability      = sirius_irdeto_klm_capability,

	.set_rootkey     = sirius_irdeto_klm_set_rootkey,
	.set_stage       = sirius_irdeto_klm_set_stage,
    .set_alg         = sirius_irdeto_klm_set_alg,
    .set_dst         = sirius_irdeto_klm_set_dst,
    .set_ekn         = sirius_irdeto_klm_set_ekn,
    .set_ecw         = sirius_irdeto_klm_set_ecw,
    .set_ready       = sirius_irdeto_klm_set_ready,
    .wait_finish     = sirius_irdeto_klm_wait_finish,
	.set_tdc_data    = sirius_irdeto_klm_set_tdc_data,
	.set_tdc_ready   = sirius_irdeto_klm_set_tdc_ready,
	.wait_tdc_idle   = sirius_irdeto_klm_wait_tdc_idle,
};

static GxSeModuleHwObjTfmOps sirius_klm_irdeto_tfm_ops = {
	.devops = &sirius_klm_irdeto_devops,
	.hwops = &sirius_klm_irdeto_ops,

	.tfm_klm = {
		.select_rootkey  = gx_tfm_object_klm_select_rootkey,
		.set_kn          = gx_tfm_object_klm_set_kn,
		.set_cw          = gx_tfm_object_klm_set_cw,
		.update_TD_param = gx_tfm_object_klm_update_TD_param,
		.capability      = gx_tfm_object_klm_capability,
	},
};

static GxSeModuleHwObj sirius_klm_irdeto_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_KLM_IRDETO,
	.ops  = &sirius_klm_irdeto_tfm_ops,
	.priv = &sirius_klm_irdeto_priv,
};

GxSeModule sirius_klm_irdeto_module = {
	.id   = GXSE_MOD_KLM_IRDETO,
	.ops  = &klm_dev_ops,
	.hwobj= &sirius_klm_irdeto_tfm,
	.res  = {
		.reg_base  = GXACPU_BASE_ADDR_KLM,
		.reg_len   = sizeof(SiriusKLMReg),
		.irqs      = {GXACPU_IRQ_KLM, -1},
		.irq_names = {"klm"},
	},
};
