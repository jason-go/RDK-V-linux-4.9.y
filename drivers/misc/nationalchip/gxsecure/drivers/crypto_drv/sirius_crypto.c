#include "sirius_crypto.h"

//#define DEBUG_INPUT
//#define DEBUG_OUTPUT
//#define DEBUG_KEY
//#define DEBUG_CTRL

static struct crypto_priv sirius_crypto_fifo_priv = {
	.param = {0},
	.crypto_done = 0,
};

int32_t sirius_crypto_get_alg(GxTfmAlg alg)
{
	switch(alg) {
	case TFM_ALG_AES128:
		return SIRIUS_CRYPTO_ALG_AES;

	case TFM_ALG_SM4:
		return SIRIUS_CRYPTO_ALG_SM4;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_crypto_get_opt(GxTfmOpt opt)
{
	switch (opt) {
	case TFM_OPT_ECB:
		return SIRIUS_CRYPTO_OPT_ECB;
	case TFM_OPT_CBC:
		return SIRIUS_CRYPTO_OPT_CBC;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto opt error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_crypto_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_ACPU_SOFT:
		return SIRIUS_CRYPTO_KEY_SOFT;

#ifdef CPU_ACPU
	case TFM_KEY_OTP_FLASH:
		return SIRIUS_CRYPTO_KEY_OTP;

	case TFM_KEY_REG:
		param->sub &= 0x3;
		return SIRIUS_CRYPTO_KEY_REG0 + param->sub;

	case TFM_KEY_BCK:
		param->sub &= 0x1;
		return SIRIUS_CRYPTO_KEY_BCMK + param->sub;

	case TFM_KEY_IK:
		param->sub &= 0x1;
		return SIRIUS_CRYPTO_KEY_IMK + param->sub;

	case TFM_KEY_CCCK:
		return SIRIUS_CRYPTO_KEY_CCCK;
#else

	case TFM_KEY_CSGK2:
		return SIRIUS_CRYPTO_KEY_CSGK2;

	case TFM_KEY_SIK:
		return SIRIUS_CRYPTO_KEY_SIK;

	case TFM_KEY_OTP_SOFT:
		return SIRIUS_CRYPTO_KEY_SOFT;
#endif

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t sirius_crypto_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	unsigned long spin_flag = 0;
	(void) vreg;
	(void) cmd;

#ifdef CPU_SCPU
	if (cmd == TFM_ENCRYPT)
		return GXSE_ERR_GENERIC;
#endif

	if (ops->get_alg(param->alg) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_opt(param->opt) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_key(&param->even_key) < 0)
		return GXSE_ERR_PARAM;

	if (NULL == param->input.buf || param->input.length == 0 ||
		NULL == param->output.buf || param->output.length < param->input.length) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto buf error\n");
		return GXSE_ERR_PARAM;
	}
	memcpy(&p->param, param, sizeof(GxTfmCrypto));

	gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
	p->crypto_done = 0;
	gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	return GXSE_SUCCESS;
}

#ifdef CPU_ACPU
static int32_t sirius_crypto_get_dst(GxTfmDstBuf *param)
{
	switch (param->id) {
	case TFM_DST_MEM:
		return SIRIUS_CRYPTO_DST_DO;

	case TFM_DST_REG:
		param->sub &= 0x3;
		return SIRIUS_CRYPTO_DST_REG0 + param->sub;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto dst error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t sirius_crypto_set_work_mode(void *vreg, void *priv,
		uint32_t crypt_mode, uint32_t work_mode, uint32_t ts_mode)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	reg->ctrl.bits.enc = crypt_mode & 0x1;
	(void) priv;
	(void) work_mode;
	(void) ts_mode;

	return GXSE_SUCCESS;
}

static int32_t sirius_crypto_set_output_buf(void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len)
{
	int32_t ret = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	(void) out;
	(void) out_len;
	if ((ret = ops->get_dst(&out_sel)) < 0)
		return ret;

	reg->ctrl.bits.rslt_des = ret;
	return GXSE_SUCCESS;
}

static int32_t sirius_crypto_enable_key(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	reg->ctrl.bits.bcm_enc_en = 1;
	reg->ctrl.bits.im_enc_en  = 1;
	reg->ctrl.bits.otp_key_en = 1;

	(void) priv;
	return GXSE_SUCCESS;
}

static int32_t sirius_crypto_disable_key(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	reg->ctrl.bits.bcm_enc_en = 0;
	reg->ctrl.bits.im_enc_en  = 0;
	reg->ctrl.bits.otp_key_en = 0;

	(void) priv;
	return GXSE_SUCCESS;
}

static int32_t sirius_crypto_dsr(GxSeModuleHwObj *obj)
{
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;
	if (p->crypto_done)
		gx_wake_event(&p->queue, p->crypto_done);

	return GXSE_SUCCESS;
}


static int32_t sirius_crypto_isr(GxSeModuleHwObj *obj)
{
	unsigned long spin_flag;
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)obj->reg;

	if (reg->crypto_int.bits.crypto_done && reg->crypto_int_en.bits.crypto_done) {
		reg->crypto_int.bits.crypto_done = 1;
		gx_spin_lock_irqsave(&p->spin_lock, spin_flag);
		p->crypto_done = 1;
		gx_spin_unlock_irqrestore(&p->spin_lock, spin_flag);
	}
#ifndef ECOS_OS
	sirius_crypto_dsr(obj);
#endif

	return GXSE_SUCCESS;
}
#endif

int32_t sirius_crypto_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	(void) priv;
	if ((ret = ops->get_alg(alg)) < 0)
		return ret;

	reg->ctrl.bits.cipher = ret;

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	int32_t ret = 0, i = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	GxTfmCrypto *param = &p->param;
	uint32_t big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	(void) oddkey_sel;
	(void) key_len;

	if ((ret = ops->get_key(&key_sel)) < 0)
		return ret;

	if (ret == SIRIUS_CRYPTO_KEY_SOFT) {
#ifdef DEBUG_KEY
		gx_fw_debug_print(0x3100);
#endif
		for (i = 0; i < 4; i++) {
			reg->soft_key[3-i] = gx_u32_get_val(key+4*i, big_endian);
#ifdef DEBUG_KEY
			gx_fw_debug_print(reg->soft_key[3-i]);
#endif
		}
#ifdef DEBUG_KEY
		gx_fw_debug_print(0x3101);
#endif
	}

	reg->ctrl.bits.key_sel = ret;

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_set_opt(void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *key, uint32_t key_len)
{
	int32_t ret = 0, i = 0;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	GxTfmCrypto *param = &p->param;
	uint32_t big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	if ((ret = ops->get_opt(opt_sel)) < 0)
		return ret;

	if (ret == SIRIUS_CRYPTO_OPT_CBC) {
		if (NULL == key || key_len != 16) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "param is illegal.\n");
			return GXSE_ERR_GENERIC;
		}

		for (i = 0; i < 4; i++) {
			reg->IV[3-i] = gx_u32_get_val(key+4*i, big_endian);
		}
	}

	reg->ctrl.bits.opt_mode = ret;

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_get_blocklen(void *vreg, void *priv)
{
	(void) vreg;
	(void) priv;
	return SIRIUS_CRYPTO_BLOCKLEN;
}

int32_t sirius_crypto_fifo_put(void *vreg, void *priv, uint8_t *buf, uint32_t len, uint32_t first_block)
{
	int32_t i, j = 0;
	int32_t pos = 0, data_len, real_len;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;

	GxTfmCrypto *param = &p->param;
	uint32_t big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	if (NULL == buf || len == 0) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "param is illegal.\n");
		return GXSE_ERR_GENERIC;
	}

	data_len = len > 128 ? 128 : len;

	reg->data_length = data_len;
	while (pos < data_len) {
		real_len = (data_len > 16) ? 4 : (data_len>>2);
#ifdef DEBUG_INPUT
		gx_fw_debug_print(0x3110);
#endif
		for (i = 0; i < real_len; i++) {
			reg->data_in[3-i] = gx_u32_get_val(buf+4*((j<<2)+i),  big_endian);
#ifdef DEBUG_INPUT
			gx_fw_debug_print(reg->data_in[3-i]);
#endif
		}
#ifdef DEBUG_INPUT
		gx_fw_debug_print(0x3111);
#endif

		reg->ctrl.bits.di_update = 1;
		if (first_block && pos == 0)
			reg->ctrl.bits.sob = 1;
		pos += real_len*4;
		j++;
	}

	return len - data_len;
}

