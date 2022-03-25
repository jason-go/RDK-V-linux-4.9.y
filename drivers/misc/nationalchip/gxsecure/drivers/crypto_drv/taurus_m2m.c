#include "gxcrypto_virdev.h"
#include "taurus_m2m_reg.h"
#include "gx3211_m2m.h"
#include "taurus_m2m.h"
#include "sirius_m2m.h"
#include "kapi/gxmailbox_uapi.h"

int32_t taurus_m2m_get_alg(GxTfmAlg alg)
{
	switch (alg) {
	case TFM_ALG_AES128:
		return TAURUS_M2M_ALG_AES128;

	case TFM_ALG_AES192:
		return TAURUS_M2M_ALG_AES192;

	case TFM_ALG_AES256:
		return TAURUS_M2M_ALG_AES256;

	case TFM_ALG_DES:
		return TAURUS_M2M_ALG_DES;

	case TFM_ALG_TDES:
		return TAURUS_M2M_ALG_TDES3K;
	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t taurus_m2m_get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_SCPU_KLM:
		return TAURUS_M2M_KEY_SCPU_KLM;

	case TFM_KEY_SCPU_MISC:
		return TAURUS_M2M_KEY_SCPU_PVR;

	case TFM_KEY_OTP_SOFT:
		return TAURUS_M2M_KEY_OTP_SOFT;

	case TFM_KEY_OTP_FIRM:
		return TAURUS_M2M_KEY_OTP_FIRM;

	case TFM_KEY_OTP_FLASH:
		return TAURUS_M2M_KEY_OTP_FLASH;

	case TFM_KEY_ACPU_SOFT:
		return TAURUS_M2M_KEY_ACPU_SOFT;

	case TFM_KEY_SCPU_SOFT:
		return TAURUS_M2M_KEY_SCPU_SOFT;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

int32_t taurus_m2m_get_opt(GxTfmOpt opt)
{
	switch (opt) {
	case TFM_OPT_ECB:
	case TFM_OPT_CBC:
	case TFM_OPT_CFB:
	case TFM_OPT_OFB:
	case TFM_OPT_CTR:
		return opt;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m opt error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t taurus_m2m_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	int32_t key = 0, alg = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	(void) vreg;
	(void) cmd;

	if ((alg = ops->get_alg(param->alg)) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_opt(param->opt) < 0)
		return GXSE_ERR_PARAM;

	if ((key = ops->get_key(&param->even_key)) < 0)
		return GXSE_ERR_PARAM;

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

static int32_t taurus_crypto_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	(void) vreg;
	(void) cmd;

	if ((ops->get_alg(param->alg)) < 0)
		return GXSE_ERR_PARAM;

	if (ops->get_opt(param->opt) < 0)
		return GXSE_ERR_PARAM;

	if (TFM_KEY_OTP_FLASH != param->even_key.id)
		return GXSE_ERR_PARAM;

	if (param->src.id < TFM_SRC_MEM || param->src.id > TFM_SRC_REG)
		return GXSE_ERR_PARAM;

	if (param->dst.id < TFM_DST_TS || param->dst.id < TFM_DST_REG)
		return GXSE_ERR_PARAM;

	if (NULL == param->input.buf || param->input.length == 0 ||
		NULL == param->output.buf || param->output.length < param->input.length) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m buf error\n");
		return GXSE_ERR_PARAM;
	}

	memcpy(&p->param, param, sizeof(GxTfmCrypto));
	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_work_mode(void *vreg, void *priv,
		uint32_t crypt_mode, uint32_t work_mode, uint32_t ts_mode)
{
	volatile int delay = 50;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) priv;
	(void) ts_mode;

	reg->ctrl1.bits.flash_encrypt = work_mode & 0x1;
	reg->ctrl1.bits.encrypt = crypt_mode & 0x1;

// 因硬件在调用aes加解密时存在出错的情况，所以每次使用前最好重置一下，调用如下接口
// 3211中存在 aes 连续解密出错问题，需 mtc_rst_contrl(0x60) 寄存器的第5bit需重置(硬件bug)
	reg->rst_ctrl.value = 0xff;
	while(delay--);
	reg->rst_ctrl.value = 0;

	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_endian(void *vreg, void *priv, uint32_t flag)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) priv;

	reg->ctrl1.bits.big_endian = flag;

	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	uint32_t des_iv_sel = TFM_FLAG_GET(p->param.flags, TFM_FLAG_CRYPT_DES_IV_SHIFT);
	GxCryptoResOps *ops = p->resops;

	if ((ret = ops->get_alg(alg)) < 0)
		return ret;

	if (ret <= TAURUS_M2M_ALG_TDES3K) {
		reg->ctrl1.bits.tri_des = ret;
		reg->ctrl1.bits.work_mode = 0;
		reg->ctrl1.bits.des_iv_sel = des_iv_sel;
	} else if (ret <= TAURUS_M2M_ALG_AES256) {
		reg->ctrl1.bits.aes_mode = ret - TAURUS_M2M_ALG_AES128;
		reg->ctrl1.bits.work_mode = 1;
	}

	return GXSE_SUCCESS;
}

void taurus_set_soft_key(void *vreg , uint32_t alg, uint8_t *key, uint32_t key_big_endian)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;

	switch (alg) {
		case TAURUS_M2M_ALG_DES:
			reg->key0_2[0].high = gx_u32_get_val(key, key_big_endian);
			reg->key0_2[0].low  = gx_u32_get_val(key+4*1, key_big_endian);
			break;
		case TAURUS_M2M_ALG_TDES3K:
			reg->key0_2[2].high = gx_u32_get_val(key+4*4, key_big_endian);
			reg->key0_2[2].low  = gx_u32_get_val(key+4*5, key_big_endian);
			reg->key0_2[1].high = gx_u32_get_val(key+4*2, key_big_endian);
			reg->key0_2[1].low  = gx_u32_get_val(key+4*3, key_big_endian);
			reg->key0_2[0].high = gx_u32_get_val(key+4*0, key_big_endian);
			reg->key0_2[0].low  = gx_u32_get_val(key+4*1, key_big_endian);
			break;
		case TAURUS_M2M_ALG_AES128:
			reg->key0_2[1].high = gx_u32_get_val(key, key_big_endian);
			reg->key0_2[1].low  = gx_u32_get_val(key+4*1, key_big_endian);
			reg->key0_2[0].high = gx_u32_get_val(key+4*2, key_big_endian);
			reg->key0_2[0].low  = gx_u32_get_val(key+4*3, key_big_endian);
			break;
		case TAURUS_M2M_ALG_AES192:
			reg->key0_2[2].high = gx_u32_get_val(key, key_big_endian);
			reg->key0_2[2].low  = gx_u32_get_val(key+4*1, key_big_endian);
			reg->key0_2[1].high = gx_u32_get_val(key+4*2, key_big_endian);
			reg->key0_2[1].low  = gx_u32_get_val(key+4*3, key_big_endian);
			reg->key0_2[0].high = gx_u32_get_val(key+4*4, key_big_endian);
			reg->key0_2[0].low  = gx_u32_get_val(key+4*5, key_big_endian);
			break;
		case TAURUS_M2M_ALG_AES256:
			reg->key3.high      = gx_u32_get_val(key, key_big_endian);
			reg->key3.low       = gx_u32_get_val(key+4*1, key_big_endian);
			reg->key0_2[2].high = gx_u32_get_val(key+4*2, key_big_endian);
			reg->key0_2[2].low  = gx_u32_get_val(key+4*3, key_big_endian);
			reg->key0_2[1].high = gx_u32_get_val(key+4*4, key_big_endian);
			reg->key0_2[1].low  = gx_u32_get_val(key+4*5, key_big_endian);
			reg->key0_2[0].high = gx_u32_get_val(key+4*6, key_big_endian);
			reg->key0_2[0].low  = gx_u32_get_val(key+4*7, key_big_endian);
			break;
		default:
			break;
	}
}

