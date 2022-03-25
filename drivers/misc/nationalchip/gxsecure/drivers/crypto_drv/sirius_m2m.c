#include "gxcrypto_virdev.h"
#include "gxse_kapi_firewall.h"
#include "sirius_m2m_reg.h"
#include "sirius_m2m.h"

typedef enum {
	M2M_DESC_SRC,
	M2M_DESC_DST,
} M2MDescType;

typedef union {
	uint32_t value;
	struct {
		unsigned eod  : 1;
		unsigned sod  : 1;
		unsigned      : 22;
		unsigned xeod : 1;
		unsigned xsod : 1;
	} bits;
} M2MDescConf;

typedef struct _m2m_desc {
	uint32_t           start_addr;
	uint32_t           length;
	M2MDescConf        conf;
	struct _m2m_desc*  next;
} M2MDesc;

typedef struct {
	uint32_t        vaddr;
	uint32_t        paddr;
	uint32_t        pos;
	uint32_t        eod;
	uint32_t        cur_size;
} M2MCmdlist;

struct m2m_priv {
	uint32_t        id;
	GxTfmCrypto     param;
	M2MCmdlist      src_cmdlist;
	M2MCmdlist      dst_cmdlist;

	gx_mutex_t      mutex;
	gx_spin_lock_t  spin_lock;
	gx_event_t      queue;
	uint32_t        total_len;
	uint32_t        total_len_init;
	uint32_t        cur_len;
	uint32_t        even_key;
	uint8_t        *align_buf;
	uint32_t        align_buf_len;
	uint8_t        *secure_buf;
	uint32_t        secure_buf_len;

	volatile uint32_t dma_done;
	volatile M2MStatus status;
};

static int32_t sirius_m2m_open_module(GxSeModuleHwObj *obj);