int32_t sirius_crypto_fifo_get(void *vreg, void *priv, uint8_t *buf, uint32_t len)
{
	int i, j = 0, temp;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	struct crypto_priv *p = (struct crypto_priv *)priv;

	GxTfmCrypto *param = &p->param;
	uint32_t big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	if (NULL == buf || len == 0) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "param is illegal.\n");
		return GXSE_ERR_GENERIC;
	}

	while (len > 0) {
		reg->ctrl.bits.do_update = 1;
		temp = (len >= 16) ? 0 : (4-(len>>2));
#ifdef DEBUG_OUTPUT
		gx_fw_debug_print(0x3120);
#endif
		for (i = 3; i >= temp; i--) {
			gx_u32_set_val(buf+((j<<2)+3-i)*4, &reg->data_out[i], big_endian);
#ifdef DEBUG_OUTPUT
			gx_fw_debug_print(reg->data_out[i]);
#endif
		}
#ifdef DEBUG_OUTPUT
		gx_fw_debug_print(0x3121);
#endif
		len -= (4-temp)*4;
		j++;
	}

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_set_ready(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;
	(void) priv;

#ifdef DEBUG_CTRL
	gx_fw_debug_print(0x3330);
	gx_fw_debug_print(reg->ctrl.value);
	gx_fw_debug_print(0x3331);
#endif

	reg->ctrl.bits.start = 1;

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	struct crypto_priv *p = (struct crypto_priv *)priv;
	(void) vreg;
	(void) is_query;

	CHECK_ISR_TIMEOUT(GXSE_LOG_MOD_CRYPTO, p->crypto_done, TFM_MASK_DONE, 150, p->queue);

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_clr_key(void *vreg, void *priv)
{
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)vreg;

#ifdef CPU_ACPU
	reg->ctrl.value |= 0xf<<20;
#else
	reg->ctrl.bits.CSGK2_clr = 1;
	reg->ctrl.bits.CSGK2_clr = 0;
	reg->ctrl.bits.SIK_clr = 1;
	reg->ctrl.bits.SIK_clr = 0;
#endif

	(void) priv;
	return GXSE_SUCCESS;
}