static int32_t taurus_m2m_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	int32_t alg = 0, evenkey = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;
	GxCryptoResOps *ops = p->resops;
	uint32_t key_big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

	(void) oddkey_sel;
	(void) key_len;

	if ((alg = ops->get_alg(param->alg)) < 0)
		return alg;

	if ((evenkey = ops->get_key(&key_sel)) < 0)
		return GXSE_ERR_GENERIC;

	reg->key_sel.bits.key_sel = evenkey;
	if (evenkey == TAURUS_M2M_KEY_ACPU_SOFT)
		taurus_set_soft_key(vreg, alg, key, key_big_endian);

	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_opt(void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *iv, uint32_t key_len)
{
	int32_t opt = 0, i = 0, max = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;
	GxCryptoResOps *ops = p->resops;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	uint32_t alg = ops->get_alg(param->alg);
	uint32_t key_big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);
	(void) key_len;

	if ((opt = ops->get_opt(opt_sel)) < 0)
		return opt;

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

int32_t taurus_m2m_set_residue_mode(void *vreg, void *priv, uint32_t mode)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) priv;

	reg->ctrl1.bits.residue = mode;
	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_shortmsg_mode(void *vreg, void *priv, uint32_t mode)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) vreg;
	(void) priv;

	reg->ctrl1.bits.short_msg = mode;
	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_input_buf(void *vreg, void *priv, GxTfmSrcBuf in_sel, uint8_t *in, uint32_t in_len)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) in_sel;
	(void) priv;

	reg->m2mr_addr = (unsigned int)in;
	reg->data_len  = in_len & 0x1fffffff;
	return GXSE_SUCCESS;
}


