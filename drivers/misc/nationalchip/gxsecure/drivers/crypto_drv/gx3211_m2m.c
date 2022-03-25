#include "gxcrypto_virdev.h"
#include "gx3211_m2m_reg.h"
#include "gx3211_m2m.h"
#include "taurus_m2m.h"

int32_t gx3211_m2m_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_MULTI:
		return GX3211_M2M_KEY_MULTI_LAYER;

	case TFM_KEY_NDS:
		return GX3211_M2M_KEY_NDS;

	case TFM_KEY_OTP_SOFT:
		return GX3211_M2M_KEY_OTP;

	case TFM_KEY_ACPU_SOFT:
		return GX3211_M2M_KEY_ACPU_SOFT;

	//GX3211_M2M_KEY_AES_CP
	//GX3211_M2M_KEY_TDES_CP
	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t gx3211_m2m_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0, key = 0;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	uint32_t des_iv_sel = TFM_FLAG_GET(p->param.flags, TFM_FLAG_CRYPT_DES_IV_SHIFT);
	GxCryptoResOps *ops = p->resops;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((ret = ops->get_alg(alg)) < 0)
		return ret;

	if ((key = ops->get_key(&p->param.even_key)) < 0)
		return GXSE_ERR_GENERIC;

	if (key == GX3211_M2M_KEY_NDS) {
		if (ret == TAURUS_M2M_ALG_DES)
			reg->nds_ctrl.bits.alg = 2;
		else if (alg == TAURUS_M2M_ALG_TDES3K)
			reg->nds_ctrl.bits.alg = 1;
		else
			reg->nds_ctrl.bits.alg = 0;

	} else {
		if (ret <= TAURUS_M2M_ALG_TDES3K) {
			reg->ctrl1.bits.tri_des = ret;
			reg->ctrl1.bits.work_mode = 0;
			reg->ctrl1.bits.des_iv_sel = des_iv_sel;
		} else if (ret <= TAURUS_M2M_ALG_AES256) {
			reg->ctrl1.bits.aes_mode = ret - TAURUS_M2M_ALG_AES128;
			reg->ctrl1.bits.work_mode = 1;
		}
	}

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_set_opt(void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *iv, uint32_t key_len)
{
	int32_t opt = 0, i = 0, max = 0, key = 0;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;
	GxCryptoResOps *ops = p->resops;
	uint32_t alg = ops->get_alg(param->alg);
	uint32_t key_big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);
	(void) key_len;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((opt = ops->get_opt(opt_sel)) < 0)
		return opt;

	if ((key = ops->get_key(&p->param.even_key)) < 0)
		return GXSE_ERR_GENERIC;

	if (key == GX3211_M2M_KEY_NDS)
		reg->nds_ctrl.bits.opt = opt;
	else
		reg->ctrl1.bits.opt = opt;

	if (opt != TAURUS_M2M_OPT_ECB && opt != TAURUS_M2M_OPT_CTR) {
		volatile unsigned int *iv_reg[2] = {&reg->iv1[0], &reg->iv2[0]};
		if (alg <= TAURUS_M2M_ALG_TDES3K) {
			iv_reg[param->iv_id][3] = gx_u32_get_val(iv+4*1, key_big_endian);
			iv_reg[param->iv_id][2] = gx_u32_get_val(iv, key_big_endian);
		} else {
			iv_reg[param->iv_id][3] = gx_u32_get_val(iv+4*3, key_big_endian);
			iv_reg[param->iv_id][2] = gx_u32_get_val(iv+4*2, key_big_endian);
			iv_reg[param->iv_id][1] = gx_u32_get_val(iv+4*1, key_big_endian);
			iv_reg[param->iv_id][0] = gx_u32_get_val(iv+4*0, key_big_endian);
		}
	}

	max = ((alg < TAURUS_M2M_ALG_AES128) ? 8 : 16) / 4;
	if (opt == TAURUS_M2M_OPT_CTR)
		for (i = 0; i < max; i++)
			reg->counter[4-max+i] = gx_u32_get_val(iv+4*i, key_big_endian);

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_set_residue_mode(void *vreg, void *priv, uint32_t mode)
{
	int32_t key = 0;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((key = ops->get_key(&p->param.even_key)) < 0)
		return GXSE_ERR_GENERIC;

	if (key == GX3211_M2M_KEY_NDS)
		reg->nds_ctrl.bits.residue = mode;
	else
		reg->ctrl1.bits.residue = mode;

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_set_shortmsg_mode(void *vreg, void *priv, uint32_t mode)
{
	int32_t key = 0;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((key = ops->get_key(&p->param.even_key)) < 0)
		return GXSE_ERR_GENERIC;

	if (key == GX3211_M2M_KEY_NDS)
		reg->nds_ctrl.bits.short_msg = mode;
	else
		reg->ctrl1.bits.short_msg = mode;

	return GXSE_SUCCESS;
}


static int32_t gx3211_m2m_set_work_mode(void *vreg, void *priv,
		uint32_t crypt_mode, uint32_t work_mode, uint32_t ts_mode)
{
	int32_t key = 0;
	volatile int delay = 50;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;
	(void) work_mode;
	(void) ts_mode;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((key = ops->get_key(&p->param.even_key)) < 0)
		return GXSE_ERR_GENERIC;

	if (key == GX3211_M2M_KEY_NDS) {
		if (crypt_mode)
			reg->nds_ctrl.bits.encrypt = 1;
		else
			reg->nds_ctrl.bits.decrypt = 1;
	} else
		reg->ctrl1.bits.encrypt = crypt_mode & 0x1;


// 因硬件在调用aes加解密时存在出错的情况，所以每次使用前最好重置一下，调用如下接口
// 3211中存在 aes 连续解密出错问题，需 mtc_rst_contrl(0x60) 寄存器的第5bit需重置(硬件bug)
	reg->rst_ctrl.value = 0xff;
	while(delay--);
	reg->rst_ctrl.value = 0;

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_clr_config(void *vreg, void *priv)
{
	volatile Gx3211M2MReg *reg = NULL;
	(void) priv;

	reg = (volatile Gx3211M2MReg *)vreg;

	reg->ctrl2.value = 0;
	reg->ctrl1.value = 0;
	reg->nds_ctrl.value = 0;
	reg->isr.value = 0xff;

	return GXSE_SUCCESS;
}

static int32_t _nds_wait_flag(volatile Gx3211M2MReg *reg, void *priv)
{
	unsigned long long cur = 0, tick = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;

	if (p->nds_wait_flag) {
		tick = gx_osdep_current_tick();
		while (!reg->nds_gen.bits.key_write_done) {
			cur = gx_osdep_current_tick();
			if (gx_time_after_eq(cur, tick + 15)) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "nds key is not ready\n");
				return GXSE_ERR_GENERIC;
			}
		};
		p->nds_wait_flag = 0;
	}
	return 0;
}

