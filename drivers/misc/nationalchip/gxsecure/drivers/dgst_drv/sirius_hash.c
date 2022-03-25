#include "gxdgst_virdev.h"
#include "sirius_hash.h"
#include "sirius_hash_reg.h"

//#define DEBUG_OUTPUT

struct dgst_priv {
	GxTfmDgst       param;
	gx_mutex_t      mutex;

	uint8_t        *align_buf;
	uint32_t        align_buf_len;

	volatile uint32_t dma_flag;
	volatile uint32_t run_flag;
	gx_event_t        queue;
	gx_spin_lock_t spin_lock;
	volatile SiriusDgstReg *reg;
};

static struct dgst_priv sirius_dgst_priv = {.param = {0},};

static int32_t _get_alg(GxTfmAlg alg)
{
	switch (alg) {
#ifdef CPU_ACPU
	case TFM_ALG_SM3:
	case TFM_ALG_SM3_HMAC:
		return SIRIUS_HASH_ALG_SM3;
#endif

	case TFM_ALG_SHA1:
		return SIRIUS_HASH_ALG_SHA1;

	case TFM_ALG_SHA256:
	case TFM_ALG_SHA256_HMAC:
		return SIRIUS_HASH_ALG_SHA256;

	default:
		gxlog_e(GXSE_LOG_MOD_DGST, "dgst alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t sirius_dgst_check_param(void *vreg, void *priv, GxTfmDgst *param)
{
	int32_t alg = 0;
	struct dgst_priv *p = (struct dgst_priv *)priv;
	(void) vreg;

	if ((alg = _get_alg(param->alg)) < 0)
		return GXSE_ERR_PARAM;

	if (param->alg == TFM_ALG_SHA256_HMAC || param->alg == TFM_ALG_SM3_HMAC) {
		if (NULL == param->hmac_key.buf || param->hmac_key.length == 0) {
			gxlog_e(GXSE_LOG_MOD_DGST, "dgst hmac buf error\n");
			return GXSE_ERR_PARAM;
		}
	}

	if (NULL == param->input.buf || param->input.length == 0 ||
		NULL == param->output.buf || param->output.length == 0) {
		gxlog_e(GXSE_LOG_MOD_DGST, "dgst buf error\n");
		return GXSE_ERR_PARAM;
	}

	if (alg == SIRIUS_HASH_ALG_SHA1) {
		if (param->output.length < SIRIUS_SHA1_OUTLEN) {
			gxlog_e(GXSE_LOG_MOD_DGST, "dgst outlen error\n");
			return GXSE_ERR_PARAM;
		}

	} else {
		if (param->output.length < SIRIUS_SHA256_OUTLEN) {
			gxlog_e(GXSE_LOG_MOD_DGST, "dgst outlen error\n");
			return GXSE_ERR_PARAM;
		}
	}

	memcpy(&p->param, param, sizeof(GxTfmDgst));
	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_set_endian(void *vreg, void *priv, uint32_t flag)
{
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	(void) priv;
	(void) flag;

	//TODO flag
	reg->ctrl1.bits.data_endian = 1;
	reg->ctrl1.bits.hash_endian = 1;

	reg->ctrl2.bits.msg_start   = 1;
	reg->ctrl2.bits.msg_start   = 0;

	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static int32_t sirius_dgst_set_work_mode(void *vreg, void *priv, uint32_t dma_mode)
{
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	(void) priv;

	reg->dma_conf.bits.dma_on   = dma_mode ? 0x1 : 0x0;

	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_set_dma_buf(void *vreg, void *priv, uint8_t *in, uint32_t in_len)
{
	struct dgst_priv *p = (struct dgst_priv *)priv;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;

	reg->dma_len = in_len;
	reg->dma_start_addr = (uint32_t)in;

	if (!p->run_flag) {
		p->dma_flag = 1;
		reg->dma_conf.bits.dma_start = 1;
		while(p->dma_flag);

	} else {
		reg->dma_conf.bits.dma_start = 1;
		while(p->run_flag);
	}
	reg->dma_conf.bits.dma_start = 0;

	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_get_blocklen(void *vreg, void *priv)
{
	(void) vreg;
	(void) priv;
	return SIRIUS_DGST_BLOCKLEN;
}

static int32_t sirius_dgst_get_align_param(void *vreg, void *priv,
		uint32_t *buf, uint32_t *size, uint32_t *align, uint32_t *flag)
{
	struct dgst_priv *p = (struct dgst_priv *)priv;
	(void) vreg;

	if (buf)
		*buf = (uint32_t)p->align_buf;

	if (size)
		*size = p->align_buf_len;

	if (align)
		*align = 4;

	if (flag)
		*flag = TFM_FORCE_ALIGN;

	return GXSE_SUCCESS;
}
#endif

static int32_t sirius_dgst_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t __alg = 0;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	(void) priv;

	if ((__alg = _get_alg(alg)) < 0)
		return GXSE_ERR_GENERIC;

	reg->ctrl1.value &= ~(0xff << 8);
	reg->ctrl1.value |= (__alg & 0xff) << 8;

	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_fifo_put(void *vreg, void *priv, uint8_t *buf, uint32_t len)
{
	int32_t i = 0;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	(void) priv;

#ifdef DEBUG_OUTPUT
	gx_fw_debug_print(0x4000);
#endif
	for (i = 0; i < len/4; i++) {
		while(reg->status.bits.hash_busy);
		reg->msg_data = gx_u32_get_val(buf+4*i, 1);
#ifdef DEBUG_OUTPUT
		gx_fw_debug_print(reg->msg_data);
#endif
	}
#ifdef DEBUG_OUTPUT
	gx_fw_debug_print(0x4001);
#endif

	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_get_result(void *vreg, void *priv, uint8_t *buf, uint32_t len)
{
	int i;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	(void) priv;
	(void) len;

	if (reg->ctrl1.bits.alg == SIRIUS_HASH_ALG_SHA1) {
		for (i = 0; i < 5; i++) {
			gx_u32_set_val(buf+4*i, &reg->sha_1[i], 1);
		}

	} else if (reg->ctrl1.bits.alg == SIRIUS_HASH_ALG_SHA256) {
		for (i = 0; i < 8; i++) {
			gx_u32_set_val(buf+4*i, &reg->sha_256[i], 1);
		}

#if defined (CPU_ACPU)
	} else if (reg->ctrl1.bits.alg == SIRIUS_HASH_ALG_SM3) {
		for (i = 0; i < 8; i++) {
			gx_u32_set_val(buf+4*i, &reg->sm3[i], 1);
		}
#endif
	}
	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_set_ready(void *vreg, void *priv, uint32_t final_val)
{
	struct dgst_priv *p = (struct dgst_priv *)priv;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	unsigned long spin_flag = 0;

	uint32_t left = final_val >> 28;
	uint32_t val  = final_val & 0xffffff;

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->run_flag = 1;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);

	if (left) {
		while(reg->status.bits.hash_busy);
		reg->ctrl2.value = (left<<2) | 2;
		if (TFM_FLAG_GET(p->param.flags, TFM_FLAG_HASH_DMA_EN) == 0)
			reg->msg_data = val;
	} else
		reg->ctrl2.bits.msg_finish = 1;

	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	struct dgst_priv *p = (struct dgst_priv *)priv;
	(void) vreg;
	(void) is_query;

#if defined (CPU_SCPU)
	while(p->run_flag);
#else
	CHECK_ISR_TIMEOUT(GXSE_LOG_MOD_KLM, (!(p->run_flag)), TFM_MASK_DONE, 150, p->queue);
#endif
	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_clr_config(void *vreg, void *priv)
{
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)vreg;
	(void) priv;

	reg->ctrl2.value = 0;
	reg->dma_conf.value = 0;
	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_init(GxSeModuleHwObj *obj)
{
	struct dgst_priv *p = (struct dgst_priv *)obj->priv;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)obj->reg;
	memset(p, 0, sizeof(struct dgst_priv));

	reg->hash_int_en.value |= 0x107;
	gx_spin_lock_init(&p->spin_lock);
	gx_event_init(&p->queue);
	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;
	p->reg = reg;

#ifdef CPU_ACPU
	p->align_buf_len = TFM_MAX_DGST_UNIT;
	if ((p->align_buf = gx_osdep_page_malloc(p->align_buf_len)) == NULL) {
		gxlog_e(GXSE_LOG_MOD_DGST, "align buffer alloc failed\n");
		return GXSE_ERR_GENERIC;
	}
	memset(p->align_buf, 0, p->align_buf_len);
	gx_osdep_cache_sync(p->align_buf, p->align_buf_len, DMA_TO_DEVICE);
#endif

	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static int32_t sirius_dgst_dsr(GxSeModuleHwObj *obj)
{
	struct dgst_priv *p = (struct dgst_priv *)obj->priv;
	if (p->run_flag == 0)
		gx_wake_event(&p->queue, (!(p->run_flag)));

	return GXSE_SUCCESS;
}
#endif

static int32_t _dgst_isr(struct dgst_priv *p)
{
	volatile SiriusDgstReg *reg = p->reg;
	unsigned long spin_flag = 0;
	int hash_finish = 0;

	if (reg->hash_int_en.bits.sha_1_done & reg->hash_int.bits.sha_1_done) {
		reg->hash_int.bits.sha_1_done = 1;
		gxlog_d(GXSE_LOG_MOD_DGST, "sha-1 done\n");
		hash_finish = 1;
	}

	if (reg->hash_int_en.bits.sha_256_done & reg->hash_int.bits.sha_256_done) {
		reg->hash_int.bits.sha_256_done = 1;
		gxlog_d(GXSE_LOG_MOD_DGST, "sha-256 done\n");
		hash_finish = 1;
	}

#if defined (CPU_ACPU)
	if (reg->hash_int_en.bits.sm3_done & reg->hash_int.bits.sm3_done) {
		reg->hash_int.bits.sm3_done = 1;
		gxlog_d(GXSE_LOG_MOD_DGST, "sm3 done\n");
		hash_finish = 1;
	}

	if (reg->hash_int_en.bits.dma_done & reg->hash_int.bits.dma_done) {
		reg->hash_int.bits.dma_done = 1;
		gxlog_d(GXSE_LOG_MOD_DGST, "dma done\n");
		p->dma_flag = 0;
	}
#endif
	if (hash_finish) {
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->run_flag = 0;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}

	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static int32_t sirius_dgst_deinit(GxSeModuleHwObj *obj)
{
	struct dgst_priv *p = (struct dgst_priv *)obj->priv;
	volatile SiriusDgstReg *reg = (volatile SiriusDgstReg *)obj->reg;

	reg->hash_int_en.value = 0;
	gx_mutex_destroy(&p->mutex);
	obj->mutex = NULL;

	gx_osdep_page_free(p->align_buf, p->align_buf_len);
	p->align_buf = NULL;
	p->align_buf_len = 0;

	return GXSE_SUCCESS;
}

static int32_t sirius_dgst_isr(GxSeModuleHwObj *obj)
{
	struct dgst_priv *p = (struct dgst_priv *)obj->priv;

	_dgst_isr(p);

#ifndef ECOS_OS
	sirius_dgst_dsr(obj);
#endif
	return GXSE_SUCCESS;
}

#else
int gx_hash_isr(int irq, void *pdata)
{
	return _dgst_isr(&sirius_dgst_priv);
}
#endif

static int32_t sirius_dgst_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	param->max_sub_num  = 1;
	param->alg = 1 << TFM_ALG_SM3 | 1 << TFM_ALG_SHA1 | 1 << TFM_ALG_SHA256 |
				1 << TFM_ALG_SM3_HMAC | 1 << TFM_ALG_SHA256_HMAC;
	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_dgst_devops = {
	.init            = sirius_dgst_init,
#ifdef CPU_ACPU
	.deinit          = sirius_dgst_deinit,
	.isr             = sirius_dgst_isr,
	.dsr             = sirius_dgst_dsr,
#endif
};

static GxDgstOps sirius_dgst_hwops = {
	.check_param     = sirius_dgst_check_param,
	.capability      = sirius_dgst_capability,

#ifdef CPU_ACPU
	.get_align_param = sirius_dgst_get_align_param,
	.get_blocklen    = sirius_dgst_get_blocklen,
	.set_work_mode   = sirius_dgst_set_work_mode,
	.set_dma_buf     = sirius_dgst_set_dma_buf,
#endif
	.set_endian      = sirius_dgst_set_endian,
	.set_alg         = sirius_dgst_set_alg,
	.fifo_put        = sirius_dgst_fifo_put,
	.get_result      = sirius_dgst_get_result,
	.set_ready       = sirius_dgst_set_ready,
	.wait_finish     = sirius_dgst_wait_finish,
	.clr_config      = sirius_dgst_clr_config,
};

static GxSeModuleHwObjTfmOps sirius_dgst_objops = {
	.devops = &sirius_dgst_devops,
	.hwops = &sirius_dgst_hwops,

	.tfm_dgst = {
		.dgst            = gx_tfm_object_dgst,
#ifdef CPU_ACPU
		.dgst_hmac       = gx_tfm_object_dgst_hmac,
#endif
		.capability      = gx_tfm_object_dgst_capability,
	},
};

static GxSeModuleHwObj sirius_hash_tfm = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_MISC_DGST,
	.ops  = &sirius_dgst_objops,
	.priv = &sirius_dgst_priv,
};

GxSeModule sirius_hash_module = {
	.id   = GXSE_MOD_MISC_DGST,
	.ops  = &dgst_dev_ops,
	.hwobj = &sirius_hash_tfm,
	.res  = {
#ifdef CPU_ACPU
		.reg_base  = GXACPU_BASE_ADDR_HASH,
		.reg_len   = sizeof(SiriusDgstReg),
		.irqs      = {GXACPU_IRQ_HASH, -1},
		.irq_names = {"hash"},
#else
		.reg_base  = GXSCPU_BASE_ADDR_HASH,
		.reg_len   = sizeof(SiriusDgstReg),
#endif
	},
};