int32_t taurus_m2m_set_output_buf(void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	(void) out_len;
	(void) out_sel;

	reg->m2mw_addr = (unsigned int)out;
	gx_osdep_cache_sync(p->param.input.buf, p->param.input.length, DMA_TO_DEVICE);

	return GXSE_SUCCESS;
}

int32_t taurus_m2m_set_ready(void *vreg, void *priv)
{
	int32_t alg;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxCryptoResOps *ops = p->resops;

	if ((alg = ops->get_alg(p->param.alg)) < 0)
		return alg;

// set_ready, taurus m2m set ready的时机
	if (alg <= TAURUS_M2M_ALG_TDES3K)
		reg->ctrl2.value = 0x1;
	else if (alg <= TAURUS_M2M_ALG_AES256)
		reg->ctrl2.value = 0x1 << 1;

	reg->ctrl1.bits.key_rdy = 0x1;

	return GXSE_SUCCESS;
}

int32_t taurus_m2m_clr_config(void *vreg, void *priv)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) priv;

	reg->ctrl2.value = 0;
	reg->ctrl1.value = 0;
	reg->isr.value = 0xff;

	return GXSE_SUCCESS;
}

int32_t taurus_m2m_isr(GxSeModuleHwObj *obj)
{
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)obj->reg;

	if (reg->isr.bits.data_finish || reg->isr.bits.otp_flash_finish) {
		gxlog_d(GXSE_LOG_MOD_CRYPTO, "data finish\n");
		p->status = M2M_STATUS_DONE;
	}

	if (reg->isr.bits.m2m_operate_illegal) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m operate illegal\n");
		p->status = M2M_STATUS_SPACE_ERR;
	}

	if (reg->isr.bits.err_operate_nds) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "nds operate illegal\n");
		p->status = M2M_STATUS_SPACE_ERR;
	}
	return GXSE_SUCCESS;
}

int32_t taurus_m2m_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	int32_t ret = GXSE_SUCCESS;
	unsigned long long cur = 0, tick = 0;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;

	if (is_query < TFM_ISR_QUERY_UNTIL)
		tick = gx_osdep_current_tick();
	while (1) {
		GxSeModuleHwObj obj = {0};
		obj.reg = vreg;
		obj.priv = priv;
		taurus_m2m_isr(&obj);

		if (is_query < TFM_ISR_QUERY_UNTIL) {
			cur = gx_osdep_current_tick();
			if (gx_time_after_eq(cur, tick + 15)) {
				ret = GXSE_ERR_GENERIC;
				break;
			}
		}

		if (p->status != M2M_STATUS_DONE)
			continue;

		gx_osdep_cache_sync(param->output.buf, param->output.length, DMA_FROM_DEVICE);
		p->status = M2M_STATUS_IDLE;
		break;
	}
	return ret;
}

int32_t taurus_m2m_set_isr_en(void *vreg, void *priv)
{
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	(void) priv;

	reg->isr_en.value = 0xff;

	return 0;
}