static int32_t _get_alg(GxTfmAlg alg)
{
	switch (alg) {
	case TFM_ALG_AES128:
		return SIRIUS_M2M_ALG_AES128;

	case TFM_ALG_AES192:
		return SIRIUS_M2M_ALG_AES192;

	case TFM_ALG_AES256:
		return SIRIUS_M2M_ALG_AES256;

	case TFM_ALG_DES:
		return SIRIUS_M2M_ALG_DES;

	case TFM_ALG_TDES:
		return SIRIUS_M2M_ALG_TDES2K;

	case TFM_ALG_SM4:
		return SIRIUS_M2M_ALG_SM4;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m alg error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int32_t _get_opt(GxTfmOpt opt)
{
	switch (opt) {
	case TFM_OPT_ECB:
	case TFM_OPT_CBC:
		return opt;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m opt error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static int _get_key(GxTfmKeyBuf *param)
{
	switch (param->id) {
	case TFM_KEY_RK:
		return SIRIUS_M2M_KEY_RK_ADDR0 + param->sub;

	case TFM_KEY_SSUK:
		return SIRIUS_M2M_KEY_SSUK;

	case TFM_KEY_OTP_SOFT:
		return SIRIUS_M2M_KEY_OTP_SOFT;

	case TFM_KEY_OTP_FIRM:
		return SIRIUS_M2M_KEY_OTP_FIRM;

	case TFM_KEY_ACPU_SOFT:
		return SIRIUS_M2M_KEY_ACPU_SOFT;

	case TFM_KEY_SCPU_SOFT:
		return SIRIUS_M2M_KEY_SCPU_SOFT;

	default:
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "crypto key error\n");
		return GXSE_ERR_GENERIC;
	}
	return GXSE_ERR_GENERIC;
}

static void _m2m_firm_enter_hw_mode(unsigned int start, unsigned int size)
{
#if defined (CFG_ENABLE_OPTEE)
	if (start && size > 0)
		gxse_firewall_config_filter(start, size, 0x30002800, 0x04000000, 0);
#endif
}

static void _m2m_normal_enter_hw_mode(struct m2m_priv *priv)
{
#if defined (CFG_ENABLE_OPTEE)
	if (M2M_CONTENT_LIMIT == 0)
		return;

	if (priv->secure_buf && priv->secure_buf_len)
		gxse_firewall_config_filter((int)priv->secure_buf, priv->secure_buf_len, 0x0fc00000, 0x0fc00000, 0);
#endif
}

static void _m2m_normal_enter_secure_mode(struct m2m_priv *priv)
{
#if defined (CFG_ENABLE_OPTEE)
	if (M2M_CONTENT_LIMIT == 0)
		return;

	if (priv->secure_buf && priv->secure_buf_len)
		gxse_firewall_config_filter((int)priv->secure_buf, priv->secure_buf_len, 0x80000000, 0x80000000, 1);
#endif
}

static int32_t sirius_m2m_check_param(void *vreg, void *priv, GxTfmCrypto *param, uint32_t cmd)
{
	int32_t key = 0, alg = 0;
	uint32_t ts_mode       = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_TS_PACKET_MODE);
	uint32_t oddkey_valid  = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ODD_KEY_VALID);
	uint32_t evenkey_valid = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_EVEN_KEY_VALID);
	uint32_t encrypt       = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ENCRYPT);
	struct m2m_priv *p = (struct m2m_priv *)priv;
	(void) vreg;
	(void) cmd;

	if ((alg = _get_alg(param->alg)) < 0)
		return GXSE_ERR_PARAM;

	if (_get_opt(param->opt) < 0)
		return GXSE_ERR_PARAM;

	if (param->src.id < TFM_SRC_MEM || param->src.id > TFM_SRC_REG)
		return GXSE_ERR_PARAM;

	if (param->dst.id < TFM_DST_TS || param->dst.id > TFM_DST_REG)
		return GXSE_ERR_PARAM;

	if (ts_mode) {
		if (param->input.length < 188) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "cipher length < 188\n");
			return GXSE_ERR_GENERIC;
		}
		if (evenkey_valid) {
			if ((key = _get_key(&param->even_key)) < 0)
				return GXSE_ERR_PARAM;

			if (key >= SIRIUS_M2M_KEY_ACPU_SOFT) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "M2M don't support this key in ts mode, key = %d\n", key);
				return GXSE_ERR_GENERIC;
			}
		}

		if (oddkey_valid) {
			if ((key = _get_key(&param->odd_key)) < 0)
				return GXSE_ERR_PARAM;

			if (key >= SIRIUS_M2M_KEY_ACPU_SOFT) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "M2M don't support this key in ts mode, key = %d\n", key);
				return GXSE_ERR_GENERIC;
			}
		}

	} else {
		if ((key = _get_key(&param->even_key)) < 0)
			return GXSE_ERR_PARAM;

		if ((alg >= SIRIUS_M2M_ALG_DES && alg <= SIRIUS_M2M_ALG_AES256) &&
			(key > SIRIUS_M2M_KEY_OTP_SOFT || key < SIRIUS_M2M_KEY_ACPU_SOFT)) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "M2M only support soft key when select this alg, alg = %d, key = %d\n", alg, key);
			return GXSE_ERR_GENERIC;
		}

		if (key == SIRIUS_M2M_KEY_OTP_FIRM) {
			if (M2M_CONTENT_LIMIT == 0) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "Can't use OTP firm key in non-secure environment\n");
				return GXSE_ERR_GENERIC;
			}
			if (alg != SIRIUS_M2M_ALG_AES128 || encrypt) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "OTP firm key only support aes128 and encryption\n");
				return GXSE_ERR_GENERIC;
			}
		}
	}

	if (NULL == param->input.buf || param->input.length == 0 ||
		NULL == param->output.buf || param->output.length < param->input.length) {
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m buf error %p %x %p %x\n",
				param->input.buf, param->input.length, param->output.buf, param->output.length);
		return GXSE_ERR_PARAM;
	}

	memcpy(&p->param, param, sizeof(GxTfmCrypto));
	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_work_mode(void *vreg, void *priv,
		uint32_t crypt_mode, uint32_t work_mode, uint32_t ts_mode)
{
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	struct m2m_priv *p = (struct m2m_priv *)priv;
	uint32_t id = p->id;
	(void) work_mode;

	if (p->src_cmdlist.vaddr == 0) {
		GxSeModuleHwObj obj = {0};
		obj.reg = vreg;
		obj.priv = priv;
		sirius_m2m_open_module(&obj);
	}

	reg->ctrl.bits.ts_mode &= ~(0x1<<id);
	reg->ctrl.bits.ts_mode |= ts_mode<<id;
	reg->channel[id].ctrl.bits.enc = crypt_mode & 0x1;

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_endian(void *vreg, void *priv, uint32_t flag)
{
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	(void) priv;

	reg->ctrl.bits.little_endian = !flag;

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_alg(void *vreg, void *priv, GxTfmAlg alg)
{
	int32_t ret = 0;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	struct m2m_priv *p = (struct m2m_priv *)priv;
	uint32_t id = p->id;

	if ((ret = _get_alg(alg)) < 0)
		return ret;

	reg->channel[id].ctrl.bits.alg = ret;
	p->param.alg = ret;

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_key(void *vreg, void *priv, GxTfmKeyBuf key_sel, GxTfmKeyBuf oddkey_sel,
		uint8_t *key, uint32_t key_len)
{
	int32_t i = 0;
	struct m2m_priv *p = (struct m2m_priv *)priv;
	GxTfmCrypto *param = &p->param;
	int32_t evenkey = 0, oddkey = 0;
	uint32_t id = p->id, softkey_valid = 0;
	uint32_t ts_mode       = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_TS_PACKET_MODE);
	uint32_t oddkey_valid  = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ODD_KEY_VALID);
	uint32_t evenkey_valid = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_EVEN_KEY_VALID);
	uint32_t alg = param->alg;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	if (ts_mode) {
		if (evenkey_valid == 0 && oddkey_valid == 0) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "m2m key undefined.\n");
			return GXSE_ERR_GENERIC;

		} else if (evenkey_valid && oddkey_valid) {
			reg->channel[id].ctrl.bits.two_key_on = 1;

		} else {
			reg->channel[id].ctrl.bits.two_key_on = 0;
			if (evenkey_valid)
				reg->channel[id].ctrl.bits.one_key_odd = 0;
			else if (oddkey_valid)
				reg->channel[id].ctrl.bits.one_key_odd = 1;
		}
		if (evenkey_valid) {
			evenkey = _get_key(&key_sel);
			reg->channel[id].ctrl.bits.key0_sel = evenkey;
			if (evenkey == SIRIUS_M2M_KEY_ACPU_SOFT)
				softkey_valid = 1;
		}

		if (oddkey_valid) {
			oddkey = _get_key(&oddkey_sel);
			reg->channel[id].ctrl.bits.key1_sel = oddkey;
			if (oddkey == SIRIUS_M2M_KEY_ACPU_SOFT)
				softkey_valid = 1;
		}

	} else {
		if ((evenkey = _get_key(&key_sel)) < 0)
			return GXSE_ERR_GENERIC;

		evenkey = _get_key(&key_sel);
		reg->channel[id].ctrl.bits.key0_sel = evenkey;
		reg->channel[id].ctrl.bits.two_key_on = 0;
		reg->channel[id].ctrl.bits.one_key_odd = 0;
		if (evenkey == SIRIUS_M2M_KEY_ACPU_SOFT)
			softkey_valid = 1;
	}

	reg->channel[id].ctrl.bits.key_update = 1;
	reg->channel[id].ctrl.bits.key_update = 0;

	if (softkey_valid) {
		uint32_t big_endian   = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

		if (alg == SIRIUS_M2M_ALG_AES256)
			key_len = 8;
		else if (alg == SIRIUS_M2M_ALG_AES192)
			key_len = 6;
		else if (alg == SIRIUS_M2M_ALG_DES)
			key_len = 2;
		else
			key_len = 4;

		for (i = 0; i < key_len; i++) {
			reg->soft_key[key_len-i-1] = gx_u32_get_val(key+4*i, big_endian);
		}
		reg->soft_key_update = 0;
		reg->soft_key_update = 1;
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_opt(void *vreg, void *priv, GxTfmOpt opt_sel, uint8_t *key, uint32_t key_len)
{
	int32_t ret = 0, i = 0;
	struct m2m_priv *p = (struct m2m_priv *)priv;
	GxTfmCrypto *param = &p->param;
	uint32_t iv_len = 0, id = p->id;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	(void) key_len;

	if ((ret = _get_opt(opt_sel)) < 0)
		return ret;

	reg->channel[id].ctrl.bits.opt_mode = ret;

	if (ret == SIRIUS_M2M_OPT_CBC) {
		uint32_t big_endian = TFM_FLAG_GET(param->flags, TFM_FLAG_KEY_BIG_ENDIAN);

		if (param->alg == SIRIUS_M2M_ALG_TDES2K || param->alg == SIRIUS_M2M_ALG_DES)
			iv_len = 2;
		else
			iv_len = 4;

		reg->iv_mask.value = id;
		for (i = 0; i < iv_len; i++)
			reg->iv[iv_len-i-1] = gx_u32_get_val(key+4*i, big_endian);

		reg->iv_update = 1;
		reg->iv_update = 0;
	}

	return GXSE_SUCCESS;
}

static void sirius_m2m_create_desc_list(void *vreg, void *priv, M2MDescType type)
{
	uint32_t ptr, i, offset, cur_size, element_end;
	uint32_t element_num, element_len, crypto_len, crypto_left = 0;
	M2MDesc *plist, *vlist, *node = NULL, *priv_node = NULL;

	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	struct m2m_priv *p = (struct m2m_priv *)priv;
	int32_t little_endian = reg->ctrl.bits.little_endian;

/* TODO circle list */
	if (type == M2M_DESC_SRC) {
		if (p->src_cmdlist.eod) {
			p->src_cmdlist.pos = 0;
			p->src_cmdlist.cur_size = 0;
		}
		vlist      = (M2MDesc *)p->src_cmdlist.vaddr;
		plist      = (M2MDesc *)p->src_cmdlist.paddr;
		ptr         = p->param.input_paddr;
		crypto_len  = p->param.input.length;
		offset      = p->src_cmdlist.pos;
		cur_size    = p->src_cmdlist.cur_size;
		element_end = p->src_cmdlist.eod;

	} else {
		if (p->dst_cmdlist.eod) {
			p->dst_cmdlist.pos = 0;
			p->dst_cmdlist.cur_size = 0;
		}
		vlist      = (M2MDesc *)p->dst_cmdlist.vaddr;
		plist      = (M2MDesc *)p->dst_cmdlist.paddr;
		ptr         = p->param.output_paddr;
		crypto_len  = p->param.output.length;
		offset      = p->dst_cmdlist.pos;
		cur_size    = p->dst_cmdlist.cur_size;
		element_end = p->dst_cmdlist.eod;
	}

	if (M2M_IS_FIRM_MODE(&p->param)) {
		uint32_t temp = 0x3;
		vlist->start_addr    = gx_u32_get_val(&ptr, !little_endian);
		vlist->length        = gx_u32_get_val(&crypto_len, !little_endian);
		vlist->conf.value    = gx_u32_get_val(&temp, !little_endian);
		vlist->next          = (M2MDesc *)gx_u32_get_val(&plist, !little_endian);
		element_end           = 1;
		gx_osdep_cache_sync((void *)p->src_cmdlist.vaddr, 4096, DMA_TO_DEVICE);
		gx_osdep_cache_sync((void *)p->dst_cmdlist.vaddr, 4096, DMA_TO_DEVICE);
		return;
	}

/* TODO we support input (0x1000/4*element_len) per-time */
	element_len = (crypto_len < 0x1000) ? crypto_len : 0x1000;
	element_num = crypto_len / element_len;
	crypto_left = crypto_len % element_len;
	element_num += crypto_left ? 1 : 0;

	for (i = 0; i < element_num; i++) {
		if (element_end && i == 0) {
			vlist->start_addr    = ptr;
			vlist->length        = element_len;
			vlist->conf.value    = M2M_CONF_SOD;
			vlist->next          = plist;
			element_end           = 0;
			node = vlist;
			gxlog_d(GXSE_LOG_MOD_CRYPTO, "%p, %x, %d\n", vlist, vlist->start_addr, vlist->length);

		} else {
			int new_ptr = ptr + i*element_len;
			node             = (M2MDesc *)((unsigned int)vlist+offset);
			priv_node        = (M2MDesc *)((unsigned int)vlist+offset-sizeof(M2MDesc));
			node->start_addr = new_ptr;
			node->length     = ((i == element_num-1) && crypto_left) ? crypto_left : element_len;
			node->conf.value = 0;
			node->next       = (M2MDesc *)((unsigned int)plist+offset);
			priv_node->next  = node->next;
			gxlog_d(GXSE_LOG_MOD_CRYPTO, "%p, %p, %x, %d\n", priv_node, node, node->start_addr, node->length);
		}

		offset   += sizeof(M2MDesc);
		cur_size += ((i == element_num-1) && crypto_left) ? crypto_left : element_len;
		if (type == M2M_DESC_SRC)
			p->cur_len = cur_size;

		if (little_endian) {
			node->start_addr = gx_swab32((node->start_addr));
			node->length     = gx_swab32((node->length));
			node->next       = (M2MDesc *)gx_swab32(((unsigned int)node->next));
			if(priv_node)
				priv_node->next = (M2MDesc *)gx_swab32(((unsigned int)priv_node->next));
		}

		if (cur_size == p->total_len) {
			node->conf.value |= M2M_CONF_EOD;
			node->conf.value = gx_u32_get_val(&node->conf.value, !little_endian);
			element_end = 1;
			if (type == M2M_DESC_DST) {
				p->cur_len = 0;
				p->total_len = 0;
				p->total_len_init = 0;
			}
			break;
		}
		node->conf.value = gx_u32_get_val(&node->conf.value, !little_endian);
	}
	gx_osdep_cache_sync((void *)p->src_cmdlist.vaddr, 4096, DMA_TO_DEVICE);
	gx_osdep_cache_sync((void *)p->dst_cmdlist.vaddr, 4096, DMA_TO_DEVICE);
	if (type == M2M_DESC_SRC) {
		p->src_cmdlist.pos = offset;
		p->src_cmdlist.cur_size = cur_size;
		p->src_cmdlist.eod = element_end;

	} else {
		p->dst_cmdlist.pos = offset;
		p->dst_cmdlist.cur_size = cur_size;
		p->dst_cmdlist.eod = element_end;
	}
}

static int32_t sirius_m2m_set_input_buf(void *vreg, void *priv, GxTfmSrcBuf in_sel, uint8_t *in, uint32_t in_len)
{
	struct m2m_priv *p = (struct m2m_priv *)priv;
	uint32_t id = p->id;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	(void) in_sel;

	if (p->cur_len == 0) {
		reg->channel[id].src_chain_ptr    = p->src_cmdlist.paddr;
		reg->channel[id].data_len         = in_len;
		reg->channel[id].weight           = 8;
		p->total_len = in_len;
		p->total_len_init = 1;
		p->param.input_paddr = (uint32_t)in;
	}

	reg->channel[id].src_update_size  = in_len;

	return GXSE_SUCCESS;
}

static int _m2m_check_buf_illegal(struct m2m_priv *p)
{
	GxTfmCrypto *param = &p->param;
	uint32_t encrypt = TFM_FLAG_GET(param->flags, TFM_FLAG_CRYPT_ENCRYPT);

	if (NULL == p->secure_buf || M2M_CONTENT_LIMIT == 0)
		return GXSE_SUCCESS;

	if (M2M_IS_FIRM_MODE(param)) {
		/*
		 * The input buffer have no limits.
		 */
		if (gxse_secmem_is_illegal((char *)"afwmem", param->output_paddr, param->output.length) &&
			gxse_secmem_is_illegal((char *)"vfwmem", param->output_paddr, param->output.length)) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "The firmware buffer is illegal.\n");
			return GXSE_ERR_GENERIC;
		}

	} else if (M2M_IS_HW_PVR_MODE(param) || M2M_IS_PVR_MODE(param)) {
		if (encrypt) {
			if (gxse_secmem_is_illegal((char *)"tswmem", param->input_paddr, param->input.length)) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "The enc_pvr input buffer is illegal.\n");
				return GXSE_ERR_GENERIC;
			}