static int32_t gx3211_m2m_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	int32_t key = 0, alg = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	volatile Gx3211M2MReg *reg = (volatile Gx3211M2MReg *)vreg;
	GxCryptoResOps *ops = p->resops;
	(void) cmd;

	if ((alg = ops->get_alg(param->alg)) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_opt(param->opt) < 0)
		return GXSE_ERR_PARAM;

	if ((key = ops->get_key(&param->even_key)) < 0)
		return GXSE_ERR_PARAM;

	if (key == GX3211_M2M_KEY_NDS) {
		_nds_wait_flag(reg, priv);
		reg->nds_gen.bits.ctrl_write_en = 0;
	}

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

static int32_t gx3211_m2m_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	int32_t alg = 0, evenkey = 0;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;
	GxCryptoResOps *ops = p->resops;
	uint32_t key_big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	(void) oddkey_sel;
	(void) key_len;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((alg = ops->get_alg(param->alg)) < 0)
		return alg;

	if ((evenkey = ops->get_key(&key_sel)) < 0)
		return GXSE_ERR_GENERIC;

	reg->ctrl1.bits.gx3211_key_sel = evenkey;

	if (evenkey == GX3211_M2M_KEY_NDS) {
		reg->nds_gen.bits.key_sel_enable = 0;
		reg->nds_gen.bits.key_sel = key_sel.sub;
		reg->nds_gen.bits.key_sel_enable = 1;
	}

	if (evenkey == GX3211_M2M_KEY_ACPU_SOFT)
		taurus_set_soft_key(vreg, alg, key, key_big_endian);

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_set_ready(void *vreg, void *priv)
{
	int32_t alg, key;
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	reg = (volatile Gx3211M2MReg *)vreg;

	if ((alg = ops->get_alg(p->param.alg)) < 0)
		return alg;

	if ((key = ops->get_key(&p->param.even_key)) < 0)
		return GXSE_ERR_GENERIC;

	if (key == GX3211_M2M_KEY_NDS) {
		reg->nds_gen.bits.ctrl_write_sel = 0;
		reg->nds_gen.bits.ctrl_write_en = 1;
		reg->ctrl2.value = 0x1 << 4;
	} else {
		if (alg <= TAURUS_M2M_ALG_TDES3K)
			reg->ctrl2.value = 0x1;
		else if (alg <= TAURUS_M2M_ALG_AES256)
			reg->ctrl2.value = 0x1 << 1;
	}

	reg->ctrl1.bits.key_rdy = 0x1;

	return GXSE_SUCCESS;
}