int32_t taurus_m2m_get_align_param(void *vreg, void *priv,
		uint32_t *buf, uint32_t *size, uint32_t *align, uint32_t *flag)
{
	struct mtc_priv *p = (struct mtc_priv *)priv;
	GxTfmCrypto *param = &p->param;
	(void) vreg;

	if (buf)
		*buf = (uint32_t)p->align_buf;

	if (size)
		*size = p->align_buf_len;

	if (align)
		*align = 16;

	if (flag) {
		uint8_t src = 0, dst = 0;
		uint32_t encrypt_flag = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ENCRYPT);

		if (M2M_IS_HW_PVR_MODE(param)) {
			if (encrypt_flag) {
				src = TFM_UNFORCE_ALIGN;
			} else {
				dst = TFM_UNFORCE_ALIGN;
			}

		} else if (M2M_IS_FIRM_MODE(param)) {
			src = TFM_UNFORCE_ALIGN;
			dst = TFM_UNFORCE_ALIGN;
		}
		*flag = src | dst << 16;
	}

	return GXSE_SUCCESS;
}

static int32_t taurus_m2m_set_pvrk(void *vreg, void *priv, GxTfmPVRKey *config)
{
	int is_acpu_key = TFM_FLAG_GET(config->flags, TFM_FLAG_CRYPT_PVRK_FROM_ACPU);
	(void) vreg;
	(void) priv;

	if (is_acpu_key) {
		config->ret = 0;
	}
#ifdef CFG_GXSE_FIRMWARE_MBOX
	else {
		config->ret = gx_mbox_set_pvrk(config->key_id, 0);
	}
#endif
	return 0;
}

static int32_t taurus_m2m_get_blocklen(void *vreg, void *priv)
{
	(void) priv;
	(void) vreg;
	return TAURUS_M2M_BLOCKLEN;
}

static int32_t taurus_crypto_fifo_put(void *vreg, void *priv, uint8_t *buf, uint32_t len, uint32_t first_block)
{
	int32_t data_len, real_len, i, alg;
	unsigned int *input = (void *)buf;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	GxCryptoResOps *ops = p->resops;
	(void) first_block;

	if (NULL == buf || len == 0) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "param is illegal.\n");
		return GXSE_ERR_GENERIC;
	}

	if ((alg = ops->get_alg(p->param.alg)) < 0)
		return alg;

	data_len = len > 64 ? 64 : len;

	real_len = data_len / 4;

	while(reg->flash_sig.bits.write_busy);
	for (i = 0; i < real_len; i++)
		reg->flash_write_data = input[i];

	return GXSE_SUCCESS;
}

static int32_t taurus_crypto_fifo_get(void *vreg, void *priv, uint8_t *buf, uint32_t len)
{
	int32_t data_len, real_len, i, alg;
	uint32_t *output = (void *)buf;
	struct mtc_priv *p = (struct mtc_priv *)priv;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)vreg;
	GxCryptoResOps *ops = p->resops;

	if (NULL == buf || len == 0) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "param is illegal.\n");
		return GXSE_ERR_GENERIC;
	}

	if ((alg = ops->get_alg(p->param.alg)) < 0)
		return alg;

	data_len = len > 64 ? 64 : len;

	real_len = data_len / 4;

	while(reg->flash_sig.bits.read_busy);
	for (i = 0; i < real_len; i++)
		output[i] = reg->flash_read_data;

	return GXSE_SUCCESS;
}

static int32_t taurus_m2m_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) priv;
	(void) vreg;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));

	param->max_sub_num  = 1;
	param->opt = 1 << TFM_OPT_ECB |
				 1 << TFM_OPT_CBC |
				 1 << TFM_OPT_CFB |
				 1 << TFM_OPT_OFB |
				 1 << TFM_OPT_CTR;
	param->key = 1 << TFM_KEY_ACPU_SOFT |
				 1 << TFM_KEY_SCPU_KLM  |
				 1 << TFM_KEY_OTP_FLASH |
				 1 << TFM_KEY_SCPU_MISC |
				 1 << TFM_KEY_SCPU_SOFT |
				 1 << TFM_KEY_OTP_SOFT  |
				 1 << TFM_KEY_OTP_FIRM;
	param->alg = 1 << TFM_ALG_AES128    |
				 1 << TFM_ALG_TDES      |
				 1 << TFM_ALG_DES       |
				 1 << TFM_ALG_AES192    |
				 1 << TFM_ALG_AES256;
	param->flags = TFM_FLAG_BIG_ENDIAN         |
				 TFM_FLAG_CRYPT_TS_PACKET_MODE |
				 TFM_FLAG_CRYPT_RESIDUAL_RBT   |
				 TFM_FLAG_CRYPT_RESIDUAL_CTS   |
				 TFM_FLAG_CRYPT_DES_IV_SHIFT   |
				 TFM_FLAG_CRYPT_SHORT_MSG_IV1  |
				 TFM_FLAG_CRYPT_SHORT_MSG_IV2  |
				 TFM_FLAG_CRYPT_EVEN_KEY_VALID |
				 TFM_FLAG_CRYPT_ODD_KEY_VALID;
	return 0;
}