#if defined (CFG_ENABLE_OPTEE)
			if (gxse_secmem_is_illegal((char *)"secmem", param->output_paddr, param->output.length)) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "The enc_pvr output buffer is illegal.\n");
				return GXSE_ERR_GENERIC;
			}
#endif

		} else {
#if defined (CFG_ENABLE_OPTEE)
			if (gxse_secmem_is_illegal((char *)"secmem", param->input_paddr, param->input.length)) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "The enc_pvr input buffer is illegal.\n");
				return GXSE_ERR_GENERIC;
			}
#endif

			if (gxse_secmem_is_illegal((char *)"tsrmem", param->output_paddr, param->output.length)) {
				gxlog_e(GXSE_LOG_MOD_CRYPTO, "The enc_pvr output buffer is illegal.\n");
				return GXSE_ERR_GENERIC;
			}
		}

	} else {
		if (gxse_secmem_is_illegal((char *)"secmem", param->input_paddr, param->input.length)) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "The normal input buffer is illegal.\n");
			return GXSE_ERR_GENERIC;
		}

		if (gxse_secmem_is_illegal((char *)"secmem", param->output_paddr, param->output.length)) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "The normal output buffer is illegal.\n");
			return GXSE_ERR_GENERIC;
		}
	}

	return 0;
}

static int32_t sirius_m2m_set_output_buf(void *vreg, void *priv, GxTfmDstBuf out_sel, uint8_t *out, uint32_t out_len)
{
	struct m2m_priv *p = (struct m2m_priv *)priv;

	uint32_t id = p->id;
	GxTfmCrypto *param = &p->param;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	(void) out_sel;

	p->param.output_paddr = (uint32_t)out;
	if (_m2m_check_buf_illegal(p) < 0)
		return GXSE_ERR_GENERIC;

	if (p->cur_len == 0)
		reg->channel[id].dst_chain_ptr = p->dst_cmdlist.paddr;
	reg->channel[id].dst_update_size = out_len;

	sirius_m2m_create_desc_list((void *)reg, priv, M2M_DESC_SRC);
	sirius_m2m_create_desc_list((void *)reg, priv, M2M_DESC_DST);

	if (M2M_IS_FIRM_MODE(param))
		_m2m_firm_enter_hw_mode(param->output_paddr, param->output.length);
	_m2m_normal_enter_hw_mode(p);

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_ready(void *vreg, void *priv)
{
	struct m2m_priv *p = (struct m2m_priv *)priv;
	uint32_t id = p->id;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	p->status = M2M_STATUS_RUNNING;
	reg->header_update.value = (0x1<<id) | (0x1<<(id+SIRIUS_M2M_MAX_MOD));

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_set_switch_clr(void *vreg, void *priv, uint32_t switch_clr)
{
	struct m2m_priv *p = (struct m2m_priv *)priv;
	uint32_t id = p->id;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	if (switch_clr)
		reg->channel[id].ctrl.bits.key_switch_clr = 1;
	return GXSE_SUCCESS;
}

static void sirius_m2m_status_isr(volatile SiriusM2MReg *reg, void *vsreg, void *priv, M2MStatus val)
{
	int32_t id = 0;
	static unsigned long spin_flag;
	volatile rM2M_STATUS *sreg = (volatile rM2M_STATUS *)vsreg;
	struct m2m_priv *p = (struct m2m_priv *)priv;

	id = p->id;
	if (sreg->value & (0x1<<id)) {
		p->status = val;
		gxlog_d(GXSE_LOG_MOD_CRYPTO, "isr %d!\n", id);

		if (val >= M2M_STATUS_ALMOST_EMPTY)
			return;

		_m2m_normal_enter_secure_mode(p);
		sreg->value = 0x1<<id;
		gx_spin_lock_irqsave(&(p->spin_lock), spin_flag);
		p->dma_done = 1;
		gx_spin_unlock_irqrestore(&(p->spin_lock), spin_flag);
		gx_wake_event(&p->queue, 0);
		if (val == M2M_STATUS_DONE) {
			while(reg->channel[id].src_remainder || reg->channel[id].dst_remainder)
				gx_msleep(10);
		}
	}
}

static int32_t sirius_m2m_isr(GxSeModuleHwObj *obj)
{
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)obj->reg;

    if (reg->m2m_int.bits.dma_done) {
		sirius_m2m_status_isr(reg, (void *)&reg->dma_done, obj->priv, M2M_STATUS_DONE);
		gxlog_d(GXSE_LOG_MOD_CRYPTO, "dma done!\n");
    }

    if (reg->m2m_int.bits.sync_byte_error) {
		sirius_m2m_status_isr(reg, (void *)&reg->sync_err, obj->priv, M2M_STATUS_RUNNING);
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "sync byte error!\n");
    }

    if (reg->m2m_int.bits.tdes_same_key) {
		sirius_m2m_status_isr(reg, (void *)&reg->key_err, obj->priv, M2M_STATUS_ERR);
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "tdes same key!\n");
    }

    if (reg->m2m_int.bits.almost_empty && reg->m2m_int_en.bits.almost_empty) {
		sirius_m2m_status_isr(reg, (void *)&reg->almost_empty, obj->priv, M2M_STATUS_ALMOST_EMPTY);
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "almost empty %x!\n", reg->almost_empty.value);
    }

    if (reg->m2m_int.bits.almost_full && reg->m2m_int_en.bits.almost_full) {
		sirius_m2m_status_isr(reg, (void *)&reg->almost_full, obj->priv, M2M_STATUS_ALMOST_FULL);
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "almost full! %x\n", reg->almost_full.value);
    }

    if (reg->m2m_int.bits.space_err) {
		sirius_m2m_status_isr(reg, (void *)&reg->space_err, obj->priv, M2M_STATUS_SPACE_ERR);
		gxlog_e(GXSE_LOG_MOD_CRYPTO, "space err!\n");
    }

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_wait_finish(void *vreg, void *priv, uint32_t is_query)
{
	int32_t ret = GXSE_SUCCESS;
	unsigned long long cur = 0;
	unsigned long long tick = 0;
	struct m2m_priv *p = (struct m2m_priv *)priv;

	if (is_query < TFM_ISR_QUERY_UNTIL)
		tick = gx_osdep_current_tick();

	while (1) {
		GxSeModuleHwObj obj = {0};
		obj.reg = vreg;
		obj.priv = priv;
		sirius_m2m_isr(&obj);

		if (is_query < TFM_ISR_QUERY_UNTIL) {
			cur = gx_osdep_current_tick();
			if (gx_time_after_eq(cur, tick + 15)) {
				ret = GXSE_ERR_GENERIC;
				break;
			}
		}

		if (p->status == M2M_STATUS_RUNNING)
			continue;

		p->status = M2M_STATUS_IDLE;
		break;
	}

	_m2m_normal_enter_secure_mode(p);

	return ret;
}