static GxCryptoResOps sirius_crypto_resops = {
	.get_alg = sirius_crypto_get_alg,
	.get_key = sirius_crypto_get_key,
	.get_opt = sirius_crypto_get_opt,
#ifdef CPU_ACPU
	.get_dst = sirius_crypto_get_dst,
#endif
};

int32_t sirius_crypto_init(GxSeModuleHwObj *obj)
{
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;

	p->reg = (volatile SiriusCryptoReg *)obj->reg;
	p->reg->crypto_int_en.value = 0x3;
	p->resops = &sirius_crypto_resops;

	gx_spin_lock_init(&p->spin_lock);
	gx_event_init(&p->queue);
	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;
#ifdef CPU_SCPU
	scpu_crypto_init(obj);
#endif
	return GXSE_SUCCESS;
}

int32_t sirius_crypto_deinit(GxSeModuleHwObj *obj)
{
	struct crypto_priv *p = (struct crypto_priv *)obj->priv;
	volatile SiriusCryptoReg *reg = (volatile SiriusCryptoReg *)obj->reg;

	reg->crypto_int_en.value = 0x0;
	gx_mutex_destroy(&p->mutex);

	return GXSE_SUCCESS;
}

int32_t sirius_crypto_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->max_sub_num = 1;
	param->alg = 1 << TFM_ALG_AES128 | 1 << TFM_ALG_SM4;
	param->opt = 1 << TFM_OPT_ECB | 1 << TFM_OPT_CBC;