static int gx3211_m2m_nds_key_generate(void *vreg, void *priv, GxTfmPVRKey *param)
{
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)priv;

	reg = (volatile Gx3211M2MReg *)vreg;

	_nds_wait_flag(reg, priv);

	if (reg->nds_key_full)
		reg->nds_key_full &= ~(1 << 0);
	reg->nds_gen.bits.key_update_ready = 0;
	reg->nds_key_write_addr &= ~(0x7 << 0);
	reg->nds_key_write_addr = param->key_id & 0x7;
	reg->nds_gen.bits.key_update_ready = 1;
	p->nds_wait_flag = 1;
	return 0;
}

static int32_t gx3211_m2m_set_pvrk(void *vreg, void *priv, GxTfmPVRKey *config)
{
	int is_acpu_key = TFM_FLAG_GET(config->flags, TFM_FLAG_CRYPT_PVRK_FROM_ACPU);
	(void) priv;

	if (is_acpu_key) {
		config->ret = 0;
	} else {
		gx3211_m2m_nds_key_generate(vreg, priv, config);
	}
	return 0;
}

static int32_t gx3211_m2m_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) priv;
	(void) vreg;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));

	param->max_sub_num  = 1;
	param->key = 1 << TFM_KEY_ACPU_SOFT | 1 << TFM_KEY_MULTI | 1 << TFM_KEY_NDS |
				 1 << TFM_KEY_OTP_SOFT;
	param->alg = 1 << TFM_ALG_TDES | 1 << TFM_ALG_AES128 | 1 << TFM_ALG_AES192 | 1 << TFM_ALG_AES256 |
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

static int32_t gx3211_m2m_init_module(GxSeModuleHwObj *obj)
{
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;

	reg = (volatile Gx3211M2MReg *)obj->reg;
	if (reg == NULL) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "reg addr is NULL\n");
		return GXSE_ERR_GENERIC;
	}

	gx_mutex_init(p->mutex);
	reg->isr_en.value = 0x0;
	obj->mutex = p->mutex;
	p->nds_wait_flag = 0;

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_open_module(GxSeModuleHwObj *obj)
{
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;
	p->align_buf_len = TFM_MAX_CIPHER_UNIT*2;
	p->align_buf = gx_osdep_page_malloc(p->align_buf_len);
	if (NULL == p->align_buf)
		goto err;

	memset(p->align_buf, 0xbf, p->align_buf_len);
	gx_osdep_cache_sync(p->align_buf, p->align_buf_len, DMA_TO_DEVICE);

	return GXSE_SUCCESS;

err:

	if (p->align_buf) {
		gx_osdep_page_free(p->align_buf, p->align_buf_len);
		p->align_buf = NULL;
		p->align_buf_len = 0;
	}

	return GXSE_ERR_GENERIC;
}