static int32_t taurus_crypto_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));
	param->max_sub_num = 1;
	param->key = 1 << TFM_KEY_OTP_FLASH;
	param->opt = 1 << TFM_OPT_ECB |
				 1 << TFM_OPT_CBC |
				 1 << TFM_OPT_CFB |
				 1 << TFM_OPT_OFB |
				 1 << TFM_OPT_CTR;
	param->alg = 1 << TFM_ALG_AES128    |
				 1 << TFM_ALG_TDES      |
				 1 << TFM_ALG_DES       |
				 1 << TFM_ALG_AES192    |
				 1 << TFM_ALG_AES256;
	param->flags = TFM_FLAG_BIG_ENDIAN         |
				 TFM_FLAG_CRYPT_TS_PACKET_MODE |
				 TFM_FLAG_CRYPT_RESIDUAL_CTS   |
				 TFM_FLAG_CRYPT_RESIDUAL_CTS   |
				 TFM_FLAG_CRYPT_DES_IV_SHIFT   |
				 TFM_FLAG_CRYPT_SHORT_MSG_IV1  |
				 TFM_FLAG_CRYPT_SHORT_MSG_IV2  |
				 TFM_FLAG_CRYPT_EVEN_KEY_VALID |
				 TFM_FLAG_CRYPT_ODD_KEY_VALID;
	return GXSE_SUCCESS;
}

static int32_t taurus_m2m_init_module(GxSeModuleHwObj *obj)
{
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)obj->reg;

	if (NULL == reg) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "reg addr is NULL\n");
		return GXSE_ERR_GENERIC;
	}

	gx_mutex_init(p->mutex);
	reg->isr_en.value = 0x0;
	obj->mutex = p->mutex;

	return GXSE_SUCCESS;
}

static int32_t taurus_m2m_open_module(GxSeModuleHwObj *obj)
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

static int32_t taurus_m2m_close_module(GxSeModuleHwObj *obj)
{
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;

	if (p->align_buf) {
		gx_osdep_page_free(p->align_buf, p->align_buf_len);
		p->align_buf = NULL;
		p->align_buf_len = 0;
	}

	return GXSE_SUCCESS;
}

static int32_t taurus_m2m_deinit_module(GxSeModuleHwObj *obj)
{
	struct mtc_priv *p = (struct mtc_priv *)obj->priv;
	volatile TaurusM2MReg *reg = (volatile TaurusM2MReg *)obj->reg;

	reg->isr_en.value = 0x0;
	gx_mutex_destroy(p->mutex);

	return GXSE_SUCCESS;
}

static GxCryptoResOps taurus_mtc_resops = {
	.get_opt = taurus_m2m_get_opt,
	.get_alg = taurus_m2m_get_alg,
	.get_key = taurus_m2m_get_key,
};

static struct mtc_priv taurus_mtc_priv = {
	.mutex = &gx3211_mtc_mutex,
	.resops = &taurus_mtc_resops,
};

GxSeModuleDevOps taurus_m2m_devops = {
	.init            = taurus_m2m_init_module,
	.deinit          = taurus_m2m_deinit_module,
	.open            = taurus_m2m_open_module,
	.close           = taurus_m2m_close_module,
	.isr             = taurus_m2m_isr,
};