#ifdef CPU_ACPU
	param->key = 1 << TFM_KEY_CCCK | 1 << TFM_KEY_OTP_FLASH | 1 << TFM_KEY_ACPU_SOFT;
#else
	param->key = 1 << TFM_KEY_CSGK2 | 1 << TFM_KEY_SIK | 1 << TFM_KEY_OTP_SOFT | 1 << TFM_KEY_ACPU_SOFT;
#endif

	return GXSE_SUCCESS;
}

static GxSeModuleDevOps sirius_crypto_devops = {
	.init            = sirius_crypto_init,
#ifdef CPU_ACPU
	.deinit          = sirius_crypto_deinit,
	.isr             = sirius_crypto_isr,
	.dsr             = sirius_crypto_dsr,
#endif
};

static GxCryptoOps sirius_crypto_ops = {
	.check_param     = sirius_crypto_check_param,
	.capability      = sirius_crypto_capability,
    .set_alg         = sirius_crypto_set_alg,
    .set_key         = sirius_crypto_set_key,
    .set_opt         = sirius_crypto_set_opt,
    .get_blocklen    = sirius_crypto_get_blocklen,
    .fifo_put        = sirius_crypto_fifo_put,
    .fifo_get        = sirius_crypto_fifo_get,
	.clr_key         = sirius_crypto_clr_key,
#ifdef CPU_SCPU
	.set_ready       = scpu_crypto_set_ready,
	.wait_finish     = scpu_crypto_wait_finish,
#else
    .wait_finish     = sirius_crypto_wait_finish,
	.set_ready       = sirius_crypto_set_ready,
    .set_output_buf  = sirius_crypto_set_output_buf,
    .set_work_mode   = sirius_crypto_set_work_mode,
	.enable_key      = sirius_crypto_enable_key,
	.disable_key     = sirius_crypto_disable_key,
#endif
};

static GxSeModuleHwObjTfmOps sirius_crypto_fifo_tfm_ops = {
	.devops = &sirius_crypto_devops,
	.hwops = &sirius_crypto_ops,

	.tfm_cipher = {
		.set_pvr_key   = gx_tfm_object_crypto_set_pvr_key,
		.clr_key       = gx_tfm_object_crypto_clr_key,
		.enable_key    = gx_tfm_object_crypto_enable_key,
		.disable_key   = gx_tfm_object_crypto_disable_key,
#ifdef CPU_ACPU
		.encrypt       = gx_tfm_object_crypto_fifo_encrypt,
#endif
		.decrypt       = gx_tfm_object_crypto_fifo_decrypt,
		.capability    = gx_tfm_object_crypto_capability,
	},
};

static GxSeModuleHwObj sirius_crypto_fifo_obj = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub  = TFM_MOD_CRYPTO,
	.ops  = &sirius_crypto_fifo_tfm_ops,
	.priv = &sirius_crypto_fifo_priv,
};

GxSeModule sirius_crypto_module = {
	.id    = GXSE_MOD_CRYPTO_FIFO,
	.ops   = &crypto_dev_ops,
	.hwobj = &sirius_crypto_fifo_obj,
	.res   = {
#ifdef CPU_ACPU
		.reg_base  = GXACPU_BASE_ADDR_CRYPTO,
		.reg_len   = sizeof(SiriusCryptoReg),
		.irqs      = {GXACPU_IRQ_CRYPTO, -1},
		.irq_names = {"crypto"},
#else
		.reg_base  = GXSCPU_BASE_ADDR_CRYPTO,
		.reg_len   = sizeof(SiriusCryptoReg),
#endif
	},
};