static int32_t sirius_m2m_set_isr_en(void *vreg, void *priv)
{
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	(void) priv;

	reg->m2m_int_en.value = 0x0;

	return 0;
}

static int32_t sirius_m2m_clr_isr_en(void *vreg, void *priv)
{
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	(void) priv;

	reg->m2m_int_en.value = 0x0;

	return 0;
}

static int32_t sirius_m2m_clr_key(void *vreg, void *priv)
{
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;
	(void) priv;

	reg->ctrl.bits.key_clr = 1;

	return 0;
}

static int32_t sirius_m2m_set_pvrk(void *vreg, void *priv, GxTfmPVRKey *config)
{
	int32_t i;
	uint8_t big_endian = TFM_FLAG_GET(config->flags, TFM_FLAG_KEY_BIG_ENDIAN);
	uint8_t acpu_flag  = TFM_FLAG_GET(config->flags, TFM_FLAG_CRYPT_PVRK_FROM_ACPU);
	uint8_t *key = (void *)config->key;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)vreg;

	(void) priv;

	config->ret = 0;

	if (config->key_id > SIRIUS_M2M_KEY_RK_ADDR15)
		return GXSE_ERR_PARAM;

	reg->rk.value = config->key_id;
	if (!acpu_flag)
		return GXSE_SUCCESS;

	for(i = 0; i < 4; i++)
		reg->record_key[3-i] = gx_u32_get_val(key+4*i, big_endian);

	reg->record_key_update = 1;
	reg->record_key_update = 0;

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_get_align_param(void *vreg, void *priv,
		uint32_t *buf, uint32_t *size, uint32_t *align, uint32_t *flag)
{
	struct m2m_priv *p = (struct m2m_priv *)priv;
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

#if defined (CFG_ENABLE_OPTEE)
		if (p->secure_buf && M2M_CONTENT_LIMIT) {
			src = TFM_FORCE_ALIGN;
			dst = TFM_FORCE_ALIGN;
		}
#endif

		if (M2M_IS_HW_PVR_MODE(param)) {
			if (encrypt_flag) {
				src = TFM_UNFORCE_ALIGN;
				dst = TFM_FORCE_ALIGN;
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

static int32_t sirius_m2m_init_module(GxSeModuleHwObj *obj)
{
	GxTfmPVRKey param = {0};
	struct m2m_priv *p = (struct m2m_priv *)obj->priv;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)obj->reg;
	int32_t id = p->id;

	memset(p, 0, sizeof(struct m2m_priv));
	reg->sync_byte_err_threshold = 3;
	reg->key_switch_threshold    = 1;
	reg->almost_empty_threshold  = 0x200;
	reg->almost_full_threshold   = 0x800;
	reg->m2m_int_en.value = 0x0;

	p->id = id;
	p->dma_done = 0;
	p->src_cmdlist.eod = 1;
	p->dst_cmdlist.eod = 1;
	gx_spin_lock_init(&(p->spin_lock));
	gx_event_init(&p->queue);
	gx_mutex_init(&p->mutex);
	obj->mutex = &p->mutex;

	memset(&param, 0, sizeof(GxTfmPVRKey));
	memset(param.key, 0x11, 16);
	param.key_id = 0;
	param.flags  = TFM_FLAG_CRYPT_PVRK_FROM_ACPU;

	if (id == 0)
		sirius_m2m_set_pvrk(obj->reg, obj->priv, &param);

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_deinit_module(GxSeModuleHwObj *obj)
{
	struct m2m_priv *p = (struct m2m_priv *)obj->priv;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)obj->reg;

	reg->m2m_int_en.value = 0x0;
	gx_mutex_destroy(&p->mutex);

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_open_module(GxSeModuleHwObj *obj)
{
	void *src_vaddr = NULL, *dst_vaddr = NULL;
	struct m2m_priv *p = (struct m2m_priv *)obj->priv;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)obj->reg;
	uint32_t id = p->id;

	if (p->src_cmdlist.vaddr)
		return GXSE_SUCCESS;

	if (gxse_secmem_probe_byname((char *)"secmem", (void *)&p->secure_buf, (void *)&p->secure_buf_len) >= 0) {
		if (p->secure_buf_len < GXM2M_SECMEM_LEN)
			goto err;

		if (NULL == (p->align_buf = gx_osdep_ioremap((uint32_t)p->secure_buf, GXM2M_SECMEM_LEN))) {
			gxlog_e(GXSE_LOG_MOD_CRYPTO, "secmem ioremap failed.\n");
			goto err;
		}
		p->align_buf_len = TFM_MAX_CIPHER_UNIT*2;
		p->src_cmdlist.paddr = (uint32_t)p->secure_buf + 0x5000 + id*0x2000;
		p->dst_cmdlist.paddr = (uint32_t)p->secure_buf + 0x5000 + id*0x2000 + 0x1000;
		p->src_cmdlist.vaddr = gx_phys_to_virt(p->src_cmdlist.paddr);
		p->dst_cmdlist.vaddr = gx_phys_to_virt(p->dst_cmdlist.paddr);
		src_vaddr = (void *)p->src_cmdlist.vaddr;
		dst_vaddr = (void *)p->dst_cmdlist.vaddr;

	} else {

		gx_dma_alloc(p->src_cmdlist.paddr, p->src_cmdlist.vaddr, 4096);
		src_vaddr = (void *)p->src_cmdlist.vaddr;
		if (NULL == src_vaddr)
			goto err;

		gx_dma_alloc(p->dst_cmdlist.paddr, p->dst_cmdlist.vaddr, 4096);
		dst_vaddr = (void *)p->dst_cmdlist.vaddr;
		if (NULL == dst_vaddr)
			goto err;


		p->align_buf_len = TFM_MAX_CIPHER_UNIT*2;
		p->align_buf = gx_osdep_page_malloc(p->align_buf_len);
		if (NULL == p->align_buf)
			goto err;
	}

	memset(src_vaddr, 0, 4096);
	memset(dst_vaddr, 0, 4096);
	memset(p->align_buf, 0xbf, p->align_buf_len);
	gx_osdep_cache_sync(src_vaddr, 4096, DMA_TO_DEVICE);
	gx_osdep_cache_sync(dst_vaddr, 4096, DMA_TO_DEVICE);
	gx_osdep_cache_sync(p->align_buf, p->align_buf_len, DMA_TO_DEVICE);
	reg->ctrl.bits.channel_en |= 0x1<<id;

	return GXSE_SUCCESS;

err:

	gxlog_e(GXSE_LOG_MOD_CRYPTO, "channel %d alloc buf failed.\n", id);
	if (p->secure_buf)
		return GXSE_ERR_GENERIC;

	if (p->src_cmdlist.vaddr) {
		gx_dma_free(p->src_cmdlist.paddr, p->src_cmdlist.vaddr, 4096);
		p->src_cmdlist.vaddr = 0;
	}

	if (p->dst_cmdlist.vaddr) {
		gx_dma_free(p->dst_cmdlist.paddr, p->dst_cmdlist.vaddr, 4096);
		p->dst_cmdlist.vaddr = 0;
	}

	if (p->align_buf) {
		gx_osdep_page_free(p->align_buf, p->align_buf_len);
		p->align_buf = NULL;
		p->align_buf_len = 0;
	}

	return GXSE_ERR_GENERIC;
}

static int32_t sirius_m2m_close_module(GxSeModuleHwObj *obj)
{
	struct m2m_priv *p = (struct m2m_priv *)obj->priv;
	volatile SiriusM2MReg *reg = (volatile SiriusM2MReg *)obj->reg;
	uint32_t id = p->id;

	if (!p->src_cmdlist.vaddr)
		return GXSE_SUCCESS;

	reg->ctrl.bits.channel_en &= ~(0x1<<id);
	if (p->secure_buf)
		return GXSE_SUCCESS;

	if (p->src_cmdlist.vaddr) {
		gx_dma_free(p->src_cmdlist.paddr, p->src_cmdlist.vaddr, 4096);
		p->src_cmdlist.vaddr = 0;
	}
	if (p->dst_cmdlist.vaddr) {
		gx_dma_free(p->dst_cmdlist.paddr, p->dst_cmdlist.vaddr, 4096);
		p->dst_cmdlist.vaddr = 0;
	}

	if (p->align_buf) {
		gx_osdep_page_free(p->align_buf, p->align_buf_len);
		p->align_buf = NULL;
		p->align_buf_len = 0;
	}

	return GXSE_SUCCESS;
}

static int32_t sirius_m2m_capability(void *vreg, void *priv, GxTfmCap *param)
{
	(void) vreg;
	(void) priv;
	memset(param, 0, sizeof(GxTfmCap));
	memset(param->key_sub_num, 0x1, sizeof(param->key_sub_num));

	param->max_sub_num  = 8;
	param->key_sub_num[TFM_KEY_RK] = 16;
	param->key = 1 << TFM_KEY_RK |
				 1 << TFM_KEY_SSUK |
				 1 << TFM_KEY_ACPU_SOFT |
				 1 << TFM_KEY_SCPU_SOFT |
				 1 << TFM_KEY_OTP_SOFT |
				 1 << TFM_KEY_OTP_FIRM;
	param->alg = 1 << TFM_ALG_AES128 |
				 1 << TFM_ALG_SM4 |
				 1 << TFM_ALG_TDES |
				 1 << TFM_ALG_DES |
				 1 << TFM_ALG_AES192 |
				 1 << TFM_ALG_AES256;
	param->opt = 1 << TFM_OPT_ECB |
				 1 << TFM_OPT_CBC;
	param->flags = TFM_FLAG_BIG_ENDIAN |
				   TFM_FLAG_CRYPT_PVRK_FROM_ACPU |
				   TFM_FLAG_CRYPT_TS_PACKET_MODE |
				   TFM_FLAG_CRYPT_EVEN_KEY_VALID |
				   TFM_FLAG_CRYPT_ODD_KEY_VALID |
				   TFM_FLAG_CRYPT_SWITCH_CLR;

	return 0;
}

static GxSeModuleDevOps sirius_m2m_devops = {
	.init            = sirius_m2m_init_module,
	.deinit          = sirius_m2m_deinit_module,
	.open            = sirius_m2m_open_module,
	.close           = sirius_m2m_close_module,
	.isr             = sirius_m2m_isr,
};

static GxCryptoOps sirius_m2m_ops = {
	.check_param     = sirius_m2m_check_param,
	.get_align_param = sirius_m2m_get_align_param,
	.capability      = sirius_m2m_capability,

    .set_pvr_key     = sirius_m2m_set_pvrk,
    .set_endian      = sirius_m2m_set_endian,
    .set_work_mode   = sirius_m2m_set_work_mode,
    .set_alg         = sirius_m2m_set_alg,
    .set_key         = sirius_m2m_set_key,
    .set_opt         = sirius_m2m_set_opt,
    .set_switch_clr  = sirius_m2m_set_switch_clr,
    .set_input_buf   = sirius_m2m_set_input_buf,
    .set_output_buf  = sirius_m2m_set_output_buf,
    .wait_finish     = sirius_m2m_wait_finish,
	.set_ready       = sirius_m2m_set_ready,
	.set_isr_en      = sirius_m2m_set_isr_en,
	.clr_isr_en      = sirius_m2m_clr_isr_en,
	.clr_key         = sirius_m2m_clr_key,
};

static GxSeModuleHwObjTfmOps sirius_crypto_dma_tfm_ops = {
	.devops = &sirius_m2m_devops,
	.hwops = &sirius_m2m_ops,

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

static struct m2m_priv sirius_m2m_priv[] = {
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 0
	{.id = 0},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 1
	{.id = 1},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 2
	{.id = 2},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 3
	{.id = 3},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 4
	{.id = 4},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 5
	{.id = 5},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 6
	{.id = 6},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 7
	{.id = 7},
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
};

static GxSeModuleHwObj sirius_m2m_dma_obj[] = {
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 0
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[0]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 1
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[1]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 2
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[2]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 3
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[3]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 4
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[4]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 5
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[5]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 6
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[6]},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 7
	{.type = GXSE_HWOBJ_TYPE_TFM, .sub = TFM_MOD_M2M, .ops = &sirius_crypto_dma_tfm_ops, .priv = &sirius_m2m_priv[7]},
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
};

GxSeModule sirius_m2m_module[] = {
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 0
	{
		.id = GXSE_MOD_CRYPTO_DMA0, .ops  = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[0],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 1
	{
		.id = GXSE_MOD_CRYPTO_DMA1, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[1],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 2
	{
		.id = GXSE_MOD_CRYPTO_DMA2, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[2],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 3
	{
		.id = GXSE_MOD_CRYPTO_DMA3, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[3],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 4
	{
		.id = GXSE_MOD_CRYPTO_DMA4, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[4],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 5
	{
		.id = GXSE_MOD_CRYPTO_DMA5, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[5],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 6
	{
		.id = GXSE_MOD_CRYPTO_DMA6, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[6],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#if CFG_CRYPTO_DMA_CHANNEL_NUM > 7
	{
		.id = GXSE_MOD_CRYPTO_DMA7, .ops = &crypto_dev_ops, .hwobj = &sirius_m2m_dma_obj[7],
		.res = {
			.reg_base  = GXACPU_BASE_ADDR_M2M,
			.reg_len   = sizeof(SiriusM2MReg),
			.irqs      = {GXACPU_IRQ_M2M, -1},
			.irq_names = {"m2m"},
		},
	},
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

};