static int32_t gx3211_m2m_close_module(GxSeModuleHwObj *obj)
{
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;

	if (p->align_buf) {
		gx_osdep_page_free(p->align_buf, p->align_buf_len);
		p->align_buf = NULL;
		p->align_buf_len = 0;
	}

	return GXSE_SUCCESS;
}

static int32_t gx3211_m2m_deinit_module(GxSeModuleHwObj *obj)
{
	volatile Gx3211M2MReg *reg = NULL;
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;

	reg = (volatile Gx3211M2MReg *)obj->reg;

	reg->isr_en.value = 0x0;
	gx_mutex_destroy(p->mutex);

	return GXSE_SUCCESS;
}

static GxCryptoResOps gx3211_mtc_resops = {
	.get_opt = taurus_m2m_get_opt,
	.get_alg = taurus_m2m_get_alg,
	.get_key = gx3211_m2m_get_key,
};

static struct mtc_priv gx3211_mtc_priv = {
	.mutex = &gx3211_mtc_mutex,
	.resops = &gx3211_mtc_resops,
};

static GxSeModuleDevOps gx3211_m2m_devops = {
	.init            = gx3211_m2m_init_module,
	.deinit          = gx3211_m2m_deinit_module,
	.open            = gx3211_m2m_open_module,
	.close           = gx3211_m2m_close_module,
	.isr             = taurus_m2m_isr,
};

GxCryptoOps gx3211_m2m_ops = {
	.check_param      = gx3211_m2m_check_param,
	.get_align_param  = taurus_m2m_get_align_param,
	.capability       = gx3211_m2m_capability,

	.set_endian       = taurus_m2m_set_endian,
	.set_work_mode    = gx3211_m2m_set_work_mode,
	.set_alg          = gx3211_m2m_set_alg,
	.set_key          = gx3211_m2m_set_key,
	.set_opt          = gx3211_m2m_set_opt,
	.set_input_buf    = taurus_m2m_set_input_buf,
	.set_output_buf   = taurus_m2m_set_output_buf,
	.set_residue_mode = gx3211_m2m_set_residue_mode,
	.set_shortmsg_mode= gx3211_m2m_set_shortmsg_mode,
	.wait_finish      = taurus_m2m_wait_finish,
	.clr_config       = gx3211_m2m_clr_config,
	.set_ready        = gx3211_m2m_set_ready,
//	.clr_isr_en       = taurus_m2m_clr_isr_en,
	.set_isr_en       = taurus_m2m_set_isr_en,
	.set_pvr_key      = gx3211_m2m_set_pvrk,
};


static GxSeModuleHwObjTfmOps gx3211_crypto_dma_tfm_ops = {
	.hwops = &gx3211_m2m_ops,
	.devops = &gx3211_m2m_devops,

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

static GxSeModuleHwObj gx3211_m2m_tfm_obj = {
	.type = GXSE_HWOBJ_TYPE_TFM,
	.sub = TFM_MOD_M2M,
	.ops = &gx3211_crypto_dma_tfm_ops,
	.priv = &gx3211_mtc_priv,
};

GxSeModule gx3211_m2m_module = {
	.id = GXSE_MOD_CRYPTO_DMA0,
	.ops  = &crypto_dev_ops,
	.hwobj = &gx3211_m2m_tfm_obj,
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"m2m"},
	},
};