GxCryptoOps taurus_m2m_ops = {
	.check_param      = taurus_m2m_check_param,
	.get_align_param  = taurus_m2m_get_align_param,
	.capability       = taurus_m2m_capability,

	.set_endian       = taurus_m2m_set_endian,
	.set_work_mode    = taurus_m2m_set_work_mode,
	.set_alg          = taurus_m2m_set_alg,
	.set_key          = taurus_m2m_set_key,
	.set_opt          = taurus_m2m_set_opt,
	.set_input_buf    = taurus_m2m_set_input_buf,
	.set_output_buf   = taurus_m2m_set_output_buf,
	.set_residue_mode = taurus_m2m_set_residue_mode,
	.set_shortmsg_mode= taurus_m2m_set_shortmsg_mode,
	.wait_finish      = taurus_m2m_wait_finish,
	.clr_config       = taurus_m2m_clr_config,
	.set_ready        = taurus_m2m_set_ready,
//	.clr_isr_en       = taurus_m2m_clr_isr_en,
	.set_isr_en       = taurus_m2m_set_isr_en,
	.set_pvr_key      = taurus_m2m_set_pvrk,

	.get_blocklen     = taurus_m2m_get_blocklen,
};

GxCryptoOps taurus_crypto_ops = {
	.check_param      = taurus_crypto_check_param,
	.capability       = taurus_crypto_capability,

	.set_endian       = taurus_m2m_set_endian,
	.set_work_mode    = taurus_m2m_set_work_mode,
	.set_alg          = taurus_m2m_set_alg,
	.set_key          = taurus_m2m_set_key,
	.set_opt          = taurus_m2m_set_opt,
	.set_residue_mode = taurus_m2m_set_residue_mode,
	.set_shortmsg_mode= taurus_m2m_set_shortmsg_mode,
	.wait_finish      = taurus_m2m_wait_finish,
	.clr_config       = taurus_m2m_clr_config,
	.set_ready        = taurus_m2m_set_ready,
//	.clr_isr_en       = taurus_m2m_clr_isr_en,
	.set_isr_en       = taurus_m2m_set_isr_en,
	.set_pvr_key      = taurus_m2m_set_pvrk,

	.get_blocklen     = taurus_m2m_get_blocklen,
	.fifo_put         = taurus_crypto_fifo_put,
	.fifo_get         = taurus_crypto_fifo_get,
};


static GxSeModuleHwObjTfmOps taurus_crypto_dma_tfm_ops = {
	.hwops = &taurus_m2m_ops,
	.devops = &taurus_m2m_devops,

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

static GxSeModuleHwObjTfmOps taurus_crypto_fifo_tfm_ops = {
	.hwops = &taurus_crypto_ops,
	.devops = &taurus_m2m_devops,

	.tfm_cipher = {
		.set_pvr_key   = gx_tfm_object_crypto_set_pvr_key,
		.clr_key       = gx_tfm_object_crypto_clr_key,
		.enable_key    = gx_tfm_object_crypto_enable_key,
		.disable_key   = gx_tfm_object_crypto_disable_key,
		.encrypt       = gx_tfm_object_crypto_fifo_encrypt,
		.decrypt       = gx_tfm_object_crypto_fifo_decrypt,
		.capability    = gx_tfm_object_crypto_capability,
	},
};


static GxSeModuleHwObj taurus_m2m_tfm_obj[TAURUS_M2M_MAX_MOD] = {
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &taurus_crypto_dma_tfm_ops, .priv = &taurus_mtc_priv},
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &taurus_crypto_fifo_tfm_ops, .priv = &taurus_mtc_priv},
};

GxSeModule taurus_crypto_module = {
	.id = GXSE_MOD_CRYPTO_FIFO,
	.ops  = &crypto_dev_ops,
	.hwobj = &taurus_m2m_tfm_obj[1],
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"m2m"},
	},
};

GxSeModule taurus_m2m_module = {
	.id = GXSE_MOD_CRYPTO_DMA0,
	.ops  = &crypto_dev_ops,
	.hwobj = &taurus_m2m_tfm_obj[0],
	.res = {
		.reg_base  = GXACPU_BASE_ADDR_TAURUS_M2M,
		.reg_len   = TAURUS_M2M_REG_LEN,
		.irqs      = {GXACPU_IRQ_TAURUS_M2M, -1},
		.irq_names = {"m2m"},
	},
};
